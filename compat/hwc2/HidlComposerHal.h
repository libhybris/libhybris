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

#include <gui/BufferQueue.h>
#include <math/mat4.h>
#include <ui/GraphicBuffer.h>
#include <utils/StrongPointer.h>

#if ANDROID_VERSION_MAJOR < 9
#include <IComposerCommandBuffer.h>
#elif ANDROID_VERSION_MAJOR < 10
#include <composer-command-buffer/2.2/ComposerCommandBuffer.h>
#elif ANDROID_VERSION_MAJOR < 11
#include <composer-command-buffer/2.3/ComposerCommandBuffer.h>
#else
#include <composer-command-buffer/2.4/ComposerCommandBuffer.h>
#endif

#if ANDROID_VERSION_MAJOR >= 9
#include <gui/HdrMetadata.h>
#endif
#if ANDROID_VERSION_MAJOR >= 10
#include <ui/DisplayedFrameStats.h>
#endif
#if ANDROID_VERSION_MAJOR >= 13
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#endif

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion -Wextra"

namespace android {
namespace Hwc2 {

namespace types = hardware::graphics::common;

namespace V2_1 = hardware::graphics::composer::V2_1;
#if ANDROID_VERSION_MAJOR >= 9
namespace V2_2 = hardware::graphics::composer::V2_2;
#endif
#if ANDROID_VERSION_MAJOR >= 10
namespace V2_3 = hardware::graphics::composer::V2_3;
#endif
#if ANDROID_VERSION_MAJOR >= 11
namespace V2_4 = hardware::graphics::composer::V2_4;
#endif

using types::V1_0::ColorTransform;
using types::V1_0::Transform;

#if ANDROID_VERSION_MAJOR < 9
using types::V1_0::ColorMode;
using types::V1_0::Dataspace;
using types::V1_0::PixelFormat;
#elif ANDROID_VERSION_MAJOR < 10
using types::V1_1::ColorMode;
using types::V1_1::Dataspace;
using types::V1_1::PixelFormat;
using types::V1_1::RenderIntent;
#else // ANDROID_VERSION_MAJOR >= 10
using types::V1_2::ColorMode;
using types::V1_2::Dataspace;
using types::V1_2::PixelFormat;
using types::V1_1::RenderIntent;

#if ANDROID_VERSION_MAJOR < 14
using types::V1_2::Hdr;
#endif

#endif // ANDROID_VERSION_MAJOR < 9

using V2_1::Config;
using V2_1::Display;
using V2_1::Error;
using V2_1::Layer;

#if ANDROID_VERSION_MAJOR < 9
using V2_1::CommandReaderBase;
using V2_1::CommandWriterBase;
using V2_1::IComposer;
using V2_1::IComposerCallback;
using V2_1::IComposerClient;
#elif ANDROID_VERSION_MAJOR < 10
using V2_2::CommandReaderBase;
using V2_2::CommandWriterBase;
using V2_2::IComposer;
using V2_2::IComposerCallback;
using V2_2::IComposerClient;
#elif ANDROID_VERSION_MAJOR < 11
using V2_3::CommandReaderBase;
using V2_3::CommandWriterBase;
using V2_3::IComposer;
using V2_3::IComposerCallback;
using V2_3::IComposerClient;
#else
using V2_4::CommandReaderBase;
using V2_4::CommandWriterBase;
using V2_4::IComposer;
using V2_4::IComposerCallback;
using V2_4::IComposerClient;
using V2_4::VsyncPeriodChangeTimeline;
using V2_4::VsyncPeriodNanos;
#endif

#if ANDROID_VERSION_MAJOR >= 9
using PerFrameMetadata = IComposerClient::PerFrameMetadata;
using PerFrameMetadataKey = IComposerClient::PerFrameMetadataKey;
#endif
#if ANDROID_VERSION_MAJOR >= 10
using PerFrameMetadataBlob = IComposerClient::PerFrameMetadataBlob;
#endif

class CommandReader : public CommandReaderBase {
public:
    ~CommandReader();

    // Parse and execute commands from the command queue.  The commands are
    // actually return values from the server and will be saved in ReturnData.
    Error parse();

    // Get and clear saved errors.
    struct CommandError {
        uint32_t location;
        Error error;
    };
    std::vector<CommandError> takeErrors();

    bool hasChanges(Display display, uint32_t* outNumChangedCompositionTypes,
                    uint32_t* outNumLayerRequestMasks) const;

    // Get and clear saved changed composition types.
    void takeChangedCompositionTypes(
            Display display, std::vector<Layer>* outLayers,
            std::vector<Composition>* outTypes);

    // Get and clear saved display requests.
    void takeDisplayRequests(Display display, uint32_t* outDisplayRequestMask,
                             std::vector<Layer>* outLayers,
                             std::vector<uint32_t>* outLayerRequestMasks);

    // Get and clear saved release fences.
    void takeReleaseFences(Display display, std::vector<Layer>* outLayers,
                           std::vector<int>* outReleaseFences);

    // Get and clear saved present fence.
    void takePresentFence(Display display, int* outPresentFence);

    // Get what stage succeeded during PresentOrValidate: Present or Validate
    void takePresentOrValidateStage(Display display, uint32_t* state);

#if ANDROID_VERSION_MAJOR >= 11
    // Get the client target properties requested by hardware composer.
    void takeClientTargetProperty(Display display,
                                  IComposerClient::ClientTargetProperty* outClientTargetProperty);
#endif

private:
    void resetData();

    bool parseSelectDisplay(uint16_t length);
    bool parseSetError(uint16_t length);
    bool parseSetChangedCompositionTypes(uint16_t length);
    bool parseSetDisplayRequests(uint16_t length);
    bool parseSetPresentFence(uint16_t length);
    bool parseSetReleaseFences(uint16_t length);
    bool parseSetPresentOrValidateDisplayResult(uint16_t length);
#if ANDROID_VERSION_MAJOR >= 11
    bool parseSetClientTargetProperty(uint16_t length);
#endif

    struct ReturnData {
        uint32_t displayRequests = 0;

        std::vector<Layer> changedLayers;
        std::vector<Composition> compositionTypes;

        std::vector<Layer> requestedLayers;
        std::vector<uint32_t> requestMasks;

        int presentFence = -1;

        std::vector<Layer> releasedLayers;
        std::vector<int> releaseFences;

        uint32_t presentOrValidateState;

#if ANDROID_VERSION_MAJOR >= 11
        // Composer 2.4 implementation can return a client target property
        // structure to indicate the client target properties that hardware
        // composer requests. The composer client must change the client target
        // properties to match this request.
        IComposerClient::ClientTargetProperty clientTargetProperty{PixelFormat::RGBA_8888,
                                                                   Dataspace::UNKNOWN};
#endif
    };

    std::vector<CommandError> mErrors;
    std::unordered_map<Display, ReturnData> mReturnData;

    // When SELECT_DISPLAY is parsed, this is updated to point to the
    // display's return data in mReturnData.  We use it to avoid repeated
    // map lookups.
    ReturnData* mCurrentReturnData;
};

// Composer is a wrapper to IComposer, a proxy to server-side composer.
class HidlComposer final : public Composer {
public:
    explicit HidlComposer(const std::string& serviceName);
    ~HidlComposer() override;

    bool isSupported(OptionalFeature) const;
    bool isVrrSupported() const;

    std::vector<Capability> getCapabilities()
            override;
    std::string dumpDebugInfo() override;

    void registerCallback(HWC2::ComposerCallback& callback) override;

    // Explicitly flush all pending commands in the command buffer.
    Error executeCommands(Display) override;

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
            std::vector<Composition>* outTypes)
            override;
    Error getColorModes(Display display, std::vector<ColorMode>* outModes) override;
    Error getDisplayAttribute(Display display, Config config, IComposerClient::Attribute attribute,
                              int32_t* outValue) override;
    Error getDisplayConfigs(Display display, std::vector<Config>* outConfigs);
#if ANDROID_VERSION_MAJOR >= 14
    Error getDisplayConfigurations(Display, int32_t maxFrameIntervalNs,
                                   std::vector<DisplayConfiguration>*);
#endif
    Error getDisplayName(Display display, std::string* outName) override;

    Error getDisplayRequests(Display display, uint32_t* outDisplayRequestMask,
                             std::vector<Layer>* outLayers,
                             std::vector<uint32_t>* outLayerRequestMasks) override;

    Error getDozeSupport(Display display, bool* outSupport) override;
    Error hasDisplayIdleTimerCapability(Display display, bool* outSupport) override;
    Error getHdrCapabilities(Display display, std::vector<Hdr>* outHdrTypes, float* outMaxLuminance,
                             float* outMaxAverageLuminance, float* outMinLuminance) override;
#if ANDROID_VERSION_MAJOR >= 14
    Error getOverlaySupport(aidl::android::hardware::graphics::composer3::OverlayProperties*
                                    outProperties) override;
#endif

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
                          const std::vector<IComposerClient::Rect>& damage,
                          float hdrSdrRatio) override;
#if ANDROID_VERSION_MAJOR < 9
    Error setColorMode(Display display, ColorMode mode) override;
#else
    Error setColorMode(Display display, ColorMode mode, RenderIntent renderIntent) override;
#endif
    Error setColorTransform(Display display, const float* matrix) override;
    Error setOutputBuffer(Display display, const native_handle_t* buffer,
                          int releaseFence) override;
    Error setPowerMode(Display display, IComposerClient::PowerMode mode) override;
    Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled) override;

    Error setClientTargetSlotCount(Display display) override;

    Error validateDisplay(Display display, nsecs_t expectedPresentTime, int32_t frameIntervalNs,
                          uint32_t* outNumTypes, uint32_t* outNumRequests) override;

    Error presentOrValidateDisplay(Display display, nsecs_t expectedPresentTime,
                                   int32_t frameIntervalNs, uint32_t* outNumTypes,
                                   uint32_t* outNumRequests, int* outPresentFence,
                                   uint32_t* state) override;

    Error setCursorPosition(Display display, Layer layer, int32_t x, int32_t y) override;
    /* see setClientTarget for the purpose of slot */
    Error setLayerBuffer(Display display, Layer layer, uint32_t slot,
                         const sp<GraphicBuffer>& buffer, int acquireFence) override;
    Error setLayerSurfaceDamage(Display display, Layer layer,
                                const std::vector<IComposerClient::Rect>& damage) override;
    Error setLayerBlendMode(Display display, Layer layer, IComposerClient::BlendMode mode) override;
    Error setLayerColor(Display display, Layer layer,
                        const Color& color) override;
    Error setLayerCompositionType(
            Display display, Layer layer,
            Composition type) override;
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

#if ANDROID_VERSION_MAJOR >= 9
    // Composer HAL 2.2
    Error setLayerPerFrameMetadata(
            Display display, Layer layer,
            const std::vector<IComposerClient::PerFrameMetadata>& perFrameMetadatas) override;
    std::vector<IComposerClient::PerFrameMetadataKey> getPerFrameMetadataKeys(
            Display display) override;
    Error getRenderIntents(Display display, ColorMode colorMode,
                           std::vector<RenderIntent>* outRenderIntents) override;
    Error getDataspaceSaturationMatrix(Dataspace dataspace, mat4* outMatrix) override;
#endif

#if ANDROID_VERSION_MAJOR >= 10
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
#endif

#if ANDROID_VERSION_MAJOR >= 11
    // Composer HAL 2.4
    Error getDisplayCapabilities(
            Display display,
            std::vector<DisplayCapability>*
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
            ClientTargetProperty*
                    outClientTargetProperty) override;
#endif

#if ANDROID_VERSION_MAJOR >= 13
    // AIDL Composer HAL
    Error setLayerBrightness(Display display, Layer layer, float brightness) override;
    Error setLayerBlockingRegion(Display display, Layer layer,
                                 const std::vector<IComposerClient::Rect>& blocking) override;
    Error setBootDisplayConfig(Display displayId, Config) override;
    Error clearBootDisplayConfig(Display displayId) override;
    Error getPreferredBootDisplayConfig(Display displayId, Config*) override;
    Error getDisplayDecorationSupport(
            Display display,
            std::optional<aidl::android::hardware::graphics::common::DisplayDecorationSupport>*
                    support) override;
    Error setIdleTimerEnabled(Display displayId, std::chrono::milliseconds timeout) override;

    Error getPhysicalDisplayOrientation(Display displayId,
                                        AidlTransform* outDisplayOrientation) override;
#endif

    void onHotplugConnect(Display) override;
    void onHotplugDisconnect(Display) override;

#if ANDROID_VERSION_MAJOR >= 14
    Error getHdrConversionCapabilities(
            std::vector<aidl::android::hardware::graphics::common::HdrConversionCapability>*)
            override;
    Error setHdrConversionStrategy(aidl::android::hardware::graphics::common::HdrConversionStrategy,
                                   Hdr*) override;
    Error setRefreshRateChangedCallbackDebugEnabled(Display, bool) override;
    Error notifyExpectedPresent(Display, nsecs_t, int32_t) override;
#endif

private:
    class CommandWriter : public CommandWriterBase {
    public:
        explicit CommandWriter(uint32_t initialMaxSize) : CommandWriterBase(initialMaxSize) {}
        ~CommandWriter() override {}
    };

    void registerCallback(const sp<IComposerCallback>& callback);

    // Many public functions above simply write a command into the command
    // queue to batch the calls.  validateDisplay and presentDisplay will call
    // this function to execute the command queue.
    Error execute();

    sp<V2_1::IComposer> mComposer;

    sp<V2_1::IComposerClient> mClient;
#if ANDROID_VERSION_MAJOR >= 9
    sp<V2_2::IComposerClient> mClient_2_2;
#endif
#if ANDROID_VERSION_MAJOR >= 10
    sp<V2_3::IComposerClient> mClient_2_3;
#endif
#if ANDROID_VERSION_MAJOR >= 11
    sp<IComposerClient> mClient_2_4;
#endif

    // 64KiB minus a small space for metadata such as read/write pointers
    static constexpr size_t kWriterInitialSize = 64 * 1024 / sizeof(uint32_t) - 16;
    // Max number of buffers that may be cached for a given layer
    // We obtain this number by:
    // 1. Tightly coupling this cache to the max size of BufferQueue
    // 2. Adding an additional slot for the layer caching feature in SurfaceFlinger (see: Planner.h)
    static const constexpr uint32_t kMaxLayerBufferCount = BufferQueue::NUM_BUFFER_SLOTS + 1;
    CommandWriter mWriter;
    CommandReader mReader;
};

} // namespace Hwc2
} // namespace android
