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
 *
 */

/* For RTLD_DEFAULT */
#define _GNU_SOURCE

#define VK_USE_PLATFORM_ANDROID_KHR 1
#define VK_USE_PLATFORM_WAYLAND_KHR 1

#include <vulkan/vulkan.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <string.h>

#ifdef WANT_VULKAN_X11_STUBS
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <X11/extensions/Xrandr.h>
#endif

#include <hybris/common/binding.h>
#include <hybris/common/floating_point_abi.h>
#include "config.h"
#include "logging.h"
#include "ws.h"

static void *vulkan_handle = NULL;

static void _init_androidvulkan()
{
    vulkan_handle = (void *) android_dlopen(getenv("LIBVULKAN") ? getenv("LIBVULKAN") : "libvulkan.so", RTLD_LAZY);
}

static inline void hybris_vulkan_initialize()
{
    _init_androidvulkan();
}

static void * _android_vulkan_dlsym(const char *symbol)
{
    if (vulkan_handle == NULL)
        _init_androidvulkan();

    return android_dlsym(vulkan_handle, symbol);
}

struct ws_vulkan_interface hybris_vulkan_interface = {
    _android_vulkan_dlsym,
};

static PFN_vkVoidFunction (*_vkGetInstanceProcAddr)(VkInstance instance, const char* pName) = NULL;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    if (_vkGetInstanceProcAddr == NULL) {
        HYBRIS_DLSYSM(vulkan, &_vkGetInstanceProcAddr, "vkGetInstanceProcAddr");
    }
    ws_vkSetInstanceProcAddrFunc((PFN_vkVoidFunction)_vkGetInstanceProcAddr);

    return ws_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyInstance( VkInstance  instance, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkInstance ,const VkAllocationCallbacks *))
        android_dlsym(vulkan_handle, "vkDestroyInstance"))
            (instance, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDevices( VkInstance  instance,  uint32_t * pPhysicalDeviceCount,  VkPhysicalDevice * pPhysicalDevices)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkInstance , uint32_t *, VkPhysicalDevice *))
        android_dlsym(vulkan_handle, "vkEnumeratePhysicalDevices"))
            (instance, pPhysicalDeviceCount, pPhysicalDevices);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceFeatures * pFeatures)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice , VkPhysicalDeviceFeatures *))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceFeatures"))
            (physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties( VkPhysicalDevice  physicalDevice,  VkFormat  format,  VkFormatProperties * pFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice , VkFormat , VkFormatProperties *))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceFormatProperties"))
            (physicalDevice, format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties( VkPhysicalDevice  physicalDevice,  VkFormat  format,  VkImageType  type,  VkImageTiling  tiling,  VkImageUsageFlags  usage,  VkImageCreateFlags  flags,  VkImageFormatProperties * pImageFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice , VkFormat , VkImageType , VkImageTiling , VkImageUsageFlags , VkImageCreateFlags , VkImageFormatProperties *))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceImageFormatProperties"))
            (physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceProperties * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice , VkPhysicalDeviceProperties *))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceProperties"))
            (physicalDevice, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties( VkPhysicalDevice  physicalDevice,  uint32_t * pQueueFamilyPropertyCount,  VkQueueFamilyProperties * pQueueFamilyProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice , uint32_t *, VkQueueFamilyProperties *))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceQueueFamilyProperties"))
            (physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceMemoryProperties * pMemoryProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice , VkPhysicalDeviceMemoryProperties *))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceMemoryProperties"))
            (physicalDevice, pMemoryProperties);
}

VkResult vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount, VkExtensionProperties* pProperties)
{
    if (_vkGetInstanceProcAddr == NULL) {
        HYBRIS_DLSYSM(vulkan, &_vkGetInstanceProcAddr, "vkGetInstanceProcAddr");
    }
    ws_vkSetInstanceProcAddrFunc((PFN_vkVoidFunction)_vkGetInstanceProcAddr);

    return ws_vkEnumerateInstanceExtensionProperties(pLayerName, pPropertyCount, pProperties);
}

#ifdef WANT_WAYLAND
VkResult vkCreateWaylandSurfaceKHR(VkInstance instance,
        const VkWaylandSurfaceCreateInfoKHR* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VkSurfaceKHR* pSurface)
{
    return ws_vkCreateWaylandSurfaceKHR(instance, pCreateInfo, pAllocator, pSurface);
}

VkBool32 vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex, struct wl_display* display)
{
    return ws_vkGetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex, display);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
    ws_vkDestroySurfaceKHR(instance, surface, pAllocator);
}
#else
VKAPI_ATTR void VKAPI_CALL vkDestroySurfaceKHR( VkInstance  instance,  VkSurfaceKHR  surface, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkInstance , VkSurfaceKHR ,const VkAllocationCallbacks *))
        android_dlsym(vulkan_handle, "vkDestroySurfaceKHR"))
            (instance, surface, pAllocator);
}
#endif

PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance instance, const char* pName)
{
    if (_vkGetInstanceProcAddr == NULL) {
        HYBRIS_DLSYSM(vulkan, &_vkGetInstanceProcAddr, "vkGetInstanceProcAddr");
    }

    if (!strcmp(pName, "vkEnumerateInstanceExtensionProperties")) {
        return (PFN_vkVoidFunction)vkEnumerateInstanceExtensionProperties;
    } else if (!strcmp(pName, "vkCreateInstance")) {
        return (PFN_vkVoidFunction)vkCreateInstance;
    } else if (!strcmp(pName, "vkGetInstanceProcAddr")) {
        return (PFN_vkVoidFunction)vkGetInstanceProcAddr;
#ifdef WANT_WAYLAND
    } else if (!strcmp(pName, "vkCreateWaylandSurfaceKHR")) {
        return (PFN_vkVoidFunction)vkCreateWaylandSurfaceKHR;
    } else if (!strcmp(pName, "vkGetPhysicalDeviceWaylandPresentationSupportKHR")) {
        return (PFN_vkVoidFunction)vkGetPhysicalDeviceWaylandPresentationSupportKHR;
    } else if (!strcmp(pName, "vkDestroySurfaceKHR")) {
        return (PFN_vkVoidFunction)vkDestroySurfaceKHR;
#endif
    }

    return (*_vkGetInstanceProcAddr)(instance, pName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr( VkDevice  device, const char * pName)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((PFN_vkVoidFunction (*)( VkDevice  ,const char * ))
        android_dlsym(vulkan_handle, "vkGetDeviceProcAddr"))
            (device, pName);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDevice( VkPhysicalDevice  physicalDevice, const VkDeviceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDevice * pDevice)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkDeviceCreateInfo * ,const VkAllocationCallbacks * , VkDevice * ))
        android_dlsym(vulkan_handle, "vkCreateDevice"))
            (physicalDevice, pCreateInfo, pAllocator, pDevice);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDevice( VkDevice  device, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDevice"))
            (device, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties( VkPhysicalDevice  physicalDevice, const char * pLayerName,  uint32_t * pPropertyCount,  VkExtensionProperties * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const char * , uint32_t * , VkExtensionProperties * ))
        android_dlsym(vulkan_handle, "vkEnumerateDeviceExtensionProperties"))
            (physicalDevice, pLayerName, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties( uint32_t * pPropertyCount,  VkLayerProperties * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( uint32_t * , VkLayerProperties * ))
        android_dlsym(vulkan_handle, "vkEnumerateInstanceLayerProperties"))
            (pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties( VkPhysicalDevice  physicalDevice,  uint32_t * pPropertyCount,  VkLayerProperties * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkLayerProperties * ))
        android_dlsym(vulkan_handle, "vkEnumerateDeviceLayerProperties"))
            (physicalDevice, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue( VkDevice  device,  uint32_t  queueFamilyIndex,  uint32_t  queueIndex,  VkQueue * pQueue)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , uint32_t  , uint32_t  , VkQueue * ))
        android_dlsym(vulkan_handle, "vkGetDeviceQueue"))
            (device, queueFamilyIndex, queueIndex, pQueue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit( VkQueue  queue,  uint32_t  submitCount, const VkSubmitInfo * pSubmits,  VkFence  fence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkQueue  , uint32_t  ,const VkSubmitInfo * , VkFence  ))
        android_dlsym(vulkan_handle, "vkQueueSubmit"))
            (queue, submitCount, pSubmits, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueWaitIdle( VkQueue  queue)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkQueue  ))
        android_dlsym(vulkan_handle, "vkQueueWaitIdle"))
            (queue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDeviceWaitIdle( VkDevice  device)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ))
        android_dlsym(vulkan_handle, "vkDeviceWaitIdle"))
            (device);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateMemory( VkDevice  device, const VkMemoryAllocateInfo * pAllocateInfo, const VkAllocationCallbacks * pAllocator,  VkDeviceMemory * pMemory)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkMemoryAllocateInfo * ,const VkAllocationCallbacks * , VkDeviceMemory * ))
        android_dlsym(vulkan_handle, "vkAllocateMemory"))
            (device, pAllocateInfo, pAllocator, pMemory);
}

VKAPI_ATTR void VKAPI_CALL vkFreeMemory( VkDevice  device,  VkDeviceMemory  memory, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDeviceMemory  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkFreeMemory"))
            (device, memory, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory( VkDevice  device,  VkDeviceMemory  memory,  VkDeviceSize  offset,  VkDeviceSize  size,  VkMemoryMapFlags  flags,  void ** ppData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeviceMemory  , VkDeviceSize  , VkDeviceSize  , VkMemoryMapFlags  , void ** ))
        android_dlsym(vulkan_handle, "vkMapMemory"))
            (device, memory, offset, size, flags, ppData);
}

VKAPI_ATTR void VKAPI_CALL vkUnmapMemory( VkDevice  device,  VkDeviceMemory  memory)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDeviceMemory  ))
        android_dlsym(vulkan_handle, "vkUnmapMemory"))
            (device, memory);
}

VKAPI_ATTR VkResult VKAPI_CALL vkFlushMappedMemoryRanges( VkDevice  device,  uint32_t  memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkMappedMemoryRange * ))
        android_dlsym(vulkan_handle, "vkFlushMappedMemoryRanges"))
            (device, memoryRangeCount, pMemoryRanges);
}

VKAPI_ATTR VkResult VKAPI_CALL vkInvalidateMappedMemoryRanges( VkDevice  device,  uint32_t  memoryRangeCount, const VkMappedMemoryRange * pMemoryRanges)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkMappedMemoryRange * ))
        android_dlsym(vulkan_handle, "vkInvalidateMappedMemoryRanges"))
            (device, memoryRangeCount, pMemoryRanges);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceMemoryCommitment( VkDevice  device,  VkDeviceMemory  memory,  VkDeviceSize * pCommittedMemoryInBytes)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDeviceMemory  , VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkGetDeviceMemoryCommitment"))
            (device, memory, pCommittedMemoryInBytes);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory( VkDevice  device,  VkBuffer  buffer,  VkDeviceMemory  memory,  VkDeviceSize  memoryOffset)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkBuffer  , VkDeviceMemory  , VkDeviceSize  ))
        android_dlsym(vulkan_handle, "vkBindBufferMemory"))
            (device, buffer, memory, memoryOffset);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory( VkDevice  device,  VkImage  image,  VkDeviceMemory  memory,  VkDeviceSize  memoryOffset)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkImage  , VkDeviceMemory  , VkDeviceSize  ))
        android_dlsym(vulkan_handle, "vkBindImageMemory"))
            (device, image, memory, memoryOffset);
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements( VkDevice  device,  VkBuffer  buffer,  VkMemoryRequirements * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkBuffer  , VkMemoryRequirements * ))
        android_dlsym(vulkan_handle, "vkGetBufferMemoryRequirements"))
            (device, buffer, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements( VkDevice  device,  VkImage  image,  VkMemoryRequirements * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkImage  , VkMemoryRequirements * ))
        android_dlsym(vulkan_handle, "vkGetImageMemoryRequirements"))
            (device, image, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements( VkDevice  device,  VkImage  image,  uint32_t * pSparseMemoryRequirementCount,  VkSparseImageMemoryRequirements * pSparseMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkImage  , uint32_t * , VkSparseImageMemoryRequirements * ))
        android_dlsym(vulkan_handle, "vkGetImageSparseMemoryRequirements"))
            (device, image, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties( VkPhysicalDevice  physicalDevice,  VkFormat  format,  VkImageType  type,  VkSampleCountFlagBits  samples,  VkImageUsageFlags  usage,  VkImageTiling  tiling,  uint32_t * pPropertyCount,  VkSparseImageFormatProperties * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkFormat  , VkImageType  , VkSampleCountFlagBits  , VkImageUsageFlags  , VkImageTiling  , uint32_t * , VkSparseImageFormatProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSparseImageFormatProperties"))
            (physicalDevice, format, type, samples, usage, tiling, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueBindSparse( VkQueue  queue,  uint32_t  bindInfoCount, const VkBindSparseInfo * pBindInfo,  VkFence  fence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkQueue  , uint32_t  ,const VkBindSparseInfo * , VkFence  ))
        android_dlsym(vulkan_handle, "vkQueueBindSparse"))
            (queue, bindInfoCount, pBindInfo, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFence( VkDevice  device, const VkFenceCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkFence * pFence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkFenceCreateInfo * ,const VkAllocationCallbacks * , VkFence * ))
        android_dlsym(vulkan_handle, "vkCreateFence"))
            (device, pCreateInfo, pAllocator, pFence);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFence( VkDevice  device,  VkFence  fence, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkFence  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyFence"))
            (device, fence, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetFences( VkDevice  device,  uint32_t  fenceCount, const VkFence * pFences)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkFence * ))
        android_dlsym(vulkan_handle, "vkResetFences"))
            (device, fenceCount, pFences);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceStatus( VkDevice  device,  VkFence  fence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkFence  ))
        android_dlsym(vulkan_handle, "vkGetFenceStatus"))
            (device, fence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitForFences( VkDevice  device,  uint32_t  fenceCount, const VkFence * pFences,  VkBool32  waitAll,  uint64_t  timeout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkFence * , VkBool32  , uint64_t  ))
        android_dlsym(vulkan_handle, "vkWaitForFences"))
            (device, fenceCount, pFences, waitAll, timeout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSemaphore( VkDevice  device, const VkSemaphoreCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkSemaphore * pSemaphore)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSemaphoreCreateInfo * ,const VkAllocationCallbacks * , VkSemaphore * ))
        android_dlsym(vulkan_handle, "vkCreateSemaphore"))
            (device, pCreateInfo, pAllocator, pSemaphore);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySemaphore( VkDevice  device,  VkSemaphore  semaphore, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSemaphore  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroySemaphore"))
            (device, semaphore, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateEvent( VkDevice  device, const VkEventCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkEvent * pEvent)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkEventCreateInfo * ,const VkAllocationCallbacks * , VkEvent * ))
        android_dlsym(vulkan_handle, "vkCreateEvent"))
            (device, pCreateInfo, pAllocator, pEvent);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyEvent( VkDevice  device,  VkEvent  event, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkEvent  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyEvent"))
            (device, event, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetEventStatus( VkDevice  device,  VkEvent  event)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkEvent  ))
        android_dlsym(vulkan_handle, "vkGetEventStatus"))
            (device, event);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetEvent( VkDevice  device,  VkEvent  event)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkEvent  ))
        android_dlsym(vulkan_handle, "vkSetEvent"))
            (device, event);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetEvent( VkDevice  device,  VkEvent  event)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkEvent  ))
        android_dlsym(vulkan_handle, "vkResetEvent"))
            (device, event);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateQueryPool( VkDevice  device, const VkQueryPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkQueryPool * pQueryPool)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkQueryPoolCreateInfo * ,const VkAllocationCallbacks * , VkQueryPool * ))
        android_dlsym(vulkan_handle, "vkCreateQueryPool"))
            (device, pCreateInfo, pAllocator, pQueryPool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyQueryPool( VkDevice  device,  VkQueryPool  queryPool, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkQueryPool  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyQueryPool"))
            (device, queryPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetQueryPoolResults( VkDevice  device,  VkQueryPool  queryPool,  uint32_t  firstQuery,  uint32_t  queryCount,  size_t  dataSize,  void * pData,  VkDeviceSize  stride,  VkQueryResultFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkQueryPool  , uint32_t  , uint32_t  , size_t  , void * , VkDeviceSize  , VkQueryResultFlags  ))
        android_dlsym(vulkan_handle, "vkGetQueryPoolResults"))
            (device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBuffer( VkDevice  device, const VkBufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkBuffer * pBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkBufferCreateInfo * ,const VkAllocationCallbacks * , VkBuffer * ))
        android_dlsym(vulkan_handle, "vkCreateBuffer"))
            (device, pCreateInfo, pAllocator, pBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBuffer( VkDevice  device,  VkBuffer  buffer, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkBuffer  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyBuffer"))
            (device, buffer, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateBufferView( VkDevice  device, const VkBufferViewCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkBufferView * pView)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkBufferViewCreateInfo * ,const VkAllocationCallbacks * , VkBufferView * ))
        android_dlsym(vulkan_handle, "vkCreateBufferView"))
            (device, pCreateInfo, pAllocator, pView);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyBufferView( VkDevice  device,  VkBufferView  bufferView, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkBufferView  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyBufferView"))
            (device, bufferView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImage( VkDevice  device, const VkImageCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkImage * pImage)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkImageCreateInfo * ,const VkAllocationCallbacks * , VkImage * ))
        android_dlsym(vulkan_handle, "vkCreateImage"))
            (device, pCreateInfo, pAllocator, pImage);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImage( VkDevice  device,  VkImage  image, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkImage  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyImage"))
            (device, image, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout( VkDevice  device,  VkImage  image, const VkImageSubresource * pSubresource,  VkSubresourceLayout * pLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkImage  ,const VkImageSubresource * , VkSubresourceLayout * ))
        android_dlsym(vulkan_handle, "vkGetImageSubresourceLayout"))
            (device, image, pSubresource, pLayout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateImageView( VkDevice  device, const VkImageViewCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkImageView * pView)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkImageViewCreateInfo * ,const VkAllocationCallbacks * , VkImageView * ))
        android_dlsym(vulkan_handle, "vkCreateImageView"))
            (device, pCreateInfo, pAllocator, pView);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyImageView( VkDevice  device,  VkImageView  imageView, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkImageView  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyImageView"))
            (device, imageView, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateShaderModule( VkDevice  device, const VkShaderModuleCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkShaderModule * pShaderModule)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkShaderModuleCreateInfo * ,const VkAllocationCallbacks * , VkShaderModule * ))
        android_dlsym(vulkan_handle, "vkCreateShaderModule"))
            (device, pCreateInfo, pAllocator, pShaderModule);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyShaderModule( VkDevice  device,  VkShaderModule  shaderModule, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkShaderModule  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyShaderModule"))
            (device, shaderModule, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineCache( VkDevice  device, const VkPipelineCacheCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkPipelineCache * pPipelineCache)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPipelineCacheCreateInfo * ,const VkAllocationCallbacks * , VkPipelineCache * ))
        android_dlsym(vulkan_handle, "vkCreatePipelineCache"))
            (device, pCreateInfo, pAllocator, pPipelineCache);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineCache( VkDevice  device,  VkPipelineCache  pipelineCache, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkPipelineCache  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyPipelineCache"))
            (device, pipelineCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineCacheData( VkDevice  device,  VkPipelineCache  pipelineCache,  size_t * pDataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipelineCache  , size_t * , void * ))
        android_dlsym(vulkan_handle, "vkGetPipelineCacheData"))
            (device, pipelineCache, pDataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMergePipelineCaches( VkDevice  device,  VkPipelineCache  dstCache,  uint32_t  srcCacheCount, const VkPipelineCache * pSrcCaches)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipelineCache  , uint32_t  ,const VkPipelineCache * ))
        android_dlsym(vulkan_handle, "vkMergePipelineCaches"))
            (device, dstCache, srcCacheCount, pSrcCaches);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateGraphicsPipelines( VkDevice  device,  VkPipelineCache  pipelineCache,  uint32_t  createInfoCount, const VkGraphicsPipelineCreateInfo * pCreateInfos, const VkAllocationCallbacks * pAllocator,  VkPipeline * pPipelines)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipelineCache  , uint32_t  ,const VkGraphicsPipelineCreateInfo * ,const VkAllocationCallbacks * , VkPipeline * ))
        android_dlsym(vulkan_handle, "vkCreateGraphicsPipelines"))
            (device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateComputePipelines( VkDevice  device,  VkPipelineCache  pipelineCache,  uint32_t  createInfoCount, const VkComputePipelineCreateInfo * pCreateInfos, const VkAllocationCallbacks * pAllocator,  VkPipeline * pPipelines)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipelineCache  , uint32_t  ,const VkComputePipelineCreateInfo * ,const VkAllocationCallbacks * , VkPipeline * ))
        android_dlsym(vulkan_handle, "vkCreateComputePipelines"))
            (device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipeline( VkDevice  device,  VkPipeline  pipeline, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkPipeline  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyPipeline"))
            (device, pipeline, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePipelineLayout( VkDevice  device, const VkPipelineLayoutCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkPipelineLayout * pPipelineLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPipelineLayoutCreateInfo * ,const VkAllocationCallbacks * , VkPipelineLayout * ))
        android_dlsym(vulkan_handle, "vkCreatePipelineLayout"))
            (device, pCreateInfo, pAllocator, pPipelineLayout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPipelineLayout( VkDevice  device,  VkPipelineLayout  pipelineLayout, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkPipelineLayout  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyPipelineLayout"))
            (device, pipelineLayout, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSampler( VkDevice  device, const VkSamplerCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkSampler * pSampler)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSamplerCreateInfo * ,const VkAllocationCallbacks * , VkSampler * ))
        android_dlsym(vulkan_handle, "vkCreateSampler"))
            (device, pCreateInfo, pAllocator, pSampler);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySampler( VkDevice  device,  VkSampler  sampler, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSampler  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroySampler"))
            (device, sampler, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorSetLayout( VkDevice  device, const VkDescriptorSetLayoutCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDescriptorSetLayout * pSetLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDescriptorSetLayoutCreateInfo * ,const VkAllocationCallbacks * , VkDescriptorSetLayout * ))
        android_dlsym(vulkan_handle, "vkCreateDescriptorSetLayout"))
            (device, pCreateInfo, pAllocator, pSetLayout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorSetLayout( VkDevice  device,  VkDescriptorSetLayout  descriptorSetLayout, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorSetLayout  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDescriptorSetLayout"))
            (device, descriptorSetLayout, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorPool( VkDevice  device, const VkDescriptorPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDescriptorPool * pDescriptorPool)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDescriptorPoolCreateInfo * ,const VkAllocationCallbacks * , VkDescriptorPool * ))
        android_dlsym(vulkan_handle, "vkCreateDescriptorPool"))
            (device, pCreateInfo, pAllocator, pDescriptorPool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorPool( VkDevice  device,  VkDescriptorPool  descriptorPool, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorPool  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDescriptorPool"))
            (device, descriptorPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetDescriptorPool( VkDevice  device,  VkDescriptorPool  descriptorPool,  VkDescriptorPoolResetFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDescriptorPool  , VkDescriptorPoolResetFlags  ))
        android_dlsym(vulkan_handle, "vkResetDescriptorPool"))
            (device, descriptorPool, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateDescriptorSets( VkDevice  device, const VkDescriptorSetAllocateInfo * pAllocateInfo,  VkDescriptorSet * pDescriptorSets)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDescriptorSetAllocateInfo * , VkDescriptorSet * ))
        android_dlsym(vulkan_handle, "vkAllocateDescriptorSets"))
            (device, pAllocateInfo, pDescriptorSets);
}

VKAPI_ATTR VkResult VKAPI_CALL vkFreeDescriptorSets( VkDevice  device,  VkDescriptorPool  descriptorPool,  uint32_t  descriptorSetCount, const VkDescriptorSet * pDescriptorSets)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDescriptorPool  , uint32_t  ,const VkDescriptorSet * ))
        android_dlsym(vulkan_handle, "vkFreeDescriptorSets"))
            (device, descriptorPool, descriptorSetCount, pDescriptorSets);
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSets( VkDevice  device,  uint32_t  descriptorWriteCount, const VkWriteDescriptorSet * pDescriptorWrites,  uint32_t  descriptorCopyCount, const VkCopyDescriptorSet * pDescriptorCopies)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , uint32_t  ,const VkWriteDescriptorSet * , uint32_t  ,const VkCopyDescriptorSet * ))
        android_dlsym(vulkan_handle, "vkUpdateDescriptorSets"))
            (device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateFramebuffer( VkDevice  device, const VkFramebufferCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkFramebuffer * pFramebuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkFramebufferCreateInfo * ,const VkAllocationCallbacks * , VkFramebuffer * ))
        android_dlsym(vulkan_handle, "vkCreateFramebuffer"))
            (device, pCreateInfo, pAllocator, pFramebuffer);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyFramebuffer( VkDevice  device,  VkFramebuffer  framebuffer, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkFramebuffer  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyFramebuffer"))
            (device, framebuffer, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass( VkDevice  device, const VkRenderPassCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkRenderPass * pRenderPass)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkRenderPassCreateInfo * ,const VkAllocationCallbacks * , VkRenderPass * ))
        android_dlsym(vulkan_handle, "vkCreateRenderPass"))
            (device, pCreateInfo, pAllocator, pRenderPass);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyRenderPass( VkDevice  device,  VkRenderPass  renderPass, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkRenderPass  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyRenderPass"))
            (device, renderPass, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetRenderAreaGranularity( VkDevice  device,  VkRenderPass  renderPass,  VkExtent2D * pGranularity)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkRenderPass  , VkExtent2D * ))
        android_dlsym(vulkan_handle, "vkGetRenderAreaGranularity"))
            (device, renderPass, pGranularity);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCommandPool( VkDevice  device, const VkCommandPoolCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkCommandPool * pCommandPool)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCommandPoolCreateInfo * ,const VkAllocationCallbacks * , VkCommandPool * ))
        android_dlsym(vulkan_handle, "vkCreateCommandPool"))
            (device, pCreateInfo, pAllocator, pCommandPool);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCommandPool( VkDevice  device,  VkCommandPool  commandPool, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCommandPool  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyCommandPool"))
            (device, commandPool, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandPool( VkDevice  device,  VkCommandPool  commandPool,  VkCommandPoolResetFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkCommandPool  , VkCommandPoolResetFlags  ))
        android_dlsym(vulkan_handle, "vkResetCommandPool"))
            (device, commandPool, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAllocateCommandBuffers( VkDevice  device, const VkCommandBufferAllocateInfo * pAllocateInfo,  VkCommandBuffer * pCommandBuffers)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCommandBufferAllocateInfo * , VkCommandBuffer * ))
        android_dlsym(vulkan_handle, "vkAllocateCommandBuffers"))
            (device, pAllocateInfo, pCommandBuffers);
}

VKAPI_ATTR void VKAPI_CALL vkFreeCommandBuffers( VkDevice  device,  VkCommandPool  commandPool,  uint32_t  commandBufferCount, const VkCommandBuffer * pCommandBuffers)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCommandPool  , uint32_t  ,const VkCommandBuffer * ))
        android_dlsym(vulkan_handle, "vkFreeCommandBuffers"))
            (device, commandPool, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBeginCommandBuffer( VkCommandBuffer  commandBuffer, const VkCommandBufferBeginInfo * pBeginInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkCommandBuffer  ,const VkCommandBufferBeginInfo * ))
        android_dlsym(vulkan_handle, "vkBeginCommandBuffer"))
            (commandBuffer, pBeginInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEndCommandBuffer( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkEndCommandBuffer"))
            (commandBuffer);
}

VKAPI_ATTR VkResult VKAPI_CALL vkResetCommandBuffer( VkCommandBuffer  commandBuffer,  VkCommandBufferResetFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkCommandBuffer  , VkCommandBufferResetFlags  ))
        android_dlsym(vulkan_handle, "vkResetCommandBuffer"))
            (commandBuffer, flags);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindPipeline( VkCommandBuffer  commandBuffer,  VkPipelineBindPoint  pipelineBindPoint,  VkPipeline  pipeline)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineBindPoint  , VkPipeline  ))
        android_dlsym(vulkan_handle, "vkCmdBindPipeline"))
            (commandBuffer, pipelineBindPoint, pipeline);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport( VkCommandBuffer  commandBuffer,  uint32_t  firstViewport,  uint32_t  viewportCount, const VkViewport * pViewports)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkViewport * ))
        android_dlsym(vulkan_handle, "vkCmdSetViewport"))
            (commandBuffer, firstViewport, viewportCount, pViewports);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor( VkCommandBuffer  commandBuffer,  uint32_t  firstScissor,  uint32_t  scissorCount, const VkRect2D * pScissors)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkRect2D * ))
        android_dlsym(vulkan_handle, "vkCmdSetScissor"))
            (commandBuffer, firstScissor, scissorCount, pScissors);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth( VkCommandBuffer  commandBuffer,  float  lineWidth)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , float  ))
        android_dlsym(vulkan_handle, "vkCmdSetLineWidth"))
            (commandBuffer, lineWidth);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias( VkCommandBuffer  commandBuffer,  float  depthBiasConstantFactor,  float  depthBiasClamp,  float  depthBiasSlopeFactor)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , float  , float  , float  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthBias"))
            (commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants( VkCommandBuffer  commandBuffer, const float  blendConstants[4])
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const float  [4]))
        android_dlsym(vulkan_handle, "vkCmdSetBlendConstants"))
            (commandBuffer, blendConstants);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds( VkCommandBuffer  commandBuffer,  float  minDepthBounds,  float  maxDepthBounds)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , float  , float  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthBounds"))
            (commandBuffer, minDepthBounds, maxDepthBounds);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask( VkCommandBuffer  commandBuffer,  VkStencilFaceFlags  faceMask,  uint32_t  compareMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkStencilFaceFlags  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetStencilCompareMask"))
            (commandBuffer, faceMask, compareMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask( VkCommandBuffer  commandBuffer,  VkStencilFaceFlags  faceMask,  uint32_t  writeMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkStencilFaceFlags  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetStencilWriteMask"))
            (commandBuffer, faceMask, writeMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference( VkCommandBuffer  commandBuffer,  VkStencilFaceFlags  faceMask,  uint32_t  reference)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkStencilFaceFlags  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetStencilReference"))
            (commandBuffer, faceMask, reference);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets( VkCommandBuffer  commandBuffer,  VkPipelineBindPoint  pipelineBindPoint,  VkPipelineLayout  layout,  uint32_t  firstSet,  uint32_t  descriptorSetCount, const VkDescriptorSet * pDescriptorSets,  uint32_t  dynamicOffsetCount, const uint32_t * pDynamicOffsets)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineBindPoint  , VkPipelineLayout  , uint32_t  , uint32_t  ,const VkDescriptorSet * , uint32_t  ,const uint32_t * ))
        android_dlsym(vulkan_handle, "vkCmdBindDescriptorSets"))
            (commandBuffer, pipelineBindPoint, layout, firstSet, descriptorSetCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkIndexType  indexType)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkIndexType  ))
        android_dlsym(vulkan_handle, "vkCmdBindIndexBuffer"))
            (commandBuffer, buffer, offset, indexType);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers( VkCommandBuffer  commandBuffer,  uint32_t  firstBinding,  uint32_t  bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBuffer * ,const VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkCmdBindVertexBuffers"))
            (commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDraw( VkCommandBuffer  commandBuffer,  uint32_t  vertexCount,  uint32_t  instanceCount,  uint32_t  firstVertex,  uint32_t  firstInstance)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDraw"))
            (commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexed( VkCommandBuffer  commandBuffer,  uint32_t  indexCount,  uint32_t  instanceCount,  uint32_t  firstIndex,  int32_t  vertexOffset,  uint32_t  firstInstance)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , uint32_t  , int32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndexed"))
            (commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirect( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  uint32_t  drawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndirect"))
            (commandBuffer, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirect( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  uint32_t  drawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndexedIndirect"))
            (commandBuffer, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatch( VkCommandBuffer  commandBuffer,  uint32_t  groupCountX,  uint32_t  groupCountY,  uint32_t  groupCountZ)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDispatch"))
            (commandBuffer, groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchIndirect( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  ))
        android_dlsym(vulkan_handle, "vkCmdDispatchIndirect"))
            (commandBuffer, buffer, offset);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer( VkCommandBuffer  commandBuffer,  VkBuffer  srcBuffer,  VkBuffer  dstBuffer,  uint32_t  regionCount, const VkBufferCopy * pRegions)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkBuffer  , uint32_t  ,const VkBufferCopy * ))
        android_dlsym(vulkan_handle, "vkCmdCopyBuffer"))
            (commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage( VkCommandBuffer  commandBuffer,  VkImage  srcImage,  VkImageLayout  srcImageLayout,  VkImage  dstImage,  VkImageLayout  dstImageLayout,  uint32_t  regionCount, const VkImageCopy * pRegions)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImage  , VkImageLayout  , VkImage  , VkImageLayout  , uint32_t  ,const VkImageCopy * ))
        android_dlsym(vulkan_handle, "vkCmdCopyImage"))
            (commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage( VkCommandBuffer  commandBuffer,  VkImage  srcImage,  VkImageLayout  srcImageLayout,  VkImage  dstImage,  VkImageLayout  dstImageLayout,  uint32_t  regionCount, const VkImageBlit * pRegions,  VkFilter  filter)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImage  , VkImageLayout  , VkImage  , VkImageLayout  , uint32_t  ,const VkImageBlit * , VkFilter  ))
        android_dlsym(vulkan_handle, "vkCmdBlitImage"))
            (commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions, filter);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage( VkCommandBuffer  commandBuffer,  VkBuffer  srcBuffer,  VkImage  dstImage,  VkImageLayout  dstImageLayout,  uint32_t  regionCount, const VkBufferImageCopy * pRegions)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkImage  , VkImageLayout  , uint32_t  ,const VkBufferImageCopy * ))
        android_dlsym(vulkan_handle, "vkCmdCopyBufferToImage"))
            (commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer( VkCommandBuffer  commandBuffer,  VkImage  srcImage,  VkImageLayout  srcImageLayout,  VkBuffer  dstBuffer,  uint32_t  regionCount, const VkBufferImageCopy * pRegions)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImage  , VkImageLayout  , VkBuffer  , uint32_t  ,const VkBufferImageCopy * ))
        android_dlsym(vulkan_handle, "vkCmdCopyImageToBuffer"))
            (commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdUpdateBuffer( VkCommandBuffer  commandBuffer,  VkBuffer  dstBuffer,  VkDeviceSize  dstOffset,  VkDeviceSize  dataSize, const void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkDeviceSize  ,const void * ))
        android_dlsym(vulkan_handle, "vkCmdUpdateBuffer"))
            (commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdFillBuffer( VkCommandBuffer  commandBuffer,  VkBuffer  dstBuffer,  VkDeviceSize  dstOffset,  VkDeviceSize  size,  uint32_t  data)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkDeviceSize  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdFillBuffer"))
            (commandBuffer, dstBuffer, dstOffset, size, data);
}

VKAPI_ATTR void VKAPI_CALL vkCmdClearColorImage( VkCommandBuffer  commandBuffer,  VkImage  image,  VkImageLayout  imageLayout, const VkClearColorValue * pColor,  uint32_t  rangeCount, const VkImageSubresourceRange * pRanges)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImage  , VkImageLayout  ,const VkClearColorValue * , uint32_t  ,const VkImageSubresourceRange * ))
        android_dlsym(vulkan_handle, "vkCmdClearColorImage"))
            (commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL vkCmdClearDepthStencilImage( VkCommandBuffer  commandBuffer,  VkImage  image,  VkImageLayout  imageLayout, const VkClearDepthStencilValue * pDepthStencil,  uint32_t  rangeCount, const VkImageSubresourceRange * pRanges)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImage  , VkImageLayout  ,const VkClearDepthStencilValue * , uint32_t  ,const VkImageSubresourceRange * ))
        android_dlsym(vulkan_handle, "vkCmdClearDepthStencilImage"))
            (commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

VKAPI_ATTR void VKAPI_CALL vkCmdClearAttachments( VkCommandBuffer  commandBuffer,  uint32_t  attachmentCount, const VkClearAttachment * pAttachments,  uint32_t  rectCount, const VkClearRect * pRects)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkClearAttachment * , uint32_t  ,const VkClearRect * ))
        android_dlsym(vulkan_handle, "vkCmdClearAttachments"))
            (commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage( VkCommandBuffer  commandBuffer,  VkImage  srcImage,  VkImageLayout  srcImageLayout,  VkImage  dstImage,  VkImageLayout  dstImageLayout,  uint32_t  regionCount, const VkImageResolve * pRegions)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImage  , VkImageLayout  , VkImage  , VkImageLayout  , uint32_t  ,const VkImageResolve * ))
        android_dlsym(vulkan_handle, "vkCmdResolveImage"))
            (commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount, pRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent( VkCommandBuffer  commandBuffer,  VkEvent  event,  VkPipelineStageFlags  stageMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkEvent  , VkPipelineStageFlags  ))
        android_dlsym(vulkan_handle, "vkCmdSetEvent"))
            (commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent( VkCommandBuffer  commandBuffer,  VkEvent  event,  VkPipelineStageFlags  stageMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkEvent  , VkPipelineStageFlags  ))
        android_dlsym(vulkan_handle, "vkCmdResetEvent"))
            (commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents( VkCommandBuffer  commandBuffer,  uint32_t  eventCount, const VkEvent * pEvents,  VkPipelineStageFlags  srcStageMask,  VkPipelineStageFlags  dstStageMask,  uint32_t  memoryBarrierCount, const VkMemoryBarrier * pMemoryBarriers,  uint32_t  bufferMemoryBarrierCount, const VkBufferMemoryBarrier * pBufferMemoryBarriers,  uint32_t  imageMemoryBarrierCount, const VkImageMemoryBarrier * pImageMemoryBarriers)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkEvent * , VkPipelineStageFlags  , VkPipelineStageFlags  , uint32_t  ,const VkMemoryBarrier * , uint32_t  ,const VkBufferMemoryBarrier * , uint32_t  ,const VkImageMemoryBarrier * ))
        android_dlsym(vulkan_handle, "vkCmdWaitEvents"))
            (commandBuffer, eventCount, pEvents, srcStageMask, dstStageMask, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier( VkCommandBuffer  commandBuffer,  VkPipelineStageFlags  srcStageMask,  VkPipelineStageFlags  dstStageMask,  VkDependencyFlags  dependencyFlags,  uint32_t  memoryBarrierCount, const VkMemoryBarrier * pMemoryBarriers,  uint32_t  bufferMemoryBarrierCount, const VkBufferMemoryBarrier * pBufferMemoryBarriers,  uint32_t  imageMemoryBarrierCount, const VkImageMemoryBarrier * pImageMemoryBarriers)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineStageFlags  , VkPipelineStageFlags  , VkDependencyFlags  , uint32_t  ,const VkMemoryBarrier * , uint32_t  ,const VkBufferMemoryBarrier * , uint32_t  ,const VkImageMemoryBarrier * ))
        android_dlsym(vulkan_handle, "vkCmdPipelineBarrier"))
            (commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginQuery( VkCommandBuffer  commandBuffer,  VkQueryPool  queryPool,  uint32_t  query,  VkQueryControlFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkQueryPool  , uint32_t  , VkQueryControlFlags  ))
        android_dlsym(vulkan_handle, "vkCmdBeginQuery"))
            (commandBuffer, queryPool, query, flags);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndQuery( VkCommandBuffer  commandBuffer,  VkQueryPool  queryPool,  uint32_t  query)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkQueryPool  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdEndQuery"))
            (commandBuffer, queryPool, query);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetQueryPool( VkCommandBuffer  commandBuffer,  VkQueryPool  queryPool,  uint32_t  firstQuery,  uint32_t  queryCount)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkQueryPool  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdResetQueryPool"))
            (commandBuffer, queryPool, firstQuery, queryCount);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp( VkCommandBuffer  commandBuffer,  VkPipelineStageFlagBits  pipelineStage,  VkQueryPool  queryPool,  uint32_t  query)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineStageFlagBits  , VkQueryPool  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteTimestamp"))
            (commandBuffer, pipelineStage, queryPool, query);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyQueryPoolResults( VkCommandBuffer  commandBuffer,  VkQueryPool  queryPool,  uint32_t  firstQuery,  uint32_t  queryCount,  VkBuffer  dstBuffer,  VkDeviceSize  dstOffset,  VkDeviceSize  stride,  VkQueryResultFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkQueryPool  , uint32_t  , uint32_t  , VkBuffer  , VkDeviceSize  , VkDeviceSize  , VkQueryResultFlags  ))
        android_dlsym(vulkan_handle, "vkCmdCopyQueryPoolResults"))
            (commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset, stride, flags);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants( VkCommandBuffer  commandBuffer,  VkPipelineLayout  layout,  VkShaderStageFlags  stageFlags,  uint32_t  offset,  uint32_t  size, const void * pValues)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineLayout  , VkShaderStageFlags  , uint32_t  , uint32_t  ,const void * ))
        android_dlsym(vulkan_handle, "vkCmdPushConstants"))
            (commandBuffer, layout, stageFlags, offset, size, pValues);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass( VkCommandBuffer  commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin,  VkSubpassContents  contents)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkRenderPassBeginInfo * , VkSubpassContents  ))
        android_dlsym(vulkan_handle, "vkCmdBeginRenderPass"))
            (commandBuffer, pRenderPassBegin, contents);
}

VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass( VkCommandBuffer  commandBuffer,  VkSubpassContents  contents)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkSubpassContents  ))
        android_dlsym(vulkan_handle, "vkCmdNextSubpass"))
            (commandBuffer, contents);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkCmdEndRenderPass"))
            (commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdExecuteCommands( VkCommandBuffer  commandBuffer,  uint32_t  commandBufferCount, const VkCommandBuffer * pCommandBuffers)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkCommandBuffer * ))
        android_dlsym(vulkan_handle, "vkCmdExecuteCommands"))
            (commandBuffer, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceVersion( uint32_t * pApiVersion)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( uint32_t * ))
        android_dlsym(vulkan_handle, "vkEnumerateInstanceVersion"))
            (pApiVersion);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory2( VkDevice  device,  uint32_t  bindInfoCount, const VkBindBufferMemoryInfo * pBindInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkBindBufferMemoryInfo * ))
        android_dlsym(vulkan_handle, "vkBindBufferMemory2"))
            (device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory2( VkDevice  device,  uint32_t  bindInfoCount, const VkBindImageMemoryInfo * pBindInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkBindImageMemoryInfo * ))
        android_dlsym(vulkan_handle, "vkBindImageMemory2"))
            (device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceGroupPeerMemoryFeatures( VkDevice  device,  uint32_t  heapIndex,  uint32_t  localDeviceIndex,  uint32_t  remoteDeviceIndex,  VkPeerMemoryFeatureFlags * pPeerMemoryFeatures)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , uint32_t  , uint32_t  , uint32_t  , VkPeerMemoryFeatureFlags * ))
        android_dlsym(vulkan_handle, "vkGetDeviceGroupPeerMemoryFeatures"))
            (device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMask( VkCommandBuffer  commandBuffer,  uint32_t  deviceMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetDeviceMask"))
            (commandBuffer, deviceMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchBase( VkCommandBuffer  commandBuffer,  uint32_t  baseGroupX,  uint32_t  baseGroupY,  uint32_t  baseGroupZ,  uint32_t  groupCountX,  uint32_t  groupCountY,  uint32_t  groupCountZ)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , uint32_t  , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDispatchBase"))
            (commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroups( VkInstance  instance,  uint32_t * pPhysicalDeviceGroupCount,  VkPhysicalDeviceGroupProperties * pPhysicalDeviceGroupProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkInstance  , uint32_t * , VkPhysicalDeviceGroupProperties * ))
        android_dlsym(vulkan_handle, "vkEnumeratePhysicalDeviceGroups"))
            (instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2( VkDevice  device, const VkImageMemoryRequirementsInfo2 * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkImageMemoryRequirementsInfo2 * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetImageMemoryRequirements2"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements2( VkDevice  device, const VkBufferMemoryRequirementsInfo2 * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkBufferMemoryRequirementsInfo2 * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetBufferMemoryRequirements2"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements2( VkDevice  device, const VkImageSparseMemoryRequirementsInfo2 * pInfo,  uint32_t * pSparseMemoryRequirementCount,  VkSparseImageMemoryRequirements2 * pSparseMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkImageSparseMemoryRequirementsInfo2 * , uint32_t * , VkSparseImageMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetImageSparseMemoryRequirements2"))
            (device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceFeatures2 * pFeatures)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkPhysicalDeviceFeatures2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceFeatures2"))
            (physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceProperties2 * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkPhysicalDeviceProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceProperties2"))
            (physicalDevice, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2( VkPhysicalDevice  physicalDevice,  VkFormat  format,  VkFormatProperties2 * pFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkFormat  , VkFormatProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceFormatProperties2"))
            (physicalDevice, format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceImageFormatInfo2 * pImageFormatInfo,  VkImageFormatProperties2 * pImageFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkPhysicalDeviceImageFormatInfo2 * , VkImageFormatProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceImageFormatProperties2"))
            (physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2( VkPhysicalDevice  physicalDevice,  uint32_t * pQueueFamilyPropertyCount,  VkQueueFamilyProperties2 * pQueueFamilyProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , uint32_t * , VkQueueFamilyProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceQueueFamilyProperties2"))
            (physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties2( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceMemoryProperties2 * pMemoryProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkPhysicalDeviceMemoryProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceMemoryProperties2"))
            (physicalDevice, pMemoryProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties2( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 * pFormatInfo,  uint32_t * pPropertyCount,  VkSparseImageFormatProperties2 * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceSparseImageFormatInfo2 * , uint32_t * , VkSparseImageFormatProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSparseImageFormatProperties2"))
            (physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkTrimCommandPool( VkDevice  device,  VkCommandPool  commandPool,  VkCommandPoolTrimFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCommandPool  , VkCommandPoolTrimFlags  ))
        android_dlsym(vulkan_handle, "vkTrimCommandPool"))
            (device, commandPool, flags);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceQueue2( VkDevice  device, const VkDeviceQueueInfo2 * pQueueInfo,  VkQueue * pQueue)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceQueueInfo2 * , VkQueue * ))
        android_dlsym(vulkan_handle, "vkGetDeviceQueue2"))
            (device, pQueueInfo, pQueue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSamplerYcbcrConversion( VkDevice  device, const VkSamplerYcbcrConversionCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkSamplerYcbcrConversion * pYcbcrConversion)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSamplerYcbcrConversionCreateInfo * ,const VkAllocationCallbacks * , VkSamplerYcbcrConversion * ))
        android_dlsym(vulkan_handle, "vkCreateSamplerYcbcrConversion"))
            (device, pCreateInfo, pAllocator, pYcbcrConversion);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySamplerYcbcrConversion( VkDevice  device,  VkSamplerYcbcrConversion  ycbcrConversion, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSamplerYcbcrConversion  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroySamplerYcbcrConversion"))
            (device, ycbcrConversion, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorUpdateTemplate( VkDevice  device, const VkDescriptorUpdateTemplateCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDescriptorUpdateTemplate * pDescriptorUpdateTemplate)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDescriptorUpdateTemplateCreateInfo * ,const VkAllocationCallbacks * , VkDescriptorUpdateTemplate * ))
        android_dlsym(vulkan_handle, "vkCreateDescriptorUpdateTemplate"))
            (device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorUpdateTemplate( VkDevice  device,  VkDescriptorUpdateTemplate  descriptorUpdateTemplate, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorUpdateTemplate  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDescriptorUpdateTemplate"))
            (device, descriptorUpdateTemplate, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSetWithTemplate( VkDevice  device,  VkDescriptorSet  descriptorSet,  VkDescriptorUpdateTemplate  descriptorUpdateTemplate, const void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorSet  , VkDescriptorUpdateTemplate  ,const void * ))
        android_dlsym(vulkan_handle, "vkUpdateDescriptorSetWithTemplate"))
            (device, descriptorSet, descriptorUpdateTemplate, pData);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferProperties( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceExternalBufferInfo * pExternalBufferInfo,  VkExternalBufferProperties * pExternalBufferProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceExternalBufferInfo * , VkExternalBufferProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceExternalBufferProperties"))
            (physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalFenceProperties( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceExternalFenceInfo * pExternalFenceInfo,  VkExternalFenceProperties * pExternalFenceProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceExternalFenceInfo * , VkExternalFenceProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceExternalFenceProperties"))
            (physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalSemaphoreProperties( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo * pExternalSemaphoreInfo,  VkExternalSemaphoreProperties * pExternalSemaphoreProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceExternalSemaphoreInfo * , VkExternalSemaphoreProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceExternalSemaphoreProperties"))
            (physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSupport( VkDevice  device, const VkDescriptorSetLayoutCreateInfo * pCreateInfo,  VkDescriptorSetLayoutSupport * pSupport)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDescriptorSetLayoutCreateInfo * , VkDescriptorSetLayoutSupport * ))
        android_dlsym(vulkan_handle, "vkGetDescriptorSetLayoutSupport"))
            (device, pCreateInfo, pSupport);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCount( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndirectCount"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCount( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndexedIndirectCount"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass2( VkDevice  device, const VkRenderPassCreateInfo2 * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkRenderPass * pRenderPass)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkRenderPassCreateInfo2 * ,const VkAllocationCallbacks * , VkRenderPass * ))
        android_dlsym(vulkan_handle, "vkCreateRenderPass2"))
            (device, pCreateInfo, pAllocator, pRenderPass);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass2( VkCommandBuffer  commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, const VkSubpassBeginInfo * pSubpassBeginInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkRenderPassBeginInfo * ,const VkSubpassBeginInfo * ))
        android_dlsym(vulkan_handle, "vkCmdBeginRenderPass2"))
            (commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass2( VkCommandBuffer  commandBuffer, const VkSubpassBeginInfo * pSubpassBeginInfo, const VkSubpassEndInfo * pSubpassEndInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkSubpassBeginInfo * ,const VkSubpassEndInfo * ))
        android_dlsym(vulkan_handle, "vkCmdNextSubpass2"))
            (commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass2( VkCommandBuffer  commandBuffer, const VkSubpassEndInfo * pSubpassEndInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkSubpassEndInfo * ))
        android_dlsym(vulkan_handle, "vkCmdEndRenderPass2"))
            (commandBuffer, pSubpassEndInfo);
}

VKAPI_ATTR void VKAPI_CALL vkResetQueryPool( VkDevice  device,  VkQueryPool  queryPool,  uint32_t  firstQuery,  uint32_t  queryCount)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkQueryPool  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkResetQueryPool"))
            (device, queryPool, firstQuery, queryCount);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreCounterValue( VkDevice  device,  VkSemaphore  semaphore,  uint64_t * pValue)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSemaphore  , uint64_t * ))
        android_dlsym(vulkan_handle, "vkGetSemaphoreCounterValue"))
            (device, semaphore, pValue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphores( VkDevice  device, const VkSemaphoreWaitInfo * pWaitInfo,  uint64_t  timeout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSemaphoreWaitInfo * , uint64_t  ))
        android_dlsym(vulkan_handle, "vkWaitSemaphores"))
            (device, pWaitInfo, timeout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSignalSemaphore( VkDevice  device, const VkSemaphoreSignalInfo * pSignalInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSemaphoreSignalInfo * ))
        android_dlsym(vulkan_handle, "vkSignalSemaphore"))
            (device, pSignalInfo);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetBufferDeviceAddress( VkDevice  device, const VkBufferDeviceAddressInfo * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkDeviceAddress (*)( VkDevice  ,const VkBufferDeviceAddressInfo * ))
        android_dlsym(vulkan_handle, "vkGetBufferDeviceAddress"))
            (device, pInfo);
}

VKAPI_ATTR uint64_t VKAPI_CALL vkGetBufferOpaqueCaptureAddress( VkDevice  device, const VkBufferDeviceAddressInfo * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((uint64_t (*)( VkDevice  ,const VkBufferDeviceAddressInfo * ))
        android_dlsym(vulkan_handle, "vkGetBufferOpaqueCaptureAddress"))
            (device, pInfo);
}

VKAPI_ATTR uint64_t VKAPI_CALL vkGetDeviceMemoryOpaqueCaptureAddress( VkDevice  device, const VkDeviceMemoryOpaqueCaptureAddressInfo * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((uint64_t (*)( VkDevice  ,const VkDeviceMemoryOpaqueCaptureAddressInfo * ))
        android_dlsym(vulkan_handle, "vkGetDeviceMemoryOpaqueCaptureAddress"))
            (device, pInfo);
}

#if VK_HEADER_VERSION >= 204

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceToolProperties( VkPhysicalDevice  physicalDevice,  uint32_t * pToolCount,  VkPhysicalDeviceToolProperties * pToolProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkPhysicalDeviceToolProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceToolProperties"))
            (physicalDevice, pToolCount, pToolProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePrivateDataSlot( VkDevice  device, const VkPrivateDataSlotCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkPrivateDataSlot * pPrivateDataSlot)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPrivateDataSlotCreateInfo * ,const VkAllocationCallbacks * , VkPrivateDataSlot * ))
        android_dlsym(vulkan_handle, "vkCreatePrivateDataSlot"))
            (device, pCreateInfo, pAllocator, pPrivateDataSlot);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPrivateDataSlot( VkDevice  device,  VkPrivateDataSlot  privateDataSlot, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkPrivateDataSlot  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyPrivateDataSlot"))
            (device, privateDataSlot, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetPrivateData( VkDevice  device,  VkObjectType  objectType,  uint64_t  objectHandle,  VkPrivateDataSlot  privateDataSlot,  uint64_t  data)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkObjectType  , uint64_t  , VkPrivateDataSlot  , uint64_t  ))
        android_dlsym(vulkan_handle, "vkSetPrivateData"))
            (device, objectType, objectHandle, privateDataSlot, data);
}

VKAPI_ATTR void VKAPI_CALL vkGetPrivateData( VkDevice  device,  VkObjectType  objectType,  uint64_t  objectHandle,  VkPrivateDataSlot  privateDataSlot,  uint64_t * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkObjectType  , uint64_t  , VkPrivateDataSlot  , uint64_t * ))
        android_dlsym(vulkan_handle, "vkGetPrivateData"))
            (device, objectType, objectHandle, privateDataSlot, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent2( VkCommandBuffer  commandBuffer,  VkEvent  event, const VkDependencyInfo * pDependencyInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkEvent  ,const VkDependencyInfo * ))
        android_dlsym(vulkan_handle, "vkCmdSetEvent2"))
            (commandBuffer, event, pDependencyInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent2( VkCommandBuffer  commandBuffer,  VkEvent  event,  VkPipelineStageFlags2  stageMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkEvent  , VkPipelineStageFlags2  ))
        android_dlsym(vulkan_handle, "vkCmdResetEvent2"))
            (commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents2( VkCommandBuffer  commandBuffer,  uint32_t  eventCount, const VkEvent * pEvents, const VkDependencyInfo * pDependencyInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkEvent * ,const VkDependencyInfo * ))
        android_dlsym(vulkan_handle, "vkCmdWaitEvents2"))
            (commandBuffer, eventCount, pEvents, pDependencyInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier2( VkCommandBuffer  commandBuffer, const VkDependencyInfo * pDependencyInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkDependencyInfo * ))
        android_dlsym(vulkan_handle, "vkCmdPipelineBarrier2"))
            (commandBuffer, pDependencyInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp2( VkCommandBuffer  commandBuffer,  VkPipelineStageFlags2  stage,  VkQueryPool  queryPool,  uint32_t  query)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineStageFlags2  , VkQueryPool  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteTimestamp2"))
            (commandBuffer, stage, queryPool, query);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2( VkQueue  queue,  uint32_t  submitCount, const VkSubmitInfo2 * pSubmits,  VkFence  fence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkQueue  , uint32_t  ,const VkSubmitInfo2 * , VkFence  ))
        android_dlsym(vulkan_handle, "vkQueueSubmit2"))
            (queue, submitCount, pSubmits, fence);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer2( VkCommandBuffer  commandBuffer, const VkCopyBufferInfo2 * pCopyBufferInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyBufferInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyBuffer2"))
            (commandBuffer, pCopyBufferInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage2( VkCommandBuffer  commandBuffer, const VkCopyImageInfo2 * pCopyImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyImage2"))
            (commandBuffer, pCopyImageInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage2( VkCommandBuffer  commandBuffer, const VkCopyBufferToImageInfo2 * pCopyBufferToImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyBufferToImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyBufferToImage2"))
            (commandBuffer, pCopyBufferToImageInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer2( VkCommandBuffer  commandBuffer, const VkCopyImageToBufferInfo2 * pCopyImageToBufferInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyImageToBufferInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyImageToBuffer2"))
            (commandBuffer, pCopyImageToBufferInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage2( VkCommandBuffer  commandBuffer, const VkBlitImageInfo2 * pBlitImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkBlitImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdBlitImage2"))
            (commandBuffer, pBlitImageInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage2( VkCommandBuffer  commandBuffer, const VkResolveImageInfo2 * pResolveImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkResolveImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdResolveImage2"))
            (commandBuffer, pResolveImageInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRendering( VkCommandBuffer  commandBuffer, const VkRenderingInfo * pRenderingInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkRenderingInfo * ))
        android_dlsym(vulkan_handle, "vkCmdBeginRendering"))
            (commandBuffer, pRenderingInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRendering( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkCmdEndRendering"))
            (commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCullMode( VkCommandBuffer  commandBuffer,  VkCullModeFlags  cullMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkCullModeFlags  ))
        android_dlsym(vulkan_handle, "vkCmdSetCullMode"))
            (commandBuffer, cullMode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetFrontFace( VkCommandBuffer  commandBuffer,  VkFrontFace  frontFace)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkFrontFace  ))
        android_dlsym(vulkan_handle, "vkCmdSetFrontFace"))
            (commandBuffer, frontFace);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveTopology( VkCommandBuffer  commandBuffer,  VkPrimitiveTopology  primitiveTopology)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPrimitiveTopology  ))
        android_dlsym(vulkan_handle, "vkCmdSetPrimitiveTopology"))
            (commandBuffer, primitiveTopology);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCount( VkCommandBuffer  commandBuffer,  uint32_t  viewportCount, const VkViewport * pViewports)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkViewport * ))
        android_dlsym(vulkan_handle, "vkCmdSetViewportWithCount"))
            (commandBuffer, viewportCount, pViewports);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCount( VkCommandBuffer  commandBuffer,  uint32_t  scissorCount, const VkRect2D * pScissors)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkRect2D * ))
        android_dlsym(vulkan_handle, "vkCmdSetScissorWithCount"))
            (commandBuffer, scissorCount, pScissors);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers2( VkCommandBuffer  commandBuffer,  uint32_t  firstBinding,  uint32_t  bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets, const VkDeviceSize * pSizes, const VkDeviceSize * pStrides)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBuffer * ,const VkDeviceSize * ,const VkDeviceSize * ,const VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkCmdBindVertexBuffers2"))
            (commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthTestEnable( VkCommandBuffer  commandBuffer,  VkBool32  depthTestEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthTestEnable"))
            (commandBuffer, depthTestEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthWriteEnable( VkCommandBuffer  commandBuffer,  VkBool32  depthWriteEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthWriteEnable"))
            (commandBuffer, depthWriteEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthCompareOp( VkCommandBuffer  commandBuffer,  VkCompareOp  depthCompareOp)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkCompareOp  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthCompareOp"))
            (commandBuffer, depthCompareOp);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBoundsTestEnable( VkCommandBuffer  commandBuffer,  VkBool32  depthBoundsTestEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthBoundsTestEnable"))
            (commandBuffer, depthBoundsTestEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilTestEnable( VkCommandBuffer  commandBuffer,  VkBool32  stencilTestEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetStencilTestEnable"))
            (commandBuffer, stencilTestEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilOp( VkCommandBuffer  commandBuffer,  VkStencilFaceFlags  faceMask,  VkStencilOp  failOp,  VkStencilOp  passOp,  VkStencilOp  depthFailOp,  VkCompareOp  compareOp)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkStencilFaceFlags  , VkStencilOp  , VkStencilOp  , VkStencilOp  , VkCompareOp  ))
        android_dlsym(vulkan_handle, "vkCmdSetStencilOp"))
            (commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizerDiscardEnable( VkCommandBuffer  commandBuffer,  VkBool32  rasterizerDiscardEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetRasterizerDiscardEnable"))
            (commandBuffer, rasterizerDiscardEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBiasEnable( VkCommandBuffer  commandBuffer,  VkBool32  depthBiasEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthBiasEnable"))
            (commandBuffer, depthBiasEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveRestartEnable( VkCommandBuffer  commandBuffer,  VkBool32  primitiveRestartEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetPrimitiveRestartEnable"))
            (commandBuffer, primitiveRestartEnable);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceBufferMemoryRequirements( VkDevice  device, const VkDeviceBufferMemoryRequirements * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceBufferMemoryRequirements * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetDeviceBufferMemoryRequirements"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageMemoryRequirements( VkDevice  device, const VkDeviceImageMemoryRequirements * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceImageMemoryRequirements * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetDeviceImageMemoryRequirements"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageSparseMemoryRequirements( VkDevice  device, const VkDeviceImageMemoryRequirements * pInfo,  uint32_t * pSparseMemoryRequirementCount,  VkSparseImageMemoryRequirements2 * pSparseMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceImageMemoryRequirements * , uint32_t * , VkSparseImageMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetDeviceImageSparseMemoryRequirements"))
            (device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

#endif

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceSupportKHR( VkPhysicalDevice  physicalDevice,  uint32_t  queueFamilyIndex,  VkSurfaceKHR  surface,  VkBool32 * pSupported)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t  , VkSurfaceKHR  , VkBool32 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSurfaceSupportKHR"))
            (physicalDevice, queueFamilyIndex, surface, pSupported);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilitiesKHR( VkPhysicalDevice  physicalDevice,  VkSurfaceKHR  surface,  VkSurfaceCapabilitiesKHR * pSurfaceCapabilities)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkSurfaceKHR  , VkSurfaceCapabilitiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR"))
            (physicalDevice, surface, pSurfaceCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormatsKHR( VkPhysicalDevice  physicalDevice,  VkSurfaceKHR  surface,  uint32_t * pSurfaceFormatCount,  VkSurfaceFormatKHR * pSurfaceFormats)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkSurfaceKHR  , uint32_t * , VkSurfaceFormatKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSurfaceFormatsKHR"))
            (physicalDevice, surface, pSurfaceFormatCount, pSurfaceFormats);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfacePresentModesKHR( VkPhysicalDevice  physicalDevice,  VkSurfaceKHR  surface,  uint32_t * pPresentModeCount,  VkPresentModeKHR * pPresentModes)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkSurfaceKHR  , uint32_t * , VkPresentModeKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSurfacePresentModesKHR"))
            (physicalDevice, surface, pPresentModeCount, pPresentModes);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSwapchainKHR( VkDevice  device, const VkSwapchainCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkSwapchainKHR * pSwapchain)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSwapchainCreateInfoKHR * ,const VkAllocationCallbacks * , VkSwapchainKHR * ))
        android_dlsym(vulkan_handle, "vkCreateSwapchainKHR"))
            (device, pCreateInfo, pAllocator, pSwapchain);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySwapchainKHR( VkDevice  device,  VkSwapchainKHR  swapchain, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSwapchainKHR  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroySwapchainKHR"))
            (device, swapchain, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainImagesKHR( VkDevice  device,  VkSwapchainKHR  swapchain,  uint32_t * pSwapchainImageCount,  VkImage * pSwapchainImages)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  , uint32_t * , VkImage * ))
        android_dlsym(vulkan_handle, "vkGetSwapchainImagesKHR"))
            (device, swapchain, pSwapchainImageCount, pSwapchainImages);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImageKHR( VkDevice  device,  VkSwapchainKHR  swapchain,  uint64_t  timeout,  VkSemaphore  semaphore,  VkFence  fence,  uint32_t * pImageIndex)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  , uint64_t  , VkSemaphore  , VkFence  , uint32_t * ))
        android_dlsym(vulkan_handle, "vkAcquireNextImageKHR"))
            (device, swapchain, timeout, semaphore, fence, pImageIndex);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueuePresentKHR( VkQueue  queue, const VkPresentInfoKHR * pPresentInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkQueue  ,const VkPresentInfoKHR * ))
        android_dlsym(vulkan_handle, "vkQueuePresentKHR"))
            (queue, pPresentInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupPresentCapabilitiesKHR( VkDevice  device,  VkDeviceGroupPresentCapabilitiesKHR * pDeviceGroupPresentCapabilities)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeviceGroupPresentCapabilitiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetDeviceGroupPresentCapabilitiesKHR"))
            (device, pDeviceGroupPresentCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceGroupSurfacePresentModesKHR( VkDevice  device,  VkSurfaceKHR  surface,  VkDeviceGroupPresentModeFlagsKHR * pModes)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSurfaceKHR  , VkDeviceGroupPresentModeFlagsKHR * ))
        android_dlsym(vulkan_handle, "vkGetDeviceGroupSurfacePresentModesKHR"))
            (device, surface, pModes);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDevicePresentRectanglesKHR( VkPhysicalDevice  physicalDevice,  VkSurfaceKHR  surface,  uint32_t * pRectCount,  VkRect2D * pRects)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkSurfaceKHR  , uint32_t * , VkRect2D * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDevicePresentRectanglesKHR"))
            (physicalDevice, surface, pRectCount, pRects);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireNextImage2KHR( VkDevice  device, const VkAcquireNextImageInfoKHR * pAcquireInfo,  uint32_t * pImageIndex)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkAcquireNextImageInfoKHR * , uint32_t * ))
        android_dlsym(vulkan_handle, "vkAcquireNextImage2KHR"))
            (device, pAcquireInfo, pImageIndex);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceDisplayPropertiesKHR( VkPhysicalDevice  physicalDevice,  uint32_t * pPropertyCount,  VkDisplayPropertiesKHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkDisplayPropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceDisplayPropertiesKHR"))
            (physicalDevice, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceDisplayPlanePropertiesKHR( VkPhysicalDevice  physicalDevice,  uint32_t * pPropertyCount,  VkDisplayPlanePropertiesKHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkDisplayPlanePropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceDisplayPlanePropertiesKHR"))
            (physicalDevice, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDisplayPlaneSupportedDisplaysKHR( VkPhysicalDevice  physicalDevice,  uint32_t  planeIndex,  uint32_t * pDisplayCount,  VkDisplayKHR * pDisplays)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t  , uint32_t * , VkDisplayKHR * ))
        android_dlsym(vulkan_handle, "vkGetDisplayPlaneSupportedDisplaysKHR"))
            (physicalDevice, planeIndex, pDisplayCount, pDisplays);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDisplayModePropertiesKHR( VkPhysicalDevice  physicalDevice,  VkDisplayKHR  display,  uint32_t * pPropertyCount,  VkDisplayModePropertiesKHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkDisplayKHR  , uint32_t * , VkDisplayModePropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetDisplayModePropertiesKHR"))
            (physicalDevice, display, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDisplayModeKHR( VkPhysicalDevice  physicalDevice,  VkDisplayKHR  display, const VkDisplayModeCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDisplayModeKHR * pMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkDisplayKHR  ,const VkDisplayModeCreateInfoKHR * ,const VkAllocationCallbacks * , VkDisplayModeKHR * ))
        android_dlsym(vulkan_handle, "vkCreateDisplayModeKHR"))
            (physicalDevice, display, pCreateInfo, pAllocator, pMode);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDisplayPlaneCapabilitiesKHR( VkPhysicalDevice  physicalDevice,  VkDisplayModeKHR  mode,  uint32_t  planeIndex,  VkDisplayPlaneCapabilitiesKHR * pCapabilities)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkDisplayModeKHR  , uint32_t  , VkDisplayPlaneCapabilitiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetDisplayPlaneCapabilitiesKHR"))
            (physicalDevice, mode, planeIndex, pCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDisplayPlaneSurfaceKHR( VkInstance  instance, const VkDisplaySurfaceCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkSurfaceKHR * pSurface)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkInstance  ,const VkDisplaySurfaceCreateInfoKHR * ,const VkAllocationCallbacks * , VkSurfaceKHR * ))
        android_dlsym(vulkan_handle, "vkCreateDisplayPlaneSurfaceKHR"))
            (instance, pCreateInfo, pAllocator, pSurface);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSharedSwapchainsKHR( VkDevice  device,  uint32_t  swapchainCount, const VkSwapchainCreateInfoKHR * pCreateInfos, const VkAllocationCallbacks * pAllocator,  VkSwapchainKHR * pSwapchains)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkSwapchainCreateInfoKHR * ,const VkAllocationCallbacks * , VkSwapchainKHR * ))
        android_dlsym(vulkan_handle, "vkCreateSharedSwapchainsKHR"))
            (device, swapchainCount, pCreateInfos, pAllocator, pSwapchains);
}

#if VK_HEADER_VERSION >= 238

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceVideoCapabilitiesKHR( VkPhysicalDevice  physicalDevice, const VkVideoProfileInfoKHR * pVideoProfile,  VkVideoCapabilitiesKHR * pCapabilities)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkVideoProfileInfoKHR * , VkVideoCapabilitiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceVideoCapabilitiesKHR"))
            (physicalDevice, pVideoProfile, pCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceVideoFormatPropertiesKHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceVideoFormatInfoKHR * pVideoFormatInfo,  uint32_t * pVideoFormatPropertyCount,  VkVideoFormatPropertiesKHR * pVideoFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkPhysicalDeviceVideoFormatInfoKHR * , uint32_t * , VkVideoFormatPropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceVideoFormatPropertiesKHR"))
            (physicalDevice, pVideoFormatInfo, pVideoFormatPropertyCount, pVideoFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateVideoSessionKHR( VkDevice  device, const VkVideoSessionCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkVideoSessionKHR * pVideoSession)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkVideoSessionCreateInfoKHR * ,const VkAllocationCallbacks * , VkVideoSessionKHR * ))
        android_dlsym(vulkan_handle, "vkCreateVideoSessionKHR"))
            (device, pCreateInfo, pAllocator, pVideoSession);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyVideoSessionKHR( VkDevice  device,  VkVideoSessionKHR  videoSession, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkVideoSessionKHR  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyVideoSessionKHR"))
            (device, videoSession, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetVideoSessionMemoryRequirementsKHR( VkDevice  device,  VkVideoSessionKHR  videoSession,  uint32_t * pMemoryRequirementsCount,  VkVideoSessionMemoryRequirementsKHR * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkVideoSessionKHR  , uint32_t * , VkVideoSessionMemoryRequirementsKHR * ))
        android_dlsym(vulkan_handle, "vkGetVideoSessionMemoryRequirementsKHR"))
            (device, videoSession, pMemoryRequirementsCount, pMemoryRequirements);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindVideoSessionMemoryKHR( VkDevice  device,  VkVideoSessionKHR  videoSession,  uint32_t  bindSessionMemoryInfoCount, const VkBindVideoSessionMemoryInfoKHR * pBindSessionMemoryInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkVideoSessionKHR  , uint32_t  ,const VkBindVideoSessionMemoryInfoKHR * ))
        android_dlsym(vulkan_handle, "vkBindVideoSessionMemoryKHR"))
            (device, videoSession, bindSessionMemoryInfoCount, pBindSessionMemoryInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateVideoSessionParametersKHR( VkDevice  device, const VkVideoSessionParametersCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkVideoSessionParametersKHR * pVideoSessionParameters)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkVideoSessionParametersCreateInfoKHR * ,const VkAllocationCallbacks * , VkVideoSessionParametersKHR * ))
        android_dlsym(vulkan_handle, "vkCreateVideoSessionParametersKHR"))
            (device, pCreateInfo, pAllocator, pVideoSessionParameters);
}

VKAPI_ATTR VkResult VKAPI_CALL vkUpdateVideoSessionParametersKHR( VkDevice  device,  VkVideoSessionParametersKHR  videoSessionParameters, const VkVideoSessionParametersUpdateInfoKHR * pUpdateInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkVideoSessionParametersKHR  ,const VkVideoSessionParametersUpdateInfoKHR * ))
        android_dlsym(vulkan_handle, "vkUpdateVideoSessionParametersKHR"))
            (device, videoSessionParameters, pUpdateInfo);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyVideoSessionParametersKHR( VkDevice  device,  VkVideoSessionParametersKHR  videoSessionParameters, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkVideoSessionParametersKHR  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyVideoSessionParametersKHR"))
            (device, videoSessionParameters, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginVideoCodingKHR( VkCommandBuffer  commandBuffer, const VkVideoBeginCodingInfoKHR * pBeginInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkVideoBeginCodingInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdBeginVideoCodingKHR"))
            (commandBuffer, pBeginInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndVideoCodingKHR( VkCommandBuffer  commandBuffer, const VkVideoEndCodingInfoKHR * pEndCodingInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkVideoEndCodingInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdEndVideoCodingKHR"))
            (commandBuffer, pEndCodingInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdControlVideoCodingKHR( VkCommandBuffer  commandBuffer, const VkVideoCodingControlInfoKHR * pCodingControlInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkVideoCodingControlInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdControlVideoCodingKHR"))
            (commandBuffer, pCodingControlInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDecodeVideoKHR( VkCommandBuffer  commandBuffer, const VkVideoDecodeInfoKHR * pDecodeInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkVideoDecodeInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdDecodeVideoKHR"))
            (commandBuffer, pDecodeInfo);
}

#endif

#if VK_HEADER_VERSION >= 197

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderingKHR( VkCommandBuffer  commandBuffer, const VkRenderingInfo * pRenderingInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkRenderingInfo * ))
        android_dlsym(vulkan_handle, "vkCmdBeginRenderingKHR"))
            (commandBuffer, pRenderingInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderingKHR( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkCmdEndRenderingKHR"))
            (commandBuffer);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFeatures2KHR( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceFeatures2 * pFeatures)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkPhysicalDeviceFeatures2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceFeatures2KHR"))
            (physicalDevice, pFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceProperties2KHR( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceProperties2 * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkPhysicalDeviceProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceProperties2KHR"))
            (physicalDevice, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceFormatProperties2KHR( VkPhysicalDevice  physicalDevice,  VkFormat  format,  VkFormatProperties2 * pFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkFormat  , VkFormatProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceFormatProperties2KHR"))
            (physicalDevice, format, pFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceImageFormatProperties2KHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceImageFormatInfo2 * pImageFormatInfo,  VkImageFormatProperties2 * pImageFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkPhysicalDeviceImageFormatInfo2 * , VkImageFormatProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceImageFormatProperties2KHR"))
            (physicalDevice, pImageFormatInfo, pImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyProperties2KHR( VkPhysicalDevice  physicalDevice,  uint32_t * pQueueFamilyPropertyCount,  VkQueueFamilyProperties2 * pQueueFamilyProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , uint32_t * , VkQueueFamilyProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceQueueFamilyProperties2KHR"))
            (physicalDevice, pQueueFamilyPropertyCount, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMemoryProperties2KHR( VkPhysicalDevice  physicalDevice,  VkPhysicalDeviceMemoryProperties2 * pMemoryProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkPhysicalDeviceMemoryProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceMemoryProperties2KHR"))
            (physicalDevice, pMemoryProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceSparseImageFormatProperties2KHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 * pFormatInfo,  uint32_t * pPropertyCount,  VkSparseImageFormatProperties2 * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceSparseImageFormatInfo2 * , uint32_t * , VkSparseImageFormatProperties2 * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSparseImageFormatProperties2KHR"))
            (physicalDevice, pFormatInfo, pPropertyCount, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceGroupPeerMemoryFeaturesKHR( VkDevice  device,  uint32_t  heapIndex,  uint32_t  localDeviceIndex,  uint32_t  remoteDeviceIndex,  VkPeerMemoryFeatureFlags * pPeerMemoryFeatures)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , uint32_t  , uint32_t  , uint32_t  , VkPeerMemoryFeatureFlags * ))
        android_dlsym(vulkan_handle, "vkGetDeviceGroupPeerMemoryFeaturesKHR"))
            (device, heapIndex, localDeviceIndex, remoteDeviceIndex, pPeerMemoryFeatures);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDeviceMaskKHR( VkCommandBuffer  commandBuffer,  uint32_t  deviceMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetDeviceMaskKHR"))
            (commandBuffer, deviceMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDispatchBaseKHR( VkCommandBuffer  commandBuffer,  uint32_t  baseGroupX,  uint32_t  baseGroupY,  uint32_t  baseGroupZ,  uint32_t  groupCountX,  uint32_t  groupCountY,  uint32_t  groupCountZ)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , uint32_t  , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDispatchBaseKHR"))
            (commandBuffer, baseGroupX, baseGroupY, baseGroupZ, groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR void VKAPI_CALL vkTrimCommandPoolKHR( VkDevice  device,  VkCommandPool  commandPool,  VkCommandPoolTrimFlags  flags)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCommandPool  , VkCommandPoolTrimFlags  ))
        android_dlsym(vulkan_handle, "vkTrimCommandPoolKHR"))
            (device, commandPool, flags);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceGroupsKHR( VkInstance  instance,  uint32_t * pPhysicalDeviceGroupCount,  VkPhysicalDeviceGroupProperties * pPhysicalDeviceGroupProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkInstance  , uint32_t * , VkPhysicalDeviceGroupProperties * ))
        android_dlsym(vulkan_handle, "vkEnumeratePhysicalDeviceGroupsKHR"))
            (instance, pPhysicalDeviceGroupCount, pPhysicalDeviceGroupProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalBufferPropertiesKHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceExternalBufferInfo * pExternalBufferInfo,  VkExternalBufferProperties * pExternalBufferProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceExternalBufferInfo * , VkExternalBufferProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceExternalBufferPropertiesKHR"))
            (physicalDevice, pExternalBufferInfo, pExternalBufferProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdKHR( VkDevice  device, const VkMemoryGetFdInfoKHR * pGetFdInfo,  int * pFd)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkMemoryGetFdInfoKHR * , int * ))
        android_dlsym(vulkan_handle, "vkGetMemoryFdKHR"))
            (device, pGetFdInfo, pFd);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryFdPropertiesKHR( VkDevice  device,  VkExternalMemoryHandleTypeFlagBits  handleType,  int  fd,  VkMemoryFdPropertiesKHR * pMemoryFdProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkExternalMemoryHandleTypeFlagBits  , int  , VkMemoryFdPropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetMemoryFdPropertiesKHR"))
            (device, handleType, fd, pMemoryFdProperties);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalSemaphorePropertiesKHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo * pExternalSemaphoreInfo,  VkExternalSemaphoreProperties * pExternalSemaphoreProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceExternalSemaphoreInfo * , VkExternalSemaphoreProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceExternalSemaphorePropertiesKHR"))
            (physicalDevice, pExternalSemaphoreInfo, pExternalSemaphoreProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkImportSemaphoreFdKHR( VkDevice  device, const VkImportSemaphoreFdInfoKHR * pImportSemaphoreFdInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkImportSemaphoreFdInfoKHR * ))
        android_dlsym(vulkan_handle, "vkImportSemaphoreFdKHR"))
            (device, pImportSemaphoreFdInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreFdKHR( VkDevice  device, const VkSemaphoreGetFdInfoKHR * pGetFdInfo,  int * pFd)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSemaphoreGetFdInfoKHR * , int * ))
        android_dlsym(vulkan_handle, "vkGetSemaphoreFdKHR"))
            (device, pGetFdInfo, pFd);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetKHR( VkCommandBuffer  commandBuffer,  VkPipelineBindPoint  pipelineBindPoint,  VkPipelineLayout  layout,  uint32_t  set,  uint32_t  descriptorWriteCount, const VkWriteDescriptorSet * pDescriptorWrites)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineBindPoint  , VkPipelineLayout  , uint32_t  , uint32_t  ,const VkWriteDescriptorSet * ))
        android_dlsym(vulkan_handle, "vkCmdPushDescriptorSetKHR"))
            (commandBuffer, pipelineBindPoint, layout, set, descriptorWriteCount, pDescriptorWrites);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetWithTemplateKHR( VkCommandBuffer  commandBuffer,  VkDescriptorUpdateTemplate  descriptorUpdateTemplate,  VkPipelineLayout  layout,  uint32_t  set, const void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkDescriptorUpdateTemplate  , VkPipelineLayout  , uint32_t  ,const void * ))
        android_dlsym(vulkan_handle, "vkCmdPushDescriptorSetWithTemplateKHR"))
            (commandBuffer, descriptorUpdateTemplate, layout, set, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDescriptorUpdateTemplateKHR( VkDevice  device, const VkDescriptorUpdateTemplateCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDescriptorUpdateTemplate * pDescriptorUpdateTemplate)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDescriptorUpdateTemplateCreateInfo * ,const VkAllocationCallbacks * , VkDescriptorUpdateTemplate * ))
        android_dlsym(vulkan_handle, "vkCreateDescriptorUpdateTemplateKHR"))
            (device, pCreateInfo, pAllocator, pDescriptorUpdateTemplate);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDescriptorUpdateTemplateKHR( VkDevice  device,  VkDescriptorUpdateTemplate  descriptorUpdateTemplate, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorUpdateTemplate  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDescriptorUpdateTemplateKHR"))
            (device, descriptorUpdateTemplate, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkUpdateDescriptorSetWithTemplateKHR( VkDevice  device,  VkDescriptorSet  descriptorSet,  VkDescriptorUpdateTemplate  descriptorUpdateTemplate, const void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorSet  , VkDescriptorUpdateTemplate  ,const void * ))
        android_dlsym(vulkan_handle, "vkUpdateDescriptorSetWithTemplateKHR"))
            (device, descriptorSet, descriptorUpdateTemplate, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRenderPass2KHR( VkDevice  device, const VkRenderPassCreateInfo2 * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkRenderPass * pRenderPass)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkRenderPassCreateInfo2 * ,const VkAllocationCallbacks * , VkRenderPass * ))
        android_dlsym(vulkan_handle, "vkCreateRenderPass2KHR"))
            (device, pCreateInfo, pAllocator, pRenderPass);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginRenderPass2KHR( VkCommandBuffer  commandBuffer, const VkRenderPassBeginInfo * pRenderPassBegin, const VkSubpassBeginInfo * pSubpassBeginInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkRenderPassBeginInfo * ,const VkSubpassBeginInfo * ))
        android_dlsym(vulkan_handle, "vkCmdBeginRenderPass2KHR"))
            (commandBuffer, pRenderPassBegin, pSubpassBeginInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdNextSubpass2KHR( VkCommandBuffer  commandBuffer, const VkSubpassBeginInfo * pSubpassBeginInfo, const VkSubpassEndInfo * pSubpassEndInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkSubpassBeginInfo * ,const VkSubpassEndInfo * ))
        android_dlsym(vulkan_handle, "vkCmdNextSubpass2KHR"))
            (commandBuffer, pSubpassBeginInfo, pSubpassEndInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndRenderPass2KHR( VkCommandBuffer  commandBuffer, const VkSubpassEndInfo * pSubpassEndInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkSubpassEndInfo * ))
        android_dlsym(vulkan_handle, "vkCmdEndRenderPass2KHR"))
            (commandBuffer, pSubpassEndInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainStatusKHR( VkDevice  device,  VkSwapchainKHR  swapchain)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  ))
        android_dlsym(vulkan_handle, "vkGetSwapchainStatusKHR"))
            (device, swapchain);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceExternalFencePropertiesKHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceExternalFenceInfo * pExternalFenceInfo,  VkExternalFenceProperties * pExternalFenceProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkPhysicalDeviceExternalFenceInfo * , VkExternalFenceProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceExternalFencePropertiesKHR"))
            (physicalDevice, pExternalFenceInfo, pExternalFenceProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkImportFenceFdKHR( VkDevice  device, const VkImportFenceFdInfoKHR * pImportFenceFdInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkImportFenceFdInfoKHR * ))
        android_dlsym(vulkan_handle, "vkImportFenceFdKHR"))
            (device, pImportFenceFdInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetFenceFdKHR( VkDevice  device, const VkFenceGetFdInfoKHR * pGetFdInfo,  int * pFd)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkFenceGetFdInfoKHR * , int * ))
        android_dlsym(vulkan_handle, "vkGetFenceFdKHR"))
            (device, pGetFdInfo, pFd);
}

VKAPI_ATTR VkResult VKAPI_CALL vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR( VkPhysicalDevice  physicalDevice,  uint32_t  queueFamilyIndex,  uint32_t * pCounterCount,  VkPerformanceCounterKHR * pCounters,  VkPerformanceCounterDescriptionKHR * pCounterDescriptions)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t  , uint32_t * , VkPerformanceCounterKHR * , VkPerformanceCounterDescriptionKHR * ))
        android_dlsym(vulkan_handle, "vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR"))
            (physicalDevice, queueFamilyIndex, pCounterCount, pCounters, pCounterDescriptions);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR( VkPhysicalDevice  physicalDevice, const VkQueryPoolPerformanceCreateInfoKHR * pPerformanceQueryCreateInfo,  uint32_t * pNumPasses)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  ,const VkQueryPoolPerformanceCreateInfoKHR * , uint32_t * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR"))
            (physicalDevice, pPerformanceQueryCreateInfo, pNumPasses);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireProfilingLockKHR( VkDevice  device, const VkAcquireProfilingLockInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkAcquireProfilingLockInfoKHR * ))
        android_dlsym(vulkan_handle, "vkAcquireProfilingLockKHR"))
            (device, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkReleaseProfilingLockKHR( VkDevice  device)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ))
        android_dlsym(vulkan_handle, "vkReleaseProfilingLockKHR"))
            (device);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilities2KHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR * pSurfaceInfo,  VkSurfaceCapabilities2KHR * pSurfaceCapabilities)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkPhysicalDeviceSurfaceInfo2KHR * , VkSurfaceCapabilities2KHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSurfaceCapabilities2KHR"))
            (physicalDevice, pSurfaceInfo, pSurfaceCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceFormats2KHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR * pSurfaceInfo,  uint32_t * pSurfaceFormatCount,  VkSurfaceFormat2KHR * pSurfaceFormats)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkPhysicalDeviceSurfaceInfo2KHR * , uint32_t * , VkSurfaceFormat2KHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSurfaceFormats2KHR"))
            (physicalDevice, pSurfaceInfo, pSurfaceFormatCount, pSurfaceFormats);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceDisplayProperties2KHR( VkPhysicalDevice  physicalDevice,  uint32_t * pPropertyCount,  VkDisplayProperties2KHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkDisplayProperties2KHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceDisplayProperties2KHR"))
            (physicalDevice, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceDisplayPlaneProperties2KHR( VkPhysicalDevice  physicalDevice,  uint32_t * pPropertyCount,  VkDisplayPlaneProperties2KHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkDisplayPlaneProperties2KHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceDisplayPlaneProperties2KHR"))
            (physicalDevice, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDisplayModeProperties2KHR( VkPhysicalDevice  physicalDevice,  VkDisplayKHR  display,  uint32_t * pPropertyCount,  VkDisplayModeProperties2KHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkDisplayKHR  , uint32_t * , VkDisplayModeProperties2KHR * ))
        android_dlsym(vulkan_handle, "vkGetDisplayModeProperties2KHR"))
            (physicalDevice, display, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDisplayPlaneCapabilities2KHR( VkPhysicalDevice  physicalDevice, const VkDisplayPlaneInfo2KHR * pDisplayPlaneInfo,  VkDisplayPlaneCapabilities2KHR * pCapabilities)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkDisplayPlaneInfo2KHR * , VkDisplayPlaneCapabilities2KHR * ))
        android_dlsym(vulkan_handle, "vkGetDisplayPlaneCapabilities2KHR"))
            (physicalDevice, pDisplayPlaneInfo, pCapabilities);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageMemoryRequirements2KHR( VkDevice  device, const VkImageMemoryRequirementsInfo2 * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkImageMemoryRequirementsInfo2 * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetImageMemoryRequirements2KHR"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetBufferMemoryRequirements2KHR( VkDevice  device, const VkBufferMemoryRequirementsInfo2 * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkBufferMemoryRequirementsInfo2 * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetBufferMemoryRequirements2KHR"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSparseMemoryRequirements2KHR( VkDevice  device, const VkImageSparseMemoryRequirementsInfo2 * pInfo,  uint32_t * pSparseMemoryRequirementCount,  VkSparseImageMemoryRequirements2 * pSparseMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkImageSparseMemoryRequirementsInfo2 * , uint32_t * , VkSparseImageMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetImageSparseMemoryRequirements2KHR"))
            (device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateSamplerYcbcrConversionKHR( VkDevice  device, const VkSamplerYcbcrConversionCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkSamplerYcbcrConversion * pYcbcrConversion)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSamplerYcbcrConversionCreateInfo * ,const VkAllocationCallbacks * , VkSamplerYcbcrConversion * ))
        android_dlsym(vulkan_handle, "vkCreateSamplerYcbcrConversionKHR"))
            (device, pCreateInfo, pAllocator, pYcbcrConversion);
}

VKAPI_ATTR void VKAPI_CALL vkDestroySamplerYcbcrConversionKHR( VkDevice  device,  VkSamplerYcbcrConversion  ycbcrConversion, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSamplerYcbcrConversion  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroySamplerYcbcrConversionKHR"))
            (device, ycbcrConversion, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindBufferMemory2KHR( VkDevice  device,  uint32_t  bindInfoCount, const VkBindBufferMemoryInfo * pBindInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkBindBufferMemoryInfo * ))
        android_dlsym(vulkan_handle, "vkBindBufferMemory2KHR"))
            (device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindImageMemory2KHR( VkDevice  device,  uint32_t  bindInfoCount, const VkBindImageMemoryInfo * pBindInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkBindImageMemoryInfo * ))
        android_dlsym(vulkan_handle, "vkBindImageMemory2KHR"))
            (device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSupportKHR( VkDevice  device, const VkDescriptorSetLayoutCreateInfo * pCreateInfo,  VkDescriptorSetLayoutSupport * pSupport)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDescriptorSetLayoutCreateInfo * , VkDescriptorSetLayoutSupport * ))
        android_dlsym(vulkan_handle, "vkGetDescriptorSetLayoutSupportKHR"))
            (device, pCreateInfo, pSupport);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountKHR( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndirectCountKHR"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountKHR( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndexedIndirectCountKHR"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSemaphoreCounterValueKHR( VkDevice  device,  VkSemaphore  semaphore,  uint64_t * pValue)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSemaphore  , uint64_t * ))
        android_dlsym(vulkan_handle, "vkGetSemaphoreCounterValueKHR"))
            (device, semaphore, pValue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWaitSemaphoresKHR( VkDevice  device, const VkSemaphoreWaitInfo * pWaitInfo,  uint64_t  timeout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSemaphoreWaitInfo * , uint64_t  ))
        android_dlsym(vulkan_handle, "vkWaitSemaphoresKHR"))
            (device, pWaitInfo, timeout);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSignalSemaphoreKHR( VkDevice  device, const VkSemaphoreSignalInfo * pSignalInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSemaphoreSignalInfo * ))
        android_dlsym(vulkan_handle, "vkSignalSemaphoreKHR"))
            (device, pSignalInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceFragmentShadingRatesKHR( VkPhysicalDevice  physicalDevice,  uint32_t * pFragmentShadingRateCount,  VkPhysicalDeviceFragmentShadingRateKHR * pFragmentShadingRates)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkPhysicalDeviceFragmentShadingRateKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceFragmentShadingRatesKHR"))
            (physicalDevice, pFragmentShadingRateCount, pFragmentShadingRates);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetFragmentShadingRateKHR( VkCommandBuffer  commandBuffer, const VkExtent2D * pFragmentSize, const VkFragmentShadingRateCombinerOpKHR  combinerOps[2])
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkExtent2D * ,const VkFragmentShadingRateCombinerOpKHR  [2]))
        android_dlsym(vulkan_handle, "vkCmdSetFragmentShadingRateKHR"))
            (commandBuffer, pFragmentSize, combinerOps);
}

#if VK_HEADER_VERSION >= 276

VKAPI_ATTR void VKAPI_CALL vkCmdSetRenderingAttachmentLocationsKHR( VkCommandBuffer  commandBuffer, const VkRenderingAttachmentLocationInfo * pLocationInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkRenderingAttachmentLocationInfo * ))
        android_dlsym(vulkan_handle, "vkCmdSetRenderingAttachmentLocationsKHR"))
            (commandBuffer, pLocationInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetRenderingInputAttachmentIndicesKHR( VkCommandBuffer  commandBuffer, const VkRenderingInputAttachmentIndexInfo * pInputAttachmentIndexInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkRenderingInputAttachmentIndexInfo * ))
        android_dlsym(vulkan_handle, "vkCmdSetRenderingInputAttachmentIndicesKHR"))
            (commandBuffer, pInputAttachmentIndexInfo);
}

#endif

#if VK_HEADER_VERSION >= 185

VKAPI_ATTR VkResult VKAPI_CALL vkWaitForPresentKHR( VkDevice  device,  VkSwapchainKHR  swapchain,  uint64_t  presentId,  uint64_t  timeout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  , uint64_t  , uint64_t  ))
        android_dlsym(vulkan_handle, "vkWaitForPresentKHR"))
            (device, swapchain, presentId, timeout);
}

#endif

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetBufferDeviceAddressKHR( VkDevice  device, const VkBufferDeviceAddressInfo * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkDeviceAddress (*)( VkDevice  ,const VkBufferDeviceAddressInfo * ))
        android_dlsym(vulkan_handle, "vkGetBufferDeviceAddressKHR"))
            (device, pInfo);
}

VKAPI_ATTR uint64_t VKAPI_CALL vkGetBufferOpaqueCaptureAddressKHR( VkDevice  device, const VkBufferDeviceAddressInfo * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((uint64_t (*)( VkDevice  ,const VkBufferDeviceAddressInfo * ))
        android_dlsym(vulkan_handle, "vkGetBufferOpaqueCaptureAddressKHR"))
            (device, pInfo);
}

VKAPI_ATTR uint64_t VKAPI_CALL vkGetDeviceMemoryOpaqueCaptureAddressKHR( VkDevice  device, const VkDeviceMemoryOpaqueCaptureAddressInfo * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((uint64_t (*)( VkDevice  ,const VkDeviceMemoryOpaqueCaptureAddressInfo * ))
        android_dlsym(vulkan_handle, "vkGetDeviceMemoryOpaqueCaptureAddressKHR"))
            (device, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDeferredOperationKHR( VkDevice  device, const VkAllocationCallbacks * pAllocator,  VkDeferredOperationKHR * pDeferredOperation)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkAllocationCallbacks * , VkDeferredOperationKHR * ))
        android_dlsym(vulkan_handle, "vkCreateDeferredOperationKHR"))
            (device, pAllocator, pDeferredOperation);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDeferredOperationKHR( VkDevice  device,  VkDeferredOperationKHR  operation, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDeferredOperationKHR  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDeferredOperationKHR"))
            (device, operation, pAllocator);
}

VKAPI_ATTR uint32_t VKAPI_CALL vkGetDeferredOperationMaxConcurrencyKHR( VkDevice  device,  VkDeferredOperationKHR  operation)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((uint32_t (*)( VkDevice  , VkDeferredOperationKHR  ))
        android_dlsym(vulkan_handle, "vkGetDeferredOperationMaxConcurrencyKHR"))
            (device, operation);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeferredOperationResultKHR( VkDevice  device,  VkDeferredOperationKHR  operation)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ))
        android_dlsym(vulkan_handle, "vkGetDeferredOperationResultKHR"))
            (device, operation);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDeferredOperationJoinKHR( VkDevice  device,  VkDeferredOperationKHR  operation)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ))
        android_dlsym(vulkan_handle, "vkDeferredOperationJoinKHR"))
            (device, operation);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineExecutablePropertiesKHR( VkDevice  device, const VkPipelineInfoKHR * pPipelineInfo,  uint32_t * pExecutableCount,  VkPipelineExecutablePropertiesKHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPipelineInfoKHR * , uint32_t * , VkPipelineExecutablePropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPipelineExecutablePropertiesKHR"))
            (device, pPipelineInfo, pExecutableCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineExecutableStatisticsKHR( VkDevice  device, const VkPipelineExecutableInfoKHR * pExecutableInfo,  uint32_t * pStatisticCount,  VkPipelineExecutableStatisticKHR * pStatistics)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPipelineExecutableInfoKHR * , uint32_t * , VkPipelineExecutableStatisticKHR * ))
        android_dlsym(vulkan_handle, "vkGetPipelineExecutableStatisticsKHR"))
            (device, pExecutableInfo, pStatisticCount, pStatistics);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelineExecutableInternalRepresentationsKHR( VkDevice  device, const VkPipelineExecutableInfoKHR * pExecutableInfo,  uint32_t * pInternalRepresentationCount,  VkPipelineExecutableInternalRepresentationKHR * pInternalRepresentations)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPipelineExecutableInfoKHR * , uint32_t * , VkPipelineExecutableInternalRepresentationKHR * ))
        android_dlsym(vulkan_handle, "vkGetPipelineExecutableInternalRepresentationsKHR"))
            (device, pExecutableInfo, pInternalRepresentationCount, pInternalRepresentations);
}

#if VK_HEADER_VERSION >= 244

VKAPI_ATTR VkResult VKAPI_CALL vkMapMemory2KHR( VkDevice  device, const VkMemoryMapInfoKHR * pMemoryMapInfo,  void ** ppData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkMemoryMapInfoKHR * , void ** ))
        android_dlsym(vulkan_handle, "vkMapMemory2KHR"))
            (device, pMemoryMapInfo, ppData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkUnmapMemory2KHR( VkDevice  device, const VkMemoryUnmapInfoKHR * pMemoryUnmapInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkMemoryUnmapInfoKHR * ))
        android_dlsym(vulkan_handle, "vkUnmapMemory2KHR"))
            (device, pMemoryUnmapInfo);
}

#endif

#if VK_HEADER_VERSION >= 274

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR( VkPhysicalDevice  physicalDevice, const VkPhysicalDeviceVideoEncodeQualityLevelInfoKHR * pQualityLevelInfo,  VkVideoEncodeQualityLevelPropertiesKHR * pQualityLevelProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkPhysicalDeviceVideoEncodeQualityLevelInfoKHR * , VkVideoEncodeQualityLevelPropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR"))
            (physicalDevice, pQualityLevelInfo, pQualityLevelProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetEncodedVideoSessionParametersKHR( VkDevice  device, const VkVideoEncodeSessionParametersGetInfoKHR * pVideoSessionParametersInfo,  VkVideoEncodeSessionParametersFeedbackInfoKHR * pFeedbackInfo,  size_t * pDataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkVideoEncodeSessionParametersGetInfoKHR * , VkVideoEncodeSessionParametersFeedbackInfoKHR * , size_t * , void * ))
        android_dlsym(vulkan_handle, "vkGetEncodedVideoSessionParametersKHR"))
            (device, pVideoSessionParametersInfo, pFeedbackInfo, pDataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEncodeVideoKHR( VkCommandBuffer  commandBuffer, const VkVideoEncodeInfoKHR * pEncodeInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkVideoEncodeInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdEncodeVideoKHR"))
            (commandBuffer, pEncodeInfo);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkCmdSetEvent2KHR( VkCommandBuffer  commandBuffer,  VkEvent  event, const VkDependencyInfo * pDependencyInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkEvent  ,const VkDependencyInfo * ))
        android_dlsym(vulkan_handle, "vkCmdSetEvent2KHR"))
            (commandBuffer, event, pDependencyInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResetEvent2KHR( VkCommandBuffer  commandBuffer,  VkEvent  event,  VkPipelineStageFlags2  stageMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkEvent  , VkPipelineStageFlags2  ))
        android_dlsym(vulkan_handle, "vkCmdResetEvent2KHR"))
            (commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWaitEvents2KHR( VkCommandBuffer  commandBuffer,  uint32_t  eventCount, const VkEvent * pEvents, const VkDependencyInfo * pDependencyInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkEvent * ,const VkDependencyInfo * ))
        android_dlsym(vulkan_handle, "vkCmdWaitEvents2KHR"))
            (commandBuffer, eventCount, pEvents, pDependencyInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPipelineBarrier2KHR( VkCommandBuffer  commandBuffer, const VkDependencyInfo * pDependencyInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkDependencyInfo * ))
        android_dlsym(vulkan_handle, "vkCmdPipelineBarrier2KHR"))
            (commandBuffer, pDependencyInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteTimestamp2KHR( VkCommandBuffer  commandBuffer,  VkPipelineStageFlags2  stage,  VkQueryPool  queryPool,  uint32_t  query)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineStageFlags2  , VkQueryPool  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteTimestamp2KHR"))
            (commandBuffer, stage, queryPool, query);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSubmit2KHR( VkQueue  queue,  uint32_t  submitCount, const VkSubmitInfo2 * pSubmits,  VkFence  fence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkQueue  , uint32_t  ,const VkSubmitInfo2 * , VkFence  ))
        android_dlsym(vulkan_handle, "vkQueueSubmit2KHR"))
            (queue, submitCount, pSubmits, fence);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteBufferMarker2AMD( VkCommandBuffer  commandBuffer,  VkPipelineStageFlags2  stage,  VkBuffer  dstBuffer,  VkDeviceSize  dstOffset,  uint32_t  marker)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineStageFlags2  , VkBuffer  , VkDeviceSize  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteBufferMarker2AMD"))
            (commandBuffer, stage, dstBuffer, dstOffset, marker);
}

VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointData2NV( VkQueue  queue,  uint32_t * pCheckpointDataCount,  VkCheckpointData2NV * pCheckpointData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkQueue  , uint32_t * , VkCheckpointData2NV * ))
        android_dlsym(vulkan_handle, "vkGetQueueCheckpointData2NV"))
            (queue, pCheckpointDataCount, pCheckpointData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBuffer2KHR( VkCommandBuffer  commandBuffer, const VkCopyBufferInfo2 * pCopyBufferInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyBufferInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyBuffer2KHR"))
            (commandBuffer, pCopyBufferInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImage2KHR( VkCommandBuffer  commandBuffer, const VkCopyImageInfo2 * pCopyImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyImage2KHR"))
            (commandBuffer, pCopyImageInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyBufferToImage2KHR( VkCommandBuffer  commandBuffer, const VkCopyBufferToImageInfo2 * pCopyBufferToImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyBufferToImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyBufferToImage2KHR"))
            (commandBuffer, pCopyBufferToImageInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyImageToBuffer2KHR( VkCommandBuffer  commandBuffer, const VkCopyImageToBufferInfo2 * pCopyImageToBufferInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyImageToBufferInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdCopyImageToBuffer2KHR"))
            (commandBuffer, pCopyImageToBufferInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBlitImage2KHR( VkCommandBuffer  commandBuffer, const VkBlitImageInfo2 * pBlitImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkBlitImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdBlitImage2KHR"))
            (commandBuffer, pBlitImageInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdResolveImage2KHR( VkCommandBuffer  commandBuffer, const VkResolveImageInfo2 * pResolveImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkResolveImageInfo2 * ))
        android_dlsym(vulkan_handle, "vkCmdResolveImage2KHR"))
            (commandBuffer, pResolveImageInfo);
}

#if VK_HEADER_VERSION >= 213

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysIndirect2KHR( VkCommandBuffer  commandBuffer,  VkDeviceAddress  indirectDeviceAddress)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkDeviceAddress  ))
        android_dlsym(vulkan_handle, "vkCmdTraceRaysIndirect2KHR"))
            (commandBuffer, indirectDeviceAddress);
}

#endif

#if VK_HEADER_VERSION >= 195

VKAPI_ATTR void VKAPI_CALL vkGetDeviceBufferMemoryRequirementsKHR( VkDevice  device, const VkDeviceBufferMemoryRequirements * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceBufferMemoryRequirements * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetDeviceBufferMemoryRequirementsKHR"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageMemoryRequirementsKHR( VkDevice  device, const VkDeviceImageMemoryRequirements * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceImageMemoryRequirements * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetDeviceImageMemoryRequirementsKHR"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageSparseMemoryRequirementsKHR( VkDevice  device, const VkDeviceImageMemoryRequirements * pInfo,  uint32_t * pSparseMemoryRequirementCount,  VkSparseImageMemoryRequirements2 * pSparseMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceImageMemoryRequirements * , uint32_t * , VkSparseImageMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetDeviceImageSparseMemoryRequirementsKHR"))
            (device, pInfo, pSparseMemoryRequirementCount, pSparseMemoryRequirements);
}

#endif

#if VK_HEADER_VERSION >= 260

VKAPI_ATTR void VKAPI_CALL vkCmdBindIndexBuffer2KHR( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkDeviceSize  size,  VkIndexType  indexType)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkDeviceSize  , VkIndexType  ))
        android_dlsym(vulkan_handle, "vkCmdBindIndexBuffer2KHR"))
            (commandBuffer, buffer, offset, size, indexType);
}

VKAPI_ATTR void VKAPI_CALL vkGetRenderingAreaGranularityKHR( VkDevice  device, const VkRenderingAreaInfoKHR * pRenderingAreaInfo,  VkExtent2D * pGranularity)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkRenderingAreaInfoKHR * , VkExtent2D * ))
        android_dlsym(vulkan_handle, "vkGetRenderingAreaGranularityKHR"))
            (device, pRenderingAreaInfo, pGranularity);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceImageSubresourceLayoutKHR( VkDevice  device, const VkDeviceImageSubresourceInfoKHR * pInfo,  VkSubresourceLayout2KHR * pLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDeviceImageSubresourceInfoKHR * , VkSubresourceLayout2KHR * ))
        android_dlsym(vulkan_handle, "vkGetDeviceImageSubresourceLayoutKHR"))
            (device, pInfo, pLayout);
}

VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout2KHR( VkDevice  device,  VkImage  image, const VkImageSubresource2KHR * pSubresource,  VkSubresourceLayout2KHR * pLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkImage  ,const VkImageSubresource2KHR * , VkSubresourceLayout2KHR * ))
        android_dlsym(vulkan_handle, "vkGetImageSubresourceLayout2KHR"))
            (device, image, pSubresource, pLayout);
}

#endif

#if VK_HEADER_VERSION >= 255

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR( VkPhysicalDevice  physicalDevice,  uint32_t * pPropertyCount,  VkCooperativeMatrixPropertiesKHR * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkCooperativeMatrixPropertiesKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR"))
            (physicalDevice, pPropertyCount, pProperties);
}

#endif

#if VK_HEADER_VERSION >= 276

VKAPI_ATTR void VKAPI_CALL vkCmdSetLineStippleKHR( VkCommandBuffer  commandBuffer,  uint32_t  lineStippleFactor,  uint16_t  lineStipplePattern)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint16_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetLineStippleKHR"))
            (commandBuffer, lineStippleFactor, lineStipplePattern);
}

#endif

#if VK_HEADER_VERSION >= 273

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCalibrateableTimeDomainsKHR( VkPhysicalDevice  physicalDevice,  uint32_t * pTimeDomainCount,  VkTimeDomainKHR * pTimeDomains)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkTimeDomainKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceCalibrateableTimeDomainsKHR"))
            (physicalDevice, pTimeDomainCount, pTimeDomains);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetCalibratedTimestampsKHR( VkDevice  device,  uint32_t  timestampCount, const VkCalibratedTimestampInfoKHR * pTimestampInfos,  uint64_t * pTimestamps,  uint64_t * pMaxDeviation)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkCalibratedTimestampInfoKHR * , uint64_t * , uint64_t * ))
        android_dlsym(vulkan_handle, "vkGetCalibratedTimestampsKHR"))
            (device, timestampCount, pTimestampInfos, pTimestamps, pMaxDeviation);
}

#endif

#if VK_HEADER_VERSION >= 274

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorSets2KHR( VkCommandBuffer  commandBuffer, const VkBindDescriptorSetsInfoKHR * pBindDescriptorSetsInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkBindDescriptorSetsInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdBindDescriptorSets2KHR"))
            (commandBuffer, pBindDescriptorSetsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushConstants2KHR( VkCommandBuffer  commandBuffer, const VkPushConstantsInfoKHR * pPushConstantsInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkPushConstantsInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdPushConstants2KHR"))
            (commandBuffer, pPushConstantsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSet2KHR( VkCommandBuffer  commandBuffer, const VkPushDescriptorSetInfoKHR * pPushDescriptorSetInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkPushDescriptorSetInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdPushDescriptorSet2KHR"))
            (commandBuffer, pPushDescriptorSetInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPushDescriptorSetWithTemplate2KHR( VkCommandBuffer  commandBuffer, const VkPushDescriptorSetWithTemplateInfoKHR * pPushDescriptorSetWithTemplateInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkPushDescriptorSetWithTemplateInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdPushDescriptorSetWithTemplate2KHR"))
            (commandBuffer, pPushDescriptorSetWithTemplateInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDescriptorBufferOffsets2EXT( VkCommandBuffer  commandBuffer, const VkSetDescriptorBufferOffsetsInfoEXT * pSetDescriptorBufferOffsetsInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkSetDescriptorBufferOffsetsInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdSetDescriptorBufferOffsets2EXT"))
            (commandBuffer, pSetDescriptorBufferOffsetsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorBufferEmbeddedSamplers2EXT( VkCommandBuffer  commandBuffer, const VkBindDescriptorBufferEmbeddedSamplersInfoEXT * pBindDescriptorBufferEmbeddedSamplersInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkBindDescriptorBufferEmbeddedSamplersInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdBindDescriptorBufferEmbeddedSamplers2EXT"))
            (commandBuffer, pBindDescriptorBufferEmbeddedSamplersInfo);
}

#endif

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT( VkInstance  instance, const VkDebugReportCallbackCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDebugReportCallbackEXT * pCallback)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkInstance  ,const VkDebugReportCallbackCreateInfoEXT * ,const VkAllocationCallbacks * , VkDebugReportCallbackEXT * ))
        android_dlsym(vulkan_handle, "vkCreateDebugReportCallbackEXT"))
            (instance, pCreateInfo, pAllocator, pCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT( VkInstance  instance,  VkDebugReportCallbackEXT  callback, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkInstance  , VkDebugReportCallbackEXT  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDebugReportCallbackEXT"))
            (instance, callback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT( VkInstance  instance,  VkDebugReportFlagsEXT  flags,  VkDebugReportObjectTypeEXT  objectType,  uint64_t  object,  size_t  location,  int32_t  messageCode, const char * pLayerPrefix, const char * pMessage)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkInstance  , VkDebugReportFlagsEXT  , VkDebugReportObjectTypeEXT  , uint64_t  , size_t  , int32_t  ,const char * ,const char * ))
        android_dlsym(vulkan_handle, "vkDebugReportMessageEXT"))
            (instance, flags, objectType, object, location, messageCode, pLayerPrefix, pMessage);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectTagEXT( VkDevice  device, const VkDebugMarkerObjectTagInfoEXT * pTagInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDebugMarkerObjectTagInfoEXT * ))
        android_dlsym(vulkan_handle, "vkDebugMarkerSetObjectTagEXT"))
            (device, pTagInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDebugMarkerSetObjectNameEXT( VkDevice  device, const VkDebugMarkerObjectNameInfoEXT * pNameInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDebugMarkerObjectNameInfoEXT * ))
        android_dlsym(vulkan_handle, "vkDebugMarkerSetObjectNameEXT"))
            (device, pNameInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerBeginEXT( VkCommandBuffer  commandBuffer, const VkDebugMarkerMarkerInfoEXT * pMarkerInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkDebugMarkerMarkerInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdDebugMarkerBeginEXT"))
            (commandBuffer, pMarkerInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerEndEXT( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkCmdDebugMarkerEndEXT"))
            (commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDebugMarkerInsertEXT( VkCommandBuffer  commandBuffer, const VkDebugMarkerMarkerInfoEXT * pMarkerInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkDebugMarkerMarkerInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdDebugMarkerInsertEXT"))
            (commandBuffer, pMarkerInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindTransformFeedbackBuffersEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstBinding,  uint32_t  bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets, const VkDeviceSize * pSizes)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBuffer * ,const VkDeviceSize * ,const VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkCmdBindTransformFeedbackBuffersEXT"))
            (commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginTransformFeedbackEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstCounterBuffer,  uint32_t  counterBufferCount, const VkBuffer * pCounterBuffers, const VkDeviceSize * pCounterBufferOffsets)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBuffer * ,const VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkCmdBeginTransformFeedbackEXT"))
            (commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndTransformFeedbackEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstCounterBuffer,  uint32_t  counterBufferCount, const VkBuffer * pCounterBuffers, const VkDeviceSize * pCounterBufferOffsets)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBuffer * ,const VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkCmdEndTransformFeedbackEXT"))
            (commandBuffer, firstCounterBuffer, counterBufferCount, pCounterBuffers, pCounterBufferOffsets);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginQueryIndexedEXT( VkCommandBuffer  commandBuffer,  VkQueryPool  queryPool,  uint32_t  query,  VkQueryControlFlags  flags,  uint32_t  index)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkQueryPool  , uint32_t  , VkQueryControlFlags  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdBeginQueryIndexedEXT"))
            (commandBuffer, queryPool, query, flags, index);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndQueryIndexedEXT( VkCommandBuffer  commandBuffer,  VkQueryPool  queryPool,  uint32_t  query,  uint32_t  index)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkQueryPool  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdEndQueryIndexedEXT"))
            (commandBuffer, queryPool, query, index);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectByteCountEXT( VkCommandBuffer  commandBuffer,  uint32_t  instanceCount,  uint32_t  firstInstance,  VkBuffer  counterBuffer,  VkDeviceSize  counterBufferOffset,  uint32_t  counterOffset,  uint32_t  vertexStride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndirectByteCountEXT"))
            (commandBuffer, instanceCount, firstInstance, counterBuffer, counterBufferOffset, counterOffset, vertexStride);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCuModuleNVX( VkDevice  device, const VkCuModuleCreateInfoNVX * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkCuModuleNVX * pModule)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCuModuleCreateInfoNVX * ,const VkAllocationCallbacks * , VkCuModuleNVX * ))
        android_dlsym(vulkan_handle, "vkCreateCuModuleNVX"))
            (device, pCreateInfo, pAllocator, pModule);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCuFunctionNVX( VkDevice  device, const VkCuFunctionCreateInfoNVX * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkCuFunctionNVX * pFunction)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCuFunctionCreateInfoNVX * ,const VkAllocationCallbacks * , VkCuFunctionNVX * ))
        android_dlsym(vulkan_handle, "vkCreateCuFunctionNVX"))
            (device, pCreateInfo, pAllocator, pFunction);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCuModuleNVX( VkDevice  device,  VkCuModuleNVX  module, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCuModuleNVX  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyCuModuleNVX"))
            (device, module, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCuFunctionNVX( VkDevice  device,  VkCuFunctionNVX  function, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCuFunctionNVX  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyCuFunctionNVX"))
            (device, function, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCuLaunchKernelNVX( VkCommandBuffer  commandBuffer, const VkCuLaunchInfoNVX * pLaunchInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCuLaunchInfoNVX * ))
        android_dlsym(vulkan_handle, "vkCmdCuLaunchKernelNVX"))
            (commandBuffer, pLaunchInfo);
}

VKAPI_ATTR uint32_t VKAPI_CALL vkGetImageViewHandleNVX( VkDevice  device, const VkImageViewHandleInfoNVX * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((uint32_t (*)( VkDevice  ,const VkImageViewHandleInfoNVX * ))
        android_dlsym(vulkan_handle, "vkGetImageViewHandleNVX"))
            (device, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetImageViewAddressNVX( VkDevice  device,  VkImageView  imageView,  VkImageViewAddressPropertiesNVX * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkImageView  , VkImageViewAddressPropertiesNVX * ))
        android_dlsym(vulkan_handle, "vkGetImageViewAddressNVX"))
            (device, imageView, pProperties);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndirectCountAMD( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndirectCountAMD"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawIndexedIndirectCountAMD( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawIndexedIndirectCountAMD"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetShaderInfoAMD( VkDevice  device,  VkPipeline  pipeline,  VkShaderStageFlagBits  shaderStage,  VkShaderInfoTypeAMD  infoType,  size_t * pInfoSize,  void * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipeline  , VkShaderStageFlagBits  , VkShaderInfoTypeAMD  , size_t * , void * ))
        android_dlsym(vulkan_handle, "vkGetShaderInfoAMD"))
            (device, pipeline, shaderStage, infoType, pInfoSize, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceExternalImageFormatPropertiesNV( VkPhysicalDevice  physicalDevice,  VkFormat  format,  VkImageType  type,  VkImageTiling  tiling,  VkImageUsageFlags  usage,  VkImageCreateFlags  flags,  VkExternalMemoryHandleTypeFlagsNV  externalHandleType,  VkExternalImageFormatPropertiesNV * pExternalImageFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkFormat  , VkImageType  , VkImageTiling  , VkImageUsageFlags  , VkImageCreateFlags  , VkExternalMemoryHandleTypeFlagsNV  , VkExternalImageFormatPropertiesNV * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceExternalImageFormatPropertiesNV"))
            (physicalDevice, format, type, tiling, usage, flags, externalHandleType, pExternalImageFormatProperties);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginConditionalRenderingEXT( VkCommandBuffer  commandBuffer, const VkConditionalRenderingBeginInfoEXT * pConditionalRenderingBegin)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkConditionalRenderingBeginInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdBeginConditionalRenderingEXT"))
            (commandBuffer, pConditionalRenderingBegin);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndConditionalRenderingEXT( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkCmdEndConditionalRenderingEXT"))
            (commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWScalingNV( VkCommandBuffer  commandBuffer,  uint32_t  firstViewport,  uint32_t  viewportCount, const VkViewportWScalingNV * pViewportWScalings)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkViewportWScalingNV * ))
        android_dlsym(vulkan_handle, "vkCmdSetViewportWScalingNV"))
            (commandBuffer, firstViewport, viewportCount, pViewportWScalings);
}

VKAPI_ATTR VkResult VKAPI_CALL vkReleaseDisplayEXT( VkPhysicalDevice  physicalDevice,  VkDisplayKHR  display)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkDisplayKHR  ))
        android_dlsym(vulkan_handle, "vkReleaseDisplayEXT"))
            (physicalDevice, display);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSurfaceCapabilities2EXT( VkPhysicalDevice  physicalDevice,  VkSurfaceKHR  surface,  VkSurfaceCapabilities2EXT * pSurfaceCapabilities)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , VkSurfaceKHR  , VkSurfaceCapabilities2EXT * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSurfaceCapabilities2EXT"))
            (physicalDevice, surface, pSurfaceCapabilities);
}

VKAPI_ATTR VkResult VKAPI_CALL vkDisplayPowerControlEXT( VkDevice  device,  VkDisplayKHR  display, const VkDisplayPowerInfoEXT * pDisplayPowerInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDisplayKHR  ,const VkDisplayPowerInfoEXT * ))
        android_dlsym(vulkan_handle, "vkDisplayPowerControlEXT"))
            (device, display, pDisplayPowerInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkRegisterDeviceEventEXT( VkDevice  device, const VkDeviceEventInfoEXT * pDeviceEventInfo, const VkAllocationCallbacks * pAllocator,  VkFence * pFence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDeviceEventInfoEXT * ,const VkAllocationCallbacks * , VkFence * ))
        android_dlsym(vulkan_handle, "vkRegisterDeviceEventEXT"))
            (device, pDeviceEventInfo, pAllocator, pFence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkRegisterDisplayEventEXT( VkDevice  device,  VkDisplayKHR  display, const VkDisplayEventInfoEXT * pDisplayEventInfo, const VkAllocationCallbacks * pAllocator,  VkFence * pFence)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDisplayKHR  ,const VkDisplayEventInfoEXT * ,const VkAllocationCallbacks * , VkFence * ))
        android_dlsym(vulkan_handle, "vkRegisterDisplayEventEXT"))
            (device, display, pDisplayEventInfo, pAllocator, pFence);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSwapchainCounterEXT( VkDevice  device,  VkSwapchainKHR  swapchain,  VkSurfaceCounterFlagBitsEXT  counter,  uint64_t * pCounterValue)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  , VkSurfaceCounterFlagBitsEXT  , uint64_t * ))
        android_dlsym(vulkan_handle, "vkGetSwapchainCounterEXT"))
            (device, swapchain, counter, pCounterValue);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRefreshCycleDurationGOOGLE( VkDevice  device,  VkSwapchainKHR  swapchain,  VkRefreshCycleDurationGOOGLE * pDisplayTimingProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  , VkRefreshCycleDurationGOOGLE * ))
        android_dlsym(vulkan_handle, "vkGetRefreshCycleDurationGOOGLE"))
            (device, swapchain, pDisplayTimingProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPastPresentationTimingGOOGLE( VkDevice  device,  VkSwapchainKHR  swapchain,  uint32_t * pPresentationTimingCount,  VkPastPresentationTimingGOOGLE * pPresentationTimings)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  , uint32_t * , VkPastPresentationTimingGOOGLE * ))
        android_dlsym(vulkan_handle, "vkGetPastPresentationTimingGOOGLE"))
            (device, swapchain, pPresentationTimingCount, pPresentationTimings);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDiscardRectangleEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstDiscardRectangle,  uint32_t  discardRectangleCount, const VkRect2D * pDiscardRectangles)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkRect2D * ))
        android_dlsym(vulkan_handle, "vkCmdSetDiscardRectangleEXT"))
            (commandBuffer, firstDiscardRectangle, discardRectangleCount, pDiscardRectangles);
}

#if VK_HEADER_VERSION >= 241

VKAPI_ATTR void VKAPI_CALL vkCmdSetDiscardRectangleEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  discardRectangleEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDiscardRectangleEnableEXT"))
            (commandBuffer, discardRectangleEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDiscardRectangleModeEXT( VkCommandBuffer  commandBuffer,  VkDiscardRectangleModeEXT  discardRectangleMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkDiscardRectangleModeEXT  ))
        android_dlsym(vulkan_handle, "vkCmdSetDiscardRectangleModeEXT"))
            (commandBuffer, discardRectangleMode);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkSetHdrMetadataEXT( VkDevice  device,  uint32_t  swapchainCount, const VkSwapchainKHR * pSwapchains, const VkHdrMetadataEXT * pMetadata)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , uint32_t  ,const VkSwapchainKHR * ,const VkHdrMetadataEXT * ))
        android_dlsym(vulkan_handle, "vkSetHdrMetadataEXT"))
            (device, swapchainCount, pSwapchains, pMetadata);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectNameEXT( VkDevice  device, const VkDebugUtilsObjectNameInfoEXT * pNameInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDebugUtilsObjectNameInfoEXT * ))
        android_dlsym(vulkan_handle, "vkSetDebugUtilsObjectNameEXT"))
            (device, pNameInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetDebugUtilsObjectTagEXT( VkDevice  device, const VkDebugUtilsObjectTagInfoEXT * pTagInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkDebugUtilsObjectTagInfoEXT * ))
        android_dlsym(vulkan_handle, "vkSetDebugUtilsObjectTagEXT"))
            (device, pTagInfo);
}

VKAPI_ATTR void VKAPI_CALL vkQueueBeginDebugUtilsLabelEXT( VkQueue  queue, const VkDebugUtilsLabelEXT * pLabelInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkQueue  ,const VkDebugUtilsLabelEXT * ))
        android_dlsym(vulkan_handle, "vkQueueBeginDebugUtilsLabelEXT"))
            (queue, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkQueueEndDebugUtilsLabelEXT( VkQueue  queue)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkQueue  ))
        android_dlsym(vulkan_handle, "vkQueueEndDebugUtilsLabelEXT"))
            (queue);
}

VKAPI_ATTR void VKAPI_CALL vkQueueInsertDebugUtilsLabelEXT( VkQueue  queue, const VkDebugUtilsLabelEXT * pLabelInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkQueue  ,const VkDebugUtilsLabelEXT * ))
        android_dlsym(vulkan_handle, "vkQueueInsertDebugUtilsLabelEXT"))
            (queue, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBeginDebugUtilsLabelEXT( VkCommandBuffer  commandBuffer, const VkDebugUtilsLabelEXT * pLabelInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkDebugUtilsLabelEXT * ))
        android_dlsym(vulkan_handle, "vkCmdBeginDebugUtilsLabelEXT"))
            (commandBuffer, pLabelInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdEndDebugUtilsLabelEXT( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkCmdEndDebugUtilsLabelEXT"))
            (commandBuffer);
}

VKAPI_ATTR void VKAPI_CALL vkCmdInsertDebugUtilsLabelEXT( VkCommandBuffer  commandBuffer, const VkDebugUtilsLabelEXT * pLabelInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkDebugUtilsLabelEXT * ))
        android_dlsym(vulkan_handle, "vkCmdInsertDebugUtilsLabelEXT"))
            (commandBuffer, pLabelInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT( VkInstance  instance, const VkDebugUtilsMessengerCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkDebugUtilsMessengerEXT * pMessenger)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkInstance  ,const VkDebugUtilsMessengerCreateInfoEXT * ,const VkAllocationCallbacks * , VkDebugUtilsMessengerEXT * ))
        android_dlsym(vulkan_handle, "vkCreateDebugUtilsMessengerEXT"))
            (instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT( VkInstance  instance,  VkDebugUtilsMessengerEXT  messenger, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkInstance  , VkDebugUtilsMessengerEXT  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyDebugUtilsMessengerEXT"))
            (instance, messenger, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkSubmitDebugUtilsMessageEXT( VkInstance  instance,  VkDebugUtilsMessageSeverityFlagBitsEXT  messageSeverity,  VkDebugUtilsMessageTypeFlagsEXT  messageTypes, const VkDebugUtilsMessengerCallbackDataEXT * pCallbackData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkInstance  , VkDebugUtilsMessageSeverityFlagBitsEXT  , VkDebugUtilsMessageTypeFlagsEXT  ,const VkDebugUtilsMessengerCallbackDataEXT * ))
        android_dlsym(vulkan_handle, "vkSubmitDebugUtilsMessageEXT"))
            (instance, messageSeverity, messageTypes, pCallbackData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleLocationsEXT( VkCommandBuffer  commandBuffer, const VkSampleLocationsInfoEXT * pSampleLocationsInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkSampleLocationsInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdSetSampleLocationsEXT"))
            (commandBuffer, pSampleLocationsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkGetPhysicalDeviceMultisamplePropertiesEXT( VkPhysicalDevice  physicalDevice,  VkSampleCountFlagBits  samples,  VkMultisamplePropertiesEXT * pMultisampleProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkPhysicalDevice  , VkSampleCountFlagBits  , VkMultisamplePropertiesEXT * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceMultisamplePropertiesEXT"))
            (physicalDevice, samples, pMultisampleProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetImageDrmFormatModifierPropertiesEXT( VkDevice  device,  VkImage  image,  VkImageDrmFormatModifierPropertiesEXT * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkImage  , VkImageDrmFormatModifierPropertiesEXT * ))
        android_dlsym(vulkan_handle, "vkGetImageDrmFormatModifierPropertiesEXT"))
            (device, image, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateValidationCacheEXT( VkDevice  device, const VkValidationCacheCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkValidationCacheEXT * pValidationCache)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkValidationCacheCreateInfoEXT * ,const VkAllocationCallbacks * , VkValidationCacheEXT * ))
        android_dlsym(vulkan_handle, "vkCreateValidationCacheEXT"))
            (device, pCreateInfo, pAllocator, pValidationCache);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyValidationCacheEXT( VkDevice  device,  VkValidationCacheEXT  validationCache, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkValidationCacheEXT  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyValidationCacheEXT"))
            (device, validationCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkMergeValidationCachesEXT( VkDevice  device,  VkValidationCacheEXT  dstCache,  uint32_t  srcCacheCount, const VkValidationCacheEXT * pSrcCaches)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkValidationCacheEXT  , uint32_t  ,const VkValidationCacheEXT * ))
        android_dlsym(vulkan_handle, "vkMergeValidationCachesEXT"))
            (device, dstCache, srcCacheCount, pSrcCaches);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetValidationCacheDataEXT( VkDevice  device,  VkValidationCacheEXT  validationCache,  size_t * pDataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkValidationCacheEXT  , size_t * , void * ))
        android_dlsym(vulkan_handle, "vkGetValidationCacheDataEXT"))
            (device, validationCache, pDataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindShadingRateImageNV( VkCommandBuffer  commandBuffer,  VkImageView  imageView,  VkImageLayout  imageLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImageView  , VkImageLayout  ))
        android_dlsym(vulkan_handle, "vkCmdBindShadingRateImageNV"))
            (commandBuffer, imageView, imageLayout);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportShadingRatePaletteNV( VkCommandBuffer  commandBuffer,  uint32_t  firstViewport,  uint32_t  viewportCount, const VkShadingRatePaletteNV * pShadingRatePalettes)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkShadingRatePaletteNV * ))
        android_dlsym(vulkan_handle, "vkCmdSetViewportShadingRatePaletteNV"))
            (commandBuffer, firstViewport, viewportCount, pShadingRatePalettes);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCoarseSampleOrderNV( VkCommandBuffer  commandBuffer,  VkCoarseSampleOrderTypeNV  sampleOrderType,  uint32_t  customSampleOrderCount, const VkCoarseSampleOrderCustomNV * pCustomSampleOrders)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkCoarseSampleOrderTypeNV  , uint32_t  ,const VkCoarseSampleOrderCustomNV * ))
        android_dlsym(vulkan_handle, "vkCmdSetCoarseSampleOrderNV"))
            (commandBuffer, sampleOrderType, customSampleOrderCount, pCustomSampleOrders);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureNV( VkDevice  device, const VkAccelerationStructureCreateInfoNV * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkAccelerationStructureNV * pAccelerationStructure)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkAccelerationStructureCreateInfoNV * ,const VkAllocationCallbacks * , VkAccelerationStructureNV * ))
        android_dlsym(vulkan_handle, "vkCreateAccelerationStructureNV"))
            (device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureNV( VkDevice  device,  VkAccelerationStructureNV  accelerationStructure, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkAccelerationStructureNV  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyAccelerationStructureNV"))
            (device, accelerationStructure, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureMemoryRequirementsNV( VkDevice  device, const VkAccelerationStructureMemoryRequirementsInfoNV * pInfo,  VkMemoryRequirements2KHR * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkAccelerationStructureMemoryRequirementsInfoNV * , VkMemoryRequirements2KHR * ))
        android_dlsym(vulkan_handle, "vkGetAccelerationStructureMemoryRequirementsNV"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindAccelerationStructureMemoryNV( VkDevice  device,  uint32_t  bindInfoCount, const VkBindAccelerationStructureMemoryInfoNV * pBindInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkBindAccelerationStructureMemoryInfoNV * ))
        android_dlsym(vulkan_handle, "vkBindAccelerationStructureMemoryNV"))
            (device, bindInfoCount, pBindInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructureNV( VkCommandBuffer  commandBuffer, const VkAccelerationStructureInfoNV * pInfo,  VkBuffer  instanceData,  VkDeviceSize  instanceOffset,  VkBool32  update,  VkAccelerationStructureNV  dst,  VkAccelerationStructureNV  src,  VkBuffer  scratch,  VkDeviceSize  scratchOffset)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkAccelerationStructureInfoNV * , VkBuffer  , VkDeviceSize  , VkBool32  , VkAccelerationStructureNV  , VkAccelerationStructureNV  , VkBuffer  , VkDeviceSize  ))
        android_dlsym(vulkan_handle, "vkCmdBuildAccelerationStructureNV"))
            (commandBuffer, pInfo, instanceData, instanceOffset, update, dst, src, scratch, scratchOffset);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureNV( VkCommandBuffer  commandBuffer,  VkAccelerationStructureNV  dst,  VkAccelerationStructureNV  src,  VkCopyAccelerationStructureModeKHR  mode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkAccelerationStructureNV  , VkAccelerationStructureNV  , VkCopyAccelerationStructureModeKHR  ))
        android_dlsym(vulkan_handle, "vkCmdCopyAccelerationStructureNV"))
            (commandBuffer, dst, src, mode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysNV( VkCommandBuffer  commandBuffer,  VkBuffer  raygenShaderBindingTableBuffer,  VkDeviceSize  raygenShaderBindingOffset,  VkBuffer  missShaderBindingTableBuffer,  VkDeviceSize  missShaderBindingOffset,  VkDeviceSize  missShaderBindingStride,  VkBuffer  hitShaderBindingTableBuffer,  VkDeviceSize  hitShaderBindingOffset,  VkDeviceSize  hitShaderBindingStride,  VkBuffer  callableShaderBindingTableBuffer,  VkDeviceSize  callableShaderBindingOffset,  VkDeviceSize  callableShaderBindingStride,  uint32_t  width,  uint32_t  height,  uint32_t  depth)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , VkDeviceSize  , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdTraceRaysNV"))
            (commandBuffer, raygenShaderBindingTableBuffer, raygenShaderBindingOffset, missShaderBindingTableBuffer, missShaderBindingOffset, missShaderBindingStride, hitShaderBindingTableBuffer, hitShaderBindingOffset, hitShaderBindingStride, callableShaderBindingTableBuffer, callableShaderBindingOffset, callableShaderBindingStride, width, height, depth);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesNV( VkDevice  device,  VkPipelineCache  pipelineCache,  uint32_t  createInfoCount, const VkRayTracingPipelineCreateInfoNV * pCreateInfos, const VkAllocationCallbacks * pAllocator,  VkPipeline * pPipelines)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipelineCache  , uint32_t  ,const VkRayTracingPipelineCreateInfoNV * ,const VkAllocationCallbacks * , VkPipeline * ))
        android_dlsym(vulkan_handle, "vkCreateRayTracingPipelinesNV"))
            (device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesKHR( VkDevice  device,  VkPipeline  pipeline,  uint32_t  firstGroup,  uint32_t  groupCount,  size_t  dataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipeline  , uint32_t  , uint32_t  , size_t  , void * ))
        android_dlsym(vulkan_handle, "vkGetRayTracingShaderGroupHandlesKHR"))
            (device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingShaderGroupHandlesNV( VkDevice  device,  VkPipeline  pipeline,  uint32_t  firstGroup,  uint32_t  groupCount,  size_t  dataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipeline  , uint32_t  , uint32_t  , size_t  , void * ))
        android_dlsym(vulkan_handle, "vkGetRayTracingShaderGroupHandlesNV"))
            (device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetAccelerationStructureHandleNV( VkDevice  device,  VkAccelerationStructureNV  accelerationStructure,  size_t  dataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkAccelerationStructureNV  , size_t  , void * ))
        android_dlsym(vulkan_handle, "vkGetAccelerationStructureHandleNV"))
            (device, accelerationStructure, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesNV( VkCommandBuffer  commandBuffer,  uint32_t  accelerationStructureCount, const VkAccelerationStructureNV * pAccelerationStructures,  VkQueryType  queryType,  VkQueryPool  queryPool,  uint32_t  firstQuery)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkAccelerationStructureNV * , VkQueryType  , VkQueryPool  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteAccelerationStructuresPropertiesNV"))
            (commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCompileDeferredNV( VkDevice  device,  VkPipeline  pipeline,  uint32_t  shader)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipeline  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCompileDeferredNV"))
            (device, pipeline, shader);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryHostPointerPropertiesEXT( VkDevice  device,  VkExternalMemoryHandleTypeFlagBits  handleType, const void * pHostPointer,  VkMemoryHostPointerPropertiesEXT * pMemoryHostPointerProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkExternalMemoryHandleTypeFlagBits  ,const void * , VkMemoryHostPointerPropertiesEXT * ))
        android_dlsym(vulkan_handle, "vkGetMemoryHostPointerPropertiesEXT"))
            (device, handleType, pHostPointer, pMemoryHostPointerProperties);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteBufferMarkerAMD( VkCommandBuffer  commandBuffer,  VkPipelineStageFlagBits  pipelineStage,  VkBuffer  dstBuffer,  VkDeviceSize  dstOffset,  uint32_t  marker)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineStageFlagBits  , VkBuffer  , VkDeviceSize  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteBufferMarkerAMD"))
            (commandBuffer, pipelineStage, dstBuffer, dstOffset, marker);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCalibrateableTimeDomainsEXT( VkPhysicalDevice  physicalDevice,  uint32_t * pTimeDomainCount,  VkTimeDomainKHR * pTimeDomains)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkTimeDomainKHR * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceCalibrateableTimeDomainsEXT"))
            (physicalDevice, pTimeDomainCount, pTimeDomains);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetCalibratedTimestampsEXT( VkDevice  device,  uint32_t  timestampCount, const VkCalibratedTimestampInfoKHR * pTimestampInfos,  uint64_t * pTimestamps,  uint64_t * pMaxDeviation)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkCalibratedTimestampInfoKHR * , uint64_t * , uint64_t * ))
        android_dlsym(vulkan_handle, "vkGetCalibratedTimestampsEXT"))
            (device, timestampCount, pTimestampInfos, pTimestamps, pMaxDeviation);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksNV( VkCommandBuffer  commandBuffer,  uint32_t  taskCount,  uint32_t  firstTask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawMeshTasksNV"))
            (commandBuffer, taskCount, firstTask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectNV( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  uint32_t  drawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawMeshTasksIndirectNV"))
            (commandBuffer, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectCountNV( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawMeshTasksIndirectCountNV"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

#if VK_HEADER_VERSION >= 241

VKAPI_ATTR void VKAPI_CALL vkCmdSetExclusiveScissorEnableNV( VkCommandBuffer  commandBuffer,  uint32_t  firstExclusiveScissor,  uint32_t  exclusiveScissorCount, const VkBool32 * pExclusiveScissorEnables)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBool32 * ))
        android_dlsym(vulkan_handle, "vkCmdSetExclusiveScissorEnableNV"))
            (commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissorEnables);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkCmdSetExclusiveScissorNV( VkCommandBuffer  commandBuffer,  uint32_t  firstExclusiveScissor,  uint32_t  exclusiveScissorCount, const VkRect2D * pExclusiveScissors)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkRect2D * ))
        android_dlsym(vulkan_handle, "vkCmdSetExclusiveScissorNV"))
            (commandBuffer, firstExclusiveScissor, exclusiveScissorCount, pExclusiveScissors);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCheckpointNV( VkCommandBuffer  commandBuffer, const void * pCheckpointMarker)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const void * ))
        android_dlsym(vulkan_handle, "vkCmdSetCheckpointNV"))
            (commandBuffer, pCheckpointMarker);
}

VKAPI_ATTR void VKAPI_CALL vkGetQueueCheckpointDataNV( VkQueue  queue,  uint32_t * pCheckpointDataCount,  VkCheckpointDataNV * pCheckpointData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkQueue  , uint32_t * , VkCheckpointDataNV * ))
        android_dlsym(vulkan_handle, "vkGetQueueCheckpointDataNV"))
            (queue, pCheckpointDataCount, pCheckpointData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkInitializePerformanceApiINTEL( VkDevice  device, const VkInitializePerformanceApiInfoINTEL * pInitializeInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkInitializePerformanceApiInfoINTEL * ))
        android_dlsym(vulkan_handle, "vkInitializePerformanceApiINTEL"))
            (device, pInitializeInfo);
}

VKAPI_ATTR void VKAPI_CALL vkUninitializePerformanceApiINTEL( VkDevice  device)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ))
        android_dlsym(vulkan_handle, "vkUninitializePerformanceApiINTEL"))
            (device);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceMarkerINTEL( VkCommandBuffer  commandBuffer, const VkPerformanceMarkerInfoINTEL * pMarkerInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkCommandBuffer  ,const VkPerformanceMarkerInfoINTEL * ))
        android_dlsym(vulkan_handle, "vkCmdSetPerformanceMarkerINTEL"))
            (commandBuffer, pMarkerInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceStreamMarkerINTEL( VkCommandBuffer  commandBuffer, const VkPerformanceStreamMarkerInfoINTEL * pMarkerInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkCommandBuffer  ,const VkPerformanceStreamMarkerInfoINTEL * ))
        android_dlsym(vulkan_handle, "vkCmdSetPerformanceStreamMarkerINTEL"))
            (commandBuffer, pMarkerInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCmdSetPerformanceOverrideINTEL( VkCommandBuffer  commandBuffer, const VkPerformanceOverrideInfoINTEL * pOverrideInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkCommandBuffer  ,const VkPerformanceOverrideInfoINTEL * ))
        android_dlsym(vulkan_handle, "vkCmdSetPerformanceOverrideINTEL"))
            (commandBuffer, pOverrideInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquirePerformanceConfigurationINTEL( VkDevice  device, const VkPerformanceConfigurationAcquireInfoINTEL * pAcquireInfo,  VkPerformanceConfigurationINTEL * pConfiguration)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPerformanceConfigurationAcquireInfoINTEL * , VkPerformanceConfigurationINTEL * ))
        android_dlsym(vulkan_handle, "vkAcquirePerformanceConfigurationINTEL"))
            (device, pAcquireInfo, pConfiguration);
}

VKAPI_ATTR VkResult VKAPI_CALL vkReleasePerformanceConfigurationINTEL( VkDevice  device,  VkPerformanceConfigurationINTEL  configuration)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPerformanceConfigurationINTEL  ))
        android_dlsym(vulkan_handle, "vkReleasePerformanceConfigurationINTEL"))
            (device, configuration);
}

VKAPI_ATTR VkResult VKAPI_CALL vkQueueSetPerformanceConfigurationINTEL( VkQueue  queue,  VkPerformanceConfigurationINTEL  configuration)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkQueue  , VkPerformanceConfigurationINTEL  ))
        android_dlsym(vulkan_handle, "vkQueueSetPerformanceConfigurationINTEL"))
            (queue, configuration);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPerformanceParameterINTEL( VkDevice  device,  VkPerformanceParameterTypeINTEL  parameter,  VkPerformanceValueINTEL * pValue)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPerformanceParameterTypeINTEL  , VkPerformanceValueINTEL * ))
        android_dlsym(vulkan_handle, "vkGetPerformanceParameterINTEL"))
            (device, parameter, pValue);
}

VKAPI_ATTR void VKAPI_CALL vkSetLocalDimmingAMD( VkDevice  device,  VkSwapchainKHR  swapChain,  VkBool32  localDimmingEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSwapchainKHR  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkSetLocalDimmingAMD"))
            (device, swapChain, localDimmingEnable);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetBufferDeviceAddressEXT( VkDevice  device, const VkBufferDeviceAddressInfo * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkDeviceAddress (*)( VkDevice  ,const VkBufferDeviceAddressInfo * ))
        android_dlsym(vulkan_handle, "vkGetBufferDeviceAddressEXT"))
            (device, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceToolPropertiesEXT( VkPhysicalDevice  physicalDevice,  uint32_t * pToolCount,  VkPhysicalDeviceToolProperties * pToolProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkPhysicalDeviceToolProperties * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceToolPropertiesEXT"))
            (physicalDevice, pToolCount, pToolProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceCooperativeMatrixPropertiesNV( VkPhysicalDevice  physicalDevice,  uint32_t * pPropertyCount,  VkCooperativeMatrixPropertiesNV * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkCooperativeMatrixPropertiesNV * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceCooperativeMatrixPropertiesNV"))
            (physicalDevice, pPropertyCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV( VkPhysicalDevice  physicalDevice,  uint32_t * pCombinationCount,  VkFramebufferMixedSamplesCombinationNV * pCombinations)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , uint32_t * , VkFramebufferMixedSamplesCombinationNV * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV"))
            (physicalDevice, pCombinationCount, pCombinations);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateHeadlessSurfaceEXT( VkInstance  instance, const VkHeadlessSurfaceCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkSurfaceKHR * pSurface)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkInstance  ,const VkHeadlessSurfaceCreateInfoEXT * ,const VkAllocationCallbacks * , VkSurfaceKHR * ))
        android_dlsym(vulkan_handle, "vkCreateHeadlessSurfaceEXT"))
            (instance, pCreateInfo, pAllocator, pSurface);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetLineStippleEXT( VkCommandBuffer  commandBuffer,  uint32_t  lineStippleFactor,  uint16_t  lineStipplePattern)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint16_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetLineStippleEXT"))
            (commandBuffer, lineStippleFactor, lineStipplePattern);
}

VKAPI_ATTR void VKAPI_CALL vkResetQueryPoolEXT( VkDevice  device,  VkQueryPool  queryPool,  uint32_t  firstQuery,  uint32_t  queryCount)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkQueryPool  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkResetQueryPoolEXT"))
            (device, queryPool, firstQuery, queryCount);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCullModeEXT( VkCommandBuffer  commandBuffer,  VkCullModeFlags  cullMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkCullModeFlags  ))
        android_dlsym(vulkan_handle, "vkCmdSetCullModeEXT"))
            (commandBuffer, cullMode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetFrontFaceEXT( VkCommandBuffer  commandBuffer,  VkFrontFace  frontFace)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkFrontFace  ))
        android_dlsym(vulkan_handle, "vkCmdSetFrontFaceEXT"))
            (commandBuffer, frontFace);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveTopologyEXT( VkCommandBuffer  commandBuffer,  VkPrimitiveTopology  primitiveTopology)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPrimitiveTopology  ))
        android_dlsym(vulkan_handle, "vkCmdSetPrimitiveTopologyEXT"))
            (commandBuffer, primitiveTopology);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWithCountEXT( VkCommandBuffer  commandBuffer,  uint32_t  viewportCount, const VkViewport * pViewports)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkViewport * ))
        android_dlsym(vulkan_handle, "vkCmdSetViewportWithCountEXT"))
            (commandBuffer, viewportCount, pViewports);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetScissorWithCountEXT( VkCommandBuffer  commandBuffer,  uint32_t  scissorCount, const VkRect2D * pScissors)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkRect2D * ))
        android_dlsym(vulkan_handle, "vkCmdSetScissorWithCountEXT"))
            (commandBuffer, scissorCount, pScissors);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindVertexBuffers2EXT( VkCommandBuffer  commandBuffer,  uint32_t  firstBinding,  uint32_t  bindingCount, const VkBuffer * pBuffers, const VkDeviceSize * pOffsets, const VkDeviceSize * pSizes, const VkDeviceSize * pStrides)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBuffer * ,const VkDeviceSize * ,const VkDeviceSize * ,const VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkCmdBindVertexBuffers2EXT"))
            (commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets, pSizes, pStrides);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthTestEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  depthTestEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthTestEnableEXT"))
            (commandBuffer, depthTestEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthWriteEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  depthWriteEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthWriteEnableEXT"))
            (commandBuffer, depthWriteEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthCompareOpEXT( VkCommandBuffer  commandBuffer,  VkCompareOp  depthCompareOp)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkCompareOp  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthCompareOpEXT"))
            (commandBuffer, depthCompareOp);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBoundsTestEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  depthBoundsTestEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthBoundsTestEnableEXT"))
            (commandBuffer, depthBoundsTestEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilTestEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  stencilTestEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetStencilTestEnableEXT"))
            (commandBuffer, stencilTestEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilOpEXT( VkCommandBuffer  commandBuffer,  VkStencilFaceFlags  faceMask,  VkStencilOp  failOp,  VkStencilOp  passOp,  VkStencilOp  depthFailOp,  VkCompareOp  compareOp)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkStencilFaceFlags  , VkStencilOp  , VkStencilOp  , VkStencilOp  , VkCompareOp  ))
        android_dlsym(vulkan_handle, "vkCmdSetStencilOpEXT"))
            (commandBuffer, faceMask, failOp, passOp, depthFailOp, compareOp);
}

#if VK_HEADER_VERSION >= 258

VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToImageEXT( VkDevice  device, const VkCopyMemoryToImageInfoEXT * pCopyMemoryToImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCopyMemoryToImageInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCopyMemoryToImageEXT"))
            (device, pCopyMemoryToImageInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyImageToMemoryEXT( VkDevice  device, const VkCopyImageToMemoryInfoEXT * pCopyImageToMemoryInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCopyImageToMemoryInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCopyImageToMemoryEXT"))
            (device, pCopyImageToMemoryInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyImageToImageEXT( VkDevice  device, const VkCopyImageToImageInfoEXT * pCopyImageToImageInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCopyImageToImageInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCopyImageToImageEXT"))
            (device, pCopyImageToImageInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkTransitionImageLayoutEXT( VkDevice  device,  uint32_t  transitionCount, const VkHostImageLayoutTransitionInfoEXT * pTransitions)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkHostImageLayoutTransitionInfoEXT * ))
        android_dlsym(vulkan_handle, "vkTransitionImageLayoutEXT"))
            (device, transitionCount, pTransitions);
}

#endif

#if VK_HEADER_VERSION >= 213

VKAPI_ATTR void VKAPI_CALL vkGetImageSubresourceLayout2EXT( VkDevice  device,  VkImage  image, const VkImageSubresource2KHR * pSubresource,  VkSubresourceLayout2KHR * pLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkImage  ,const VkImageSubresource2KHR * , VkSubresourceLayout2KHR * ))
        android_dlsym(vulkan_handle, "vkGetImageSubresourceLayout2EXT"))
            (device, image, pSubresource, pLayout);
}

#endif

#if VK_HEADER_VERSION >= 237

VKAPI_ATTR VkResult VKAPI_CALL vkReleaseSwapchainImagesEXT( VkDevice  device, const VkReleaseSwapchainImagesInfoEXT * pReleaseInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkReleaseSwapchainImagesInfoEXT * ))
        android_dlsym(vulkan_handle, "vkReleaseSwapchainImagesEXT"))
            (device, pReleaseInfo);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkGetGeneratedCommandsMemoryRequirementsNV( VkDevice  device, const VkGeneratedCommandsMemoryRequirementsInfoNV * pInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkGeneratedCommandsMemoryRequirementsInfoNV * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetGeneratedCommandsMemoryRequirementsNV"))
            (device, pInfo, pMemoryRequirements);
}

VKAPI_ATTR void VKAPI_CALL vkCmdPreprocessGeneratedCommandsNV( VkCommandBuffer  commandBuffer, const VkGeneratedCommandsInfoNV * pGeneratedCommandsInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkGeneratedCommandsInfoNV * ))
        android_dlsym(vulkan_handle, "vkCmdPreprocessGeneratedCommandsNV"))
            (commandBuffer, pGeneratedCommandsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdExecuteGeneratedCommandsNV( VkCommandBuffer  commandBuffer,  VkBool32  isPreprocessed, const VkGeneratedCommandsInfoNV * pGeneratedCommandsInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ,const VkGeneratedCommandsInfoNV * ))
        android_dlsym(vulkan_handle, "vkCmdExecuteGeneratedCommandsNV"))
            (commandBuffer, isPreprocessed, pGeneratedCommandsInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindPipelineShaderGroupNV( VkCommandBuffer  commandBuffer,  VkPipelineBindPoint  pipelineBindPoint,  VkPipeline  pipeline,  uint32_t  groupIndex)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineBindPoint  , VkPipeline  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdBindPipelineShaderGroupNV"))
            (commandBuffer, pipelineBindPoint, pipeline, groupIndex);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateIndirectCommandsLayoutNV( VkDevice  device, const VkIndirectCommandsLayoutCreateInfoNV * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkIndirectCommandsLayoutNV * pIndirectCommandsLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkIndirectCommandsLayoutCreateInfoNV * ,const VkAllocationCallbacks * , VkIndirectCommandsLayoutNV * ))
        android_dlsym(vulkan_handle, "vkCreateIndirectCommandsLayoutNV"))
            (device, pCreateInfo, pAllocator, pIndirectCommandsLayout);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyIndirectCommandsLayoutNV( VkDevice  device,  VkIndirectCommandsLayoutNV  indirectCommandsLayout, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkIndirectCommandsLayoutNV  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyIndirectCommandsLayoutNV"))
            (device, indirectCommandsLayout, pAllocator);
}

#if VK_HEADER_VERSION >= 254

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias2EXT( VkCommandBuffer  commandBuffer, const VkDepthBiasInfoEXT * pDepthBiasInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkDepthBiasInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthBias2EXT"))
            (commandBuffer, pDepthBiasInfo);
}

#endif

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireDrmDisplayEXT( VkPhysicalDevice  physicalDevice,  int32_t  drmFd,  VkDisplayKHR  display)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , int32_t  , VkDisplayKHR  ))
        android_dlsym(vulkan_handle, "vkAcquireDrmDisplayEXT"))
            (physicalDevice, drmFd, display);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDrmDisplayEXT( VkPhysicalDevice  physicalDevice,  int32_t  drmFd,  uint32_t  connectorId,  VkDisplayKHR * display)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  , int32_t  , uint32_t  , VkDisplayKHR * ))
        android_dlsym(vulkan_handle, "vkGetDrmDisplayEXT"))
            (physicalDevice, drmFd, connectorId, display);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreatePrivateDataSlotEXT( VkDevice  device, const VkPrivateDataSlotCreateInfo * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkPrivateDataSlot * pPrivateDataSlot)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPrivateDataSlotCreateInfo * ,const VkAllocationCallbacks * , VkPrivateDataSlot * ))
        android_dlsym(vulkan_handle, "vkCreatePrivateDataSlotEXT"))
            (device, pCreateInfo, pAllocator, pPrivateDataSlot);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyPrivateDataSlotEXT( VkDevice  device,  VkPrivateDataSlot  privateDataSlot, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkPrivateDataSlot  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyPrivateDataSlotEXT"))
            (device, privateDataSlot, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkSetPrivateDataEXT( VkDevice  device,  VkObjectType  objectType,  uint64_t  objectHandle,  VkPrivateDataSlot  privateDataSlot,  uint64_t  data)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkObjectType  , uint64_t  , VkPrivateDataSlot  , uint64_t  ))
        android_dlsym(vulkan_handle, "vkSetPrivateDataEXT"))
            (device, objectType, objectHandle, privateDataSlot, data);
}

VKAPI_ATTR void VKAPI_CALL vkGetPrivateDataEXT( VkDevice  device,  VkObjectType  objectType,  uint64_t  objectHandle,  VkPrivateDataSlot  privateDataSlot,  uint64_t * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkObjectType  , uint64_t  , VkPrivateDataSlot  , uint64_t * ))
        android_dlsym(vulkan_handle, "vkGetPrivateDataEXT"))
            (device, objectType, objectHandle, privateDataSlot, pData);
}

#if VK_HEADER_VERSION >= 269

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCudaModuleNV( VkDevice  device, const VkCudaModuleCreateInfoNV * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkCudaModuleNV * pModule)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCudaModuleCreateInfoNV * ,const VkAllocationCallbacks * , VkCudaModuleNV * ))
        android_dlsym(vulkan_handle, "vkCreateCudaModuleNV"))
            (device, pCreateInfo, pAllocator, pModule);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetCudaModuleCacheNV( VkDevice  device,  VkCudaModuleNV  module,  size_t * pCacheSize,  void * pCacheData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkCudaModuleNV  , size_t * , void * ))
        android_dlsym(vulkan_handle, "vkGetCudaModuleCacheNV"))
            (device, module, pCacheSize, pCacheData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateCudaFunctionNV( VkDevice  device, const VkCudaFunctionCreateInfoNV * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkCudaFunctionNV * pFunction)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkCudaFunctionCreateInfoNV * ,const VkAllocationCallbacks * , VkCudaFunctionNV * ))
        android_dlsym(vulkan_handle, "vkCreateCudaFunctionNV"))
            (device, pCreateInfo, pAllocator, pFunction);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCudaModuleNV( VkDevice  device,  VkCudaModuleNV  module, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCudaModuleNV  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyCudaModuleNV"))
            (device, module, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyCudaFunctionNV( VkDevice  device,  VkCudaFunctionNV  function, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkCudaFunctionNV  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyCudaFunctionNV"))
            (device, function, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCudaLaunchKernelNV( VkCommandBuffer  commandBuffer, const VkCudaLaunchInfoNV * pLaunchInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCudaLaunchInfoNV * ))
        android_dlsym(vulkan_handle, "vkCmdCudaLaunchKernelNV"))
            (commandBuffer, pLaunchInfo);
}

#endif

#if VK_HEADER_VERSION >= 235

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutSizeEXT( VkDevice  device,  VkDescriptorSetLayout  layout,  VkDeviceSize * pLayoutSizeInBytes)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorSetLayout  , VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkGetDescriptorSetLayoutSizeEXT"))
            (device, layout, pLayoutSizeInBytes);
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutBindingOffsetEXT( VkDevice  device,  VkDescriptorSetLayout  layout,  uint32_t  binding,  VkDeviceSize * pOffset)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorSetLayout  , uint32_t  , VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkGetDescriptorSetLayoutBindingOffsetEXT"))
            (device, layout, binding, pOffset);
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorEXT( VkDevice  device, const VkDescriptorGetInfoEXT * pDescriptorInfo,  size_t  dataSize,  void * pDescriptor)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDescriptorGetInfoEXT * , size_t  , void * ))
        android_dlsym(vulkan_handle, "vkGetDescriptorEXT"))
            (device, pDescriptorInfo, dataSize, pDescriptor);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorBuffersEXT( VkCommandBuffer  commandBuffer,  uint32_t  bufferCount, const VkDescriptorBufferBindingInfoEXT * pBindingInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkDescriptorBufferBindingInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdBindDescriptorBuffersEXT"))
            (commandBuffer, bufferCount, pBindingInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDescriptorBufferOffsetsEXT( VkCommandBuffer  commandBuffer,  VkPipelineBindPoint  pipelineBindPoint,  VkPipelineLayout  layout,  uint32_t  firstSet,  uint32_t  setCount, const uint32_t * pBufferIndices, const VkDeviceSize * pOffsets)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineBindPoint  , VkPipelineLayout  , uint32_t  , uint32_t  ,const uint32_t * ,const VkDeviceSize * ))
        android_dlsym(vulkan_handle, "vkCmdSetDescriptorBufferOffsetsEXT"))
            (commandBuffer, pipelineBindPoint, layout, firstSet, setCount, pBufferIndices, pOffsets);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindDescriptorBufferEmbeddedSamplersEXT( VkCommandBuffer  commandBuffer,  VkPipelineBindPoint  pipelineBindPoint,  VkPipelineLayout  layout,  uint32_t  set)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineBindPoint  , VkPipelineLayout  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdBindDescriptorBufferEmbeddedSamplersEXT"))
            (commandBuffer, pipelineBindPoint, layout, set);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetBufferOpaqueCaptureDescriptorDataEXT( VkDevice  device, const VkBufferCaptureDescriptorDataInfoEXT * pInfo,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkBufferCaptureDescriptorDataInfoEXT * , void * ))
        android_dlsym(vulkan_handle, "vkGetBufferOpaqueCaptureDescriptorDataEXT"))
            (device, pInfo, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetImageOpaqueCaptureDescriptorDataEXT( VkDevice  device, const VkImageCaptureDescriptorDataInfoEXT * pInfo,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkImageCaptureDescriptorDataInfoEXT * , void * ))
        android_dlsym(vulkan_handle, "vkGetImageOpaqueCaptureDescriptorDataEXT"))
            (device, pInfo, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetImageViewOpaqueCaptureDescriptorDataEXT( VkDevice  device, const VkImageViewCaptureDescriptorDataInfoEXT * pInfo,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkImageViewCaptureDescriptorDataInfoEXT * , void * ))
        android_dlsym(vulkan_handle, "vkGetImageViewOpaqueCaptureDescriptorDataEXT"))
            (device, pInfo, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetSamplerOpaqueCaptureDescriptorDataEXT( VkDevice  device, const VkSamplerCaptureDescriptorDataInfoEXT * pInfo,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkSamplerCaptureDescriptorDataInfoEXT * , void * ))
        android_dlsym(vulkan_handle, "vkGetSamplerOpaqueCaptureDescriptorDataEXT"))
            (device, pInfo, pData);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT( VkDevice  device, const VkAccelerationStructureCaptureDescriptorDataInfoEXT * pInfo,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkAccelerationStructureCaptureDescriptorDataInfoEXT * , void * ))
        android_dlsym(vulkan_handle, "vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT"))
            (device, pInfo, pData);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkCmdSetFragmentShadingRateEnumNV( VkCommandBuffer  commandBuffer,  VkFragmentShadingRateNV  shadingRate, const VkFragmentShadingRateCombinerOpKHR  combinerOps[2])
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkFragmentShadingRateNV  ,const VkFragmentShadingRateCombinerOpKHR  [2]))
        android_dlsym(vulkan_handle, "vkCmdSetFragmentShadingRateEnumNV"))
            (commandBuffer, shadingRate, combinerOps);
}

#if VK_HEADER_VERSION >= 230

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceFaultInfoEXT( VkDevice  device,  VkDeviceFaultCountsEXT * pFaultCounts,  VkDeviceFaultInfoEXT * pFaultInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeviceFaultCountsEXT * , VkDeviceFaultInfoEXT * ))
        android_dlsym(vulkan_handle, "vkGetDeviceFaultInfoEXT"))
            (device, pFaultCounts, pFaultInfo);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkCmdSetVertexInputEXT( VkCommandBuffer  commandBuffer,  uint32_t  vertexBindingDescriptionCount, const VkVertexInputBindingDescription2EXT * pVertexBindingDescriptions,  uint32_t  vertexAttributeDescriptionCount, const VkVertexInputAttributeDescription2EXT * pVertexAttributeDescriptions)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkVertexInputBindingDescription2EXT * , uint32_t  ,const VkVertexInputAttributeDescription2EXT * ))
        android_dlsym(vulkan_handle, "vkCmdSetVertexInputEXT"))
            (commandBuffer, vertexBindingDescriptionCount, pVertexBindingDescriptions, vertexAttributeDescriptionCount, pVertexAttributeDescriptions);
}

#if VK_HEADER_VERSION >= 184

VKAPI_ATTR VkResult VKAPI_CALL vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI( VkDevice  device,  VkRenderPass  renderpass,  VkExtent2D * pMaxWorkgroupSize)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkRenderPass  , VkExtent2D * ))
        android_dlsym(vulkan_handle, "vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI"))
            (device, renderpass, pMaxWorkgroupSize);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkCmdSubpassShadingHUAWEI( VkCommandBuffer  commandBuffer)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ))
        android_dlsym(vulkan_handle, "vkCmdSubpassShadingHUAWEI"))
            (commandBuffer);
}

#if VK_HEADER_VERSION >= 185

VKAPI_ATTR void VKAPI_CALL vkCmdBindInvocationMaskHUAWEI( VkCommandBuffer  commandBuffer,  VkImageView  imageView,  VkImageLayout  imageLayout)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImageView  , VkImageLayout  ))
        android_dlsym(vulkan_handle, "vkCmdBindInvocationMaskHUAWEI"))
            (commandBuffer, imageView, imageLayout);
}

#endif

#if VK_HEADER_VERSION >= 184

VKAPI_ATTR VkResult VKAPI_CALL vkGetMemoryRemoteAddressNV( VkDevice  device, const VkMemoryGetRemoteAddressInfoNV * pMemoryGetRemoteAddressInfo,  VkRemoteAddressNV * pAddress)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkMemoryGetRemoteAddressInfoNV * , VkRemoteAddressNV * ))
        android_dlsym(vulkan_handle, "vkGetMemoryRemoteAddressNV"))
            (device, pMemoryGetRemoteAddressInfo, pAddress);
}

#endif

#if VK_HEADER_VERSION >= 213

VKAPI_ATTR VkResult VKAPI_CALL vkGetPipelinePropertiesEXT( VkDevice  device, const VkPipelineInfoEXT * pPipelineInfo,  VkBaseOutStructure * pPipelineProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkPipelineInfoEXT * , VkBaseOutStructure * ))
        android_dlsym(vulkan_handle, "vkGetPipelinePropertiesEXT"))
            (device, pPipelineInfo, pPipelineProperties);
}

#endif

VKAPI_ATTR void VKAPI_CALL vkCmdSetPatchControlPointsEXT( VkCommandBuffer  commandBuffer,  uint32_t  patchControlPoints)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetPatchControlPointsEXT"))
            (commandBuffer, patchControlPoints);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizerDiscardEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  rasterizerDiscardEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetRasterizerDiscardEnableEXT"))
            (commandBuffer, rasterizerDiscardEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBiasEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  depthBiasEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthBiasEnableEXT"))
            (commandBuffer, depthBiasEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetLogicOpEXT( VkCommandBuffer  commandBuffer,  VkLogicOp  logicOp)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkLogicOp  ))
        android_dlsym(vulkan_handle, "vkCmdSetLogicOpEXT"))
            (commandBuffer, logicOp);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetPrimitiveRestartEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  primitiveRestartEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetPrimitiveRestartEnableEXT"))
            (commandBuffer, primitiveRestartEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetColorWriteEnableEXT( VkCommandBuffer  commandBuffer,  uint32_t  attachmentCount, const VkBool32 * pColorWriteEnables)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkBool32 * ))
        android_dlsym(vulkan_handle, "vkCmdSetColorWriteEnableEXT"))
            (commandBuffer, attachmentCount, pColorWriteEnables);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMultiEXT( VkCommandBuffer  commandBuffer,  uint32_t  drawCount, const VkMultiDrawInfoEXT * pVertexInfo,  uint32_t  instanceCount,  uint32_t  firstInstance,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkMultiDrawInfoEXT * , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawMultiEXT"))
            (commandBuffer, drawCount, pVertexInfo, instanceCount, firstInstance, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMultiIndexedEXT( VkCommandBuffer  commandBuffer,  uint32_t  drawCount, const VkMultiDrawIndexedInfoEXT * pIndexInfo,  uint32_t  instanceCount,  uint32_t  firstInstance,  uint32_t  stride, const int32_t * pVertexOffset)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkMultiDrawIndexedInfoEXT * , uint32_t  , uint32_t  , uint32_t  ,const int32_t * ))
        android_dlsym(vulkan_handle, "vkCmdDrawMultiIndexedEXT"))
            (commandBuffer, drawCount, pIndexInfo, instanceCount, firstInstance, stride, pVertexOffset);
}

#if VK_HEADER_VERSION >= 230

VKAPI_ATTR VkResult VKAPI_CALL vkCreateMicromapEXT( VkDevice  device, const VkMicromapCreateInfoEXT * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkMicromapEXT * pMicromap)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkMicromapCreateInfoEXT * ,const VkAllocationCallbacks * , VkMicromapEXT * ))
        android_dlsym(vulkan_handle, "vkCreateMicromapEXT"))
            (device, pCreateInfo, pAllocator, pMicromap);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyMicromapEXT( VkDevice  device,  VkMicromapEXT  micromap, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkMicromapEXT  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyMicromapEXT"))
            (device, micromap, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildMicromapsEXT( VkCommandBuffer  commandBuffer,  uint32_t  infoCount, const VkMicromapBuildInfoEXT * pInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkMicromapBuildInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdBuildMicromapsEXT"))
            (commandBuffer, infoCount, pInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBuildMicromapsEXT( VkDevice  device,  VkDeferredOperationKHR  deferredOperation,  uint32_t  infoCount, const VkMicromapBuildInfoEXT * pInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  , uint32_t  ,const VkMicromapBuildInfoEXT * ))
        android_dlsym(vulkan_handle, "vkBuildMicromapsEXT"))
            (device, deferredOperation, infoCount, pInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyMicromapEXT( VkDevice  device,  VkDeferredOperationKHR  deferredOperation, const VkCopyMicromapInfoEXT * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ,const VkCopyMicromapInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCopyMicromapEXT"))
            (device, deferredOperation, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyMicromapToMemoryEXT( VkDevice  device,  VkDeferredOperationKHR  deferredOperation, const VkCopyMicromapToMemoryInfoEXT * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ,const VkCopyMicromapToMemoryInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCopyMicromapToMemoryEXT"))
            (device, deferredOperation, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToMicromapEXT( VkDevice  device,  VkDeferredOperationKHR  deferredOperation, const VkCopyMemoryToMicromapInfoEXT * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ,const VkCopyMemoryToMicromapInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCopyMemoryToMicromapEXT"))
            (device, deferredOperation, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWriteMicromapsPropertiesEXT( VkDevice  device,  uint32_t  micromapCount, const VkMicromapEXT * pMicromaps,  VkQueryType  queryType,  size_t  dataSize,  void * pData,  size_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkMicromapEXT * , VkQueryType  , size_t  , void * , size_t  ))
        android_dlsym(vulkan_handle, "vkWriteMicromapsPropertiesEXT"))
            (device, micromapCount, pMicromaps, queryType, dataSize, pData, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyMicromapEXT( VkCommandBuffer  commandBuffer, const VkCopyMicromapInfoEXT * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyMicromapInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdCopyMicromapEXT"))
            (commandBuffer, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyMicromapToMemoryEXT( VkCommandBuffer  commandBuffer, const VkCopyMicromapToMemoryInfoEXT * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyMicromapToMemoryInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdCopyMicromapToMemoryEXT"))
            (commandBuffer, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToMicromapEXT( VkCommandBuffer  commandBuffer, const VkCopyMemoryToMicromapInfoEXT * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyMemoryToMicromapInfoEXT * ))
        android_dlsym(vulkan_handle, "vkCmdCopyMemoryToMicromapEXT"))
            (commandBuffer, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteMicromapsPropertiesEXT( VkCommandBuffer  commandBuffer,  uint32_t  micromapCount, const VkMicromapEXT * pMicromaps,  VkQueryType  queryType,  VkQueryPool  queryPool,  uint32_t  firstQuery)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkMicromapEXT * , VkQueryType  , VkQueryPool  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteMicromapsPropertiesEXT"))
            (commandBuffer, micromapCount, pMicromaps, queryType, queryPool, firstQuery);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceMicromapCompatibilityEXT( VkDevice  device, const VkMicromapVersionInfoEXT * pVersionInfo,  VkAccelerationStructureCompatibilityKHR * pCompatibility)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkMicromapVersionInfoEXT * , VkAccelerationStructureCompatibilityKHR * ))
        android_dlsym(vulkan_handle, "vkGetDeviceMicromapCompatibilityEXT"))
            (device, pVersionInfo, pCompatibility);
}

VKAPI_ATTR void VKAPI_CALL vkGetMicromapBuildSizesEXT( VkDevice  device,  VkAccelerationStructureBuildTypeKHR  buildType, const VkMicromapBuildInfoEXT * pBuildInfo,  VkMicromapBuildSizesInfoEXT * pSizeInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkAccelerationStructureBuildTypeKHR  ,const VkMicromapBuildInfoEXT * , VkMicromapBuildSizesInfoEXT * ))
        android_dlsym(vulkan_handle, "vkGetMicromapBuildSizesEXT"))
            (device, buildType, pBuildInfo, pSizeInfo);
}

#endif

#if VK_HEADER_VERSION >= 239

VKAPI_ATTR void VKAPI_CALL vkCmdDrawClusterHUAWEI( VkCommandBuffer  commandBuffer,  uint32_t  groupCountX,  uint32_t  groupCountY,  uint32_t  groupCountZ)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawClusterHUAWEI"))
            (commandBuffer, groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawClusterIndirectHUAWEI( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  ))
        android_dlsym(vulkan_handle, "vkCmdDrawClusterIndirectHUAWEI"))
            (commandBuffer, buffer, offset);
}

#endif

#if VK_HEADER_VERSION >= 191

VKAPI_ATTR void VKAPI_CALL vkSetDeviceMemoryPriorityEXT( VkDevice  device,  VkDeviceMemory  memory,  float  priority)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDeviceMemory  , float  ))
        android_dlsym(vulkan_handle, "vkSetDeviceMemoryPriorityEXT"))
            (device, memory, priority);
}

#endif

#if VK_HEADER_VERSION >= 207

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetLayoutHostMappingInfoVALVE( VkDevice  device, const VkDescriptorSetBindingReferenceVALVE * pBindingReference,  VkDescriptorSetLayoutHostMappingInfoVALVE * pHostMapping)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkDescriptorSetBindingReferenceVALVE * , VkDescriptorSetLayoutHostMappingInfoVALVE * ))
        android_dlsym(vulkan_handle, "vkGetDescriptorSetLayoutHostMappingInfoVALVE"))
            (device, pBindingReference, pHostMapping);
}

VKAPI_ATTR void VKAPI_CALL vkGetDescriptorSetHostMappingVALVE( VkDevice  device,  VkDescriptorSet  descriptorSet,  void ** ppData)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkDescriptorSet  , void ** ))
        android_dlsym(vulkan_handle, "vkGetDescriptorSetHostMappingVALVE"))
            (device, descriptorSet, ppData);
}

#endif

#if VK_HEADER_VERSION >= 233

VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryIndirectNV( VkCommandBuffer  commandBuffer,  VkDeviceAddress  copyBufferAddress,  uint32_t  copyCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkDeviceAddress  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdCopyMemoryIndirectNV"))
            (commandBuffer, copyBufferAddress, copyCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToImageIndirectNV( VkCommandBuffer  commandBuffer,  VkDeviceAddress  copyBufferAddress,  uint32_t  copyCount,  uint32_t  stride,  VkImage  dstImage,  VkImageLayout  dstImageLayout, const VkImageSubresourceLayers * pImageSubresources)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkDeviceAddress  , uint32_t  , uint32_t  , VkImage  , VkImageLayout  ,const VkImageSubresourceLayers * ))
        android_dlsym(vulkan_handle, "vkCmdCopyMemoryToImageIndirectNV"))
            (commandBuffer, copyBufferAddress, copyCount, stride, dstImage, dstImageLayout, pImageSubresources);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDecompressMemoryNV( VkCommandBuffer  commandBuffer,  uint32_t  decompressRegionCount, const VkDecompressMemoryRegionNV * pDecompressMemoryRegions)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkDecompressMemoryRegionNV * ))
        android_dlsym(vulkan_handle, "vkCmdDecompressMemoryNV"))
            (commandBuffer, decompressRegionCount, pDecompressMemoryRegions);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDecompressMemoryIndirectCountNV( VkCommandBuffer  commandBuffer,  VkDeviceAddress  indirectCommandsAddress,  VkDeviceAddress  indirectCommandsCountAddress,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkDeviceAddress  , VkDeviceAddress  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDecompressMemoryIndirectCountNV"))
            (commandBuffer, indirectCommandsAddress, indirectCommandsCountAddress, stride);
}

#endif

#if VK_HEADER_VERSION >= 258

VKAPI_ATTR void VKAPI_CALL vkGetPipelineIndirectMemoryRequirementsNV( VkDevice  device, const VkComputePipelineCreateInfo * pCreateInfo,  VkMemoryRequirements2 * pMemoryRequirements)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkComputePipelineCreateInfo * , VkMemoryRequirements2 * ))
        android_dlsym(vulkan_handle, "vkGetPipelineIndirectMemoryRequirementsNV"))
            (device, pCreateInfo, pMemoryRequirements);
}

#endif

#if VK_HEADER_VERSION == 258

#endif

#if VK_HEADER_VERSION >= 259

VKAPI_ATTR void VKAPI_CALL vkCmdUpdatePipelineIndirectBufferNV( VkCommandBuffer  commandBuffer,  VkPipelineBindPoint  pipelineBindPoint,  VkPipeline  pipeline)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPipelineBindPoint  , VkPipeline  ))
        android_dlsym(vulkan_handle, "vkCmdUpdatePipelineIndirectBufferNV"))
            (commandBuffer, pipelineBindPoint, pipeline);
}

#endif

#if VK_HEADER_VERSION >= 258

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetPipelineIndirectDeviceAddressNV( VkDevice  device, const VkPipelineIndirectDeviceAddressInfoNV * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkDeviceAddress (*)( VkDevice  ,const VkPipelineIndirectDeviceAddressInfoNV * ))
        android_dlsym(vulkan_handle, "vkGetPipelineIndirectDeviceAddressNV"))
            (device, pInfo);
}

#endif

#if VK_HEADER_VERSION >= 230

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClampEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  depthClampEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthClampEnableEXT"))
            (commandBuffer, depthClampEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetPolygonModeEXT( VkCommandBuffer  commandBuffer,  VkPolygonMode  polygonMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkPolygonMode  ))
        android_dlsym(vulkan_handle, "vkCmdSetPolygonModeEXT"))
            (commandBuffer, polygonMode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizationSamplesEXT( VkCommandBuffer  commandBuffer,  VkSampleCountFlagBits  rasterizationSamples)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkSampleCountFlagBits  ))
        android_dlsym(vulkan_handle, "vkCmdSetRasterizationSamplesEXT"))
            (commandBuffer, rasterizationSamples);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleMaskEXT( VkCommandBuffer  commandBuffer,  VkSampleCountFlagBits  samples, const VkSampleMask * pSampleMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkSampleCountFlagBits  ,const VkSampleMask * ))
        android_dlsym(vulkan_handle, "vkCmdSetSampleMaskEXT"))
            (commandBuffer, samples, pSampleMask);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetAlphaToCoverageEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  alphaToCoverageEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetAlphaToCoverageEnableEXT"))
            (commandBuffer, alphaToCoverageEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetAlphaToOneEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  alphaToOneEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetAlphaToOneEnableEXT"))
            (commandBuffer, alphaToOneEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetLogicOpEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  logicOpEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetLogicOpEnableEXT"))
            (commandBuffer, logicOpEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetColorBlendEnableEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstAttachment,  uint32_t  attachmentCount, const VkBool32 * pColorBlendEnables)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkBool32 * ))
        android_dlsym(vulkan_handle, "vkCmdSetColorBlendEnableEXT"))
            (commandBuffer, firstAttachment, attachmentCount, pColorBlendEnables);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetColorBlendEquationEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstAttachment,  uint32_t  attachmentCount, const VkColorBlendEquationEXT * pColorBlendEquations)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkColorBlendEquationEXT * ))
        android_dlsym(vulkan_handle, "vkCmdSetColorBlendEquationEXT"))
            (commandBuffer, firstAttachment, attachmentCount, pColorBlendEquations);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetColorWriteMaskEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstAttachment,  uint32_t  attachmentCount, const VkColorComponentFlags * pColorWriteMasks)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkColorComponentFlags * ))
        android_dlsym(vulkan_handle, "vkCmdSetColorWriteMaskEXT"))
            (commandBuffer, firstAttachment, attachmentCount, pColorWriteMasks);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetTessellationDomainOriginEXT( VkCommandBuffer  commandBuffer,  VkTessellationDomainOrigin  domainOrigin)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkTessellationDomainOrigin  ))
        android_dlsym(vulkan_handle, "vkCmdSetTessellationDomainOriginEXT"))
            (commandBuffer, domainOrigin);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetRasterizationStreamEXT( VkCommandBuffer  commandBuffer,  uint32_t  rasterizationStream)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetRasterizationStreamEXT"))
            (commandBuffer, rasterizationStream);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetConservativeRasterizationModeEXT( VkCommandBuffer  commandBuffer,  VkConservativeRasterizationModeEXT  conservativeRasterizationMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkConservativeRasterizationModeEXT  ))
        android_dlsym(vulkan_handle, "vkCmdSetConservativeRasterizationModeEXT"))
            (commandBuffer, conservativeRasterizationMode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetExtraPrimitiveOverestimationSizeEXT( VkCommandBuffer  commandBuffer,  float  extraPrimitiveOverestimationSize)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , float  ))
        android_dlsym(vulkan_handle, "vkCmdSetExtraPrimitiveOverestimationSizeEXT"))
            (commandBuffer, extraPrimitiveOverestimationSize);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClipEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  depthClipEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthClipEnableEXT"))
            (commandBuffer, depthClipEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetSampleLocationsEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  sampleLocationsEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetSampleLocationsEnableEXT"))
            (commandBuffer, sampleLocationsEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetColorBlendAdvancedEXT( VkCommandBuffer  commandBuffer,  uint32_t  firstAttachment,  uint32_t  attachmentCount, const VkColorBlendAdvancedEXT * pColorBlendAdvanced)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkColorBlendAdvancedEXT * ))
        android_dlsym(vulkan_handle, "vkCmdSetColorBlendAdvancedEXT"))
            (commandBuffer, firstAttachment, attachmentCount, pColorBlendAdvanced);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetProvokingVertexModeEXT( VkCommandBuffer  commandBuffer,  VkProvokingVertexModeEXT  provokingVertexMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkProvokingVertexModeEXT  ))
        android_dlsym(vulkan_handle, "vkCmdSetProvokingVertexModeEXT"))
            (commandBuffer, provokingVertexMode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetLineRasterizationModeEXT( VkCommandBuffer  commandBuffer,  VkLineRasterizationModeEXT  lineRasterizationMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkLineRasterizationModeEXT  ))
        android_dlsym(vulkan_handle, "vkCmdSetLineRasterizationModeEXT"))
            (commandBuffer, lineRasterizationMode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetLineStippleEnableEXT( VkCommandBuffer  commandBuffer,  VkBool32  stippledLineEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetLineStippleEnableEXT"))
            (commandBuffer, stippledLineEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthClipNegativeOneToOneEXT( VkCommandBuffer  commandBuffer,  VkBool32  negativeOneToOne)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetDepthClipNegativeOneToOneEXT"))
            (commandBuffer, negativeOneToOne);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportWScalingEnableNV( VkCommandBuffer  commandBuffer,  VkBool32  viewportWScalingEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetViewportWScalingEnableNV"))
            (commandBuffer, viewportWScalingEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetViewportSwizzleNV( VkCommandBuffer  commandBuffer,  uint32_t  firstViewport,  uint32_t  viewportCount, const VkViewportSwizzleNV * pViewportSwizzles)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  ,const VkViewportSwizzleNV * ))
        android_dlsym(vulkan_handle, "vkCmdSetViewportSwizzleNV"))
            (commandBuffer, firstViewport, viewportCount, pViewportSwizzles);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageToColorEnableNV( VkCommandBuffer  commandBuffer,  VkBool32  coverageToColorEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetCoverageToColorEnableNV"))
            (commandBuffer, coverageToColorEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageToColorLocationNV( VkCommandBuffer  commandBuffer,  uint32_t  coverageToColorLocation)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetCoverageToColorLocationNV"))
            (commandBuffer, coverageToColorLocation);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageModulationModeNV( VkCommandBuffer  commandBuffer,  VkCoverageModulationModeNV  coverageModulationMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkCoverageModulationModeNV  ))
        android_dlsym(vulkan_handle, "vkCmdSetCoverageModulationModeNV"))
            (commandBuffer, coverageModulationMode);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageModulationTableEnableNV( VkCommandBuffer  commandBuffer,  VkBool32  coverageModulationTableEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetCoverageModulationTableEnableNV"))
            (commandBuffer, coverageModulationTableEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageModulationTableNV( VkCommandBuffer  commandBuffer,  uint32_t  coverageModulationTableCount, const float * pCoverageModulationTable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const float * ))
        android_dlsym(vulkan_handle, "vkCmdSetCoverageModulationTableNV"))
            (commandBuffer, coverageModulationTableCount, pCoverageModulationTable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetShadingRateImageEnableNV( VkCommandBuffer  commandBuffer,  VkBool32  shadingRateImageEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetShadingRateImageEnableNV"))
            (commandBuffer, shadingRateImageEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetRepresentativeFragmentTestEnableNV( VkCommandBuffer  commandBuffer,  VkBool32  representativeFragmentTestEnable)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBool32  ))
        android_dlsym(vulkan_handle, "vkCmdSetRepresentativeFragmentTestEnableNV"))
            (commandBuffer, representativeFragmentTestEnable);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetCoverageReductionModeNV( VkCommandBuffer  commandBuffer,  VkCoverageReductionModeNV  coverageReductionMode)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkCoverageReductionModeNV  ))
        android_dlsym(vulkan_handle, "vkCmdSetCoverageReductionModeNV"))
            (commandBuffer, coverageReductionMode);
}

#endif

#if VK_HEADER_VERSION >= 219

VKAPI_ATTR void VKAPI_CALL vkGetShaderModuleIdentifierEXT( VkDevice  device,  VkShaderModule  shaderModule,  VkShaderModuleIdentifierEXT * pIdentifier)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkShaderModule  , VkShaderModuleIdentifierEXT * ))
        android_dlsym(vulkan_handle, "vkGetShaderModuleIdentifierEXT"))
            (device, shaderModule, pIdentifier);
}

VKAPI_ATTR void VKAPI_CALL vkGetShaderModuleCreateInfoIdentifierEXT( VkDevice  device, const VkShaderModuleCreateInfo * pCreateInfo,  VkShaderModuleIdentifierEXT * pIdentifier)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkShaderModuleCreateInfo * , VkShaderModuleIdentifierEXT * ))
        android_dlsym(vulkan_handle, "vkGetShaderModuleCreateInfoIdentifierEXT"))
            (device, pCreateInfo, pIdentifier);
}

#endif

#if VK_HEADER_VERSION >= 230

VKAPI_ATTR VkResult VKAPI_CALL vkGetPhysicalDeviceOpticalFlowImageFormatsNV( VkPhysicalDevice  physicalDevice, const VkOpticalFlowImageFormatInfoNV * pOpticalFlowImageFormatInfo,  uint32_t * pFormatCount,  VkOpticalFlowImageFormatPropertiesNV * pImageFormatProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkPhysicalDevice  ,const VkOpticalFlowImageFormatInfoNV * , uint32_t * , VkOpticalFlowImageFormatPropertiesNV * ))
        android_dlsym(vulkan_handle, "vkGetPhysicalDeviceOpticalFlowImageFormatsNV"))
            (physicalDevice, pOpticalFlowImageFormatInfo, pFormatCount, pImageFormatProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateOpticalFlowSessionNV( VkDevice  device, const VkOpticalFlowSessionCreateInfoNV * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkOpticalFlowSessionNV * pSession)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkOpticalFlowSessionCreateInfoNV * ,const VkAllocationCallbacks * , VkOpticalFlowSessionNV * ))
        android_dlsym(vulkan_handle, "vkCreateOpticalFlowSessionNV"))
            (device, pCreateInfo, pAllocator, pSession);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyOpticalFlowSessionNV( VkDevice  device,  VkOpticalFlowSessionNV  session, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkOpticalFlowSessionNV  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyOpticalFlowSessionNV"))
            (device, session, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBindOpticalFlowSessionImageNV( VkDevice  device,  VkOpticalFlowSessionNV  session,  VkOpticalFlowSessionBindingPointNV  bindingPoint,  VkImageView  view,  VkImageLayout  layout)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkOpticalFlowSessionNV  , VkOpticalFlowSessionBindingPointNV  , VkImageView  , VkImageLayout  ))
        android_dlsym(vulkan_handle, "vkBindOpticalFlowSessionImageNV"))
            (device, session, bindingPoint, view, layout);
}

VKAPI_ATTR void VKAPI_CALL vkCmdOpticalFlowExecuteNV( VkCommandBuffer  commandBuffer,  VkOpticalFlowSessionNV  session, const VkOpticalFlowExecuteInfoNV * pExecuteInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkOpticalFlowSessionNV  ,const VkOpticalFlowExecuteInfoNV * ))
        android_dlsym(vulkan_handle, "vkCmdOpticalFlowExecuteNV"))
            (commandBuffer, session, pExecuteInfo);
}

#endif

#if VK_HEADER_VERSION >= 246

VKAPI_ATTR VkResult VKAPI_CALL vkCreateShadersEXT( VkDevice  device,  uint32_t  createInfoCount, const VkShaderCreateInfoEXT * pCreateInfos, const VkAllocationCallbacks * pAllocator,  VkShaderEXT * pShaders)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkShaderCreateInfoEXT * ,const VkAllocationCallbacks * , VkShaderEXT * ))
        android_dlsym(vulkan_handle, "vkCreateShadersEXT"))
            (device, createInfoCount, pCreateInfos, pAllocator, pShaders);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyShaderEXT( VkDevice  device,  VkShaderEXT  shader, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkShaderEXT  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyShaderEXT"))
            (device, shader, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetShaderBinaryDataEXT( VkDevice  device,  VkShaderEXT  shader,  size_t * pDataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkShaderEXT  , size_t * , void * ))
        android_dlsym(vulkan_handle, "vkGetShaderBinaryDataEXT"))
            (device, shader, pDataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBindShadersEXT( VkCommandBuffer  commandBuffer,  uint32_t  stageCount, const VkShaderStageFlagBits * pStages, const VkShaderEXT * pShaders)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkShaderStageFlagBits * ,const VkShaderEXT * ))
        android_dlsym(vulkan_handle, "vkCmdBindShadersEXT"))
            (commandBuffer, stageCount, pStages, pShaders);
}

#endif

#if VK_HEADER_VERSION >= 222

VKAPI_ATTR VkResult VKAPI_CALL vkGetFramebufferTilePropertiesQCOM( VkDevice  device,  VkFramebuffer  framebuffer,  uint32_t * pPropertiesCount,  VkTilePropertiesQCOM * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkFramebuffer  , uint32_t * , VkTilePropertiesQCOM * ))
        android_dlsym(vulkan_handle, "vkGetFramebufferTilePropertiesQCOM"))
            (device, framebuffer, pPropertiesCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetDynamicRenderingTilePropertiesQCOM( VkDevice  device, const VkRenderingInfo * pRenderingInfo,  VkTilePropertiesQCOM * pProperties)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkRenderingInfo * , VkTilePropertiesQCOM * ))
        android_dlsym(vulkan_handle, "vkGetDynamicRenderingTilePropertiesQCOM"))
            (device, pRenderingInfo, pProperties);
}

#endif

#if VK_HEADER_VERSION >= 266

VKAPI_ATTR VkResult VKAPI_CALL vkSetLatencySleepModeNV( VkDevice  device,  VkSwapchainKHR  swapchain, const VkLatencySleepModeInfoNV * pSleepModeInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  ,const VkLatencySleepModeInfoNV * ))
        android_dlsym(vulkan_handle, "vkSetLatencySleepModeNV"))
            (device, swapchain, pSleepModeInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkLatencySleepNV( VkDevice  device,  VkSwapchainKHR  swapchain, const VkLatencySleepInfoNV * pSleepInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkSwapchainKHR  ,const VkLatencySleepInfoNV * ))
        android_dlsym(vulkan_handle, "vkLatencySleepNV"))
            (device, swapchain, pSleepInfo);
}

VKAPI_ATTR void VKAPI_CALL vkSetLatencyMarkerNV( VkDevice  device,  VkSwapchainKHR  swapchain, const VkSetLatencyMarkerInfoNV * pLatencyMarkerInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSwapchainKHR  ,const VkSetLatencyMarkerInfoNV * ))
        android_dlsym(vulkan_handle, "vkSetLatencyMarkerNV"))
            (device, swapchain, pLatencyMarkerInfo);
}

VKAPI_ATTR void VKAPI_CALL vkGetLatencyTimingsNV( VkDevice  device,  VkSwapchainKHR  swapchain,  VkGetLatencyMarkerInfoNV * pLatencyMarkerInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkSwapchainKHR  , VkGetLatencyMarkerInfoNV * ))
        android_dlsym(vulkan_handle, "vkGetLatencyTimingsNV"))
            (device, swapchain, pLatencyMarkerInfo);
}

VKAPI_ATTR void VKAPI_CALL vkQueueNotifyOutOfBandNV( VkQueue  queue, const VkOutOfBandQueueTypeInfoNV * pQueueTypeInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkQueue  ,const VkOutOfBandQueueTypeInfoNV * ))
        android_dlsym(vulkan_handle, "vkQueueNotifyOutOfBandNV"))
            (queue, pQueueTypeInfo);
}

#endif

#if VK_HEADER_VERSION >= 250

VKAPI_ATTR void VKAPI_CALL vkCmdSetAttachmentFeedbackLoopEnableEXT( VkCommandBuffer  commandBuffer,  VkImageAspectFlags  aspectMask)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkImageAspectFlags  ))
        android_dlsym(vulkan_handle, "vkCmdSetAttachmentFeedbackLoopEnableEXT"))
            (commandBuffer, aspectMask);
}

#endif

VKAPI_ATTR VkResult VKAPI_CALL vkCreateAccelerationStructureKHR( VkDevice  device, const VkAccelerationStructureCreateInfoKHR * pCreateInfo, const VkAllocationCallbacks * pAllocator,  VkAccelerationStructureKHR * pAccelerationStructure)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  ,const VkAccelerationStructureCreateInfoKHR * ,const VkAllocationCallbacks * , VkAccelerationStructureKHR * ))
        android_dlsym(vulkan_handle, "vkCreateAccelerationStructureKHR"))
            (device, pCreateInfo, pAllocator, pAccelerationStructure);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyAccelerationStructureKHR( VkDevice  device,  VkAccelerationStructureKHR  accelerationStructure, const VkAllocationCallbacks * pAllocator)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkAccelerationStructureKHR  ,const VkAllocationCallbacks * ))
        android_dlsym(vulkan_handle, "vkDestroyAccelerationStructureKHR"))
            (device, accelerationStructure, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresKHR( VkCommandBuffer  commandBuffer,  uint32_t  infoCount, const VkAccelerationStructureBuildGeometryInfoKHR * pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const* ppBuildRangeInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkAccelerationStructureBuildGeometryInfoKHR * ,const VkAccelerationStructureBuildRangeInfoKHR * const* ))
        android_dlsym(vulkan_handle, "vkCmdBuildAccelerationStructuresKHR"))
            (commandBuffer, infoCount, pInfos, ppBuildRangeInfos);
}

VKAPI_ATTR void VKAPI_CALL vkCmdBuildAccelerationStructuresIndirectKHR( VkCommandBuffer  commandBuffer,  uint32_t  infoCount, const VkAccelerationStructureBuildGeometryInfoKHR * pInfos, const VkDeviceAddress * pIndirectDeviceAddresses, const uint32_t * pIndirectStrides, const uint32_t * const* ppMaxPrimitiveCounts)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkAccelerationStructureBuildGeometryInfoKHR * ,const VkDeviceAddress * ,const uint32_t * ,const uint32_t * const* ))
        android_dlsym(vulkan_handle, "vkCmdBuildAccelerationStructuresIndirectKHR"))
            (commandBuffer, infoCount, pInfos, pIndirectDeviceAddresses, pIndirectStrides, ppMaxPrimitiveCounts);
}

VKAPI_ATTR VkResult VKAPI_CALL vkBuildAccelerationStructuresKHR( VkDevice  device,  VkDeferredOperationKHR  deferredOperation,  uint32_t  infoCount, const VkAccelerationStructureBuildGeometryInfoKHR * pInfos, const VkAccelerationStructureBuildRangeInfoKHR * const* ppBuildRangeInfos)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  , uint32_t  ,const VkAccelerationStructureBuildGeometryInfoKHR * ,const VkAccelerationStructureBuildRangeInfoKHR * const* ))
        android_dlsym(vulkan_handle, "vkBuildAccelerationStructuresKHR"))
            (device, deferredOperation, infoCount, pInfos, ppBuildRangeInfos);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureKHR( VkDevice  device,  VkDeferredOperationKHR  deferredOperation, const VkCopyAccelerationStructureInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ,const VkCopyAccelerationStructureInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCopyAccelerationStructureKHR"))
            (device, deferredOperation, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyAccelerationStructureToMemoryKHR( VkDevice  device,  VkDeferredOperationKHR  deferredOperation, const VkCopyAccelerationStructureToMemoryInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ,const VkCopyAccelerationStructureToMemoryInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCopyAccelerationStructureToMemoryKHR"))
            (device, deferredOperation, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCopyMemoryToAccelerationStructureKHR( VkDevice  device,  VkDeferredOperationKHR  deferredOperation, const VkCopyMemoryToAccelerationStructureInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  ,const VkCopyMemoryToAccelerationStructureInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCopyMemoryToAccelerationStructureKHR"))
            (device, deferredOperation, pInfo);
}

VKAPI_ATTR VkResult VKAPI_CALL vkWriteAccelerationStructuresPropertiesKHR( VkDevice  device,  uint32_t  accelerationStructureCount, const VkAccelerationStructureKHR * pAccelerationStructures,  VkQueryType  queryType,  size_t  dataSize,  void * pData,  size_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , uint32_t  ,const VkAccelerationStructureKHR * , VkQueryType  , size_t  , void * , size_t  ))
        android_dlsym(vulkan_handle, "vkWriteAccelerationStructuresPropertiesKHR"))
            (device, accelerationStructureCount, pAccelerationStructures, queryType, dataSize, pData, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureKHR( VkCommandBuffer  commandBuffer, const VkCopyAccelerationStructureInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyAccelerationStructureInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdCopyAccelerationStructureKHR"))
            (commandBuffer, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyAccelerationStructureToMemoryKHR( VkCommandBuffer  commandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyAccelerationStructureToMemoryInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdCopyAccelerationStructureToMemoryKHR"))
            (commandBuffer, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdCopyMemoryToAccelerationStructureKHR( VkCommandBuffer  commandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkCopyMemoryToAccelerationStructureInfoKHR * ))
        android_dlsym(vulkan_handle, "vkCmdCopyMemoryToAccelerationStructureKHR"))
            (commandBuffer, pInfo);
}

VKAPI_ATTR VkDeviceAddress VKAPI_CALL vkGetAccelerationStructureDeviceAddressKHR( VkDevice  device, const VkAccelerationStructureDeviceAddressInfoKHR * pInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkDeviceAddress (*)( VkDevice  ,const VkAccelerationStructureDeviceAddressInfoKHR * ))
        android_dlsym(vulkan_handle, "vkGetAccelerationStructureDeviceAddressKHR"))
            (device, pInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdWriteAccelerationStructuresPropertiesKHR( VkCommandBuffer  commandBuffer,  uint32_t  accelerationStructureCount, const VkAccelerationStructureKHR * pAccelerationStructures,  VkQueryType  queryType,  VkQueryPool  queryPool,  uint32_t  firstQuery)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ,const VkAccelerationStructureKHR * , VkQueryType  , VkQueryPool  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdWriteAccelerationStructuresPropertiesKHR"))
            (commandBuffer, accelerationStructureCount, pAccelerationStructures, queryType, queryPool, firstQuery);
}

VKAPI_ATTR void VKAPI_CALL vkGetDeviceAccelerationStructureCompatibilityKHR( VkDevice  device, const VkAccelerationStructureVersionInfoKHR * pVersionInfo,  VkAccelerationStructureCompatibilityKHR * pCompatibility)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  ,const VkAccelerationStructureVersionInfoKHR * , VkAccelerationStructureCompatibilityKHR * ))
        android_dlsym(vulkan_handle, "vkGetDeviceAccelerationStructureCompatibilityKHR"))
            (device, pVersionInfo, pCompatibility);
}

VKAPI_ATTR void VKAPI_CALL vkGetAccelerationStructureBuildSizesKHR( VkDevice  device,  VkAccelerationStructureBuildTypeKHR  buildType, const VkAccelerationStructureBuildGeometryInfoKHR * pBuildInfo, const uint32_t * pMaxPrimitiveCounts,  VkAccelerationStructureBuildSizesInfoKHR * pSizeInfo)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkDevice  , VkAccelerationStructureBuildTypeKHR  ,const VkAccelerationStructureBuildGeometryInfoKHR * ,const uint32_t * , VkAccelerationStructureBuildSizesInfoKHR * ))
        android_dlsym(vulkan_handle, "vkGetAccelerationStructureBuildSizesKHR"))
            (device, buildType, pBuildInfo, pMaxPrimitiveCounts, pSizeInfo);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysKHR( VkCommandBuffer  commandBuffer, const VkStridedDeviceAddressRegionKHR * pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR * pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR * pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR * pCallableShaderBindingTable,  uint32_t  width,  uint32_t  height,  uint32_t  depth)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkStridedDeviceAddressRegionKHR * ,const VkStridedDeviceAddressRegionKHR * ,const VkStridedDeviceAddressRegionKHR * ,const VkStridedDeviceAddressRegionKHR * , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdTraceRaysKHR"))
            (commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, width, height, depth);
}

VKAPI_ATTR VkResult VKAPI_CALL vkCreateRayTracingPipelinesKHR( VkDevice  device,  VkDeferredOperationKHR  deferredOperation,  VkPipelineCache  pipelineCache,  uint32_t  createInfoCount, const VkRayTracingPipelineCreateInfoKHR * pCreateInfos, const VkAllocationCallbacks * pAllocator,  VkPipeline * pPipelines)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkDeferredOperationKHR  , VkPipelineCache  , uint32_t  ,const VkRayTracingPipelineCreateInfoKHR * ,const VkAllocationCallbacks * , VkPipeline * ))
        android_dlsym(vulkan_handle, "vkCreateRayTracingPipelinesKHR"))
            (device, deferredOperation, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRayTracingCaptureReplayShaderGroupHandlesKHR( VkDevice  device,  VkPipeline  pipeline,  uint32_t  firstGroup,  uint32_t  groupCount,  size_t  dataSize,  void * pData)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkResult (*)( VkDevice  , VkPipeline  , uint32_t  , uint32_t  , size_t  , void * ))
        android_dlsym(vulkan_handle, "vkGetRayTracingCaptureReplayShaderGroupHandlesKHR"))
            (device, pipeline, firstGroup, groupCount, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL vkCmdTraceRaysIndirectKHR( VkCommandBuffer  commandBuffer, const VkStridedDeviceAddressRegionKHR * pRaygenShaderBindingTable, const VkStridedDeviceAddressRegionKHR * pMissShaderBindingTable, const VkStridedDeviceAddressRegionKHR * pHitShaderBindingTable, const VkStridedDeviceAddressRegionKHR * pCallableShaderBindingTable,  VkDeviceAddress  indirectDeviceAddress)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  ,const VkStridedDeviceAddressRegionKHR * ,const VkStridedDeviceAddressRegionKHR * ,const VkStridedDeviceAddressRegionKHR * ,const VkStridedDeviceAddressRegionKHR * , VkDeviceAddress  ))
        android_dlsym(vulkan_handle, "vkCmdTraceRaysIndirectKHR"))
            (commandBuffer, pRaygenShaderBindingTable, pMissShaderBindingTable, pHitShaderBindingTable, pCallableShaderBindingTable, indirectDeviceAddress);
}

VKAPI_ATTR VkDeviceSize VKAPI_CALL vkGetRayTracingShaderGroupStackSizeKHR( VkDevice  device,  VkPipeline  pipeline,  uint32_t  group,  VkShaderGroupShaderKHR  groupShader)
{
    if (!vulkan_handle) _init_androidvulkan();

    return ((VkDeviceSize (*)( VkDevice  , VkPipeline  , uint32_t  , VkShaderGroupShaderKHR  ))
        android_dlsym(vulkan_handle, "vkGetRayTracingShaderGroupStackSizeKHR"))
            (device, pipeline, group, groupShader);
}

VKAPI_ATTR void VKAPI_CALL vkCmdSetRayTracingPipelineStackSizeKHR( VkCommandBuffer  commandBuffer,  uint32_t  pipelineStackSize)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdSetRayTracingPipelineStackSizeKHR"))
            (commandBuffer, pipelineStackSize);
}

#if VK_HEADER_VERSION >= 226

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksEXT( VkCommandBuffer  commandBuffer,  uint32_t  groupCountX,  uint32_t  groupCountY,  uint32_t  groupCountZ)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , uint32_t  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawMeshTasksEXT"))
            (commandBuffer, groupCountX, groupCountY, groupCountZ);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectEXT( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  uint32_t  drawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawMeshTasksIndirectEXT"))
            (commandBuffer, buffer, offset, drawCount, stride);
}

VKAPI_ATTR void VKAPI_CALL vkCmdDrawMeshTasksIndirectCountEXT( VkCommandBuffer  commandBuffer,  VkBuffer  buffer,  VkDeviceSize  offset,  VkBuffer  countBuffer,  VkDeviceSize  countBufferOffset,  uint32_t  maxDrawCount,  uint32_t  stride)
{
    if (!vulkan_handle) _init_androidvulkan();

    ((void (*)( VkCommandBuffer  , VkBuffer  , VkDeviceSize  , VkBuffer  , VkDeviceSize  , uint32_t  , uint32_t  ))
        android_dlsym(vulkan_handle, "vkCmdDrawMeshTasksIndirectCountEXT"))
            (commandBuffer, buffer, offset, countBuffer, countBufferOffset, maxDrawCount, stride);
}

#endif

/* X11 stubs */

#ifdef WANT_VULKAN_X11_STUBS

typedef VkFlags VkXlibSurfaceCreateFlagsKHR;
typedef struct VkXlibSurfaceCreateInfoKHR {
    VkStructureType                sType;
    const void*                    pNext;
    VkXlibSurfaceCreateFlagsKHR    flags;
    Display*                       dpy;
    Window                         window;
} VkXlibSurfaceCreateInfoKHR;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateXlibSurfaceKHR(
    VkInstance                                  instance,
    const VkXlibSurfaceCreateInfoKHR*           pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXlibPresentationSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex,
    Display*                                    dpy,
    VisualID                                    visualID)
{
    return VK_FALSE;
}

typedef VkFlags VkXcbSurfaceCreateFlagsKHR;
typedef struct VkXcbSurfaceCreateInfoKHR {
    VkStructureType               sType;
    const void*                   pNext;
    VkXcbSurfaceCreateFlagsKHR    flags;
    xcb_connection_t*             connection;
    xcb_window_t                  window;
} VkXcbSurfaceCreateInfoKHR;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateXcbSurfaceKHR(
    VkInstance                                  instance,
    const VkXcbSurfaceCreateInfoKHR*            pCreateInfo,
    const VkAllocationCallbacks*                pAllocator,
    VkSurfaceKHR*                               pSurface)
{
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VKAPI_ATTR VkBool32 VKAPI_CALL vkGetPhysicalDeviceXcbPresentationSupportKHR(
    VkPhysicalDevice                            physicalDevice,
    uint32_t                                    queueFamilyIndex,
    xcb_connection_t*                           connection,
    xcb_visualid_t                              visual_id)
{
    return VK_FALSE;
}

VKAPI_ATTR VkResult VKAPI_CALL vkAcquireXlibDisplayEXT(
    VkPhysicalDevice                            physicalDevice,
    Display*                                    dpy,
    VkDisplayKHR                                display)
{
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL vkGetRandROutputDisplayEXT(
    VkPhysicalDevice                            physicalDevice,
    Display*                                    dpy,
    RROutput                                    rrOutput,
    VkDisplayKHR*                               pDisplay)
{
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

#endif

// vim:ts=4:sw=4:noexpandtab
