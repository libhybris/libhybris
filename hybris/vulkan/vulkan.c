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

#include <hybris/common/binding.h>
#include <hybris/common/floating_point_abi.h>
#include "config.h"
#include "logging.h"
#include "ws.h"

static void *vulkan_handle = NULL;

/*
 * This generates a function that when first called overwrites it's plt entry with new address.
 * Subsequent calls jump directly at the target function in the android library.
 * This means effectively 0 call overhead after the first call.
 */

#define VULKAN_IDLOAD(sym) \
 __asm__ (".type " #sym ", %gnu_indirect_function"); \
typeof(sym) * sym ## _dispatch (void) __asm__ (#sym);\
typeof(sym) * sym ## _dispatch (void) \
{ \
    if (!vulkan_handle) _init_androidvulkan(); \
    return (void *) android_dlsym(vulkan_handle, #sym); \
}

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

/* Use IDLOAD approach also for float functions, since vulkan uses the aapcs-vfp calling convention even on android */

VkResult vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
    if (_vkGetInstanceProcAddr == NULL) {
        HYBRIS_DLSYSM(vulkan, &_vkGetInstanceProcAddr, "vkGetInstanceProcAddr");
    }
    ws_vkSetInstanceProcAddrFunc((PFN_vkVoidFunction)_vkGetInstanceProcAddr);

    return ws_vkCreateInstance(pCreateInfo, pAllocator, pInstance);
}

VULKAN_IDLOAD(vkDestroyInstance);
VULKAN_IDLOAD(vkEnumeratePhysicalDevices);
VULKAN_IDLOAD(vkGetPhysicalDeviceFeatures);
VULKAN_IDLOAD(vkGetPhysicalDeviceFormatProperties);
VULKAN_IDLOAD(vkGetPhysicalDeviceImageFormatProperties);
VULKAN_IDLOAD(vkGetPhysicalDeviceProperties);
VULKAN_IDLOAD(vkGetPhysicalDeviceQueueFamilyProperties);
VULKAN_IDLOAD(vkGetPhysicalDeviceMemoryProperties);

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
VULKAN_IDLOAD(vkDestroySurfaceKHR);
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

VULKAN_IDLOAD(vkGetDeviceProcAddr);
VULKAN_IDLOAD(vkCreateDevice);
VULKAN_IDLOAD(vkDestroyDevice);
VULKAN_IDLOAD(vkEnumerateDeviceExtensionProperties);
VULKAN_IDLOAD(vkEnumerateInstanceLayerProperties);
VULKAN_IDLOAD(vkEnumerateDeviceLayerProperties);
VULKAN_IDLOAD(vkGetDeviceQueue);
VULKAN_IDLOAD(vkQueueSubmit);
VULKAN_IDLOAD(vkQueueWaitIdle);
VULKAN_IDLOAD(vkDeviceWaitIdle);
VULKAN_IDLOAD(vkAllocateMemory);
VULKAN_IDLOAD(vkFreeMemory);
VULKAN_IDLOAD(vkMapMemory);
VULKAN_IDLOAD(vkUnmapMemory);
VULKAN_IDLOAD(vkFlushMappedMemoryRanges);
VULKAN_IDLOAD(vkInvalidateMappedMemoryRanges);
VULKAN_IDLOAD(vkGetDeviceMemoryCommitment);
VULKAN_IDLOAD(vkBindBufferMemory);
VULKAN_IDLOAD(vkBindImageMemory);
VULKAN_IDLOAD(vkGetBufferMemoryRequirements);
VULKAN_IDLOAD(vkGetImageMemoryRequirements);
VULKAN_IDLOAD(vkGetImageSparseMemoryRequirements);
VULKAN_IDLOAD(vkGetPhysicalDeviceSparseImageFormatProperties);
VULKAN_IDLOAD(vkQueueBindSparse);
VULKAN_IDLOAD(vkCreateFence);
VULKAN_IDLOAD(vkDestroyFence);
VULKAN_IDLOAD(vkResetFences);
VULKAN_IDLOAD(vkGetFenceStatus);
VULKAN_IDLOAD(vkWaitForFences);
VULKAN_IDLOAD(vkCreateSemaphore);
VULKAN_IDLOAD(vkDestroySemaphore);
VULKAN_IDLOAD(vkCreateEvent);
VULKAN_IDLOAD(vkDestroyEvent);
VULKAN_IDLOAD(vkGetEventStatus);
VULKAN_IDLOAD(vkSetEvent);
VULKAN_IDLOAD(vkResetEvent);
VULKAN_IDLOAD(vkCreateQueryPool);
VULKAN_IDLOAD(vkDestroyQueryPool);
VULKAN_IDLOAD(vkGetQueryPoolResults);
VULKAN_IDLOAD(vkCreateBuffer);
VULKAN_IDLOAD(vkDestroyBuffer);
VULKAN_IDLOAD(vkCreateBufferView);
VULKAN_IDLOAD(vkDestroyBufferView);
VULKAN_IDLOAD(vkCreateImage);
VULKAN_IDLOAD(vkDestroyImage);
VULKAN_IDLOAD(vkGetImageSubresourceLayout);
VULKAN_IDLOAD(vkCreateImageView);
VULKAN_IDLOAD(vkDestroyImageView);
VULKAN_IDLOAD(vkCreateShaderModule);
VULKAN_IDLOAD(vkDestroyShaderModule);
VULKAN_IDLOAD(vkCreatePipelineCache);
VULKAN_IDLOAD(vkDestroyPipelineCache);
VULKAN_IDLOAD(vkGetPipelineCacheData);
VULKAN_IDLOAD(vkMergePipelineCaches);
VULKAN_IDLOAD(vkCreateGraphicsPipelines);
VULKAN_IDLOAD(vkCreateComputePipelines);
VULKAN_IDLOAD(vkDestroyPipeline);
VULKAN_IDLOAD(vkCreatePipelineLayout);
VULKAN_IDLOAD(vkDestroyPipelineLayout);
VULKAN_IDLOAD(vkCreateSampler);
VULKAN_IDLOAD(vkDestroySampler);
VULKAN_IDLOAD(vkCreateDescriptorSetLayout);
VULKAN_IDLOAD(vkDestroyDescriptorSetLayout);
VULKAN_IDLOAD(vkCreateDescriptorPool);
VULKAN_IDLOAD(vkDestroyDescriptorPool);
VULKAN_IDLOAD(vkResetDescriptorPool);
VULKAN_IDLOAD(vkAllocateDescriptorSets);
VULKAN_IDLOAD(vkFreeDescriptorSets);
VULKAN_IDLOAD(vkUpdateDescriptorSets);
VULKAN_IDLOAD(vkCreateFramebuffer);
VULKAN_IDLOAD(vkDestroyFramebuffer);
VULKAN_IDLOAD(vkCreateRenderPass);
VULKAN_IDLOAD(vkDestroyRenderPass);
VULKAN_IDLOAD(vkGetRenderAreaGranularity);
VULKAN_IDLOAD(vkCreateCommandPool);
VULKAN_IDLOAD(vkDestroyCommandPool);
VULKAN_IDLOAD(vkResetCommandPool);
VULKAN_IDLOAD(vkAllocateCommandBuffers);
VULKAN_IDLOAD(vkFreeCommandBuffers);
VULKAN_IDLOAD(vkBeginCommandBuffer);
VULKAN_IDLOAD(vkEndCommandBuffer);
VULKAN_IDLOAD(vkResetCommandBuffer);
VULKAN_IDLOAD(vkCmdBindPipeline);
VULKAN_IDLOAD(vkCmdSetViewport);
VULKAN_IDLOAD(vkCmdSetScissor);
VULKAN_IDLOAD(vkCmdSetLineWidth);
VULKAN_IDLOAD(vkCmdSetDepthBias);
VULKAN_IDLOAD(vkCmdSetBlendConstants);
VULKAN_IDLOAD(vkCmdSetDepthBounds);
VULKAN_IDLOAD(vkCmdSetStencilCompareMask);
VULKAN_IDLOAD(vkCmdSetStencilWriteMask);
VULKAN_IDLOAD(vkCmdSetStencilReference);
VULKAN_IDLOAD(vkCmdBindDescriptorSets);
VULKAN_IDLOAD(vkCmdBindIndexBuffer);
VULKAN_IDLOAD(vkCmdBindVertexBuffers);
VULKAN_IDLOAD(vkCmdDraw);
VULKAN_IDLOAD(vkCmdDrawIndexed);
VULKAN_IDLOAD(vkCmdDrawIndirect);
VULKAN_IDLOAD(vkCmdDrawIndexedIndirect);
VULKAN_IDLOAD(vkCmdDispatch);
VULKAN_IDLOAD(vkCmdDispatchIndirect);
VULKAN_IDLOAD(vkCmdCopyBuffer);
VULKAN_IDLOAD(vkCmdCopyImage);
VULKAN_IDLOAD(vkCmdBlitImage);
VULKAN_IDLOAD(vkCmdCopyBufferToImage);
VULKAN_IDLOAD(vkCmdCopyImageToBuffer);
VULKAN_IDLOAD(vkCmdUpdateBuffer);
VULKAN_IDLOAD(vkCmdFillBuffer);
VULKAN_IDLOAD(vkCmdClearColorImage);
VULKAN_IDLOAD(vkCmdClearDepthStencilImage);
VULKAN_IDLOAD(vkCmdClearAttachments);
VULKAN_IDLOAD(vkCmdResolveImage);
VULKAN_IDLOAD(vkCmdSetEvent);
VULKAN_IDLOAD(vkCmdResetEvent);
VULKAN_IDLOAD(vkCmdWaitEvents);
VULKAN_IDLOAD(vkCmdPipelineBarrier);
VULKAN_IDLOAD(vkCmdBeginQuery);
VULKAN_IDLOAD(vkCmdEndQuery);
VULKAN_IDLOAD(vkCmdResetQueryPool);
VULKAN_IDLOAD(vkCmdWriteTimestamp);
VULKAN_IDLOAD(vkCmdCopyQueryPoolResults);
VULKAN_IDLOAD(vkCmdPushConstants);
VULKAN_IDLOAD(vkCmdBeginRenderPass);
VULKAN_IDLOAD(vkCmdNextSubpass);
VULKAN_IDLOAD(vkCmdEndRenderPass);
VULKAN_IDLOAD(vkCmdExecuteCommands);
VULKAN_IDLOAD(vkEnumerateInstanceVersion);
VULKAN_IDLOAD(vkBindBufferMemory2);
VULKAN_IDLOAD(vkBindImageMemory2);
VULKAN_IDLOAD(vkGetDeviceGroupPeerMemoryFeatures);
VULKAN_IDLOAD(vkCmdSetDeviceMask);
VULKAN_IDLOAD(vkCmdDispatchBase);
VULKAN_IDLOAD(vkEnumeratePhysicalDeviceGroups);
VULKAN_IDLOAD(vkGetImageMemoryRequirements2);
VULKAN_IDLOAD(vkGetBufferMemoryRequirements2);
VULKAN_IDLOAD(vkGetImageSparseMemoryRequirements2);
VULKAN_IDLOAD(vkGetPhysicalDeviceFeatures2);
VULKAN_IDLOAD(vkGetPhysicalDeviceProperties2);
VULKAN_IDLOAD(vkGetPhysicalDeviceFormatProperties2);
VULKAN_IDLOAD(vkGetPhysicalDeviceImageFormatProperties2);
VULKAN_IDLOAD(vkGetPhysicalDeviceQueueFamilyProperties2);
VULKAN_IDLOAD(vkGetPhysicalDeviceMemoryProperties2);
VULKAN_IDLOAD(vkGetPhysicalDeviceSparseImageFormatProperties2);
VULKAN_IDLOAD(vkTrimCommandPool);
VULKAN_IDLOAD(vkGetDeviceQueue2);
VULKAN_IDLOAD(vkCreateSamplerYcbcrConversion);
VULKAN_IDLOAD(vkDestroySamplerYcbcrConversion);
VULKAN_IDLOAD(vkCreateDescriptorUpdateTemplate);
VULKAN_IDLOAD(vkDestroyDescriptorUpdateTemplate);
VULKAN_IDLOAD(vkUpdateDescriptorSetWithTemplate);
VULKAN_IDLOAD(vkGetPhysicalDeviceExternalBufferProperties);
VULKAN_IDLOAD(vkGetPhysicalDeviceExternalFenceProperties);
VULKAN_IDLOAD(vkGetPhysicalDeviceExternalSemaphoreProperties);
VULKAN_IDLOAD(vkGetDescriptorSetLayoutSupport);
VULKAN_IDLOAD(vkCmdDrawIndirectCount);
VULKAN_IDLOAD(vkCmdDrawIndexedIndirectCount);
VULKAN_IDLOAD(vkCreateRenderPass2);
VULKAN_IDLOAD(vkCmdBeginRenderPass2);
VULKAN_IDLOAD(vkCmdNextSubpass2);
VULKAN_IDLOAD(vkCmdEndRenderPass2);
VULKAN_IDLOAD(vkResetQueryPool);
VULKAN_IDLOAD(vkGetSemaphoreCounterValue);
VULKAN_IDLOAD(vkWaitSemaphores);
VULKAN_IDLOAD(vkSignalSemaphore);
VULKAN_IDLOAD(vkGetBufferDeviceAddress);
VULKAN_IDLOAD(vkGetBufferOpaqueCaptureAddress);
VULKAN_IDLOAD(vkGetDeviceMemoryOpaqueCaptureAddress);
#if VK_HEADER_VERSION >= 204
VULKAN_IDLOAD(vkGetPhysicalDeviceToolProperties);
VULKAN_IDLOAD(vkCreatePrivateDataSlot);
VULKAN_IDLOAD(vkDestroyPrivateDataSlot);
VULKAN_IDLOAD(vkSetPrivateData);
VULKAN_IDLOAD(vkGetPrivateData);
VULKAN_IDLOAD(vkCmdSetEvent2);
VULKAN_IDLOAD(vkCmdResetEvent2);
VULKAN_IDLOAD(vkCmdWaitEvents2);
VULKAN_IDLOAD(vkCmdPipelineBarrier2);
VULKAN_IDLOAD(vkCmdWriteTimestamp2);
VULKAN_IDLOAD(vkQueueSubmit2);
VULKAN_IDLOAD(vkCmdCopyBuffer2);
VULKAN_IDLOAD(vkCmdCopyImage2);
VULKAN_IDLOAD(vkCmdCopyBufferToImage2);
VULKAN_IDLOAD(vkCmdCopyImageToBuffer2);
VULKAN_IDLOAD(vkCmdBlitImage2);
VULKAN_IDLOAD(vkCmdResolveImage2);
VULKAN_IDLOAD(vkCmdBeginRendering);
VULKAN_IDLOAD(vkCmdEndRendering);
VULKAN_IDLOAD(vkCmdSetCullMode);
VULKAN_IDLOAD(vkCmdSetFrontFace);
VULKAN_IDLOAD(vkCmdSetPrimitiveTopology);
VULKAN_IDLOAD(vkCmdSetViewportWithCount);
VULKAN_IDLOAD(vkCmdSetScissorWithCount);
VULKAN_IDLOAD(vkCmdBindVertexBuffers2);
VULKAN_IDLOAD(vkCmdSetDepthTestEnable);
VULKAN_IDLOAD(vkCmdSetDepthWriteEnable);
VULKAN_IDLOAD(vkCmdSetDepthCompareOp);
VULKAN_IDLOAD(vkCmdSetDepthBoundsTestEnable);
VULKAN_IDLOAD(vkCmdSetStencilTestEnable);
VULKAN_IDLOAD(vkCmdSetStencilOp);
VULKAN_IDLOAD(vkCmdSetRasterizerDiscardEnable);
VULKAN_IDLOAD(vkCmdSetDepthBiasEnable);
VULKAN_IDLOAD(vkCmdSetPrimitiveRestartEnable);
VULKAN_IDLOAD(vkGetDeviceBufferMemoryRequirements);
VULKAN_IDLOAD(vkGetDeviceImageMemoryRequirements);
VULKAN_IDLOAD(vkGetDeviceImageSparseMemoryRequirements);
#endif
VULKAN_IDLOAD(vkGetPhysicalDeviceSurfaceSupportKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceSurfaceFormatsKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceSurfacePresentModesKHR);
VULKAN_IDLOAD(vkCreateSwapchainKHR);
VULKAN_IDLOAD(vkDestroySwapchainKHR);
VULKAN_IDLOAD(vkGetSwapchainImagesKHR);
VULKAN_IDLOAD(vkAcquireNextImageKHR);
VULKAN_IDLOAD(vkQueuePresentKHR);
VULKAN_IDLOAD(vkGetDeviceGroupPresentCapabilitiesKHR);
VULKAN_IDLOAD(vkGetDeviceGroupSurfacePresentModesKHR);
VULKAN_IDLOAD(vkGetPhysicalDevicePresentRectanglesKHR);
VULKAN_IDLOAD(vkAcquireNextImage2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceDisplayPropertiesKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceDisplayPlanePropertiesKHR);
VULKAN_IDLOAD(vkGetDisplayPlaneSupportedDisplaysKHR);
VULKAN_IDLOAD(vkGetDisplayModePropertiesKHR);
VULKAN_IDLOAD(vkCreateDisplayModeKHR);
VULKAN_IDLOAD(vkGetDisplayPlaneCapabilitiesKHR);
VULKAN_IDLOAD(vkCreateDisplayPlaneSurfaceKHR);
VULKAN_IDLOAD(vkCreateSharedSwapchainsKHR);
#if VK_HEADER_VERSION >= 238
VULKAN_IDLOAD(vkGetPhysicalDeviceVideoCapabilitiesKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceVideoFormatPropertiesKHR);
VULKAN_IDLOAD(vkCreateVideoSessionKHR);
VULKAN_IDLOAD(vkDestroyVideoSessionKHR);
VULKAN_IDLOAD(vkGetVideoSessionMemoryRequirementsKHR);
VULKAN_IDLOAD(vkBindVideoSessionMemoryKHR);
VULKAN_IDLOAD(vkCreateVideoSessionParametersKHR);
VULKAN_IDLOAD(vkUpdateVideoSessionParametersKHR);
VULKAN_IDLOAD(vkDestroyVideoSessionParametersKHR);
VULKAN_IDLOAD(vkCmdBeginVideoCodingKHR);
VULKAN_IDLOAD(vkCmdEndVideoCodingKHR);
VULKAN_IDLOAD(vkCmdControlVideoCodingKHR);
VULKAN_IDLOAD(vkCmdDecodeVideoKHR);
#endif
#if VK_HEADER_VERSION >= 197
VULKAN_IDLOAD(vkCmdBeginRenderingKHR);
VULKAN_IDLOAD(vkCmdEndRenderingKHR);
#endif
VULKAN_IDLOAD(vkGetPhysicalDeviceFeatures2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceProperties2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceFormatProperties2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceImageFormatProperties2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceQueueFamilyProperties2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceMemoryProperties2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceSparseImageFormatProperties2KHR);
VULKAN_IDLOAD(vkGetDeviceGroupPeerMemoryFeaturesKHR);
VULKAN_IDLOAD(vkCmdSetDeviceMaskKHR);
VULKAN_IDLOAD(vkCmdDispatchBaseKHR);
VULKAN_IDLOAD(vkTrimCommandPoolKHR);
VULKAN_IDLOAD(vkEnumeratePhysicalDeviceGroupsKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceExternalBufferPropertiesKHR);
VULKAN_IDLOAD(vkGetMemoryFdKHR);
VULKAN_IDLOAD(vkGetMemoryFdPropertiesKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceExternalSemaphorePropertiesKHR);
VULKAN_IDLOAD(vkImportSemaphoreFdKHR);
VULKAN_IDLOAD(vkGetSemaphoreFdKHR);
VULKAN_IDLOAD(vkCmdPushDescriptorSetKHR);
VULKAN_IDLOAD(vkCmdPushDescriptorSetWithTemplateKHR);
VULKAN_IDLOAD(vkCreateDescriptorUpdateTemplateKHR);
VULKAN_IDLOAD(vkDestroyDescriptorUpdateTemplateKHR);
VULKAN_IDLOAD(vkUpdateDescriptorSetWithTemplateKHR);
VULKAN_IDLOAD(vkCreateRenderPass2KHR);
VULKAN_IDLOAD(vkCmdBeginRenderPass2KHR);
VULKAN_IDLOAD(vkCmdNextSubpass2KHR);
VULKAN_IDLOAD(vkCmdEndRenderPass2KHR);
VULKAN_IDLOAD(vkGetSwapchainStatusKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceExternalFencePropertiesKHR);
VULKAN_IDLOAD(vkImportFenceFdKHR);
VULKAN_IDLOAD(vkGetFenceFdKHR);
VULKAN_IDLOAD(vkEnumeratePhysicalDeviceQueueFamilyPerformanceQueryCountersKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceQueueFamilyPerformanceQueryPassesKHR);
VULKAN_IDLOAD(vkAcquireProfilingLockKHR);
VULKAN_IDLOAD(vkReleaseProfilingLockKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceSurfaceCapabilities2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceSurfaceFormats2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceDisplayProperties2KHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceDisplayPlaneProperties2KHR);
VULKAN_IDLOAD(vkGetDisplayModeProperties2KHR);
VULKAN_IDLOAD(vkGetDisplayPlaneCapabilities2KHR);
VULKAN_IDLOAD(vkGetImageMemoryRequirements2KHR);
VULKAN_IDLOAD(vkGetBufferMemoryRequirements2KHR);
VULKAN_IDLOAD(vkGetImageSparseMemoryRequirements2KHR);
VULKAN_IDLOAD(vkCreateSamplerYcbcrConversionKHR);
VULKAN_IDLOAD(vkDestroySamplerYcbcrConversionKHR);
VULKAN_IDLOAD(vkBindBufferMemory2KHR);
VULKAN_IDLOAD(vkBindImageMemory2KHR);
VULKAN_IDLOAD(vkGetDescriptorSetLayoutSupportKHR);
VULKAN_IDLOAD(vkCmdDrawIndirectCountKHR);
VULKAN_IDLOAD(vkCmdDrawIndexedIndirectCountKHR);
VULKAN_IDLOAD(vkGetSemaphoreCounterValueKHR);
VULKAN_IDLOAD(vkWaitSemaphoresKHR);
VULKAN_IDLOAD(vkSignalSemaphoreKHR);
VULKAN_IDLOAD(vkGetPhysicalDeviceFragmentShadingRatesKHR);
VULKAN_IDLOAD(vkCmdSetFragmentShadingRateKHR);
#if VK_HEADER_VERSION >= 276
VULKAN_IDLOAD(vkCmdSetRenderingAttachmentLocationsKHR);
VULKAN_IDLOAD(vkCmdSetRenderingInputAttachmentIndicesKHR);
#endif
#if VK_HEADER_VERSION >= 185
VULKAN_IDLOAD(vkWaitForPresentKHR);
#endif
VULKAN_IDLOAD(vkGetBufferDeviceAddressKHR);
VULKAN_IDLOAD(vkGetBufferOpaqueCaptureAddressKHR);
VULKAN_IDLOAD(vkGetDeviceMemoryOpaqueCaptureAddressKHR);
VULKAN_IDLOAD(vkCreateDeferredOperationKHR);
VULKAN_IDLOAD(vkDestroyDeferredOperationKHR);
VULKAN_IDLOAD(vkGetDeferredOperationMaxConcurrencyKHR);
VULKAN_IDLOAD(vkGetDeferredOperationResultKHR);
VULKAN_IDLOAD(vkDeferredOperationJoinKHR);
VULKAN_IDLOAD(vkGetPipelineExecutablePropertiesKHR);
VULKAN_IDLOAD(vkGetPipelineExecutableStatisticsKHR);
VULKAN_IDLOAD(vkGetPipelineExecutableInternalRepresentationsKHR);
#if VK_HEADER_VERSION >= 244
VULKAN_IDLOAD(vkMapMemory2KHR);
VULKAN_IDLOAD(vkUnmapMemory2KHR);
#endif
#if VK_HEADER_VERSION >= 274
VULKAN_IDLOAD(vkGetPhysicalDeviceVideoEncodeQualityLevelPropertiesKHR);
VULKAN_IDLOAD(vkGetEncodedVideoSessionParametersKHR);
VULKAN_IDLOAD(vkCmdEncodeVideoKHR);
#endif
VULKAN_IDLOAD(vkCmdSetEvent2KHR);
VULKAN_IDLOAD(vkCmdResetEvent2KHR);
VULKAN_IDLOAD(vkCmdWaitEvents2KHR);
VULKAN_IDLOAD(vkCmdPipelineBarrier2KHR);
VULKAN_IDLOAD(vkCmdWriteTimestamp2KHR);
VULKAN_IDLOAD(vkQueueSubmit2KHR);
VULKAN_IDLOAD(vkCmdWriteBufferMarker2AMD);
VULKAN_IDLOAD(vkGetQueueCheckpointData2NV);
VULKAN_IDLOAD(vkCmdCopyBuffer2KHR);
VULKAN_IDLOAD(vkCmdCopyImage2KHR);
VULKAN_IDLOAD(vkCmdCopyBufferToImage2KHR);
VULKAN_IDLOAD(vkCmdCopyImageToBuffer2KHR);
VULKAN_IDLOAD(vkCmdBlitImage2KHR);
VULKAN_IDLOAD(vkCmdResolveImage2KHR);
#if VK_HEADER_VERSION >= 213
VULKAN_IDLOAD(vkCmdTraceRaysIndirect2KHR);
#endif
#if VK_HEADER_VERSION >= 195
VULKAN_IDLOAD(vkGetDeviceBufferMemoryRequirementsKHR);
VULKAN_IDLOAD(vkGetDeviceImageMemoryRequirementsKHR);
VULKAN_IDLOAD(vkGetDeviceImageSparseMemoryRequirementsKHR);
#endif
#if VK_HEADER_VERSION >= 260
VULKAN_IDLOAD(vkCmdBindIndexBuffer2KHR);
VULKAN_IDLOAD(vkGetRenderingAreaGranularityKHR);
VULKAN_IDLOAD(vkGetDeviceImageSubresourceLayoutKHR);
VULKAN_IDLOAD(vkGetImageSubresourceLayout2KHR);
#endif
#if VK_HEADER_VERSION >= 255
VULKAN_IDLOAD(vkGetPhysicalDeviceCooperativeMatrixPropertiesKHR);
#endif
#if VK_HEADER_VERSION >= 276
VULKAN_IDLOAD(vkCmdSetLineStippleKHR);
#endif
#if VK_HEADER_VERSION >= 273
VULKAN_IDLOAD(vkGetPhysicalDeviceCalibrateableTimeDomainsKHR);
VULKAN_IDLOAD(vkGetCalibratedTimestampsKHR);
#endif
#if VK_HEADER_VERSION >= 274
VULKAN_IDLOAD(vkCmdBindDescriptorSets2KHR);
VULKAN_IDLOAD(vkCmdPushConstants2KHR);
VULKAN_IDLOAD(vkCmdPushDescriptorSet2KHR);
VULKAN_IDLOAD(vkCmdPushDescriptorSetWithTemplate2KHR);
VULKAN_IDLOAD(vkCmdSetDescriptorBufferOffsets2EXT);
VULKAN_IDLOAD(vkCmdBindDescriptorBufferEmbeddedSamplers2EXT);
#endif
VULKAN_IDLOAD(vkCreateDebugReportCallbackEXT);
VULKAN_IDLOAD(vkDestroyDebugReportCallbackEXT);
VULKAN_IDLOAD(vkDebugReportMessageEXT);
VULKAN_IDLOAD(vkDebugMarkerSetObjectTagEXT);
VULKAN_IDLOAD(vkDebugMarkerSetObjectNameEXT);
VULKAN_IDLOAD(vkCmdDebugMarkerBeginEXT);
VULKAN_IDLOAD(vkCmdDebugMarkerEndEXT);
VULKAN_IDLOAD(vkCmdDebugMarkerInsertEXT);
VULKAN_IDLOAD(vkCmdBindTransformFeedbackBuffersEXT);
VULKAN_IDLOAD(vkCmdBeginTransformFeedbackEXT);
VULKAN_IDLOAD(vkCmdEndTransformFeedbackEXT);
VULKAN_IDLOAD(vkCmdBeginQueryIndexedEXT);
VULKAN_IDLOAD(vkCmdEndQueryIndexedEXT);
VULKAN_IDLOAD(vkCmdDrawIndirectByteCountEXT);
VULKAN_IDLOAD(vkCreateCuModuleNVX);
VULKAN_IDLOAD(vkCreateCuFunctionNVX);
VULKAN_IDLOAD(vkDestroyCuModuleNVX);
VULKAN_IDLOAD(vkDestroyCuFunctionNVX);
VULKAN_IDLOAD(vkCmdCuLaunchKernelNVX);
VULKAN_IDLOAD(vkGetImageViewHandleNVX);
VULKAN_IDLOAD(vkGetImageViewAddressNVX);
VULKAN_IDLOAD(vkCmdDrawIndirectCountAMD);
VULKAN_IDLOAD(vkCmdDrawIndexedIndirectCountAMD);
VULKAN_IDLOAD(vkGetShaderInfoAMD);
VULKAN_IDLOAD(vkGetPhysicalDeviceExternalImageFormatPropertiesNV);
VULKAN_IDLOAD(vkCmdBeginConditionalRenderingEXT);
VULKAN_IDLOAD(vkCmdEndConditionalRenderingEXT);
VULKAN_IDLOAD(vkCmdSetViewportWScalingNV);
VULKAN_IDLOAD(vkReleaseDisplayEXT);
VULKAN_IDLOAD(vkGetPhysicalDeviceSurfaceCapabilities2EXT);
VULKAN_IDLOAD(vkDisplayPowerControlEXT);
VULKAN_IDLOAD(vkRegisterDeviceEventEXT);
VULKAN_IDLOAD(vkRegisterDisplayEventEXT);
VULKAN_IDLOAD(vkGetSwapchainCounterEXT);
VULKAN_IDLOAD(vkGetRefreshCycleDurationGOOGLE);
VULKAN_IDLOAD(vkGetPastPresentationTimingGOOGLE);
VULKAN_IDLOAD(vkCmdSetDiscardRectangleEXT);
#if VK_HEADER_VERSION >= 241
VULKAN_IDLOAD(vkCmdSetDiscardRectangleEnableEXT);
VULKAN_IDLOAD(vkCmdSetDiscardRectangleModeEXT);
#endif
VULKAN_IDLOAD(vkSetHdrMetadataEXT);
VULKAN_IDLOAD(vkSetDebugUtilsObjectNameEXT);
VULKAN_IDLOAD(vkSetDebugUtilsObjectTagEXT);
VULKAN_IDLOAD(vkQueueBeginDebugUtilsLabelEXT);
VULKAN_IDLOAD(vkQueueEndDebugUtilsLabelEXT);
VULKAN_IDLOAD(vkQueueInsertDebugUtilsLabelEXT);
VULKAN_IDLOAD(vkCmdBeginDebugUtilsLabelEXT);
VULKAN_IDLOAD(vkCmdEndDebugUtilsLabelEXT);
VULKAN_IDLOAD(vkCmdInsertDebugUtilsLabelEXT);
VULKAN_IDLOAD(vkCreateDebugUtilsMessengerEXT);
VULKAN_IDLOAD(vkDestroyDebugUtilsMessengerEXT);
VULKAN_IDLOAD(vkSubmitDebugUtilsMessageEXT);
VULKAN_IDLOAD(vkCmdSetSampleLocationsEXT);
VULKAN_IDLOAD(vkGetPhysicalDeviceMultisamplePropertiesEXT);
VULKAN_IDLOAD(vkGetImageDrmFormatModifierPropertiesEXT);
VULKAN_IDLOAD(vkCreateValidationCacheEXT);
VULKAN_IDLOAD(vkDestroyValidationCacheEXT);
VULKAN_IDLOAD(vkMergeValidationCachesEXT);
VULKAN_IDLOAD(vkGetValidationCacheDataEXT);
VULKAN_IDLOAD(vkCmdBindShadingRateImageNV);
VULKAN_IDLOAD(vkCmdSetViewportShadingRatePaletteNV);
VULKAN_IDLOAD(vkCmdSetCoarseSampleOrderNV);
VULKAN_IDLOAD(vkCreateAccelerationStructureNV);
VULKAN_IDLOAD(vkDestroyAccelerationStructureNV);
VULKAN_IDLOAD(vkGetAccelerationStructureMemoryRequirementsNV);
VULKAN_IDLOAD(vkBindAccelerationStructureMemoryNV);
VULKAN_IDLOAD(vkCmdBuildAccelerationStructureNV);
VULKAN_IDLOAD(vkCmdCopyAccelerationStructureNV);
VULKAN_IDLOAD(vkCmdTraceRaysNV);
VULKAN_IDLOAD(vkCreateRayTracingPipelinesNV);
VULKAN_IDLOAD(vkGetRayTracingShaderGroupHandlesKHR);
VULKAN_IDLOAD(vkGetRayTracingShaderGroupHandlesNV);
VULKAN_IDLOAD(vkGetAccelerationStructureHandleNV);
VULKAN_IDLOAD(vkCmdWriteAccelerationStructuresPropertiesNV);
VULKAN_IDLOAD(vkCompileDeferredNV);
VULKAN_IDLOAD(vkGetMemoryHostPointerPropertiesEXT);
VULKAN_IDLOAD(vkCmdWriteBufferMarkerAMD);
VULKAN_IDLOAD(vkGetPhysicalDeviceCalibrateableTimeDomainsEXT);
VULKAN_IDLOAD(vkGetCalibratedTimestampsEXT);
VULKAN_IDLOAD(vkCmdDrawMeshTasksNV);
VULKAN_IDLOAD(vkCmdDrawMeshTasksIndirectNV);
VULKAN_IDLOAD(vkCmdDrawMeshTasksIndirectCountNV);
#if VK_HEADER_VERSION >= 241
VULKAN_IDLOAD(vkCmdSetExclusiveScissorEnableNV);
#endif
VULKAN_IDLOAD(vkCmdSetExclusiveScissorNV);
VULKAN_IDLOAD(vkCmdSetCheckpointNV);
VULKAN_IDLOAD(vkGetQueueCheckpointDataNV);
VULKAN_IDLOAD(vkInitializePerformanceApiINTEL);
VULKAN_IDLOAD(vkUninitializePerformanceApiINTEL);
VULKAN_IDLOAD(vkCmdSetPerformanceMarkerINTEL);
VULKAN_IDLOAD(vkCmdSetPerformanceStreamMarkerINTEL);
VULKAN_IDLOAD(vkCmdSetPerformanceOverrideINTEL);
VULKAN_IDLOAD(vkAcquirePerformanceConfigurationINTEL);
VULKAN_IDLOAD(vkReleasePerformanceConfigurationINTEL);
VULKAN_IDLOAD(vkQueueSetPerformanceConfigurationINTEL);
VULKAN_IDLOAD(vkGetPerformanceParameterINTEL);
VULKAN_IDLOAD(vkSetLocalDimmingAMD);
VULKAN_IDLOAD(vkGetBufferDeviceAddressEXT);
VULKAN_IDLOAD(vkGetPhysicalDeviceToolPropertiesEXT);
VULKAN_IDLOAD(vkGetPhysicalDeviceCooperativeMatrixPropertiesNV);
VULKAN_IDLOAD(vkGetPhysicalDeviceSupportedFramebufferMixedSamplesCombinationsNV);
VULKAN_IDLOAD(vkCreateHeadlessSurfaceEXT);
VULKAN_IDLOAD(vkCmdSetLineStippleEXT);
VULKAN_IDLOAD(vkResetQueryPoolEXT);
VULKAN_IDLOAD(vkCmdSetCullModeEXT);
VULKAN_IDLOAD(vkCmdSetFrontFaceEXT);
VULKAN_IDLOAD(vkCmdSetPrimitiveTopologyEXT);
VULKAN_IDLOAD(vkCmdSetViewportWithCountEXT);
VULKAN_IDLOAD(vkCmdSetScissorWithCountEXT);
VULKAN_IDLOAD(vkCmdBindVertexBuffers2EXT);
VULKAN_IDLOAD(vkCmdSetDepthTestEnableEXT);
VULKAN_IDLOAD(vkCmdSetDepthWriteEnableEXT);
VULKAN_IDLOAD(vkCmdSetDepthCompareOpEXT);
VULKAN_IDLOAD(vkCmdSetDepthBoundsTestEnableEXT);
VULKAN_IDLOAD(vkCmdSetStencilTestEnableEXT);
VULKAN_IDLOAD(vkCmdSetStencilOpEXT);
#if VK_HEADER_VERSION >= 258
VULKAN_IDLOAD(vkCopyMemoryToImageEXT);
VULKAN_IDLOAD(vkCopyImageToMemoryEXT);
VULKAN_IDLOAD(vkCopyImageToImageEXT);
VULKAN_IDLOAD(vkTransitionImageLayoutEXT);
#endif
#if VK_HEADER_VERSION >= 213
VULKAN_IDLOAD(vkGetImageSubresourceLayout2EXT);
#endif
#if VK_HEADER_VERSION >= 237
VULKAN_IDLOAD(vkReleaseSwapchainImagesEXT);
#endif
VULKAN_IDLOAD(vkGetGeneratedCommandsMemoryRequirementsNV);
VULKAN_IDLOAD(vkCmdPreprocessGeneratedCommandsNV);
VULKAN_IDLOAD(vkCmdExecuteGeneratedCommandsNV);
VULKAN_IDLOAD(vkCmdBindPipelineShaderGroupNV);
VULKAN_IDLOAD(vkCreateIndirectCommandsLayoutNV);
VULKAN_IDLOAD(vkDestroyIndirectCommandsLayoutNV);
#if VK_HEADER_VERSION >= 254
VULKAN_IDLOAD(vkCmdSetDepthBias2EXT);
#endif
VULKAN_IDLOAD(vkAcquireDrmDisplayEXT);
VULKAN_IDLOAD(vkGetDrmDisplayEXT);
VULKAN_IDLOAD(vkCreatePrivateDataSlotEXT);
VULKAN_IDLOAD(vkDestroyPrivateDataSlotEXT);
VULKAN_IDLOAD(vkSetPrivateDataEXT);
VULKAN_IDLOAD(vkGetPrivateDataEXT);
#if VK_HEADER_VERSION >= 269
VULKAN_IDLOAD(vkCreateCudaModuleNV);
VULKAN_IDLOAD(vkGetCudaModuleCacheNV);
VULKAN_IDLOAD(vkCreateCudaFunctionNV);
VULKAN_IDLOAD(vkDestroyCudaModuleNV);
VULKAN_IDLOAD(vkDestroyCudaFunctionNV);
VULKAN_IDLOAD(vkCmdCudaLaunchKernelNV);
#endif
#if VK_HEADER_VERSION >= 235
VULKAN_IDLOAD(vkGetDescriptorSetLayoutSizeEXT);
VULKAN_IDLOAD(vkGetDescriptorSetLayoutBindingOffsetEXT);
VULKAN_IDLOAD(vkGetDescriptorEXT);
VULKAN_IDLOAD(vkCmdBindDescriptorBuffersEXT);
VULKAN_IDLOAD(vkCmdSetDescriptorBufferOffsetsEXT);
VULKAN_IDLOAD(vkCmdBindDescriptorBufferEmbeddedSamplersEXT);
VULKAN_IDLOAD(vkGetBufferOpaqueCaptureDescriptorDataEXT);
VULKAN_IDLOAD(vkGetImageOpaqueCaptureDescriptorDataEXT);
VULKAN_IDLOAD(vkGetImageViewOpaqueCaptureDescriptorDataEXT);
VULKAN_IDLOAD(vkGetSamplerOpaqueCaptureDescriptorDataEXT);
VULKAN_IDLOAD(vkGetAccelerationStructureOpaqueCaptureDescriptorDataEXT);
#endif
VULKAN_IDLOAD(vkCmdSetFragmentShadingRateEnumNV);
#if VK_HEADER_VERSION >= 230
VULKAN_IDLOAD(vkGetDeviceFaultInfoEXT);
#endif
VULKAN_IDLOAD(vkCmdSetVertexInputEXT);
#if VK_HEADER_VERSION >= 184
VULKAN_IDLOAD(vkGetDeviceSubpassShadingMaxWorkgroupSizeHUAWEI);
#else
VULKAN_IDLOAD(vkGetSubpassShadingMaxWorkgroupSizeHUAWEI);
#endif
VULKAN_IDLOAD(vkCmdSubpassShadingHUAWEI);
#if VK_HEADER_VERSION >= 185
VULKAN_IDLOAD(vkCmdBindInvocationMaskHUAWEI);
#endif
#if VK_HEADER_VERSION >= 184
VULKAN_IDLOAD(vkGetMemoryRemoteAddressNV);
#endif
#if VK_HEADER_VERSION >= 213
VULKAN_IDLOAD(vkGetPipelinePropertiesEXT);
#endif
VULKAN_IDLOAD(vkCmdSetPatchControlPointsEXT);
VULKAN_IDLOAD(vkCmdSetRasterizerDiscardEnableEXT);
VULKAN_IDLOAD(vkCmdSetDepthBiasEnableEXT);
VULKAN_IDLOAD(vkCmdSetLogicOpEXT);
VULKAN_IDLOAD(vkCmdSetPrimitiveRestartEnableEXT);
VULKAN_IDLOAD(vkCmdSetColorWriteEnableEXT);
VULKAN_IDLOAD(vkCmdDrawMultiEXT);
VULKAN_IDLOAD(vkCmdDrawMultiIndexedEXT);
#if VK_HEADER_VERSION >= 230
VULKAN_IDLOAD(vkCreateMicromapEXT);
VULKAN_IDLOAD(vkDestroyMicromapEXT);
VULKAN_IDLOAD(vkCmdBuildMicromapsEXT);
VULKAN_IDLOAD(vkBuildMicromapsEXT);
VULKAN_IDLOAD(vkCopyMicromapEXT);
VULKAN_IDLOAD(vkCopyMicromapToMemoryEXT);
VULKAN_IDLOAD(vkCopyMemoryToMicromapEXT);
VULKAN_IDLOAD(vkWriteMicromapsPropertiesEXT);
VULKAN_IDLOAD(vkCmdCopyMicromapEXT);
VULKAN_IDLOAD(vkCmdCopyMicromapToMemoryEXT);
VULKAN_IDLOAD(vkCmdCopyMemoryToMicromapEXT);
VULKAN_IDLOAD(vkCmdWriteMicromapsPropertiesEXT);
VULKAN_IDLOAD(vkGetDeviceMicromapCompatibilityEXT);
VULKAN_IDLOAD(vkGetMicromapBuildSizesEXT);
#endif
#if VK_HEADER_VERSION >= 239
VULKAN_IDLOAD(vkCmdDrawClusterHUAWEI);
VULKAN_IDLOAD(vkCmdDrawClusterIndirectHUAWEI);
#endif
#if VK_HEADER_VERSION >= 191
VULKAN_IDLOAD(vkSetDeviceMemoryPriorityEXT);
#endif
#if VK_HEADER_VERSION >= 207
VULKAN_IDLOAD(vkGetDescriptorSetLayoutHostMappingInfoVALVE);
VULKAN_IDLOAD(vkGetDescriptorSetHostMappingVALVE);
#endif
#if VK_HEADER_VERSION >= 233
VULKAN_IDLOAD(vkCmdCopyMemoryIndirectNV);
VULKAN_IDLOAD(vkCmdCopyMemoryToImageIndirectNV);
VULKAN_IDLOAD(vkCmdDecompressMemoryNV);
VULKAN_IDLOAD(vkCmdDecompressMemoryIndirectCountNV);
#endif
#if VK_HEADER_VERSION >= 258
VULKAN_IDLOAD(vkGetPipelineIndirectMemoryRequirementsNV);
#endif
#if VK_HEADER_VERSION == 258
VULKAN_IDLOAD(vkCmdUpdatePipelineIndirectBuffer);
#endif
#if VK_HEADER_VERSION >= 259
VULKAN_IDLOAD(vkCmdUpdatePipelineIndirectBufferNV);
#endif
#if VK_HEADER_VERSION >= 258
VULKAN_IDLOAD(vkGetPipelineIndirectDeviceAddressNV);
#endif
#if VK_HEADER_VERSION >= 230
VULKAN_IDLOAD(vkCmdSetDepthClampEnableEXT);
VULKAN_IDLOAD(vkCmdSetPolygonModeEXT);
VULKAN_IDLOAD(vkCmdSetRasterizationSamplesEXT);
VULKAN_IDLOAD(vkCmdSetSampleMaskEXT);
VULKAN_IDLOAD(vkCmdSetAlphaToCoverageEnableEXT);
VULKAN_IDLOAD(vkCmdSetAlphaToOneEnableEXT);
VULKAN_IDLOAD(vkCmdSetLogicOpEnableEXT);
VULKAN_IDLOAD(vkCmdSetColorBlendEnableEXT);
VULKAN_IDLOAD(vkCmdSetColorBlendEquationEXT);
VULKAN_IDLOAD(vkCmdSetColorWriteMaskEXT);
VULKAN_IDLOAD(vkCmdSetTessellationDomainOriginEXT);
VULKAN_IDLOAD(vkCmdSetRasterizationStreamEXT);
VULKAN_IDLOAD(vkCmdSetConservativeRasterizationModeEXT);
VULKAN_IDLOAD(vkCmdSetExtraPrimitiveOverestimationSizeEXT);
VULKAN_IDLOAD(vkCmdSetDepthClipEnableEXT);
VULKAN_IDLOAD(vkCmdSetSampleLocationsEnableEXT);
VULKAN_IDLOAD(vkCmdSetColorBlendAdvancedEXT);
VULKAN_IDLOAD(vkCmdSetProvokingVertexModeEXT);
VULKAN_IDLOAD(vkCmdSetLineRasterizationModeEXT);
VULKAN_IDLOAD(vkCmdSetLineStippleEnableEXT);
VULKAN_IDLOAD(vkCmdSetDepthClipNegativeOneToOneEXT);
VULKAN_IDLOAD(vkCmdSetViewportWScalingEnableNV);
VULKAN_IDLOAD(vkCmdSetViewportSwizzleNV);
VULKAN_IDLOAD(vkCmdSetCoverageToColorEnableNV);
VULKAN_IDLOAD(vkCmdSetCoverageToColorLocationNV);
VULKAN_IDLOAD(vkCmdSetCoverageModulationModeNV);
VULKAN_IDLOAD(vkCmdSetCoverageModulationTableEnableNV);
VULKAN_IDLOAD(vkCmdSetCoverageModulationTableNV);
VULKAN_IDLOAD(vkCmdSetShadingRateImageEnableNV);
VULKAN_IDLOAD(vkCmdSetRepresentativeFragmentTestEnableNV);
VULKAN_IDLOAD(vkCmdSetCoverageReductionModeNV);
#endif
#if VK_HEADER_VERSION >= 219
VULKAN_IDLOAD(vkGetShaderModuleIdentifierEXT);
VULKAN_IDLOAD(vkGetShaderModuleCreateInfoIdentifierEXT);
#endif
#if VK_HEADER_VERSION >= 230
VULKAN_IDLOAD(vkGetPhysicalDeviceOpticalFlowImageFormatsNV);
VULKAN_IDLOAD(vkCreateOpticalFlowSessionNV);
VULKAN_IDLOAD(vkDestroyOpticalFlowSessionNV);
VULKAN_IDLOAD(vkBindOpticalFlowSessionImageNV);
VULKAN_IDLOAD(vkCmdOpticalFlowExecuteNV);
#endif
#if VK_HEADER_VERSION >= 246
VULKAN_IDLOAD(vkCreateShadersEXT);
VULKAN_IDLOAD(vkDestroyShaderEXT);
VULKAN_IDLOAD(vkGetShaderBinaryDataEXT);
VULKAN_IDLOAD(vkCmdBindShadersEXT);
#endif
#if VK_HEADER_VERSION >= 222
VULKAN_IDLOAD(vkGetFramebufferTilePropertiesQCOM);
VULKAN_IDLOAD(vkGetDynamicRenderingTilePropertiesQCOM);
#endif
#if VK_HEADER_VERSION >= 266
VULKAN_IDLOAD(vkSetLatencySleepModeNV);
VULKAN_IDLOAD(vkLatencySleepNV);
VULKAN_IDLOAD(vkSetLatencyMarkerNV);
VULKAN_IDLOAD(vkGetLatencyTimingsNV);
VULKAN_IDLOAD(vkQueueNotifyOutOfBandNV);
#endif
#if VK_HEADER_VERSION >= 250
VULKAN_IDLOAD(vkCmdSetAttachmentFeedbackLoopEnableEXT);
#endif
VULKAN_IDLOAD(vkCreateAccelerationStructureKHR);
VULKAN_IDLOAD(vkDestroyAccelerationStructureKHR);
VULKAN_IDLOAD(vkCmdBuildAccelerationStructuresKHR);
VULKAN_IDLOAD(vkCmdBuildAccelerationStructuresIndirectKHR);
VULKAN_IDLOAD(vkBuildAccelerationStructuresKHR);
VULKAN_IDLOAD(vkCopyAccelerationStructureKHR);
VULKAN_IDLOAD(vkCopyAccelerationStructureToMemoryKHR);
VULKAN_IDLOAD(vkCopyMemoryToAccelerationStructureKHR);
VULKAN_IDLOAD(vkWriteAccelerationStructuresPropertiesKHR);
VULKAN_IDLOAD(vkCmdCopyAccelerationStructureKHR);
VULKAN_IDLOAD(vkCmdCopyAccelerationStructureToMemoryKHR);
VULKAN_IDLOAD(vkCmdCopyMemoryToAccelerationStructureKHR);
VULKAN_IDLOAD(vkGetAccelerationStructureDeviceAddressKHR);
VULKAN_IDLOAD(vkCmdWriteAccelerationStructuresPropertiesKHR);
VULKAN_IDLOAD(vkGetDeviceAccelerationStructureCompatibilityKHR);
VULKAN_IDLOAD(vkGetAccelerationStructureBuildSizesKHR);
VULKAN_IDLOAD(vkCmdTraceRaysKHR);
VULKAN_IDLOAD(vkCreateRayTracingPipelinesKHR);
VULKAN_IDLOAD(vkGetRayTracingCaptureReplayShaderGroupHandlesKHR);
VULKAN_IDLOAD(vkCmdTraceRaysIndirectKHR);
VULKAN_IDLOAD(vkGetRayTracingShaderGroupStackSizeKHR);
VULKAN_IDLOAD(vkCmdSetRayTracingPipelineStackSizeKHR);
#if VK_HEADER_VERSION >= 226
VULKAN_IDLOAD(vkCmdDrawMeshTasksEXT);
VULKAN_IDLOAD(vkCmdDrawMeshTasksIndirectEXT);
VULKAN_IDLOAD(vkCmdDrawMeshTasksIndirectCountEXT);
#endif

// vim:ts=4:sw=4:noexpandtab
