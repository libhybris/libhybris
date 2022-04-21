/*
 * Copyright (c) 2022 Jolla Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <android-config.h>
#include <assert.h>
#include <algorithm>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ws.h"

#define VK_USE_PLATFORM_ANDROID_KHR 1
#define VK_USE_PLATFORM_WAYLAND_KHR 1
extern "C" {
#include <vulkanplatformcommon.h>
};
#include <vulkanhybris.h>

extern "C" {
#include <wayland-client.h>
#include <wayland-egl.h>
}

#include <vulkan/vulkan.h>

#include <hybris/gralloc/gralloc.h>
#include <hybris/common/binding.h>
#include "logging.h"
#include "server_wlegl_buffer.h"
#include "wayland-android-client-protocol.h"
#include "wayland_window.h"

struct WaylandDisplay {
    wl_display *wl_dpy;
    wl_event_queue *queue;
    wl_registry *registry;
    android_wlegl *wlegl;
    WaylandNativeWindow *window;
};

static bool init_done = false;

/* Keep track of active Vulkan window surfaces */
static std::map<VkSurfaceKHR,struct WaylandDisplay *> _surface_window_map;

int vulkan_wayland_has_mapping(VkSurfaceKHR surface)
{
    return (_surface_window_map.find(surface) != _surface_window_map.end());
}

void vulkan_wayland_push_mapping(VkSurfaceKHR surface, struct WaylandDisplay *wdpy)
{
    assert(!vulkan_wayland_has_mapping(surface));

    _surface_window_map[surface] = wdpy;
}

struct WaylandDisplay *vulkan_wayland_pop_mapping(VkSurfaceKHR surface)
{
    std::map<VkSurfaceKHR, struct WaylandDisplay *>::iterator it;
    it = _surface_window_map.find(surface);

    /* Caller must check with vulkan_helper_has_mapping() before */
    assert(it != _surface_window_map.end());

    struct WaylandDisplay *result = it->second;
    _surface_window_map.erase(it);
    return result;
}

static VkResult (*_vkCreateAndroidSurfaceKHR)(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) = NULL;
static PFN_vkVoidFunction (*_vkDestroySurfaceKHR)(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator) = NULL;
static VkResult (*_vkEnumerateInstanceExtensionProperties)(const char *pLayerName, uint32_t *pPropertyCount, VkExtensionProperties *pProperties) = NULL;
static VkResult (*_vkCreateInstance)(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) = NULL;
static PFN_vkVoidFunction (*_vkGetInstanceProcAddr)(VkInstance instance, const char *pName) = NULL;

extern "C" void waylandws_init_module(struct ws_vulkan_interface *vulkan_iface)
{
    if (init_done) {
        return;
    }
    hybris_gralloc_initialize(0);
    vulkanplatformcommon_init(vulkan_iface);
    init_done = true;
}

static void registry_handle_global(void *data, wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
    WaylandDisplay *dpy = (WaylandDisplay *)data;

    if (strcmp(interface, "android_wlegl") == 0) {
        dpy->wlegl = static_cast<struct android_wlegl *>(wl_registry_bind(registry, name, &android_wlegl_interface, std::min(2u, version)));
    }
}

static const wl_registry_listener registry_listener = {
    registry_handle_global
};

static void callback_done(void *data, wl_callback *cb, uint32_t d)
{
    WaylandDisplay *dpy = (WaylandDisplay *)data;

    wl_callback_destroy(cb);
    if (!dpy->wlegl) {
        fprintf(stderr, "Fatal: the server doesn't advertise the android_wlegl global!");
        abort();
    }
}

static const wl_callback_listener callback_listener = {
    callback_done
};

void freeWaylandDisplay(WaylandDisplay *wdpy)
{
    int ret = 0;
    // We still have the sync callback on flight, wait for it to arrive
    while (ret == 0 && !wdpy->wlegl) {
        ret = wl_display_dispatch_queue(wdpy->wl_dpy, wdpy->queue);
    }
    assert(ret >= 0);
    android_wlegl_destroy(wdpy->wlegl);
    wl_registry_destroy(wdpy->registry);
    wl_event_queue_destroy(wdpy->queue);
    delete wdpy;
}

static VkResult waylandws_vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
    VkResult res;

    if (_vkEnumerateInstanceExtensionProperties == NULL) {
        _vkEnumerateInstanceExtensionProperties = (VkResult (*)(const char*, uint32_t*, VkExtensionProperties*))
            (*_vkGetInstanceProcAddr)(NULL, "vkEnumerateInstanceExtensionProperties");
    }

    res = (*_vkEnumerateInstanceExtensionProperties)(pLayerName, pPropertyCount, pProperties);
    if (res == VK_SUCCESS && *pPropertyCount > 0 && pProperties != NULL) {
        // Find and replace Android surface extension with wayland surface extension
        uint32_t i;
        for (i = 0; i < *pPropertyCount; i++) {
            if (strcmp(pProperties[i].extensionName, VK_KHR_ANDROID_SURFACE_EXTENSION_NAME) == 0) {
                strncpy(pProperties[i].extensionName, VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, VK_MAX_EXTENSION_NAME_SIZE);
            }
        }
    }
    return res;
}

VkResult waylandws_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance)
{
    VkInstanceCreateInfo createInfo = *pCreateInfo;
    VkResult result;
    // Temporary array to replace wayland surface extension with Android surface extension
    char **enabledExtensions = (char **)malloc(pCreateInfo->enabledExtensionCount * sizeof(char *));
    uint32_t i;

    if (_vkCreateInstance == NULL) {
        _vkCreateInstance = (VkResult (*)(const VkInstanceCreateInfo *, const VkAllocationCallbacks *, VkInstance *))
            (*_vkGetInstanceProcAddr)(NULL, "vkCreateInstance");
    }

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        enabledExtensions[i] = (char *)malloc(VK_MAX_EXTENSION_NAME_SIZE * sizeof(char));
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME) == 0) {
            strncpy(enabledExtensions[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, VK_MAX_EXTENSION_NAME_SIZE);
        } else {
            strncpy(enabledExtensions[i], pCreateInfo->ppEnabledExtensionNames[i], VK_MAX_EXTENSION_NAME_SIZE);
        }
    }
    createInfo.ppEnabledExtensionNames = enabledExtensions;

    // Call actual vkCreateInstance
    result = (*_vkCreateInstance)(&createInfo, pAllocator, pInstance);

    // Free temporary array
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        free(enabledExtensions[i]);
    }
    free(enabledExtensions);

    return result;
}

static VkResult waylandws_vkCreateWaylandSurfaceKHR(VkInstance instance,
        const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSurfaceKHR* pSurface)
{
    VkAndroidSurfaceCreateInfoKHR createInfo;
    VkResult result;
    WaylandDisplay *wdpy = new WaylandDisplay;
    WaylandNativeWindow *win;
    struct wl_egl_window *window;
    int ret;

    HYBRIS_TRACE_BEGIN("hybris-vulkan", "vkCreateWaylandSurfaceKHR", "");

    if (_vkCreateAndroidSurfaceKHR == NULL) {
        _vkCreateAndroidSurfaceKHR = (VkResult (*)(VkInstance, const VkAndroidSurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *))
            (*_vkGetInstanceProcAddr)(instance, "vkCreateAndroidSurfaceKHR");
    }

    wdpy->wl_dpy = pCreateInfo->display;
    wdpy->wlegl = NULL;
    wdpy->queue = wl_display_create_queue(wdpy->wl_dpy);
    wdpy->registry = wl_display_get_registry(wdpy->wl_dpy);
    wl_proxy_set_queue((wl_proxy *) wdpy->registry, wdpy->queue);
    wl_registry_add_listener(wdpy->registry, &registry_listener, wdpy);

    wl_callback *cb = wl_display_sync(wdpy->wl_dpy);
    wl_proxy_set_queue((wl_proxy *) cb, wdpy->queue);
    wl_callback_add_listener(cb, &callback_listener, wdpy);

    ret = 0;
    while (ret == 0 && !wdpy->wlegl) {
        ret = wl_display_dispatch_queue(wdpy->wl_dpy, wdpy->queue);
    }
    assert(ret >= 0);

    window = wl_egl_window_create(pCreateInfo->surface, 1, 1);

    HYBRIS_TRACE_BEGIN("native-vulkan", "vkCreateWaylandSurfaceKHR", "");

    win = new WaylandNativeWindow((struct wl_egl_window *)window, wdpy->wl_dpy, wdpy->wlegl);
    win->common.incRef(&win->common);
    createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.window = win;

    result = (*_vkCreateAndroidSurfaceKHR)(instance, &createInfo, pAllocator, pSurface);

    HYBRIS_TRACE_END("native-vulkan", "vkCreateWaylandSurfaceKHR", "");

    if (result == VK_SUCCESS) {
        wdpy->window = win;
        vulkan_wayland_push_mapping(*pSurface, wdpy);
    } else {
        HYBRIS_ERROR("vkCreateAndroidSurfaceKHR failed");
        win->common.decRef(&win->common);
        freeWaylandDisplay(wdpy);
    }

    HYBRIS_TRACE_END("hybris-vulkan", "vkCreateWaylandSurfaceKHR", "");
    return result;
}

static VkBool32 waylandws_vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display* display)
{
    return VK_TRUE;
}

static void waylandws_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
    if (vulkan_wayland_has_mapping(surface)) {
        WaylandDisplay *wdpy = (WaylandDisplay *)vulkan_wayland_pop_mapping(surface);
        WaylandNativeWindow *window = (WaylandNativeWindow *)wdpy->window;

        if (_vkDestroySurfaceKHR == NULL) {
            _vkDestroySurfaceKHR = (PFN_vkVoidFunction (*)(VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *))
                (*_vkGetInstanceProcAddr)(instance, "vkDestroySurfaceKHR");
        }

        window->destroyWlEGLWindow();
        window->common.decRef(&window->common);
        _vkDestroySurfaceKHR(instance, surface, pAllocator);
        freeWaylandDisplay(wdpy);
    }
}

extern "C" void waylandws_vkSetInstanceProcAddrFunc(PFN_vkVoidFunction addr)
{
    if (_vkGetInstanceProcAddr == NULL)
        _vkGetInstanceProcAddr = (PFN_vkVoidFunction (*)(VkInstance, const char*))addr;
}

struct ws_module ws_module_info = {
    waylandws_init_module,

    waylandws_vkEnumerateInstanceExtensionProperties,
    waylandws_vkCreateInstance,
    waylandws_vkCreateWaylandSurfaceKHR,
    waylandws_vkGetPhysicalDeviceWaylandPresentationSupportKHR,
    waylandws_vkDestroySurfaceKHR,
    waylandws_vkSetInstanceProcAddrFunc,
};
