/*
 * Copyright (C) 2018 TheKit <nekit1000@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <ui/Fence.h>
#include <ui/FloatRect.h>
#include <ui/GraphicBuffer.h>
#include <ui/Region.h>
#include <sync/sync.h>

#include "HWC2.h"
#include "hwc2_compatibility_layer.h"

class HWComposerCallback : public HWC2::ComposerCallback
{
public:
    HWComposerCallback(HWC2EventListener* listener);

    void onVsyncReceived(int32_t sequenceId, hwc2_display_t display,
                        int64_t timestamp) override;
    void onHotplugReceived(int32_t sequenceId, hwc2_display_t display,
                        HWC2::Connection connection,
                        bool primaryDisplay) override;
    void onRefreshReceived(int32_t sequenceId,
                           hwc2_display_t display) override;
private:
    HWC2EventListener *listener;
};

HWComposerCallback::HWComposerCallback(HWC2EventListener* listener) :
    listener(listener)
{
}

void HWComposerCallback::onVsyncReceived(int32_t sequenceId,
                                         hwc2_display_t display,
                                         int64_t timestamp)
{
    listener->on_vsync_received(listener, sequenceId, display, timestamp);
}

void HWComposerCallback::onHotplugReceived(int32_t sequenceId,
                                           hwc2_display_t display,
                                           HWC2::Connection connection,
                                           bool primaryDisplay)
{
    listener->on_hotplug_received(listener, sequenceId, display,
                                  connection == HWC2::Connection::Connected,
                                  primaryDisplay);
}

void HWComposerCallback::onRefreshReceived(int32_t sequenceId,
                                           hwc2_display_t display)
{
    listener->on_refresh_received(listener, sequenceId, display);
}

struct hwc2_compat_device
{
    HWC2::Device *self;
};

struct hwc2_compat_display
{
    HWC2::Display *self;
};

struct hwc2_compat_layer
{
    HWC2::Layer *self;
};

struct hwc2_compat_out_fences
{
    std::unordered_map<HWC2::Layer*, android::sp<android::Fence>> fences;
};

hwc2_compat_device_t* hwc2_compat_device_new(bool useVrComposer)
{
    hwc2_compat_device_t *device = (hwc2_compat_device_t*) malloc(
        sizeof(hwc2_compat_device_t));
    if (!device)
        return nullptr;

    device->self = new HWC2::Device(useVrComposer);

    return device;
}

void hwc2_compat_device_register_callback(hwc2_compat_device_t *device,
                                          HWC2EventListener* listener,
                                          int composerSequenceId)
{
    device->self->registerCallback(new HWComposerCallback(listener),
                                composerSequenceId);
}

void hwc2_compat_device_on_hotplug(hwc2_compat_device_t* device,
                                    hwc2_display_t displayId, bool connected)
{
    device->self->onHotplug(displayId,
                            static_cast<HWC2::Connection>(connected));
}

hwc2_compat_display_t* hwc2_compat_device_get_display_by_id(
                            hwc2_compat_device_t *device, hwc2_display_t id)
{
    hwc2_compat_display_t *display = (hwc2_compat_display_t*) malloc(
        sizeof(hwc2_compat_display_t));
    if (!display)
        return nullptr;

    display->self = device->self->getDisplayById(id);

    if (!display->self) {
        free(display);
        return nullptr;
    }

    return display;
}

void hwc2_compat_device_destroy_display(hwc2_compat_device_t* device,
                                        hwc2_compat_display_t* display)
{
    device->self->destroyDisplay(display->self->getId());
    free(display);
}

HWC2DisplayConfig* hwc2_compat_display_get_active_config(
                            hwc2_compat_display_t* display)
{
    HWC2DisplayConfig* config = (HWC2DisplayConfig*) malloc(
        sizeof(HWC2DisplayConfig));

    std::shared_ptr<const HWC2::Display::Config> activeConfig;
    auto error = display->self->getActiveConfig(&activeConfig);
    if (error == HWC2::Error::BadConfig) {
        fprintf(stderr, "getActiveConfig: No config active, returning null");
    } else if (error != HWC2::Error::None) {
        fprintf(stderr, "getActiveConfig failed for display %d: %s (%d)",
                static_cast<int32_t>(display->self->getId()),
                to_string(error).c_str(),
                static_cast<int32_t>(error));
    } else if (!activeConfig) {
        fprintf(stderr, "getActiveConfig returned empty config for display %d",
                static_cast<int32_t>(display->self->getId()));
    } else {
        config->id = activeConfig->getId();
        config->display = activeConfig->getDisplayId();
        config->width = activeConfig->getWidth();
        config->height = activeConfig->getHeight();
        config->vsyncPeriod = activeConfig->getVsyncPeriod();
        config->dpiX = activeConfig->getDpiX();
        config->dpiY = activeConfig->getDpiY();

        return config;
    }

    return nullptr;
}

hwc2_error_t hwc2_compat_display_accept_changes(hwc2_compat_display_t* display)
{
    HWC2::Error error = display->self->acceptChanges();
    return static_cast<hwc2_error_t>(error);
}

hwc2_compat_layer_t* hwc2_compat_display_create_layer(hwc2_compat_display_t* display)
{
    hwc2_compat_layer_t *layer = (hwc2_compat_layer_t*) malloc(
        sizeof(hwc2_compat_layer_t));
    if (!layer)
        return nullptr;

    if (display->self->createLayer(&layer->self) != HWC2::Error::None)
        return nullptr;

    return layer;
}

void hwc2_compat_display_destroy_layer(hwc2_compat_display_t* display,
                                       hwc2_compat_layer_t* layer)
{
    if (display->self->destroyLayer(layer->self) != HWC2::Error::None)
        delete layer->self;

    free(layer);
}

hwc2_error_t hwc2_compat_display_get_release_fences(hwc2_compat_display_t* display,
                                       hwc2_compat_out_fences_t** outFences)
{
    hwc2_compat_out_fences_t *fences = new struct hwc2_compat_out_fences;

    HWC2::Error error = display->self->getReleaseFences(&fences->fences);
    if (error != HWC2::Error::None) {
        delete fences;
    } else {
        *outFences = fences;
    }

    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_present(hwc2_compat_display_t* display,
                                    int32_t* outPresentFence)
{
    android::sp<android::Fence> presentFence;
    HWC2::Error error = display->self->present(&presentFence);

    if (presentFence != NULL) {
        *outPresentFence = presentFence->dup();
    } else {
        *outPresentFence = -1;
    }

    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_set_client_target(hwc2_compat_display_t* display,
                                            uint32_t slot,
                                            struct ANativeWindowBuffer* buffer,
                                            const int32_t acquireFenceFd,
                                            android_dataspace_t dataspace)
{
    android::sp<android::GraphicBuffer> target(
        new android::GraphicBuffer(buffer->handle,
            android::GraphicBuffer::WRAP_HANDLE,
            buffer->width, buffer->height,
            buffer->format, /* layerCount */ 1,
            buffer->usage, buffer->stride));

    android::sp<android::Fence> acquireFence(
            new android::Fence(acquireFenceFd));

    HWC2::Error error = display->self->setClientTarget(0, target,
                                        acquireFence, HAL_DATASPACE_UNKNOWN);

    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_set_power_mode(hwc2_compat_display_t* display,
                                        int mode)
{
    HWC2::Error error = display->self->setPowerMode(
        static_cast<HWC2::PowerMode>(mode));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_set_vsync_enabled(hwc2_compat_display_t* display,
                                           int enabled)
{
    HWC2::Error error = display->self->setVsyncEnabled(
        static_cast<HWC2::Vsync>(enabled));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_validate(hwc2_compat_display_t* display,
                                 uint32_t* outNumTypes,
                                 uint32_t* outNumRequests)
{
    HWC2::Error error = display->self->validate(outNumTypes, outNumRequests);
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_buffer(hwc2_compat_layer_t* layer,
                                          uint32_t slot,
                                          struct ANativeWindowBuffer* buffer,
                                          const int32_t acquireFenceFd)
{
    android::sp<android::GraphicBuffer> target(
        new android::GraphicBuffer(buffer->handle,
            android::GraphicBuffer::WRAP_HANDLE,
            buffer->width, buffer->height,
            buffer->format, /* layerCount */ 1,
            buffer->usage, buffer->stride));

    android::sp<android::Fence> acquireFence(
            new android::Fence(acquireFenceFd));

    HWC2::Error error = layer->self->setBuffer(0, target, acquireFence);

    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_blend_mode(hwc2_compat_layer_t* layer, int mode)
{
    HWC2::Error error = layer->self->setBlendMode(
        static_cast<HWC2::BlendMode>(mode));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_color(hwc2_compat_layer_t* layer,
                                    hwc_color_t color)
{
    HWC2::Error error = layer->self->setColor(color);
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_composition_type(hwc2_compat_layer_t* layer,
                                            int type)
{
    HWC2::Error error = layer->self->setCompositionType(
        static_cast<HWC2::Composition>(type));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_dataspace(hwc2_compat_layer_t* layer,
                                        android_dataspace_t dataspace)
{
    HWC2::Error error = layer->self->setDataspace(dataspace);
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_display_frame(hwc2_compat_layer_t* layer,
                                            int32_t left, int32_t top,
                                            int32_t right, int32_t bottom)
{
    android::Rect r = {left, top, right, bottom};

    HWC2::Error error = layer->self->setDisplayFrame(r);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_plane_alpha(hwc2_compat_layer_t* layer,
                                        float alpha)
{
    HWC2::Error error = layer->self->setPlaneAlpha(alpha);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_sideband_stream(hwc2_compat_layer_t* layer,
                                            const native_handle_t* stream)
{
    HWC2::Error error = layer->self->setSidebandStream(stream);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_source_crop(hwc2_compat_layer_t* layer,
                                        float left, float top,
                                        float right, float bottom)
{
    android::FloatRect r = {left, top, right, bottom};

    HWC2::Error error = layer->self->setSourceCrop(r);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_transform(hwc2_compat_layer_t* layer,
                                        int transform)
{
    HWC2::Error error = layer->self->setTransform(
        static_cast<HWC2::Transform>(transform));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_visible_region(hwc2_compat_layer_t* layer,
                                            int32_t left, int32_t top,
                                            int32_t right, int32_t bottom)
{
    android::Rect r = {left, top, right, bottom};

    HWC2::Error error = layer->self->setVisibleRegion(android::Region(r));
    return static_cast<hwc2_error_t>(error);
}

int32_t hwc2_compat_out_fences_get_fence(hwc2_compat_out_fences_t* fences,
                                         hwc2_compat_layer_t* layer)
{
    auto iter = fences->fences.find(layer->self);

    if(iter != fences->fences.end()) {
        return iter->second->dup();
    } else {
        return -1;
    }
}

void hwc2_compat_out_fences_destroy(hwc2_compat_out_fences_t* fences)
{
    delete fences;
}
