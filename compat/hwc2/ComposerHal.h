/*
 * Copyright 2016 The Android Open Source Project
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

#pragma once

#include <memory>

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wextra"

#include <composer-command-buffer/2.4/ComposerCommandBuffer.h>
#include <gui/BufferQueue.h>
#include <gui/HdrMetadata.h>
#include <math/mat4.h>
#include <ui/DisplayedFrameStats.h>
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>

#include <aidl/android/hardware/graphics/common/DisplayDecorationSupport.h>
#include <aidl/android/hardware/graphics/composer3/Capability.h>
#include <aidl/android/hardware/graphics/composer3/ClientTargetPropertyWithBrightness.h>
#include <aidl/android/hardware/graphics/composer3/Color.h>
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCapability.h>
#include <aidl/android/hardware/graphics/composer3/IComposerCallback.h>

#include <aidl/android/hardware/graphics/common/Transform.h>
#include <optional>

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion -Wextra"

namespace android {
namespace HWC2 {
struct ComposerCallback;
} // namespace HWC2

namespace Hwc2 {

namespace types = hardware::graphics::common;

namespace V2_1 = hardware::graphics::composer::V2_1;
namespace V2_2 = hardware::graphics::composer::V2_2;
namespace V2_3 = hardware::graphics::composer::V2_3;
namespace V2_4 = hardware::graphics::composer::V2_4;
namespace V3_0 = ::aidl::android::hardware::graphics::composer3;

using types::V1_0::ColorTransform;
using types::V1_0::Transform;
using types::V1_1::RenderIntent;
using types::V1_2::ColorMode;
using types::V1_2::Dataspace;
using types::V1_2::Hdr;
using types::V1_2::PixelFormat;

using V2_1::Config;
using V2_1::Display;
using V2_1::Error;
using V2_1::Layer;
using V2_4::CommandReaderBase;
using V2_4::CommandWriterBase;
using V2_4::IComposer;
using V2_4::IComposerCallback;
using V2_4::IComposerClient;
using V2_4::VsyncPeriodChangeTimeline;
using V2_4::VsyncPeriodNanos;
using PerFrameMetadata = IComposerClient::PerFrameMetadata;
using PerFrameMetadataKey = IComposerClient::PerFrameMetadataKey;
using PerFrameMetadataBlob = IComposerClient::PerFrameMetadataBlob;
using AidlTransform = ::aidl::android::hardware::graphics::common::Transform;

class Composer {
public:
    static std::unique_ptr<Composer> create(const std::string& serviceName);

    virtual ~Composer() = 0;

    enum class OptionalFeature {
        RefreshRateSwitching,
        ExpectedPresentTime,
        // Whether setDisplayBrightness is able to be applied as part of a display command.
        DisplayBrightnessCommand,
        KernelIdleTimer,
        PhysicalDisplayOrientation,
    };

    virtual bool isSupported(OptionalFeature) const = 0;

    virtual std::vector<aidl::android::hardware::graphics::composer3::Capability>
    getCapabilities() = 0;
    virtual std::string dumpDebugInfo() = 0;

    virtual void registerCallback(HWC2::ComposerCallback& callback) = 0;

    // Reset all pending commands in the command buffer. Useful if you want to
    // skip a frame but have already queued some commands.
    virtual void resetCommands() = 0;

    // Explicitly flush all pending commands in the command buffer.
    virtual Error executeCommands() = 0;

    virtual uint32_t getMaxVirtualDisplayCount() = 0;
    virtual Error createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat*,
                                       Display* outDisplay) = 0;
    virtual Error destroyVirtualDisplay(Display display) = 0;

    virtual Error acceptDisplayChanges(Display display) = 0;

    virtual Error createLayer(Display display, Layer* outLayer) = 0;
    virtual Error destroyLayer(Display display, Layer layer) = 0;

    virtual Error getActiveConfig(Display display, Config* outConfig) = 0;
    virtual Error getChangedCompositionTypes(Display display, std::vector<Layer>* outLayers,
                                             std::vector<V3_0::Composition>* outTypes) = 0;
    virtual Error getColorModes(Display display, std::vector<ColorMode>* outModes) = 0;
    virtual Error getDisplayAttribute(Display display, Config config,
                                      IComposerClient::Attribute attribute, int32_t* outValue) = 0;
    virtual Error getDisplayConfigs(Display display, std::vector<Config>* outConfigs) = 0;
    virtual Error getDisplayName(Display display, std::string* outName) = 0;

    virtual Error getDisplayRequests(Display display, uint32_t* outDisplayRequestMask,
                                     std::vector<Layer>* outLayers,
                                     std::vector<uint32_t>* outLayerRequestMasks) = 0;

    virtual Error getDozeSupport(Display display, bool* outSupport) = 0;
    virtual Error hasDisplayIdleTimerCapability(Display display, bool* outSupport) = 0;
    virtual Error getHdrCapabilities(Display display, std::vector<Hdr>* outTypes,
                                     float* outMaxLuminance, float* outMaxAverageLuminance,
                                     float* outMinLuminance) = 0;

    virtual Error getReleaseFences(Display display, std::vector<Layer>* outLayers,
                                   std::vector<int>* outReleaseFences) = 0;

    virtual Error presentDisplay(Display display, int* outPresentFence) = 0;

    virtual Error setActiveConfig(Display display, Config config) = 0;

    /*
     * The composer caches client targets internally.  When target is nullptr,
     * the composer uses slot to look up the client target from its cache.
     * When target is not nullptr, the cache is updated with the new target.
     */
    virtual Error setClientTarget(Display display, uint32_t slot, const sp<GraphicBuffer>& target,
                                  int acquireFence, Dataspace dataspace,
                                  const std::vector<IComposerClient::Rect>& damage) = 0;
    virtual Error setColorMode(Display display, ColorMode mode, RenderIntent renderIntent) = 0;
    virtual Error setColorTransform(Display display, const float* matrix) = 0;
    virtual Error setOutputBuffer(Display display, const native_handle_t* buffer,
                                  int releaseFence) = 0;
    virtual Error setPowerMode(Display display, IComposerClient::PowerMode mode) = 0;
    virtual Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled) = 0;

    virtual Error setClientTargetSlotCount(Display display) = 0;

    virtual Error validateDisplay(Display display, nsecs_t expectedPresentTime,
                                  uint32_t* outNumTypes, uint32_t* outNumRequests) = 0;

    virtual Error presentOrValidateDisplay(Display display, nsecs_t expectedPresentTime,
                                           uint32_t* outNumTypes, uint32_t* outNumRequests,
                                           int* outPresentFence, uint32_t* state) = 0;

    virtual Error setCursorPosition(Display display, Layer layer, int32_t x, int32_t y) = 0;
    /* see setClientTarget for the purpose of slot */
    virtual Error setLayerBuffer(Display display, Layer layer, uint32_t slot,
                                 const sp<GraphicBuffer>& buffer, int acquireFence) = 0;
    virtual Error setLayerSurfaceDamage(Display display, Layer layer,
                                        const std::vector<IComposerClient::Rect>& damage) = 0;
    virtual Error setLayerBlendMode(Display display, Layer layer,
                                    IComposerClient::BlendMode mode) = 0;
    virtual Error setLayerColor(
            Display display, Layer layer,
            const aidl::android::hardware::graphics::composer3::Color& color) = 0;
    virtual Error setLayerCompositionType(
            Display display, Layer layer,
            aidl::android::hardware::graphics::composer3::Composition type) = 0;
    virtual Error setLayerDataspace(Display display, Layer layer, Dataspace dataspace) = 0;
    virtual Error setLayerDisplayFrame(Display display, Layer layer,
                                       const IComposerClient::Rect& frame) = 0;
    virtual Error setLayerPlaneAlpha(Display display, Layer layer, float alpha) = 0;
    virtual Error setLayerSidebandStream(Display display, Layer layer,
                                         const native_handle_t* stream) = 0;
    virtual Error setLayerSourceCrop(Display display, Layer layer,
                                     const IComposerClient::FRect& crop) = 0;
    virtual Error setLayerTransform(Display display, Layer layer, Transform transform) = 0;
    virtual Error setLayerVisibleRegion(Display display, Layer layer,
                                        const std::vector<IComposerClient::Rect>& visible) = 0;
    virtual Error setLayerZOrder(Display display, Layer layer, uint32_t z) = 0;

    // Composer HAL 2.2
    virtual Error setLayerPerFrameMetadata(
            Display display, Layer layer,
            const std::vector<IComposerClient::PerFrameMetadata>& perFrameMetadatas) = 0;
    virtual std::vector<IComposerClient::PerFrameMetadataKey> getPerFrameMetadataKeys(
            Display display) = 0;
    virtual Error getRenderIntents(Display display, ColorMode colorMode,
            std::vector<RenderIntent>* outRenderIntents) = 0;
    virtual Error getDataspaceSaturationMatrix(Dataspace dataspace, mat4* outMatrix) = 0;

    // Composer HAL 2.3
    virtual Error getDisplayIdentificationData(Display display, uint8_t* outPort,
                                               std::vector<uint8_t>* outData) = 0;
    virtual Error setLayerColorTransform(Display display, Layer layer,
                                         const float* matrix) = 0;
    virtual Error getDisplayedContentSamplingAttributes(Display display, PixelFormat* outFormat,
                                                        Dataspace* outDataspace,
                                                        uint8_t* outComponentMask) = 0;
    virtual Error setDisplayContentSamplingEnabled(Display display, bool enabled,
                                                   uint8_t componentMask, uint64_t maxFrames) = 0;
    virtual Error getDisplayedContentSample(Display display, uint64_t maxFrames, uint64_t timestamp,
                                            DisplayedFrameStats* outStats) = 0;
    virtual Error setLayerPerFrameMetadataBlobs(
            Display display, Layer layer, const std::vector<PerFrameMetadataBlob>& metadata) = 0;
    // Options for setting the display brightness
    struct DisplayBrightnessOptions {
        // If true, then immediately submits a brightness change request to composer. Otherwise,
        // submission of the brightness change may be deferred until presenting the next frame.
        // applyImmediately should only be false if OptionalFeature::DisplayBrightnessCommand is
        // supported.
        bool applyImmediately = true;

        bool operator==(const DisplayBrightnessOptions& other) const {
            return applyImmediately == other.applyImmediately;
        }
    };
    virtual Error setDisplayBrightness(Display display, float brightness, float brightnessNits,
                                       const DisplayBrightnessOptions& options) = 0;

    // Composer HAL 2.4
    virtual Error getDisplayCapabilities(Display display,
                                         std::vector<V3_0::DisplayCapability>* outCapabilities) = 0;
    virtual V2_4::Error getDisplayConnectionType(
            Display display, IComposerClient::DisplayConnectionType* outType) = 0;
    virtual V2_4::Error getDisplayVsyncPeriod(Display display,
                                              VsyncPeriodNanos* outVsyncPeriod) = 0;
    virtual V2_4::Error setActiveConfigWithConstraints(
            Display display, Config config,
            const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
            VsyncPeriodChangeTimeline* outTimeline) = 0;

    virtual V2_4::Error setAutoLowLatencyMode(Display displayId, bool on) = 0;
    virtual V2_4::Error getSupportedContentTypes(
            Display displayId,
            std::vector<IComposerClient::ContentType>* outSupportedContentTypes) = 0;
    virtual V2_4::Error setContentType(Display displayId,
                                       IComposerClient::ContentType contentType) = 0;
    virtual V2_4::Error setLayerGenericMetadata(Display display, Layer layer,
                                                const std::string& key, bool mandatory,
                                                const std::vector<uint8_t>& value) = 0;
    virtual V2_4::Error getLayerGenericMetadataKeys(
            std::vector<IComposerClient::LayerGenericMetadataKey>* outKeys) = 0;

    virtual Error getClientTargetProperty(
            Display display, V3_0::ClientTargetPropertyWithBrightness* outClientTargetProperty) = 0;

    // AIDL Composer
    virtual Error setLayerBrightness(Display display, Layer layer, float brightness) = 0;
    virtual Error setLayerBlockingRegion(Display display, Layer layer,
                                         const std::vector<IComposerClient::Rect>& blocking) = 0;
    virtual Error setBootDisplayConfig(Display displayId, Config) = 0;
    virtual Error clearBootDisplayConfig(Display displayId) = 0;
    virtual Error getPreferredBootDisplayConfig(Display displayId, Config*) = 0;
    virtual Error getDisplayDecorationSupport(
            Display display,
            std::optional<::aidl::android::hardware::graphics::common::DisplayDecorationSupport>*
                    support) = 0;
    virtual Error setIdleTimerEnabled(Display displayId, std::chrono::milliseconds timeout) = 0;
    virtual Error getPhysicalDisplayOrientation(Display displayId,
                                                AidlTransform* outDisplayOrientation) = 0;
};

} // namespace Hwc2
} // namespace android
