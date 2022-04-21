/*
 * test_vulkan: Test Vulkan implementation
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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-server.h>

#define VK_USE_PLATFORM_WAYLAND_KHR 1
#include <vulkan/vulkan.h>

VkDevice device;
VkPhysicalDevice usedPhysicalDevice;
VkQueue graphicsQueue;
VkQueue presentQueue;
VkSwapchainKHR swapchain = VK_NULL_HANDLE;
VkImage *swapchainImages = NULL;
uint32_t imageCount;
VkFormat swapchainImageFormat;
VkCommandPool commandPool;
VkCommandBuffer *commandBuffers = NULL;

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence *fences = NULL;
uint32_t graphicsFamily = 0;
uint32_t presentFamily = 0;
VkSurfaceKHR surface = VK_NULL_HANDLE;

struct SwapchainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct wl_data {
    struct wl_display *wldisplay = NULL;
    struct wl_compositor *wlcompositor = NULL;
    struct wl_surface *wlsurface;
    struct wl_region *wlregion;
    struct wl_shell *wlshell;
    struct wl_shell_surface *wlshell_surface;
    struct wl_output *wloutput;
    uint32_t width;
    uint32_t height;
} wl_data;

static void handle_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
}

static void handle_configure(void *data, struct wl_shell_surface *shell_surface,
                             uint32_t edges, int32_t width, int32_t height)
{
}

static void handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done
};

static void
display_handle_geometry(void *data, struct wl_output *output, int x, int y,
                        int physical_width, int physical_height, int subpixel,
                        const char *make, const char *model, int transform)
{
}

static void
display_handle_mode(void *data, struct wl_output *output, uint32_t flags,
                    int width, int height, int refresh)
{
    struct wl_data *driverdata = (struct wl_data *)data;
    driverdata->width = width;
    driverdata->height = height;
}

static void
display_handle_done(void *data, struct wl_output *output)
{
}

static void
display_handle_scale(void *data, struct wl_output *output, int32_t factor)
{
}

static const struct wl_output_listener output_listener = {
    display_handle_geometry,
    display_handle_mode,
    display_handle_done,
    display_handle_scale
};

static void global_registry_handler(void *data, struct wl_registry *registry,
                                    uint32_t id, const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        wl_data.wlcompositor = (wl_compositor *)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        wl_data.wlshell = (wl_shell *)wl_registry_bind(registry, id, &wl_shell_interface, 1);
    } else if (strcmp(interface, "wl_output") == 0) {
        wl_data.wloutput = (wl_output *)wl_registry_bind(registry, id, &wl_output_interface, 2);
        wl_output_add_listener(wl_data.wloutput, &output_listener, &wl_data);
    }
}

static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

static void get_server_references(void)
{
    wl_data.wldisplay = wl_display_connect(NULL);
    if (wl_data.wldisplay == NULL) {
        fprintf(stderr, "Failed to connect to display\n");
        exit(1);
    }

    struct wl_registry *registry = wl_display_get_registry(wl_data.wldisplay);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(wl_data.wldisplay);
    wl_display_roundtrip(wl_data.wldisplay);

    if (wl_data.wlcompositor == NULL || wl_data.wlshell == NULL) {
        fprintf(stderr, "Failed to find compositor or shell\n");
        exit(1);
    }
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
                && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
{
    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

SwapchainSupportDetails querySwapchainSupport(VkPhysicalDevice device)
{
    SwapchainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface,
            &presentModeCount, details.presentModes.data());
    }

    return details;
}

void createSyncObjects()
{
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &semaphoreInfo, NULL, &imageAvailableSemaphore) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, NULL, &renderFinishedSemaphore) != VK_SUCCESS) {
        printf("Failed to create synchronization objects for a frame\n");
        exit(1);
    }

    fences = (VkFence *)malloc(sizeof(VkFence) * imageCount);
    if (!fences) {
        printf("Failed to allocate fences\n");
        exit(2);
    }

    for (uint32_t i = 0; i < imageCount; i++) {
        VkResult result;

        VkFenceCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        createInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        result = vkCreateFence(device, &createInfo, NULL, &fences[i]);
        if (result != VK_SUCCESS) {
            for (; i > 0; i--) {
                vkDestroyFence(device, fences[i - 1], NULL);
            }
            free(fences);
            fences = NULL;
            printf("Failed to create fences\n");
            exit(2);
        }
    }
}

VkResult vkGetBestGraphicsQueue(VkPhysicalDevice physicalDevice)
{
    VkResult ret = VK_ERROR_INITIALIZATION_FAILED;
    uint32_t queueFamilyPropertiesCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertiesCount, 0);

    VkQueueFamilyProperties *const queueFamilyProperties = (VkQueueFamilyProperties*)malloc(
        sizeof(VkQueueFamilyProperties) * queueFamilyPropertiesCount);

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
        &queueFamilyPropertiesCount, queueFamilyProperties);

    graphicsFamily = queueFamilyPropertiesCount;
    presentFamily = queueFamilyPropertiesCount;

    for (uint32_t i = 0; i < queueFamilyPropertiesCount; i++) {
        if (VK_QUEUE_GRAPHICS_BIT & queueFamilyProperties[i].queueFlags) {
            graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

        if (presentSupport) {
            presentFamily = i;
        }

        if (graphicsFamily != queueFamilyPropertiesCount && presentFamily != queueFamilyPropertiesCount) {
            ret = VK_SUCCESS;
            break;
        }
    }
    free(queueFamilyProperties);

    return ret;
}

#define check_result(result) \
    if (VK_SUCCESS != (result)) { \
        fprintf(stderr, "Failure at %i %s with result %i\n", __LINE__, __FILE__, result); exit(-1); \
    }

static void recordPipelineImageBarrier(VkCommandBuffer commandBuffer,
                                       VkAccessFlags srcAccessMask,
                                       VkAccessFlags dstAccessMask,
                                       VkImageLayout srcLayout,
                                       VkImageLayout dstLayout,
                                       VkImage image)
{
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.srcAccessMask = srcAccessMask;
    barrier.dstAccessMask = dstAccessMask;
    barrier.oldLayout = srcLayout;
    barrier.newLayout = dstLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, NULL, 0, NULL, 1,
                         &barrier);
}

static void rerecordCommandBuffer(uint32_t frameIndex, const VkClearColorValue *clearColor)
{
    VkCommandBuffer commandBuffer = commandBuffers[frameIndex];
    VkImage image = swapchainImages[frameIndex];
    VkCommandBufferBeginInfo beginInfo = {};
    VkImageSubresourceRange clearRange = {};

    check_result(vkResetCommandBuffer(commandBuffer, 0));

    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    check_result(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    recordPipelineImageBarrier(commandBuffer, 0, VK_ACCESS_TRANSFER_WRITE_BIT,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, image);

    clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    clearRange.baseMipLevel = 0;
    clearRange.levelCount = 1;
    clearRange.baseArrayLayer = 0;
    clearRange.layerCount = 1;
    vkCmdClearColorImage(commandBuffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, clearColor, 1,
                         &clearRange);

    recordPipelineImageBarrier(commandBuffer, VK_ACCESS_TRANSFER_WRITE_BIT,
                               VK_ACCESS_MEMORY_READ_BIT,
                               VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, image);
    check_result(vkEndCommandBuffer(commandBuffer));
}

bool createSwapchain()
{
    /*
     * Select mode and format
     */
    SwapchainSupportDetails swapchainSupport = querySwapchainSupport(usedPhysicalDevice);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.presentModes);
    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);

    imageCount = swapchainSupport.capabilities.minImageCount + 1;
    if (swapchainSupport.capabilities.maxImageCount > 0
            && imageCount > swapchainSupport.capabilities.maxImageCount) {
        imageCount = swapchainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = (VkExtent2D){wl_data.width, wl_data.height};
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;

    uint32_t queueFamilyIndices[] = {graphicsFamily, presentFamily};

    if (graphicsFamily != presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapchainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = swapchain;

    if (vkCreateSwapchainKHR(device, &createInfo, 0, &swapchain) != VK_SUCCESS) {
        return false;
    }

    /*
     * Get swapchain images
     */
    check_result(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL));

    swapchainImages = (VkImage *)malloc(sizeof(VkImage) * imageCount);
    if (!swapchainImages) {
        printf("Failed to allocate swapchain images\n");
        exit(2);
    }
    check_result(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages));
    if (createInfo.oldSwapchain) {
        vkDestroySwapchainKHR(device, createInfo.oldSwapchain, NULL);
    }

    swapchainImageFormat = surfaceFormat.format;

    /*
     * Create command pool
     */
    VkCommandPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolCreateInfo.flags =
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    poolCreateInfo.queueFamilyIndex = graphicsFamily;
    check_result(vkCreateCommandPool(device, &poolCreateInfo, NULL, &commandPool));

    /*
     * Create command buffers
     */
    VkCommandBufferAllocateInfo allocateInfo = {};
    allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = imageCount;
    commandBuffers = (VkCommandBuffer *)malloc(sizeof(VkCommandBuffer) * imageCount);
    check_result(vkAllocateCommandBuffers(device, &allocateInfo, commandBuffers));

    return true;
}

const char *physicalDeviceTypeString(VkPhysicalDeviceType type)
{
    const char *typeString;
    switch (type) {
    case VK_PHYSICAL_DEVICE_TYPE_OTHER:
        typeString = "Other";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
        typeString = "Integrated GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
        typeString = "Discrete GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
        typeString = "Virtual GPU";
        break;
    case VK_PHYSICAL_DEVICE_TYPE_CPU:
        typeString = "CPU";
        break;
    default:
        typeString = "unknown";
    }
    return typeString;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    /*
     * Wayland Setup
     */
    get_server_references();

    wl_data.wlsurface = wl_compositor_create_surface(wl_data.wlcompositor);
    if (wl_data.wlsurface == NULL) {
        fprintf(stderr, "Failed to create surface\n");
        exit(1);
    }

    wl_data.wlshell_surface = wl_shell_get_shell_surface(wl_data.wlshell, wl_data.wlsurface);
    wl_shell_surface_add_listener(wl_data.wlshell_surface, &shell_surface_listener, &wl_data);
    wl_shell_surface_set_class(wl_data.wlshell_surface, "test");
    wl_shell_surface_set_title(wl_data.wlshell_surface, "test_vulkan");
    wl_shell_surface_set_toplevel(wl_data.wlshell_surface);

    wl_data.wlregion = wl_compositor_create_region(wl_data.wlcompositor);
    wl_region_add(wl_data.wlregion, 0, 0, wl_data.width, wl_data.height);
    wl_surface_set_opaque_region(wl_data.wlsurface, wl_data.wlregion);

    /*
     * Create instance
     */
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "test_vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    uint32_t instExtCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &instExtCount, NULL);

    /*
     * Enumerate the instance extensions
     */
    VkExtensionProperties* inst_exts =
        (VkExtensionProperties *)malloc(instExtCount * sizeof(VkExtensionProperties));
    vkEnumerateInstanceExtensionProperties(NULL, &instExtCount, inst_exts);

    char **enabledExtensions = (char **)malloc(instExtCount * sizeof(char *));

    for (uint32_t i = 0; i < instExtCount; i++) {
        enabledExtensions[i] = (char *)malloc(VK_MAX_EXTENSION_NAME_SIZE * sizeof(char));
        strncpy(enabledExtensions[i], inst_exts[i].extensionName, VK_MAX_EXTENSION_NAME_SIZE);
    }

    printf("Found %u instance extensions\n", instExtCount);
    free(inst_exts);

    /*
     * Create instance
     */
    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    instanceCreateInfo.enabledExtensionCount = instExtCount;
    instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
    instanceCreateInfo.enabledLayerCount = 0;
    instanceCreateInfo.pNext = NULL;

    VkInstance instance;
    check_result(vkCreateInstance(&instanceCreateInfo, 0, &instance));

    /*
     * Find physical device
     */
    uint32_t physicalDeviceCount = 0;
    check_result(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, 0));

    printf("Found %u physical device(s)\n", physicalDeviceCount);

    VkPhysicalDevice* const physicalDevices = (VkPhysicalDevice*)malloc(
        sizeof(VkPhysicalDevice) * physicalDeviceCount);

    check_result(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));

    if (physicalDeviceCount > 0) {
        /*
         * Prinf device information
         */
        usedPhysicalDevice = physicalDevices[0];

        VkPhysicalDeviceProperties deviceProperties = {};

        vkGetPhysicalDeviceProperties(usedPhysicalDevice, &deviceProperties);
        printf("  Device name: %s\n", deviceProperties.deviceName);
        printf("  Device type: %s\n", physicalDeviceTypeString(deviceProperties.deviceType));
        printf("  API version: %u.%u.%u\n", VK_VERSION_MAJOR(deviceProperties.apiVersion)
                                        , VK_VERSION_MINOR(deviceProperties.apiVersion)
                                        , VK_VERSION_PATCH(deviceProperties.apiVersion));
        printf("  Driver version: %u.%u.%u\n", VK_VERSION_MAJOR(deviceProperties.driverVersion)
                                           , VK_VERSION_MINOR(deviceProperties.driverVersion)
                                           , VK_VERSION_PATCH(deviceProperties.driverVersion));
        printf("  Device ID %u\n", deviceProperties.deviceID);
        printf("  Vendor ID %u\n", deviceProperties.vendorID);

        uint32_t deviceExtensionCount = 0;
        check_result(vkEnumerateDeviceExtensionProperties(usedPhysicalDevice, NULL,
                                                          &deviceExtensionCount, NULL));
        printf("  Found %u device extensions\n", deviceExtensionCount);

        /*
         * Create surface
         */
        VkWaylandSurfaceCreateInfoKHR surfaceCreateInfo = {};
        surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
        surfaceCreateInfo.pNext = NULL;
        surfaceCreateInfo.flags = 0;
        surfaceCreateInfo.display = wl_data.wldisplay;
        surfaceCreateInfo.surface =  wl_data.wlsurface;
        check_result(vkCreateWaylandSurfaceKHR(instance, &surfaceCreateInfo,
                                               NULL, &surface));

        /*
         * Create device
         */
        uint32_t queueFamilyIndex = 0;
        check_result(vkGetBestGraphicsQueue(usedPhysicalDevice));

        static const char *const deviceExtensionNames[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        };
        const float queuePriority = 1.0f;
        VkDeviceQueueCreateInfo deviceQueueCreateInfo = {};
        deviceQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        deviceQueueCreateInfo.queueCount = 1;
        deviceQueueCreateInfo.pQueuePriorities = &queuePriority;

        VkDeviceCreateInfo deviceCreateInfo = {};
        deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        deviceCreateInfo.queueCreateInfoCount = 1;
        deviceCreateInfo.pQueueCreateInfos = &deviceQueueCreateInfo;
        deviceCreateInfo.pEnabledFeatures = NULL;
        deviceCreateInfo.enabledExtensionCount = 1;
        deviceCreateInfo.ppEnabledExtensionNames = deviceExtensionNames;

        check_result(vkCreateDevice(usedPhysicalDevice, &deviceCreateInfo, 0, &device));

        /*
         * Get queues
         */
        vkGetDeviceQueue(device, graphicsFamily, 0, &graphicsQueue);
        if (graphicsFamily != presentFamily) {
            vkGetDeviceQueue(device, presentFamily, 0, &presentQueue);
        } else {
            presentQueue = graphicsQueue;
        }

        /*
         * Create swapchain
         */
        if (!createSwapchain()) {
            printf("Failed to create swapchain\n");
            exit(1);
        }

        /*
         * Create sync objects
         */
        createSyncObjects();

        /*
         * Render loop
         */
        printf("Start render loop\n");
        for (uint32_t currentFrame = 0; currentFrame < 3600; currentFrame++) {
            VkClearColorValue clearColor = { {0} };
            float time = currentFrame / 120.0;
            uint32_t imageIndex;

            if (currentFrame % 60 == 0) printf("Frame %u\n", currentFrame);
            check_result(vkAcquireNextImageKHR(device, swapchain, UINT64_MAX,
                imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex));

            check_result(vkWaitForFences(device, 1, &fences[imageIndex], VK_FALSE, UINT64_MAX));
            check_result(vkResetFences(device, 1, &fences[imageIndex]));

            clearColor.float32[0] = (float)(0.5 + 0.5 * sin(time));
            clearColor.float32[1] = (float)(0.5 + 0.5 * sin(time + M_PI * 2 / 3));
            clearColor.float32[2] = (float)(0.5 + 0.5 * sin(time + M_PI * 4 / 3));
            clearColor.float32[3] = 1;
            rerecordCommandBuffer(imageIndex, &clearColor);

            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};

            VkSubmitInfo submitInfo = {};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = &imageAvailableSemaphore;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = &renderFinishedSemaphore;

            check_result(vkQueueSubmit(graphicsQueue, 1, &submitInfo, fences[imageIndex]));

            VkPresentInfoKHR presentInfo = {};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = &renderFinishedSemaphore;
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = &swapchain;
            presentInfo.pImageIndices = &imageIndex;

            check_result(vkQueuePresentKHR(presentQueue, &presentInfo));

            wl_display_dispatch_pending(wl_data.wldisplay);
            wl_display_flush(wl_data.wldisplay);
        }

        /*
         * Cleanup vulkan setup
         */
        for (uint32_t i = 0; i < imageCount; i++) {
            vkDestroyFence(device, fences[i], NULL);
        }
        free(fences);

        vkFreeCommandBuffers(device, commandPool, imageCount, commandBuffers);
        free(commandBuffers);

        vkDestroyCommandPool(device, commandPool, NULL);

        vkDestroySwapchainKHR(device, swapchain, NULL);
        free(swapchainImages);

        vkDestroySemaphore(device, imageAvailableSemaphore, NULL);
        vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
        vkDestroyDevice(device, NULL);
        vkDestroySurfaceKHR(instance, surface, NULL);
    }

    free(physicalDevices);

    vkDestroyInstance(instance, NULL);

    for (uint32_t i = 0; i < instExtCount; i++) {
        free(enabledExtensions[i]);
    }

    free(enabledExtensions);

    return 0;
}
