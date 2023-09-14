/*
 * Copyright 2021 The Android Open Source Project
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

#include "ComposerHal.h"

#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#pragma clang diagnostic ignored "-Wextra"

#include <android/hardware/graphics/composer/2.4/IComposer.h>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>

#include <aidl/android/hardware/graphics/composer3/IComposer.h>
#include <aidl/android/hardware/graphics/composer3/IComposerClient.h>
#include <android/hardware/graphics/composer3/ComposerClientReader.h>
#include <android/hardware/graphics/composer3/ComposerClientWriter.h>

#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCapability.h>

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion -Wextra"

namespace android::Hwc2 {

using aidl::android::hardware::graphics::common::DisplayDecorationSupport;
using aidl::android::hardware::graphics::composer3::ComposerClientReader;
using aidl::android::hardware::graphics::composer3::ComposerClientWriter;

class AidlIComposerCallbackWrapper;

// Composer is a wrapper to IComposer, a proxy to server-side composer.
class AidlComposer final : public Hwc2::Composer {
public:
    static bool isDeclared(const std::string& serviceName);

    explicit AidlComposer(const std::string& serviceName);
    ~AidlComposer() override;

    bool isSupported(OptionalFeature) const;

    std::vector<aidl::android::hardware::graphics::composer3::Capability> getCapabilities()
            override;
    std::string dumpDebugInfo() override;

    void registerCallback(HWC2::ComposerCallback& callback) override;

    // Reset all pending commands in the command buffer. Useful if you want to
    // skip a frame but have already queued some commands.
    void resetCommands() override;

    // Explicitly flush all pending commands in the command buffer.
    Error executeCommands() override;

    uint32_t getMaxVirtualDisplayCount() override;
    Error createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat* format,
                               Display* outDisplay) override;
    Error destroyVirtualDisplay(Display display) override;

    Error acceptDisplayChanges(Display display) override;

    Error createLayer(Display display, Layer* outLayer) override;
    Error destroyLayer(Display display, Layer layer) override;

    Error getActiveConfig(Display display, Config* outConfig) override;
    Error getChangedCompositionTypes(
            Display display, std::vector<Layer>* outLayers,
            std::vector<aidl::android::hardware::graphics::composer3::Composition>* outTypes)
            override;
    Error getColorModes(Display display, std::vector<ColorMode>* outModes) override;
    Error getDisplayAttribute(Display display, Config config, IComposerClient::Attribute attribute,
                              int32_t* outValue) override;
    Error getDisplayConfigs(Display display, std::vector<Config>* outConfigs);
    Error getDisplayName(Display display, std::string* outName) override;

    Error getDisplayRequests(Display display, uint32_t* outDisplayRequestMask,
                             std::vector<Layer>* outLayers,
                             std::vector<uint32_t>* outLayerRequestMasks) override;

    Error getDozeSupport(Display display, bool* outSupport) override;
    Error hasDisplayIdleTimerCapability(Display display, bool* outSupport) override;
    Error getHdrCapabilities(Display display, std::vector<Hdr>* outTypes, float* outMaxLuminance,
                             float* outMaxAverageLuminance, float* outMinLuminance) override;

    Error getReleaseFences(Display display, std::vector<Layer>* outLayers,
                           std::vector<int>* outReleaseFences) override;

    Error presentDisplay(Display display, int* outPresentFence) override;

    Error setActiveConfig(Display display, Config config) override;

    /*
     * The composer caches client targets internally.  When target is nullptr,
     * the composer uses slot to look up the client target from its cache.
     * When target is not nullptr, the cache is updated with the new target.
     */
    Error setClientTarget(Display display, uint32_t slot, const sp<GraphicBuffer>& target,
                          int acquireFence, Dataspace dataspace,
                          const std::vector<IComposerClient::Rect>& damage) override;
    Error setColorMode(Display display, ColorMode mode, RenderIntent renderIntent) override;
    Error setColorTransform(Display display, const float* matrix) override;
    Error setOutputBuffer(Display display, const native_handle_t* buffer,
                          int releaseFence) override;
    Error setPowerMode(Display display, IComposerClient::PowerMode mode) override;
    Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled) override;

    Error setClientTargetSlotCount(Display display) override;

    Error validateDisplay(Display display, nsecs_t expectedPresentTime, uint32_t* outNumTypes,
                          uint32_t* outNumRequests) override;

    Error presentOrValidateDisplay(Display display, nsecs_t expectedPresentTime,
                                   uint32_t* outNumTypes, uint32_t* outNumRequests,
                                   int* outPresentFence, uint32_t* state) override;

    Error setCursorPosition(Display display, Layer layer, int32_t x, int32_t y) override;
    /* see setClientTarget for the purpose of slot */
    Error setLayerBuffer(Display display, Layer layer, uint32_t slot,
                         const sp<GraphicBuffer>& buffer, int acquireFence) override;
    Error setLayerSurfaceDamage(Display display, Layer layer,
                                const std::vector<IComposerClient::Rect>& damage) override;
    Error setLayerBlendMode(Display display, Layer layer, IComposerClient::BlendMode mode) override;
    Error setLayerColor(Display display, Layer layer, const Color& color) override;
    Error setLayerCompositionType(
            Display display, Layer layer,
            aidl::android::hardware::graphics::composer3::Composition type) override;
    Error setLayerDataspace(Display display, Layer layer, Dataspace dataspace) override;
    Error setLayerDisplayFrame(Display display, Layer layer,
                               const IComposerClient::Rect& frame) override;
    Error setLayerPlaneAlpha(Display display, Layer layer, float alpha) override;
    Error setLayerSidebandStream(Display display, Layer layer,
                                 const native_handle_t* stream) override;
    Error setLayerSourceCrop(Display display, Layer layer,
                             const IComposerClient::FRect& crop) override;
    Error setLayerTransform(Display display, Layer layer, Transform transform) override;
    Error setLayerVisibleRegion(Display display, Layer layer,
                                const std::vector<IComposerClient::Rect>& visible) override;
    Error setLayerZOrder(Display display, Layer layer, uint32_t z) override;

    // Composer HAL 2.2
    Error setLayerPerFrameMetadata(
            Display display, Layer layer,
            const std::vector<IComposerClient::PerFrameMetadata>& perFrameMetadatas) override;
    std::vector<IComposerClient::PerFrameMetadataKey> getPerFrameMetadataKeys(
            Display display) override;
    Error getRenderIntents(Display display, ColorMode colorMode,
                           std::vector<RenderIntent>* outRenderIntents) override;
    Error getDataspaceSaturationMatrix(Dataspace dataspace, mat4* outMatrix) override;

    // Composer HAL 2.3
    Error getDisplayIdentificationData(Display display, uint8_t* outPort,
                                       std::vector<uint8_t>* outData) override;
    Error setLayerColorTransform(Display display, Layer layer, const float* matrix) override;
    Error getDisplayedContentSamplingAttributes(Display display, PixelFormat* outFormat,
                                                Dataspace* outDataspace,
                                                uint8_t* outComponentMask) override;
    Error setDisplayContentSamplingEnabled(Display display, bool enabled, uint8_t componentMask,
                                           uint64_t maxFrames) override;
    Error getDisplayedContentSample(Display display, uint64_t maxFrames, uint64_t timestamp,
                                    DisplayedFrameStats* outStats) override;
    Error setLayerPerFrameMetadataBlobs(
            Display display, Layer layer,
            const std::vector<IComposerClient::PerFrameMetadataBlob>& metadata) override;
    Error setDisplayBrightness(Display display, float brightness, float brightnessNits,
                               const DisplayBrightnessOptions& options) override;

    // Composer HAL 2.4
    Error getDisplayCapabilities(
            Display display,
            std::vector<aidl::android::hardware::graphics::composer3::DisplayCapability>*
                    outCapabilities) override;
    V2_4::Error getDisplayConnectionType(Display display,
                                         IComposerClient::DisplayConnectionType* outType) override;
    V2_4::Error getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriod) override;
    V2_4::Error setActiveConfigWithConstraints(
            Display display, Config config,
            const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
            VsyncPeriodChangeTimeline* outTimeline) override;
    V2_4::Error setAutoLowLatencyMode(Display displayId, bool on) override;
    V2_4::Error getSupportedContentTypes(
            Display displayId,
            std::vector<IComposerClient::ContentType>* outSupportedContentTypes) override;
    V2_4::Error setContentType(Display displayId,
                               IComposerClient::ContentType contentType) override;
    V2_4::Error setLayerGenericMetadata(Display display, Layer layer, const std::string& key,
                                        bool mandatory, const std::vector<uint8_t>& value) override;
    V2_4::Error getLayerGenericMetadataKeys(
            std::vector<IComposerClient::LayerGenericMetadataKey>* outKeys) override;
    Error getClientTargetProperty(
            Display display,
            aidl::android::hardware::graphics::composer3::ClientTargetPropertyWithBrightness*
                    outClientTargetProperty) override;

    // AIDL Composer HAL
    Error setLayerBrightness(Display display, Layer layer, float brightness) override;
    Error setLayerBlockingRegion(Display display, Layer layer,
                                 const std::vector<IComposerClient::Rect>& blocking) override;
    Error setBootDisplayConfig(Display displayId, Config) override;
    Error clearBootDisplayConfig(Display displayId) override;
    Error getPreferredBootDisplayConfig(Display displayId, Config*) override;
    Error getDisplayDecorationSupport(Display display,
                                      std::optional<DisplayDecorationSupport>* support) override;
    Error setIdleTimerEnabled(Display displayId, std::chrono::milliseconds timeout) override;

    Error getPhysicalDisplayOrientation(Display displayId,
                                        AidlTransform* outDisplayOrientation) override;

private:
    // Many public functions above simply write a command into the command
    // queue to batch the calls.  validateDisplay and presentDisplay will call
    // this function to execute the command queue.
    Error execute();

    // returns the default instance name for the given service
    static std::string instance(const std::string& serviceName);

    // 64KiB minus a small space for metadata such as read/write pointers
    static constexpr size_t kWriterInitialSize = 64 * 1024 / sizeof(uint32_t) - 16;
    // Max number of buffers that may be cached for a given layer
    // We obtain this number by:
    // 1. Tightly coupling this cache to the max size of BufferQueue
    // 2. Adding an additional slot for the layer caching feature in SurfaceFlinger (see: Planner.h)
    static const constexpr uint32_t kMaxLayerBufferCount = BufferQueue::NUM_BUFFER_SLOTS + 1;
    ComposerClientWriter mWriter;
    ComposerClientReader mReader;

    // Aidl interface
    using AidlIComposer = aidl::android::hardware::graphics::composer3::IComposer;
    using AidlIComposerClient = aidl::android::hardware::graphics::composer3::IComposerClient;
    std::shared_ptr<AidlIComposer> mAidlComposer;
    std::shared_ptr<AidlIComposerClient> mAidlComposerClient;
    std::shared_ptr<AidlIComposerCallbackWrapper> mAidlComposerCallback;
};

} // namespace android::Hwc2
