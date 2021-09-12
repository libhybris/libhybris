/*
 * Copyright (c) 2013-2022 Jolla Ltd.
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
 *
 */

#ifndef __LIBHYBRIS_VULKAN_WS_H
#define __LIBHYBRIS_VULKAN_WS_H

#define VK_USE_PLATFORM_ANDROID_KHR 1
#define VK_USE_PLATFORM_WAYLAND_KHR 1

#include "config.h"
#include <vulkan/vulkan.h>

struct ws_vulkan_interface {
    void * (*android_vulkan_dlsym)(const char *symbol);
};

/* Defined in vulkan.c */
extern struct ws_vulkan_interface hybris_vulkan_interface;

struct ws_module {
    void (*init_module)(struct ws_vulkan_interface *vulkan_interface);

    VkResult (*vkEnumerateInstanceExtensionProperties)(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
    VkResult (*vkCreateInstance)(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);
#ifdef WANT_WAYLAND
    VkResult (*vkCreateWaylandSurfaceKHR)(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
    VkBool32 (*vkGetPhysicalDeviceWaylandPresentationSupportKHR)(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display* display);
    void (*vkDestroySurfaceKHR)(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator);
#endif
    void (*vkSetInstanceProcAddrFunc)(PFN_vkVoidFunction addr);
};

VkResult ws_vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties);
VkResult ws_vkCreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);
#ifdef WANT_WAYLAND
VkResult ws_vkCreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkSurfaceKHR* pSurface);
VkBool32 ws_vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display* display);
void ws_vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator);
#endif
void ws_vkSetInstanceProcAddrFunc(PFN_vkVoidFunction addr);
#endif
