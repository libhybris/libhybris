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
#define VK_NO_PROTOTYPES 1

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

#if defined(__LP64__) && __LP64__
#define BIONIC_LIB "/system/lib64/libvulkan.so"
#else
#define BIONIC_LIB "/system/lib/libvulkan.so"
#endif

HYBRIS_LIBRARY_INITIALIZE(vulkan, BIONIC_LIB)

static void * _android_vulkan_dlsym(const char *symbol)
{
    if (vulkan_handle == NULL)
        hybris_vulkan_initialize();

    return android_dlsym(vulkan_handle, symbol);
}

struct ws_vulkan_interface hybris_vulkan_interface = {
    _android_vulkan_dlsym,
};

static PFN_vkVoidFunction (*_vkGetInstanceProcAddr)(VkInstance instance, const char* pName) = NULL;

VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator,
                          VkInstance* pInstance)
{
    if (_vkGetInstanceProcAddr == NULL) {
        HYBRIS_DLSYSM(vulkan, &_vkGetInstanceProcAddr, "vkGetInstanceProcAddr");
    }
    ws_vkSetInstanceProcAddrFunc((PFN_vkVoidFunction)_vkGetInstanceProcAddr);

    return ws_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkDestroyInstance, VkInstance, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkEnumeratePhysicalDevices, VkInstance, uint32_t *, VkPhysicalDevice *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkGetPhysicalDeviceFeatures, VkPhysicalDevice, VkPhysicalDeviceFeatures *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceFormatProperties, VkPhysicalDevice, VkFormat, VkFormatProperties *)
HYBRIS_IMPLEMENT_FUNCTION7(vulkan, VkResult, vkGetPhysicalDeviceImageFormatProperties, VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkImageFormatProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkGetPhysicalDeviceProperties, VkPhysicalDevice, VkPhysicalDeviceProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceQueueFamilyProperties, VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkGetPhysicalDeviceMemoryProperties, VkPhysicalDevice, VkPhysicalDeviceMemoryProperties *)

VkResult vkEnumerateInstanceExtensionProperties(const char* pLayerName, uint32_t* pPropertyCount,
                                                VkExtensionProperties* pProperties)
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

VkBool32 vkGetPhysicalDeviceWaylandPresentationSupportKHR(VkPhysicalDevice physicalDevice,
        uint32_t queueFamilyIndex, struct wl_display* display)
{
    return ws_vkGetPhysicalDeviceWaylandPresentationSupportKHR(physicalDevice, queueFamilyIndex, display);
}

void vkDestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks* pAllocator)
{
    ws_vkDestroySurfaceKHR(instance, surface, pAllocator);
}
#else
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroySurfaceKHR, VkInstance, VkSurfaceKHR, const VkAllocationCallbacks *)
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

HYBRIS_IMPLEMENT_FUNCTION2(vulkan, PFN_vkVoidFunction, vkGetDeviceProcAddr, VkDevice, const char *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateDevice, VkPhysicalDevice, const VkDeviceCreateInfo *, const VkAllocationCallbacks *, VkDevice *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkDestroyDevice, VkDevice, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkEnumerateDeviceExtensionProperties, VkPhysicalDevice, const char *, uint32_t *, VkExtensionProperties *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkEnumerateInstanceLayerProperties, uint32_t *, VkLayerProperties *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkEnumerateDeviceLayerProperties, VkPhysicalDevice, uint32_t *, VkLayerProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetDeviceQueue, VkDevice, uint32_t, uint32_t, VkQueue *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkQueueSubmit, VkQueue, uint32_t, const VkSubmitInfo *, VkFence)
HYBRIS_IMPLEMENT_FUNCTION1(vulkan, VkResult, vkQueueWaitIdle, VkQueue)
HYBRIS_IMPLEMENT_FUNCTION1(vulkan, VkResult, vkDeviceWaitIdle, VkDevice)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkAllocateMemory, VkDevice, const VkMemoryAllocateInfo *, const VkAllocationCallbacks *, VkDeviceMemory *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkFreeMemory, VkDevice, VkDeviceMemory, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkMapMemory, VkDevice, VkDeviceMemory, VkDeviceSize, VkDeviceSize, VkMemoryMapFlags, void **)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkUnmapMemory, VkDevice, VkDeviceMemory)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkFlushMappedMemoryRanges, VkDevice, uint32_t, const VkMappedMemoryRange *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkInvalidateMappedMemoryRanges, VkDevice, uint32_t, const VkMappedMemoryRange *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDeviceMemoryCommitment, VkDevice, VkDeviceMemory, VkDeviceSize *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkBindBufferMemory, VkDevice, VkBuffer, VkDeviceMemory, VkDeviceSize)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkBindImageMemory, VkDevice, VkImage, VkDeviceMemory, VkDeviceSize)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetBufferMemoryRequirements, VkDevice, VkBuffer, VkMemoryRequirements *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetImageMemoryRequirements, VkDevice, VkImage, VkMemoryRequirements *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetImageSparseMemoryRequirements, VkDevice, VkImage, uint32_t *, VkSparseImageMemoryRequirements *)
HYBRIS_IMPLEMENT_VOID_FUNCTION8(vulkan, vkGetPhysicalDeviceSparseImageFormatProperties, VkPhysicalDevice, VkFormat, VkImageType, VkSampleCountFlagBits, VkImageUsageFlags, VkImageTiling, uint32_t *, VkSparseImageFormatProperties *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkQueueBindSparse, VkQueue, uint32_t, const VkBindSparseInfo *, VkFence)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateFence, VkDevice, const VkFenceCreateInfo *, const VkAllocationCallbacks *, VkFence *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyFence, VkDevice, VkFence, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkResetFences, VkDevice, uint32_t, const VkFence *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkGetFenceStatus, VkDevice, VkFence)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkWaitForFences, VkDevice, uint32_t, const VkFence *, VkBool32, uint64_t)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateSemaphore, VkDevice, const VkSemaphoreCreateInfo *, const VkAllocationCallbacks *, VkSemaphore *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroySemaphore, VkDevice, VkSemaphore, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateEvent, VkDevice, const VkEventCreateInfo *, const VkAllocationCallbacks *, VkEvent *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyEvent, VkDevice, VkEvent, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkGetEventStatus, VkDevice, VkEvent)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkSetEvent, VkDevice, VkEvent)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkResetEvent, VkDevice, VkEvent)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateQueryPool, VkDevice, const VkQueryPoolCreateInfo *, const VkAllocationCallbacks *, VkQueryPool *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyQueryPool, VkDevice, VkQueryPool, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION8(vulkan, VkResult, vkGetQueryPoolResults, VkDevice, VkQueryPool, uint32_t, uint32_t, size_t, void *, VkDeviceSize, VkQueryResultFlags)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateBuffer, VkDevice, const VkBufferCreateInfo *, const VkAllocationCallbacks *, VkBuffer *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyBuffer, VkDevice, VkBuffer, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateBufferView, VkDevice, const VkBufferViewCreateInfo *, const VkAllocationCallbacks *, VkBufferView *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyBufferView, VkDevice, VkBufferView, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateImage, VkDevice, const VkImageCreateInfo *, const VkAllocationCallbacks *, VkImage *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyImage, VkDevice, VkImage, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetImageSubresourceLayout, VkDevice, VkImage, const VkImageSubresource *, VkSubresourceLayout *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateImageView, VkDevice, const VkImageViewCreateInfo *, const VkAllocationCallbacks *, VkImageView *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyImageView, VkDevice, VkImageView, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateShaderModule, VkDevice, const VkShaderModuleCreateInfo *, const VkAllocationCallbacks *, VkShaderModule *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyShaderModule, VkDevice, VkShaderModule, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreatePipelineCache, VkDevice, const VkPipelineCacheCreateInfo *, const VkAllocationCallbacks *, VkPipelineCache *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyPipelineCache, VkDevice, VkPipelineCache, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPipelineCacheData, VkDevice, VkPipelineCache, size_t *, void *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkMergePipelineCaches, VkDevice, VkPipelineCache, uint32_t, const VkPipelineCache *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkCreateGraphicsPipelines, VkDevice, VkPipelineCache, uint32_t, const VkGraphicsPipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkCreateComputePipelines, VkDevice, VkPipelineCache, uint32_t, const VkComputePipelineCreateInfo *, const VkAllocationCallbacks *, VkPipeline *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyPipeline, VkDevice, VkPipeline, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreatePipelineLayout, VkDevice, const VkPipelineLayoutCreateInfo *, const VkAllocationCallbacks *, VkPipelineLayout *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyPipelineLayout, VkDevice, VkPipelineLayout, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateSampler, VkDevice, const VkSamplerCreateInfo *, const VkAllocationCallbacks *, VkSampler *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroySampler, VkDevice, VkSampler, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateDescriptorSetLayout, VkDevice, const VkDescriptorSetLayoutCreateInfo *, const VkAllocationCallbacks *, VkDescriptorSetLayout *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyDescriptorSetLayout, VkDevice, VkDescriptorSetLayout, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateDescriptorPool, VkDevice, const VkDescriptorPoolCreateInfo *, const VkAllocationCallbacks *, VkDescriptorPool *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyDescriptorPool, VkDevice, VkDescriptorPool, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkResetDescriptorPool, VkDevice, VkDescriptorPool, VkDescriptorPoolResetFlags)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkAllocateDescriptorSets, VkDevice, const VkDescriptorSetAllocateInfo *, VkDescriptorSet *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkFreeDescriptorSets, VkDevice, VkDescriptorPool, uint32_t, const VkDescriptorSet *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkUpdateDescriptorSets, VkDevice, uint32_t, const VkWriteDescriptorSet *, uint32_t, const VkCopyDescriptorSet *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateFramebuffer, VkDevice, const VkFramebufferCreateInfo *, const VkAllocationCallbacks *, VkFramebuffer *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyFramebuffer, VkDevice, VkFramebuffer, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateRenderPass, VkDevice, const VkRenderPassCreateInfo *, const VkAllocationCallbacks *, VkRenderPass *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyRenderPass, VkDevice, VkRenderPass, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetRenderAreaGranularity, VkDevice, VkRenderPass, VkExtent2D *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateCommandPool, VkDevice, const VkCommandPoolCreateInfo *, const VkAllocationCallbacks *, VkCommandPool *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyCommandPool, VkDevice, VkCommandPool, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkResetCommandPool, VkDevice, VkCommandPool, VkCommandPoolResetFlags)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkAllocateCommandBuffers, VkDevice, const VkCommandBufferAllocateInfo *, VkCommandBuffer *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkFreeCommandBuffers, VkDevice, VkCommandPool, uint32_t, const VkCommandBuffer *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkBeginCommandBuffer, VkCommandBuffer, const VkCommandBufferBeginInfo *)
HYBRIS_IMPLEMENT_FUNCTION1(vulkan, VkResult, vkEndCommandBuffer, VkCommandBuffer)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkResetCommandBuffer, VkCommandBuffer, VkCommandBufferResetFlags)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdBindPipeline, VkCommandBuffer, VkPipelineBindPoint, VkPipeline)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetViewport, VkCommandBuffer, uint32_t, uint32_t, const VkViewport *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetScissor, VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetLineWidth, VkCommandBuffer, float)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetDepthBias, VkCommandBuffer, float, float, float)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetBlendConstants, VkCommandBuffer, const float)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetDepthBounds, VkCommandBuffer, float, float)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetStencilCompareMask, VkCommandBuffer, VkStencilFaceFlags, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetStencilWriteMask, VkCommandBuffer, VkStencilFaceFlags, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetStencilReference, VkCommandBuffer, VkStencilFaceFlags, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION8(vulkan, vkCmdBindDescriptorSets, VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkDescriptorSet *, uint32_t, const uint32_t *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdBindIndexBuffer, VkCommandBuffer, VkBuffer, VkDeviceSize, VkIndexType)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdBindVertexBuffers, VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdDraw, VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdDrawIndexed, VkCommandBuffer, uint32_t, uint32_t, uint32_t, int32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdDrawIndirect, VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdDrawIndexedIndirect, VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdDispatch, VkCommandBuffer, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdDispatchIndirect, VkCommandBuffer, VkBuffer, VkDeviceSize)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdCopyBuffer, VkCommandBuffer, VkBuffer, VkBuffer, uint32_t, const VkBufferCopy *)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdCopyImage, VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageCopy *)
HYBRIS_IMPLEMENT_VOID_FUNCTION8(vulkan, vkCmdBlitImage, VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageBlit *, VkFilter)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdCopyBufferToImage, VkCommandBuffer, VkBuffer, VkImage, VkImageLayout, uint32_t, const VkBufferImageCopy *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdCopyImageToBuffer, VkCommandBuffer, VkImage, VkImageLayout, VkBuffer, uint32_t, const VkBufferImageCopy *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdUpdateBuffer, VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, const void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdFillBuffer, VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdClearColorImage, VkCommandBuffer, VkImage, VkImageLayout, const VkClearColorValue *, uint32_t, const VkImageSubresourceRange *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdClearDepthStencilImage, VkCommandBuffer, VkImage, VkImageLayout, const VkClearDepthStencilValue *, uint32_t, const VkImageSubresourceRange *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdClearAttachments, VkCommandBuffer, uint32_t, const VkClearAttachment *, uint32_t, const VkClearRect *)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdResolveImage, VkCommandBuffer, VkImage, VkImageLayout, VkImage, VkImageLayout, uint32_t, const VkImageResolve *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetEvent, VkCommandBuffer, VkEvent, VkPipelineStageFlags)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdResetEvent, VkCommandBuffer, VkEvent, VkPipelineStageFlags)
HYBRIS_IMPLEMENT_VOID_FUNCTION11(vulkan, vkCmdWaitEvents, VkCommandBuffer, uint32_t, const VkEvent *, VkPipelineStageFlags, VkPipelineStageFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier *)
HYBRIS_IMPLEMENT_VOID_FUNCTION10(vulkan, vkCmdPipelineBarrier, VkCommandBuffer, VkPipelineStageFlags, VkPipelineStageFlags, VkDependencyFlags, uint32_t, const VkMemoryBarrier *, uint32_t, const VkBufferMemoryBarrier *, uint32_t, const VkImageMemoryBarrier *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdBeginQuery, VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdEndQuery, VkCommandBuffer, VkQueryPool, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdResetQueryPool, VkCommandBuffer, VkQueryPool, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdWriteTimestamp, VkCommandBuffer, VkPipelineStageFlagBits, VkQueryPool, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION8(vulkan, vkCmdCopyQueryPoolResults, VkCommandBuffer, VkQueryPool, uint32_t, uint32_t, VkBuffer, VkDeviceSize, VkDeviceSize, VkQueryResultFlags)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdPushConstants, VkCommandBuffer, VkPipelineLayout, VkShaderStageFlags, uint32_t, uint32_t, const void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdBeginRenderPass, VkCommandBuffer, const VkRenderPassBeginInfo *, VkSubpassContents)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdNextSubpass, VkCommandBuffer, VkSubpassContents)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkCmdEndRenderPass, VkCommandBuffer)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdExecuteCommands, VkCommandBuffer, uint32_t, const VkCommandBuffer *)
HYBRIS_IMPLEMENT_FUNCTION1(vulkan, VkResult, vkEnumerateInstanceVersion, uint32_t *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkBindBufferMemory2, VkDevice, uint32_t, const VkBindBufferMemoryInfo *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkBindImageMemory2, VkDevice, uint32_t, const VkBindImageMemoryInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkGetDeviceGroupPeerMemoryFeatures, VkDevice, uint32_t, uint32_t, uint32_t, VkPeerMemoryFeatureFlags *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDeviceMask, VkCommandBuffer, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdDispatchBase, VkCommandBuffer, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkEnumeratePhysicalDeviceGroups, VkInstance, uint32_t *, VkPhysicalDeviceGroupProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetImageMemoryRequirements2, VkDevice, const VkImageMemoryRequirementsInfo2 *, VkMemoryRequirements2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetBufferMemoryRequirements2, VkDevice, const VkBufferMemoryRequirementsInfo2 *, VkMemoryRequirements2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetImageSparseMemoryRequirements2, VkDevice, const VkImageSparseMemoryRequirementsInfo2 *, uint32_t *, VkSparseImageMemoryRequirements2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkGetPhysicalDeviceFeatures2, VkPhysicalDevice, VkPhysicalDeviceFeatures2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkGetPhysicalDeviceProperties2, VkPhysicalDevice, VkPhysicalDeviceProperties2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceFormatProperties2, VkPhysicalDevice, VkFormat, VkFormatProperties2 *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceImageFormatProperties2, VkPhysicalDevice, const VkPhysicalDeviceImageFormatInfo2 *, VkImageFormatProperties2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceQueueFamilyProperties2, VkPhysicalDevice, uint32_t *, VkQueueFamilyProperties2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkGetPhysicalDeviceMemoryProperties2, VkPhysicalDevice, VkPhysicalDeviceMemoryProperties2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetPhysicalDeviceSparseImageFormatProperties2, VkPhysicalDevice, const VkPhysicalDeviceSparseImageFormatInfo2 *, uint32_t *, VkSparseImageFormatProperties2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkTrimCommandPool, VkDevice, VkCommandPool, VkCommandPoolTrimFlags)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDeviceQueue2, VkDevice, const VkDeviceQueueInfo2 *, VkQueue *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateSamplerYcbcrConversion, VkDevice, const VkSamplerYcbcrConversionCreateInfo *, const VkAllocationCallbacks *, VkSamplerYcbcrConversion *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroySamplerYcbcrConversion, VkDevice, VkSamplerYcbcrConversion, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateDescriptorUpdateTemplate, VkDevice, const VkDescriptorUpdateTemplateCreateInfo *, const VkAllocationCallbacks *, VkDescriptorUpdateTemplate *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyDescriptorUpdateTemplate, VkDevice, VkDescriptorUpdateTemplate, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkUpdateDescriptorSetWithTemplate, VkDevice, VkDescriptorSet, VkDescriptorUpdateTemplate, const void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceExternalBufferProperties, VkPhysicalDevice, const VkPhysicalDeviceExternalBufferInfo *, VkExternalBufferProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceExternalFenceProperties, VkPhysicalDevice, const VkPhysicalDeviceExternalFenceInfo *, VkExternalFenceProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceExternalSemaphoreProperties, VkPhysicalDevice, const VkPhysicalDeviceExternalSemaphoreInfo *, VkExternalSemaphoreProperties *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDescriptorSetLayoutSupport, VkDevice, const VkDescriptorSetLayoutCreateInfo *, VkDescriptorSetLayoutSupport *)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdDrawIndirectCount, VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdDrawIndexedIndirectCount, VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkResetQueryPool, VkDevice, VkQueryPool, uint32_t, uint32_t)

#if VK_HEADER_VERSION >= 204
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceToolProperties, VkPhysicalDevice, uint32_t *, VkPhysicalDeviceToolProperties *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreatePrivateDataSlot, VkDevice, const VkPrivateDataSlotCreateInfo *, const VkAllocationCallbacks *, VkPrivateDataSlot *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyPrivateDataSlot, VkDevice, VkPrivateDataSlot, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkSetPrivateData, VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkGetPrivateData, VkDevice, VkObjectType, uint64_t, VkPrivateDataSlot, uint64_t *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetEvent2, VkCommandBuffer, VkEvent, const VkDependencyInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdResetEvent2, VkCommandBuffer, VkEvent, VkPipelineStageFlags2)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdWaitEvents2, VkCommandBuffer, uint32_t, const VkEvent *, const VkDependencyInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdPipelineBarrier2, VkCommandBuffer, const VkDependencyInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdWriteTimestamp2, VkCommandBuffer, VkPipelineStageFlags2, VkQueryPool, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkQueueSubmit2, VkQueue, uint32_t, const VkSubmitInfo2 *, VkFence)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyBuffer2, VkCommandBuffer, const VkCopyBufferInfo2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyImage2, VkCommandBuffer, const VkCopyImageInfo2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyBufferToImage2, VkCommandBuffer, const VkCopyBufferToImageInfo2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyImageToBuffer2, VkCommandBuffer, const VkCopyImageToBufferInfo2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdBlitImage2, VkCommandBuffer, const VkBlitImageInfo2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdResolveImage2, VkCommandBuffer, const VkResolveImageInfo2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetCullMode, VkCommandBuffer, VkCullModeFlags)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetFrontFace, VkCommandBuffer, VkFrontFace)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetPrimitiveTopology, VkCommandBuffer, VkPrimitiveTopology)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetViewportWithCount, VkCommandBuffer, uint32_t, const VkViewport *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetScissorWithCount, VkCommandBuffer, uint32_t, const VkRect2D *)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdBindVertexBuffers2, VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *, const VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthTestEnable, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthWriteEnable, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthCompareOp, VkCommandBuffer, VkCompareOp)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthBoundsTestEnable, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetStencilTestEnable, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdSetStencilOp, VkCommandBuffer, VkStencilFaceFlags, VkStencilOp, VkStencilOp, VkStencilOp, VkCompareOp)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetRasterizerDiscardEnable, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthBiasEnable, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetPrimitiveRestartEnable, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDeviceBufferMemoryRequirements, VkDevice, const VkDeviceBufferMemoryRequirements *, VkMemoryRequirements2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDeviceImageMemoryRequirements, VkDevice, const VkDeviceImageMemoryRequirements *, VkMemoryRequirements2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetDeviceImageSparseMemoryRequirements, VkDevice, const VkDeviceImageMemoryRequirements *, uint32_t *, VkSparseImageMemoryRequirements2 *)
#endif

HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPhysicalDeviceSurfaceSupportKHR, VkPhysicalDevice, uint32_t, VkSurfaceKHR, VkBool32 *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceSurfaceCapabilitiesKHR, VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilitiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPhysicalDeviceSurfaceFormatsKHR, VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkSurfaceFormatKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPhysicalDeviceSurfacePresentModesKHR, VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkPresentModeKHR *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkCreateSwapchainKHR, VkDevice, const VkSwapchainCreateInfoKHR *, const VkSwapchainCreateInfoKHR *, const VkAllocationCallbacks *, VkSwapchainKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroySwapchainKHR, VkDevice, VkSwapchainKHR, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetSwapchainImagesKHR, VkDevice, VkSwapchainKHR, uint32_t *, VkImage *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkAcquireNextImageKHR, VkDevice, VkSwapchainKHR, uint64_t, VkSemaphore, VkFence, uint32_t *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkQueuePresentKHR, VkQueue, const VkPresentInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkGetDeviceGroupPresentCapabilitiesKHR, VkDevice, VkDeviceGroupPresentCapabilitiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetDeviceGroupSurfacePresentModesKHR, VkDevice, VkSurfaceKHR, VkDeviceGroupPresentModeFlagsKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPhysicalDevicePresentRectanglesKHR, VkPhysicalDevice, VkSurfaceKHR, uint32_t *, VkRect2D *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkAcquireNextImage2KHR, VkDevice, const VkAcquireNextImageInfoKHR *, uint32_t *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceDisplayPropertiesKHR, VkPhysicalDevice, uint32_t *, VkDisplayPropertiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceDisplayPlanePropertiesKHR, VkPhysicalDevice, uint32_t *, VkDisplayPlanePropertiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetDisplayPlaneSupportedDisplaysKHR, VkPhysicalDevice, uint32_t, uint32_t *, VkDisplayKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetDisplayModePropertiesKHR, VkPhysicalDevice, VkDisplayKHR, uint32_t *, VkDisplayModePropertiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkCreateDisplayModeKHR, VkPhysicalDevice, VkDisplayKHR, const VkDisplayModeCreateInfoKHR *, const VkAllocationCallbacks *, VkDisplayModeKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetDisplayPlaneCapabilitiesKHR, VkPhysicalDevice, VkDisplayModeKHR, uint32_t, VkDisplayPlaneCapabilitiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateDisplayPlaneSurfaceKHR, VkInstance, const VkDisplaySurfaceCreateInfoKHR *, const VkAllocationCallbacks *, VkSurfaceKHR *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkCreateSharedSwapchainsKHR, VkDevice, uint32_t, const VkSwapchainCreateInfoKHR *, const VkSwapchainCreateInfoKHR *, const VkAllocationCallbacks *, VkSwapchainKHR *)

#if VK_HEADER_VERSION >= 238
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceVideoCapabilitiesKHR, VkPhysicalDevice, const VkVideoProfileInfoKHR *, VkVideoCapabilitiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPhysicalDeviceVideoFormatPropertiesKHR, VkPhysicalDevice, const VkPhysicalDeviceVideoFormatInfoKHR *, uint32_t *, VkVideoFormatPropertiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateVideoSessionKHR, VkDevice, const VkVideoSessionCreateInfoKHR *, const VkAllocationCallbacks *, VkVideoSessionKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyVideoSessionKHR, VkDevice, VkVideoSessionKHR, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetVideoSessionMemoryRequirementsKHR, VkDevice, VkVideoSessionKHR, uint32_t *, VkVideoSessionMemoryRequirementsKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkBindVideoSessionMemoryKHR, VkDevice, VkVideoSessionKHR, uint32_t, const VkBindVideoSessionMemoryInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateVideoSessionParametersKHR, VkDevice, const VkVideoSessionParametersCreateInfoKHR *, const VkAllocationCallbacks *, VkVideoSessionParametersKHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkUpdateVideoSessionParametersKHR, VkDevice, VkVideoSessionParametersKHR, const VkVideoSessionParametersUpdateInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyVideoSessionParametersKHR, VkDevice, VkVideoSessionParametersKHR, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdBeginVideoCodingKHR, VkCommandBuffer, const VkVideoBeginCodingInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdEndVideoCodingKHR, VkCommandBuffer, const VkVideoEndCodingInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdControlVideoCodingKHR, VkCommandBuffer, const VkVideoCodingControlInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdDecodeVideoKHR, VkCommandBuffer, const VkVideoDecodeInfoKHR *)
#endif

#if VK_HEADER_VERSION >= 197
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdBeginRendering, VkCommandBuffer, const VkRenderingInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkCmdEndRendering, VkCommandBuffer)
#endif

HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetMemoryFdKHR, VkDevice, const VkMemoryGetFdInfoKHR *, int *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetMemoryFdPropertiesKHR, VkDevice, VkExternalMemoryHandleTypeFlagBits, int, VkMemoryFdPropertiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkImportSemaphoreFdKHR, VkDevice, const VkImportSemaphoreFdInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetSemaphoreFdKHR, VkDevice, const VkSemaphoreGetFdInfoKHR *, int *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdPushDescriptorSet, VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const VkWriteDescriptorSet *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdPushDescriptorSetWithTemplate, VkCommandBuffer, VkDescriptorUpdateTemplate, VkPipelineLayout, uint32_t, const void *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateRenderPass2, VkDevice, const VkRenderPassCreateInfo2 *, const VkAllocationCallbacks *, VkRenderPass *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdBeginRenderPass2, VkCommandBuffer, const VkRenderPassBeginInfo *, const VkSubpassBeginInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdNextSubpass2, VkCommandBuffer, const VkSubpassBeginInfo *, const VkSubpassEndInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdEndRenderPass2, VkCommandBuffer, const VkSubpassEndInfo *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkGetSwapchainStatusKHR, VkDevice, VkSwapchainKHR)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkImportFenceFdKHR, VkDevice, const VkImportFenceFdInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetFenceFdKHR, VkDevice, const VkFenceGetFdInfoKHR *, int *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR, VkPhysicalDevice, uint32_t, uint32_t *, VkPerformanceCounterKHR *, VkPerformanceCounterDescriptionKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR, VkPhysicalDevice, const VkQueryPoolPerformanceCreateInfoKHR *, uint32_t *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkAcquireProfilingLockKHR, VkDevice, const VkAcquireProfilingLockInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkReleaseProfilingLockKHR, VkDevice)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceSurfaceCapabilities2KHR, VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *, VkSurfaceCapabilities2KHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPhysicalDeviceSurfaceFormats2KHR, VkPhysicalDevice, const VkPhysicalDeviceSurfaceInfo2KHR *, uint32_t *, VkSurfaceFormat2KHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceDisplayProperties2KHR, VkPhysicalDevice, uint32_t *, VkDisplayProperties2KHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceDisplayPlaneProperties2KHR, VkPhysicalDevice, uint32_t *, VkDisplayPlaneProperties2KHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetDisplayModeProperties2KHR, VkPhysicalDevice, VkDisplayKHR, uint32_t *, VkDisplayModeProperties2KHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetDisplayPlaneCapabilities2KHR, VkPhysicalDevice, const VkDisplayPlaneInfo2KHR *, VkDisplayPlaneCapabilities2KHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetSemaphoreCounterValue, VkDevice, VkSemaphore, uint64_t *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkWaitSemaphores, VkDevice, const VkSemaphoreWaitInfo *, uint64_t)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkSignalSemaphore, VkDevice, const VkSemaphoreSignalInfo *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceFragmentShadingRatesKHR, VkPhysicalDevice, uint32_t *, VkPhysicalDeviceFragmentShadingRateKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetFragmentShadingRateKHR, VkCommandBuffer, const VkExtent2D *, const VkFragmentShadingRateCombinerOpKHR)

#if VK_HEADER_VERSION >= 276
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetRenderingAttachmentLocations, VkCommandBuffer, const VkRenderingAttachmentLocationInfo *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetRenderingInputAttachmentIndices, VkCommandBuffer, const VkRenderingInputAttachmentIndexInfo *)
#endif

#if VK_HEADER_VERSION >= 185
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkWaitForPresentKHR, VkDevice, VkSwapchainKHR, uint64_t, uint64_t)
#endif

HYBRIS_IMPLEMENT_FUNCTION2(vulkan, uint64_t, vkGetBufferOpaqueCaptureAddress, VkDevice, const VkBufferDeviceAddressInfo *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, uint64_t, vkGetDeviceMemoryOpaqueCaptureAddress, VkDevice, const VkDeviceMemoryOpaqueCaptureAddressInfo *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCreateDeferredOperationKHR, VkDevice, const VkAllocationCallbacks *, VkDeferredOperationKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyDeferredOperationKHR, VkDevice, VkDeferredOperationKHR, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, uint32_t, vkGetDeferredOperationMaxConcurrencyKHR, VkDevice, VkDeferredOperationKHR)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkGetDeferredOperationResultKHR, VkDevice, VkDeferredOperationKHR)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkDeferredOperationJoinKHR, VkDevice, VkDeferredOperationKHR)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPipelineExecutablePropertiesKHR, VkDevice, const VkPipelineInfoKHR *, uint32_t *, VkPipelineExecutablePropertiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPipelineExecutableStatisticsKHR, VkDevice, const VkPipelineExecutableInfoKHR *, uint32_t *, VkPipelineExecutableStatisticKHR *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPipelineExecutableInternalRepresentationsKHR, VkDevice, const VkPipelineExecutableInfoKHR *, uint32_t *, VkPipelineExecutableInternalRepresentationKHR *)

#if VK_HEADER_VERSION >= 244
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkMapMemory2, VkDevice, const VkMemoryMapInfoKHR *, void **)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkUnmapMemory2, VkDevice, const VkMemoryUnmapInfoKHR *)
#endif

#if VK_HEADER_VERSION >= 274
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR, VkPhysicalDevice, const VkPhysicalDeviceVideoEncodeQualityLevelInfoKHR *, VkVideoEncodeQualityLevelPropertiesKHR *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkGetEncodedVideoSessionParametersKHR, VkDevice, const VkVideoEncodeSessionParametersGetInfoKHR *, VkVideoEncodeSessionParametersFeedbackInfoKHR *, size_t *, void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdEncodeVideoKHR, VkCommandBuffer, const VkVideoEncodeInfoKHR *)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdWriteBufferMarker2AMD, VkCommandBuffer, VkPipelineStageFlags2, VkBuffer, VkDeviceSize, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetQueueCheckpointData2NV, VkQueue, uint32_t *, VkCheckpointData2NV *)

#if VK_HEADER_VERSION >= 213
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdTraceRaysIndirect2KHR, VkCommandBuffer, VkDeviceAddress)
#endif

#if VK_HEADER_VERSION >= 260
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdBindIndexBuffer2, VkCommandBuffer, VkBuffer, VkDeviceSize, VkDeviceSize, VkIndexType)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetRenderingAreaGranularity, VkDevice, const VkRenderingAreaInfoKHR *, VkExtent2D *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDeviceImageSubresourceLayout, VkDevice, const VkDeviceImageSubresourceInfoKHR *, VkSubresourceLayout2KHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetImageSubresourceLayout2, VkDevice, VkImage, const VkImageSubresource2KHR *, VkSubresourceLayout2KHR *)
#endif

#if VK_HEADER_VERSION >= 255
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR, VkPhysicalDevice, uint32_t *, VkCooperativeMatrixPropertiesKHR *)
#endif

#if VK_HEADER_VERSION >= 273
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceCalibrateableTimeDomainsKHR, VkPhysicalDevice, uint32_t *, VkTimeDomainKHR *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkGetCalibratedTimestampsKHR, VkDevice, uint32_t, const VkCalibratedTimestampInfoKHR *, uint64_t *, uint64_t *)
#endif

#if VK_HEADER_VERSION >= 274
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdBindDescriptorSets2, VkCommandBuffer, const VkBindDescriptorSetsInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdPushConstants2, VkCommandBuffer, const VkPushConstantsInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdPushDescriptorSet2, VkCommandBuffer, const VkPushDescriptorSetInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdPushDescriptorSetWithTemplate2, VkCommandBuffer, const VkPushDescriptorSetWithTemplateInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDescriptorBufferOffsets2EXT, VkCommandBuffer, const VkSetDescriptorBufferOffsetsInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdBindDescriptorBufferEmbeddedSamplers2EXT, VkCommandBuffer, const VkBindDescriptorBufferEmbeddedSamplersInfoEXT *)
#endif

HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateDebugReportCallbackEXT, VkInstance, const VkDebugReportCallbackCreateInfoEXT *, const VkAllocationCallbacks *, VkDebugReportCallbackEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyDebugReportCallbackEXT, VkInstance, VkDebugReportCallbackEXT, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION8(vulkan, vkDebugReportMessageEXT, VkInstance, VkDebugReportFlagsEXT, VkDebugReportObjectTypeEXT, uint64_t, size_t, int32_t, const char *, const char *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkDebugMarkerSetObjectTagEXT, VkDevice, const VkDebugMarkerObjectTagInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkDebugMarkerSetObjectNameEXT, VkDevice, const VkDebugMarkerObjectNameInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdDebugMarkerBeginEXT, VkCommandBuffer, const VkDebugMarkerMarkerInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkCmdDebugMarkerEndEXT, VkCommandBuffer)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdDebugMarkerInsertEXT, VkCommandBuffer, const VkDebugMarkerMarkerInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdBindTransformFeedbackBuffersEXT, VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *, const VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdBeginTransformFeedbackEXT, VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdEndTransformFeedbackEXT, VkCommandBuffer, uint32_t, uint32_t, const VkBuffer *, const VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdBeginQueryIndexedEXT, VkCommandBuffer, VkQueryPool, uint32_t, VkQueryControlFlags, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdEndQueryIndexedEXT, VkCommandBuffer, VkQueryPool, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdDrawIndirectByteCountEXT, VkCommandBuffer, uint32_t, uint32_t, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateCuModuleNVX, VkDevice, const VkCuModuleCreateInfoNVX *, const VkAllocationCallbacks *, VkCuModuleNVX *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateCuFunctionNVX, VkDevice, const VkCuFunctionCreateInfoNVX *, const VkAllocationCallbacks *, VkCuFunctionNVX *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyCuModuleNVX, VkDevice, VkCuModuleNVX, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyCuFunctionNVX, VkDevice, VkCuFunctionNVX, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCuLaunchKernelNVX, VkCommandBuffer, const VkCuLaunchInfoNVX *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, uint32_t, vkGetImageViewHandleNVX, VkDevice, const VkImageViewHandleInfoNVX *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetImageViewAddressNVX, VkDevice, VkImageView, VkImageViewAddressPropertiesNVX *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkGetShaderInfoAMD, VkDevice, VkPipeline, VkShaderStageFlagBits, VkShaderInfoTypeAMD, size_t *, void *)
HYBRIS_IMPLEMENT_FUNCTION8(vulkan, VkResult, vkGetPhysicalDeviceExternalImageFormatPropertiesNV, VkPhysicalDevice, VkFormat, VkImageType, VkImageTiling, VkImageUsageFlags, VkImageCreateFlags, VkExternalMemoryHandleTypeFlagsNV, VkExternalImageFormatPropertiesNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdBeginConditionalRenderingEXT, VkCommandBuffer, const VkConditionalRenderingBeginInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkCmdEndConditionalRenderingEXT, VkCommandBuffer)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetViewportWScalingNV, VkCommandBuffer, uint32_t, uint32_t, const VkViewportWScalingNV *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkReleaseDisplayEXT, VkPhysicalDevice, VkDisplayKHR)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceSurfaceCapabilities2EXT, VkPhysicalDevice, VkSurfaceKHR, VkSurfaceCapabilities2EXT *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkDisplayPowerControlEXT, VkDevice, VkDisplayKHR, const VkDisplayPowerInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkRegisterDeviceEventEXT, VkDevice, const VkDeviceEventInfoEXT *, const VkAllocationCallbacks *, VkFence *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkRegisterDisplayEventEXT, VkDevice, VkDisplayKHR, const VkDisplayEventInfoEXT *, const VkAllocationCallbacks *, VkFence *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetSwapchainCounterEXT, VkDevice, VkSwapchainKHR, VkSurfaceCounterFlagBitsEXT, uint64_t *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetRefreshCycleDurationGOOGLE, VkDevice, VkSwapchainKHR, VkRefreshCycleDurationGOOGLE *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPastPresentationTimingGOOGLE, VkDevice, VkSwapchainKHR, uint32_t *, VkPastPresentationTimingGOOGLE *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetDiscardRectangleEXT, VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *)

#if VK_HEADER_VERSION >= 241
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDiscardRectangleEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDiscardRectangleModeEXT, VkCommandBuffer, VkDiscardRectangleModeEXT)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkSetHdrMetadataEXT, VkDevice, uint32_t, const VkSwapchainKHR *, const VkHdrMetadataEXT *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkSetDebugUtilsObjectNameEXT, VkDevice, const VkDebugUtilsObjectNameInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkSetDebugUtilsObjectTagEXT, VkDevice, const VkDebugUtilsObjectTagInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkQueueBeginDebugUtilsLabelEXT, VkQueue, const VkDebugUtilsLabelEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkQueueEndDebugUtilsLabelEXT, VkQueue)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkQueueInsertDebugUtilsLabelEXT, VkQueue, const VkDebugUtilsLabelEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdBeginDebugUtilsLabelEXT, VkCommandBuffer, const VkDebugUtilsLabelEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkCmdEndDebugUtilsLabelEXT, VkCommandBuffer)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdInsertDebugUtilsLabelEXT, VkCommandBuffer, const VkDebugUtilsLabelEXT *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateDebugUtilsMessengerEXT, VkInstance, const VkDebugUtilsMessengerCreateInfoEXT *, const VkAllocationCallbacks *, VkDebugUtilsMessengerEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyDebugUtilsMessengerEXT, VkInstance, VkDebugUtilsMessengerEXT, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkSubmitDebugUtilsMessageEXT, VkInstance, VkDebugUtilsMessageSeverityFlagBitsEXT, VkDebugUtilsMessageTypeFlagsEXT, const VkDebugUtilsMessengerCallbackDataEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetSampleLocationsEXT, VkCommandBuffer, const VkSampleLocationsInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPhysicalDeviceMultisamplePropertiesEXT, VkPhysicalDevice, VkSampleCountFlagBits, VkMultisamplePropertiesEXT *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetImageDrmFormatModifierPropertiesEXT, VkDevice, VkImage, VkImageDrmFormatModifierPropertiesEXT *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateValidationCacheEXT, VkDevice, const VkValidationCacheCreateInfoEXT *, const VkAllocationCallbacks *, VkValidationCacheEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyValidationCacheEXT, VkDevice, VkValidationCacheEXT, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkMergeValidationCachesEXT, VkDevice, VkValidationCacheEXT, uint32_t, const VkValidationCacheEXT *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetValidationCacheDataEXT, VkDevice, VkValidationCacheEXT, size_t *, void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdBindShadingRateImageNV, VkCommandBuffer, VkImageView, VkImageLayout)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetViewportShadingRatePaletteNV, VkCommandBuffer, uint32_t, uint32_t, const VkShadingRatePaletteNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetCoarseSampleOrderNV, VkCommandBuffer, VkCoarseSampleOrderTypeNV, uint32_t, const VkCoarseSampleOrderCustomNV *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateAccelerationStructureNV, VkDevice, const VkAccelerationStructureCreateInfoNV *, const VkAllocationCallbacks *, VkAccelerationStructureNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyAccelerationStructureNV, VkDevice, VkAccelerationStructureNV, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetAccelerationStructureMemoryRequirementsNV, VkDevice, const VkAccelerationStructureMemoryRequirementsInfoNV *, VkMemoryRequirements2 *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkBindAccelerationStructureMemoryNV, VkDevice, uint32_t, const VkBindAccelerationStructureMemoryInfoNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION9(vulkan, vkCmdBuildAccelerationStructureNV, VkCommandBuffer, const VkAccelerationStructureInfoNV *, VkBuffer, VkDeviceSize, VkBool32, VkAccelerationStructureNV, VkAccelerationStructureNV, VkBuffer, VkDeviceSize)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdCopyAccelerationStructureNV, VkCommandBuffer, VkAccelerationStructureNV, VkAccelerationStructureNV, VkCopyAccelerationStructureModeKHR)
HYBRIS_IMPLEMENT_VOID_FUNCTION15(vulkan, vkCmdTraceRaysNV, VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, VkBuffer, VkDeviceSize, VkDeviceSize, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkCreateRayTracingPipelinesNV, VkDevice, VkPipelineCache, uint32_t, const VkRayTracingPipelineCreateInfoNV *, const VkAllocationCallbacks *, VkPipeline *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkGetRayTracingShaderGroupHandlesKHR, VkDevice, VkPipeline, uint32_t, uint32_t, size_t, void *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetAccelerationStructureHandleNV, VkDevice, VkAccelerationStructureNV, size_t, void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdWriteAccelerationStructuresPropertiesNV, VkCommandBuffer, uint32_t, const VkAccelerationStructureNV *, VkQueryType, VkQueryPool, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCompileDeferredNV, VkDevice, VkPipeline, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetMemoryHostPointerPropertiesEXT, VkDevice, VkExternalMemoryHandleTypeFlagBits, const void *, VkMemoryHostPointerPropertiesEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdWriteBufferMarkerAMD, VkCommandBuffer, VkPipelineStageFlagBits, VkBuffer, VkDeviceSize, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdDrawMeshTasksNV, VkCommandBuffer, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdDrawMeshTasksIndirectNV, VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdDrawMeshTasksIndirectCountNV, VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t)

#if VK_HEADER_VERSION >= 241
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetExclusiveScissorEnableNV, VkCommandBuffer, uint32_t, uint32_t, const VkBool32 *)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetExclusiveScissorNV, VkCommandBuffer, uint32_t, uint32_t, const VkRect2D *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetCheckpointNV, VkCommandBuffer, const void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetQueueCheckpointDataNV, VkQueue, uint32_t *, VkCheckpointDataNV *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkInitializePerformanceApiINTEL, VkDevice, const VkInitializePerformanceApiInfoINTEL *)
HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkUninitializePerformanceApiINTEL, VkDevice)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkCmdSetPerformanceMarkerINTEL, VkCommandBuffer, const VkPerformanceMarkerInfoINTEL *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkCmdSetPerformanceStreamMarkerINTEL, VkCommandBuffer, const VkPerformanceStreamMarkerInfoINTEL *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkCmdSetPerformanceOverrideINTEL, VkCommandBuffer, const VkPerformanceOverrideInfoINTEL *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkAcquirePerformanceConfigurationINTEL, VkDevice, const VkPerformanceConfigurationAcquireInfoINTEL *, VkPerformanceConfigurationINTEL *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkReleasePerformanceConfigurationINTEL, VkDevice, VkPerformanceConfigurationINTEL)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkQueueSetPerformanceConfigurationINTEL, VkQueue, VkPerformanceConfigurationINTEL)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPerformanceParameterINTEL, VkDevice, VkPerformanceParameterTypeINTEL, VkPerformanceValueINTEL *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkSetLocalDimmingAMD, VkDevice, VkSwapchainKHR, VkBool32)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkDeviceAddress, vkGetBufferDeviceAddress, VkDevice, const VkBufferDeviceAddressInfo *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceCooperativeMatrixPropertiesNV, VkPhysicalDevice, uint32_t *, VkCooperativeMatrixPropertiesNV *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV, VkPhysicalDevice, uint32_t *, VkFramebufferMixedSamplesCombinationNV *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateHeadlessSurfaceEXT, VkInstance, const VkHeadlessSurfaceCreateInfoEXT *, const VkAllocationCallbacks *, VkSurfaceKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetLineStipple, VkCommandBuffer, uint32_t, uint16_t)

#if VK_HEADER_VERSION >= 258
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkCopyMemoryToImage, VkDevice, const VkCopyMemoryToImageInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkCopyImageToMemory, VkDevice, const VkCopyImageToMemoryInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkCopyImageToImage, VkDevice, const VkCopyImageToImageInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkTransitionImageLayout, VkDevice, uint32_t, const VkHostImageLayoutTransitionInfoEXT *)
#endif

#if VK_HEADER_VERSION >= 237
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkResult, vkReleaseSwapchainImagesKHR, VkDevice, const VkReleaseSwapchainImagesInfoEXT *)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetGeneratedCommandsMemoryRequirementsNV, VkDevice, const VkGeneratedCommandsMemoryRequirementsInfoNV *, VkMemoryRequirements2 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdPreprocessGeneratedCommandsNV, VkCommandBuffer, const VkGeneratedCommandsInfoNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdExecuteGeneratedCommandsNV, VkCommandBuffer, VkBool32, const VkGeneratedCommandsInfoNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdBindPipelineShaderGroupNV, VkCommandBuffer, VkPipelineBindPoint, VkPipeline, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateIndirectCommandsLayoutNV, VkDevice, const VkIndirectCommandsLayoutCreateInfoNV *, const VkAllocationCallbacks *, VkIndirectCommandsLayoutNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyIndirectCommandsLayoutNV, VkDevice, VkIndirectCommandsLayoutNV, const VkAllocationCallbacks *)

#if VK_HEADER_VERSION >= 254
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthBias2EXT, VkCommandBuffer, const VkDepthBiasInfoEXT *)
#endif

HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkAcquireDrmDisplayEXT, VkPhysicalDevice, int32_t, VkDisplayKHR)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetDrmDisplayEXT, VkPhysicalDevice, int32_t, uint32_t, VkDisplayKHR *)

#if VK_HEADER_VERSION >= 269
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateCudaModuleNV, VkDevice, const VkCudaModuleCreateInfoNV *, const VkAllocationCallbacks *, VkCudaModuleNV *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetCudaModuleCacheNV, VkDevice, VkCudaModuleNV, size_t *, void *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateCudaFunctionNV, VkDevice, const VkCudaFunctionCreateInfoNV *, const VkAllocationCallbacks *, VkCudaFunctionNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyCudaModuleNV, VkDevice, VkCudaModuleNV, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyCudaFunctionNV, VkDevice, VkCudaFunctionNV, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCudaLaunchKernelNV, VkCommandBuffer, const VkCudaLaunchInfoNV *)
#endif

#if VK_HEADER_VERSION >= 235
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDescriptorSetLayoutSizeEXT, VkDevice, VkDescriptorSetLayout, VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetDescriptorSetLayoutBindingOffsetEXT, VkDevice, VkDescriptorSetLayout, uint32_t, VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetDescriptorEXT, VkDevice, const VkDescriptorGetInfoEXT *, size_t, void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdBindDescriptorBuffersEXT, VkCommandBuffer, uint32_t, const VkDescriptorBufferBindingInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdSetDescriptorBufferOffsetsEXT, VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t, uint32_t, const uint32_t *, const VkDeviceSize *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdBindDescriptorBufferEmbeddedSamplersEXT, VkCommandBuffer, VkPipelineBindPoint, VkPipelineLayout, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetBufferOpaqueCaptureDescriptorDataEXT, VkDevice, const VkBufferCaptureDescriptorDataInfoEXT *, void *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetImageOpaqueCaptureDescriptorDataEXT, VkDevice, const VkImageCaptureDescriptorDataInfoEXT *, void *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetImageViewOpaqueCaptureDescriptorDataEXT, VkDevice, const VkImageViewCaptureDescriptorDataInfoEXT *, void *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetSamplerOpaqueCaptureDescriptorDataEXT, VkDevice, const VkSamplerCaptureDescriptorDataInfoEXT *, void *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT, VkDevice, const VkAccelerationStructureCaptureDescriptorDataInfoEXT *, void *)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetFragmentShadingRateEnumNV, VkCommandBuffer, VkFragmentShadingRateNV, const VkFragmentShadingRateCombinerOpKHR)

#if VK_HEADER_VERSION >= 230
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetDeviceFaultInfoEXT, VkDevice, VkDeviceFaultCountsEXT *, VkDeviceFaultInfoEXT *)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdSetVertexInputEXT, VkCommandBuffer, uint32_t, const VkVertexInputBindingDescription2EXT *, uint32_t, const VkVertexInputAttributeDescription2EXT *)

#if VK_HEADER_VERSION >= 184
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI, VkDevice, VkRenderPass, VkExtent2D *)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION1(vulkan, vkCmdSubpassShadingHUAWEI, VkCommandBuffer)

#if VK_HEADER_VERSION >= 185
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdBindInvocationMaskHUAWEI, VkCommandBuffer, VkImageView, VkImageLayout)
#endif

#if VK_HEADER_VERSION >= 184
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetMemoryRemoteAddressNV, VkDevice, const VkMemoryGetRemoteAddressInfoNV *, VkRemoteAddressNV *)
#endif

#if VK_HEADER_VERSION >= 213
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetPipelinePropertiesEXT, VkDevice, const VkPipelineInfoKHR *, VkBaseOutStructure *)
#endif

HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetPatchControlPointsEXT, VkCommandBuffer, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetLogicOpEXT, VkCommandBuffer, VkLogicOp)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetColorWriteEnableEXT, VkCommandBuffer, uint32_t, const VkBool32 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdDrawMultiEXT, VkCommandBuffer, uint32_t, const VkMultiDrawInfoEXT *, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdDrawMultiIndexedEXT, VkCommandBuffer, uint32_t, const VkMultiDrawIndexedInfoEXT *, uint32_t, uint32_t, uint32_t, const int32_t *)

#if VK_HEADER_VERSION >= 230
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateMicromapEXT, VkDevice, const VkMicromapCreateInfoEXT *, const VkAllocationCallbacks *, VkMicromapEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyMicromapEXT, VkDevice, VkMicromapEXT, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdBuildMicromapsEXT, VkCommandBuffer, uint32_t, const VkMicromapBuildInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkBuildMicromapsEXT, VkDevice, VkDeferredOperationKHR, uint32_t, const VkMicromapBuildInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCopyMicromapEXT, VkDevice, VkDeferredOperationKHR, const VkCopyMicromapInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCopyMicromapToMemoryEXT, VkDevice, VkDeferredOperationKHR, const VkCopyMicromapToMemoryInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCopyMemoryToMicromapEXT, VkDevice, VkDeferredOperationKHR, const VkCopyMemoryToMicromapInfoEXT *)
HYBRIS_IMPLEMENT_FUNCTION7(vulkan, VkResult, vkWriteMicromapsPropertiesEXT, VkDevice, uint32_t, const VkMicromapEXT *, VkQueryType, size_t, void *, size_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyMicromapEXT, VkCommandBuffer, const VkCopyMicromapInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyMicromapToMemoryEXT, VkCommandBuffer, const VkCopyMicromapToMemoryInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyMemoryToMicromapEXT, VkCommandBuffer, const VkCopyMemoryToMicromapInfoEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdWriteMicromapsPropertiesEXT, VkCommandBuffer, uint32_t, const VkMicromapEXT *, VkQueryType, VkQueryPool, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDeviceMicromapCompatibilityEXT, VkDevice, const VkMicromapVersionInfoEXT *, VkAccelerationStructureCompatibilityKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkGetMicromapBuildSizesEXT, VkDevice, VkAccelerationStructureBuildTypeKHR, const VkMicromapBuildInfoEXT *, VkMicromapBuildSizesInfoEXT *)
#endif

#if VK_HEADER_VERSION >= 239
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdDrawClusterHUAWEI, VkCommandBuffer, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdDrawClusterIndirectHUAWEI, VkCommandBuffer, VkBuffer, VkDeviceSize)
#endif

#if VK_HEADER_VERSION >= 191
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkSetDeviceMemoryPriorityEXT, VkDevice, VkDeviceMemory, float)
#endif

#if VK_HEADER_VERSION >= 207
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDescriptorSetLayoutHostMappingInfoVALVE, VkDevice, const VkDescriptorSetBindingReferenceVALVE *, VkDescriptorSetLayoutHostMappingInfoVALVE *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDescriptorSetHostMappingVALVE, VkDevice, VkDescriptorSet, void **)
#endif

#if VK_HEADER_VERSION >= 233
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdCopyMemoryIndirectNV, VkCommandBuffer, VkDeviceAddress, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdCopyMemoryToImageIndirectNV, VkCommandBuffer, VkDeviceAddress, uint32_t, uint32_t, VkImage, VkImageLayout, const VkImageSubresourceLayers *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdDecompressMemoryNV, VkCommandBuffer, uint32_t, const VkDecompressMemoryRegionNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdDecompressMemoryIndirectCountNV, VkCommandBuffer, VkDeviceAddress, VkDeviceAddress, uint32_t)
#endif

#if VK_HEADER_VERSION >= 258
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetPipelineIndirectMemoryRequirementsNV, VkDevice, const VkComputePipelineCreateInfo *, VkMemoryRequirements2 *)
#endif

#if VK_HEADER_VERSION == 258
#endif

#if VK_HEADER_VERSION >= 259
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdUpdatePipelineIndirectBufferNV, VkCommandBuffer, VkPipelineBindPoint, VkPipeline)
#endif

#if VK_HEADER_VERSION >= 258
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkDeviceAddress, vkGetPipelineIndirectDeviceAddressNV, VkDevice, const VkPipelineIndirectDeviceAddressInfoNV *)
#endif

#if VK_HEADER_VERSION >= 230
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthClampEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetPolygonModeEXT, VkCommandBuffer, VkPolygonMode)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetRasterizationSamplesEXT, VkCommandBuffer, VkSampleCountFlagBits)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetSampleMaskEXT, VkCommandBuffer, VkSampleCountFlagBits, const VkSampleMask *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetAlphaToCoverageEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetAlphaToOneEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetLogicOpEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetColorBlendEnableEXT, VkCommandBuffer, uint32_t, uint32_t, const VkBool32 *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetColorBlendEquationEXT, VkCommandBuffer, uint32_t, uint32_t, const VkColorBlendEquationEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetColorWriteMaskEXT, VkCommandBuffer, uint32_t, uint32_t, const VkColorComponentFlags *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetTessellationDomainOriginEXT, VkCommandBuffer, VkTessellationDomainOrigin)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetRasterizationStreamEXT, VkCommandBuffer, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetConservativeRasterizationModeEXT, VkCommandBuffer, VkConservativeRasterizationModeEXT)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetExtraPrimitiveOverestimationSizeEXT, VkCommandBuffer, float)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthClipEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetSampleLocationsEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetColorBlendAdvancedEXT, VkCommandBuffer, uint32_t, uint32_t, const VkColorBlendAdvancedEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetProvokingVertexModeEXT, VkCommandBuffer, VkProvokingVertexModeEXT)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetLineRasterizationModeEXT, VkCommandBuffer, VkLineRasterizationModeEXT)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetLineStippleEnableEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetDepthClipNegativeOneToOneEXT, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetViewportWScalingEnableNV, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdSetViewportSwizzleNV, VkCommandBuffer, uint32_t, uint32_t, const VkViewportSwizzleNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetCoverageToColorEnableNV, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetCoverageToColorLocationNV, VkCommandBuffer, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetCoverageModulationModeNV, VkCommandBuffer, VkCoverageModulationModeNV)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetCoverageModulationTableEnableNV, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdSetCoverageModulationTableNV, VkCommandBuffer, uint32_t, const float *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetShadingRateImageEnableNV, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetRepresentativeFragmentTestEnableNV, VkCommandBuffer, VkBool32)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetCoverageReductionModeNV, VkCommandBuffer, VkCoverageReductionModeNV)
#endif

#if VK_HEADER_VERSION >= 219
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetShaderModuleIdentifierEXT, VkDevice, VkShaderModule, VkShaderModuleIdentifierEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetShaderModuleCreateInfoIdentifierEXT, VkDevice, const VkShaderModuleCreateInfo *, VkShaderModuleIdentifierEXT *)
#endif

#if VK_HEADER_VERSION >= 230
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetPhysicalDeviceOpticalFlowImageFormatsNV, VkPhysicalDevice, const VkOpticalFlowImageFormatInfoNV *, uint32_t *, VkOpticalFlowImageFormatPropertiesNV *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateOpticalFlowSessionNV, VkDevice, const VkOpticalFlowSessionCreateInfoNV *, const VkAllocationCallbacks *, VkOpticalFlowSessionNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyOpticalFlowSessionNV, VkDevice, VkOpticalFlowSessionNV, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkBindOpticalFlowSessionImageNV, VkDevice, VkOpticalFlowSessionNV, VkOpticalFlowSessionBindingPointNV, VkImageView, VkImageLayout)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkCmdOpticalFlowExecuteNV, VkCommandBuffer, VkOpticalFlowSessionNV, const VkOpticalFlowExecuteInfoNV *)
#endif

#if VK_HEADER_VERSION >= 246
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkCreateShadersEXT, VkDevice, uint32_t, const VkShaderCreateInfoEXT *, const VkAllocationCallbacks *, VkShaderEXT *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyShaderEXT, VkDevice, VkShaderEXT, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetShaderBinaryDataEXT, VkDevice, VkShaderEXT, size_t *, void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdBindShadersEXT, VkCommandBuffer, uint32_t, const VkShaderStageFlagBits *, const VkShaderEXT *)
#endif

#if VK_HEADER_VERSION >= 222
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkGetFramebufferTilePropertiesQCOM, VkDevice, VkFramebuffer, uint32_t *, VkTilePropertiesQCOM *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkGetDynamicRenderingTilePropertiesQCOM, VkDevice, const VkRenderingInfo *, VkTilePropertiesQCOM *)
#endif

#if VK_HEADER_VERSION >= 266
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkSetLatencySleepModeNV, VkDevice, VkSwapchainKHR, const VkLatencySleepModeInfoNV *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkLatencySleepNV, VkDevice, VkSwapchainKHR, const VkLatencySleepInfoNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkSetLatencyMarkerNV, VkDevice, VkSwapchainKHR, const VkSetLatencyMarkerInfoNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetLatencyTimingsNV, VkDevice, VkSwapchainKHR, VkGetLatencyMarkerInfoNV *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkQueueNotifyOutOfBandNV, VkQueue, const VkOutOfBandQueueTypeInfoNV *)
#endif

#if VK_HEADER_VERSION >= 250
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetAttachmentFeedbackLoopEnableEXT, VkCommandBuffer, VkImageAspectFlags)
#endif

HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkResult, vkCreateAccelerationStructureKHR, VkDevice, const VkAccelerationStructureCreateInfoKHR *, const VkAllocationCallbacks *, VkAccelerationStructureKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkDestroyAccelerationStructureKHR, VkDevice, VkAccelerationStructureKHR, const VkAllocationCallbacks *)
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdBuildAccelerationStructuresKHR, VkCommandBuffer, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR *, const VkAccelerationStructureBuildRangeInfoKHR * const*)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdBuildAccelerationStructuresIndirectKHR, VkCommandBuffer, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR *, const VkDeviceAddress *, const uint32_t *, const uint32_t * const*)
HYBRIS_IMPLEMENT_FUNCTION5(vulkan, VkResult, vkBuildAccelerationStructuresKHR, VkDevice, VkDeferredOperationKHR, uint32_t, const VkAccelerationStructureBuildGeometryInfoKHR *, const VkAccelerationStructureBuildRangeInfoKHR * const*)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCopyAccelerationStructureKHR, VkDevice, VkDeferredOperationKHR, const VkCopyAccelerationStructureInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCopyAccelerationStructureToMemoryKHR, VkDevice, VkDeferredOperationKHR, const VkCopyAccelerationStructureToMemoryInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION3(vulkan, VkResult, vkCopyMemoryToAccelerationStructureKHR, VkDevice, VkDeferredOperationKHR, const VkCopyMemoryToAccelerationStructureInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION7(vulkan, VkResult, vkWriteAccelerationStructuresPropertiesKHR, VkDevice, uint32_t, const VkAccelerationStructureKHR *, VkQueryType, size_t, void *, size_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyAccelerationStructureKHR, VkCommandBuffer, const VkCopyAccelerationStructureInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyAccelerationStructureToMemoryKHR, VkCommandBuffer, const VkCopyAccelerationStructureToMemoryInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdCopyMemoryToAccelerationStructureKHR, VkCommandBuffer, const VkCopyMemoryToAccelerationStructureInfoKHR *)
HYBRIS_IMPLEMENT_FUNCTION2(vulkan, VkDeviceAddress, vkGetAccelerationStructureDeviceAddressKHR, VkDevice, const VkAccelerationStructureDeviceAddressInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdWriteAccelerationStructuresPropertiesKHR, VkCommandBuffer, uint32_t, const VkAccelerationStructureKHR *, VkQueryType, VkQueryPool, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION3(vulkan, vkGetDeviceAccelerationStructureCompatibilityKHR, VkDevice, const VkAccelerationStructureVersionInfoKHR *, VkAccelerationStructureCompatibilityKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkGetAccelerationStructureBuildSizesKHR, VkDevice, VkAccelerationStructureBuildTypeKHR, const VkAccelerationStructureBuildGeometryInfoKHR *, const uint32_t *, VkAccelerationStructureBuildSizesInfoKHR *)
HYBRIS_IMPLEMENT_VOID_FUNCTION8(vulkan, vkCmdTraceRaysKHR, VkCommandBuffer, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_FUNCTION7(vulkan, VkResult, vkCreateRayTracingPipelinesKHR, VkDevice, VkDeferredOperationKHR, VkPipelineCache, uint32_t, const VkRayTracingPipelineCreateInfoKHR *, const VkAllocationCallbacks *, VkPipeline *)
HYBRIS_IMPLEMENT_FUNCTION6(vulkan, VkResult, vkGetRayTracingCaptureReplayShaderGroupHandlesKHR, VkDevice, VkPipeline, uint32_t, uint32_t, size_t, void *)
HYBRIS_IMPLEMENT_VOID_FUNCTION6(vulkan, vkCmdTraceRaysIndirectKHR, VkCommandBuffer, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, const VkStridedDeviceAddressRegionKHR *, VkDeviceAddress)
HYBRIS_IMPLEMENT_FUNCTION4(vulkan, VkDeviceSize, vkGetRayTracingShaderGroupStackSizeKHR, VkDevice, VkPipeline, uint32_t, VkShaderGroupShaderKHR)
HYBRIS_IMPLEMENT_VOID_FUNCTION2(vulkan, vkCmdSetRayTracingPipelineStackSizeKHR, VkCommandBuffer, uint32_t)

#if VK_HEADER_VERSION >= 226
HYBRIS_IMPLEMENT_VOID_FUNCTION4(vulkan, vkCmdDrawMeshTasksEXT, VkCommandBuffer, uint32_t, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION5(vulkan, vkCmdDrawMeshTasksIndirectEXT, VkCommandBuffer, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
HYBRIS_IMPLEMENT_VOID_FUNCTION7(vulkan, vkCmdDrawMeshTasksIndirectCountEXT, VkCommandBuffer, VkBuffer, VkDeviceSize, VkBuffer, VkDeviceSize, uint32_t, uint32_t)
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
