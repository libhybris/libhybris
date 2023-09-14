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
#include <cutils/properties.h>

#include "HWC2.h"
#include "ComposerHal.h"
#include "hwc2_compatibility_layer.h"

using namespace android;
using aidl::android::hardware::graphics::composer3::Capability;
using aidl::android::hardware::graphics::composer3::Color;
using aidl::android::hardware::graphics::composer3::Composition;

namespace hal = android::hardware::graphics::composer::hal;

class HWComposerCallback : public HWC2::ComposerCallback
{
public:
    HWComposerCallback(HWC2EventListener* listener) :
        listener(listener) { }

    void onComposerHalHotplug(hal::HWDisplayId display, hal::Connection connection) {
        listener->on_hotplug_received(listener, 0, display,
                                    connection == hal::Connection::CONNECTED,
                                    true);
    }

    void onComposerHalRefresh(hal::HWDisplayId display) {
        listener->on_refresh_received(listener, 0, display);
    }

    void onComposerHalVsync(hal::HWDisplayId display, int64_t timestamp,
                            std::optional<hal::VsyncPeriodNanos>) {
        listener->on_vsync_received(listener, 0, display, timestamp);
    }

    void onComposerHalVsyncPeriodTimingChanged(hal::HWDisplayId,
                                               const hal::VsyncPeriodChangeTimeline&) { }

    void onComposerHalSeamlessPossible(hal::HWDisplayId) { }
    void onComposerHalVsyncIdle(hal::HWDisplayId) { }

    virtual ~HWComposerCallback() { };
private:
    HWC2EventListener *listener;
};

struct hwc2_compat_device
{
    HWC2::Device *self;
    std::unique_ptr<HWComposerCallback> listener;
};

struct hwc2_compat_display
{
    HWC2::Display *self;
};

struct hwc2_compat_layer
{
    std::shared_ptr<HWC2::Layer> self;
};

struct hwc2_compat_out_fences
{
    std::unordered_map<HWC2::Layer*, android::sp<android::Fence>> fences;
};

hwc2_compat_device_t* hwc2_compat_device_new(bool useVrComposer)
{
    hwc2_compat_device_t *device = new hwc2_compat_device_t();
    if (!device)
        return nullptr;

    char buf[PROPERTY_VALUE_MAX] = {};
    property_get("debug.sf.hwc_service_name", buf, "default");

    device->self = new HWC2::Device(Hwc2::Composer::create(buf));

    bool presentTimestamp =
        !device->self->getCapabilities().count(Capability::PRESENT_FENCE_IS_NOT_RELIABLE);
    property_set("service.sf.present_timestamp", presentTimestamp ? "1" : "0");

    return device;
}

void hwc2_compat_device_register_callback(hwc2_compat_device_t *device,
                                          HWC2EventListener* listener,
                                          int composerSequenceId /*unused*/)
{
    device->listener = std::make_unique<HWComposerCallback>(listener);
    device->self->registerCallback(*device->listener);
}

void hwc2_compat_device_on_hotplug(hwc2_compat_device_t* device,
                                    hwc2_display_t displayId, bool connected)
{
    device->self->onHotplug(displayId,
                            connected ? hal::Connection::CONNECTED
                                      : hal::Connection::DISCONNECTED);
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
    if (error == hal::Error::BAD_CONFIG) {
        fprintf(stderr, "getActiveConfig: No config active, returning null");
    } else if (error != hal::Error::NONE) {
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
    hal::Error error = display->self->acceptChanges();
    return static_cast<hwc2_error_t>(error);
}

hwc2_compat_layer_t* hwc2_compat_display_create_layer(hwc2_compat_display_t* display)
{
    hwc2_compat_layer_t *layer = new hwc2_compat_layer_t();
    if (!layer)
        return nullptr;

    if (display->self->createLayer(&layer->self) != hal::Error::NONE)
        return nullptr;

    return layer;
}

void hwc2_compat_display_destroy_layer(hwc2_compat_display_t* display,
                                       hwc2_compat_layer_t* layer)
{
    delete layer;
}

hwc2_error_t hwc2_compat_display_get_release_fences(hwc2_compat_display_t* display,
                                       hwc2_compat_out_fences_t** outFences)
{
    hwc2_compat_out_fences_t *fences = new struct hwc2_compat_out_fences;

    hal::Error error = display->self->getReleaseFences(&fences->fences);
    if (error != hal::Error::NONE) {
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
    hal::Error error = display->self->present(&presentFence);

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

    hal::Error error = display->self->setClientTarget(0, target,
                                        acquireFence, hal::Dataspace::UNKNOWN);

    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_set_power_mode(hwc2_compat_display_t* display,
                                        int mode)
{
    hal::Error error = display->self->setPowerMode(
        static_cast<hal::PowerMode>(mode));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_set_vsync_enabled(hwc2_compat_display_t* display,
                                           int enabled)
{
    hal::Error error = display->self->setVsyncEnabled(
        enabled ? hal::Vsync::ENABLE : hal::Vsync::DISABLE);
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_display_validate(hwc2_compat_display_t* display,
                                 uint32_t* outNumTypes,
                                 uint32_t* outNumRequests)
{
    const int expectedPresentTime = 0;
    hal::Error error = display->self->validate(expectedPresentTime, outNumTypes, outNumRequests);
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

    hal::Error error = layer->self->setBuffer(0, target, acquireFence);

    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_blend_mode(hwc2_compat_layer_t* layer, int mode)
{
    hal::Error error = layer->self->setBlendMode(
        static_cast<hal::BlendMode>(mode));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_color(hwc2_compat_layer_t* layer,
                                    hwc_color_t color)
{
    Color aidl_color{
        color.r / 255.0f,
        color.g / 255.0f,
        color.b / 255.0f,
        color.a / 255.0f,
    };
    hal::Error error = layer->self->setColor(aidl_color);
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_composition_type(hwc2_compat_layer_t* layer,
                                            int type)
{
    hal::Error error = layer->self->setCompositionType(
        static_cast<Composition>(type));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_dataspace(hwc2_compat_layer_t* layer,
                                        android_dataspace_t dataspace)
{
    hal::Error error = layer->self->setDataspace(
        static_cast<hal::Dataspace>(dataspace));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_display_frame(hwc2_compat_layer_t* layer,
                                            int32_t left, int32_t top,
                                            int32_t right, int32_t bottom)
{
    android::Rect r = {left, top, right, bottom};

    hal::Error error = layer->self->setDisplayFrame(r);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_plane_alpha(hwc2_compat_layer_t* layer,
                                        float alpha)
{
    hal::Error error = layer->self->setPlaneAlpha(alpha);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_sideband_stream(hwc2_compat_layer_t* layer,
                                            const native_handle_t* stream)
{
    hal::Error error = layer->self->setSidebandStream(stream);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_source_crop(hwc2_compat_layer_t* layer,
                                        float left, float top,
                                        float right, float bottom)
{
    android::FloatRect r = {left, top, right, bottom};

    hal::Error error = layer->self->setSourceCrop(r);
    return static_cast<hwc2_error_t>(error);
}
hwc2_error_t hwc2_compat_layer_set_transform(hwc2_compat_layer_t* layer,
                                        int transform)
{
    hal::Error error = layer->self->setTransform(
        static_cast<Hwc2::Transform>(transform));
    return static_cast<hwc2_error_t>(error);
}

hwc2_error_t hwc2_compat_layer_set_visible_region(hwc2_compat_layer_t* layer,
                                            int32_t left, int32_t top,
                                            int32_t right, int32_t bottom)
{
    android::Rect r = {left, top, right, bottom};

    hal::Error error = layer->self->setVisibleRegion(android::Region(r));
    return static_cast<hwc2_error_t>(error);
}

int32_t hwc2_compat_out_fences_get_fence(hwc2_compat_out_fences_t* fences,
                                         hwc2_compat_layer_t* layer)
{
    auto iter = fences->fences.find(layer->self.get());

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
