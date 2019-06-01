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

#ifndef ANDROID_SF_COMPOSER_HAL_H
#define ANDROID_SF_COMPOSER_HAL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <android/frameworks/vr/composer/1.0/IVrComposerClient.h>
#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <utils/StrongPointer.h>
#include <IComposerCommandBuffer.h>

namespace android {

namespace Hwc2 {

using android::frameworks::vr::composer::V1_0::IVrComposerClient;

using android::hardware::graphics::common::V1_0::ColorMode;
using android::hardware::graphics::common::V1_0::ColorTransform;
using android::hardware::graphics::common::V1_0::Dataspace;
using android::hardware::graphics::common::V1_0::Hdr;
using android::hardware::graphics::common::V1_0::PixelFormat;
using android::hardware::graphics::common::V1_0::Transform;

using android::hardware::graphics::composer::V2_1::IComposer;
using android::hardware::graphics::composer::V2_1::IComposerCallback;
using android::hardware::graphics::composer::V2_1::IComposerClient;
using android::hardware::graphics::composer::V2_1::Error;
using android::hardware::graphics::composer::V2_1::Display;
using android::hardware::graphics::composer::V2_1::Layer;
using android::hardware::graphics::composer::V2_1::Config;

using android::hardware::graphics::composer::V2_1::CommandWriterBase;
using android::hardware::graphics::composer::V2_1::CommandReaderBase;

using android::hardware::kSynchronizedReadWrite;
using android::hardware::MessageQueue;
using android::hardware::MQDescriptorSync;
using android::hardware::hidl_vec;
using android::hardware::hidl_handle;

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
    void takeChangedCompositionTypes(Display display,
            std::vector<Layer>* outLayers,
            std::vector<IComposerClient::Composition>* outTypes);

    // Get and clear saved display requests.
    void takeDisplayRequests(Display display,
        uint32_t* outDisplayRequestMask, std::vector<Layer>* outLayers,
        std::vector<uint32_t>* outLayerRequestMasks);

    // Get and clear saved release fences.
    void takeReleaseFences(Display display, std::vector<Layer>* outLayers,
            std::vector<int>* outReleaseFences);

    // Get and clear saved present fence.
    void takePresentFence(Display display, int* outPresentFence);

    // Get what stage succeeded during PresentOrValidate: Present or Validate
    void takePresentOrValidateStage(Display display, uint32_t * state);

private:
    void resetData();

    bool parseSelectDisplay(uint16_t length);
    bool parseSetError(uint16_t length);
    bool parseSetChangedCompositionTypes(uint16_t length);
    bool parseSetDisplayRequests(uint16_t length);
    bool parseSetPresentFence(uint16_t length);
    bool parseSetReleaseFences(uint16_t length);
    bool parseSetPresentOrValidateDisplayResult(uint16_t length);

    struct ReturnData {
        uint32_t displayRequests = 0;

        std::vector<Layer> changedLayers;
        std::vector<IComposerClient::Composition> compositionTypes;

        std::vector<Layer> requestedLayers;
        std::vector<uint32_t> requestMasks;

        int presentFence = -1;

        std::vector<Layer> releasedLayers;
        std::vector<int> releaseFences;

        uint32_t presentOrValidateState;
    };

    std::vector<CommandError> mErrors;
    std::unordered_map<Display, ReturnData> mReturnData;

    // When SELECT_DISPLAY is parsed, this is updated to point to the
    // display's return data in mReturnData.  We use it to avoid repeated
    // map lookups.
    ReturnData* mCurrentReturnData;
};

// Composer is a wrapper to IComposer, a proxy to server-side composer.
class Composer {
public:
    Composer(bool useVrComposer);

    std::vector<IComposer::Capability> getCapabilities();
    std::string dumpDebugInfo();

    void registerCallback(const sp<IComposerCallback>& callback);

    // Returns true if the connected composer service is running in a remote
    // process, false otherwise. This will return false if the service is
    // configured in passthrough mode, for example.
    bool isRemote();

    // Reset all pending commands in the command buffer. Useful if you want to
    // skip a frame but have already queued some commands.
    void resetCommands();

    uint32_t getMaxVirtualDisplayCount();
    bool isUsingVrComposer() const { return mIsUsingVrComposer; }
    Error createVirtualDisplay(uint32_t width, uint32_t height,
            PixelFormat* format, Display* outDisplay);
    Error destroyVirtualDisplay(Display display);

    Error acceptDisplayChanges(Display display);

    Error createLayer(Display display, Layer* outLayer);
    Error destroyLayer(Display display, Layer layer);

    Error getActiveConfig(Display display, Config* outConfig);
    Error getChangedCompositionTypes(Display display,
            std::vector<Layer>* outLayers,
            std::vector<IComposerClient::Composition>* outTypes);
    Error getColorModes(Display display, std::vector<ColorMode>* outModes);
    Error getDisplayAttribute(Display display, Config config,
            IComposerClient::Attribute attribute, int32_t* outValue);
    Error getDisplayConfigs(Display display, std::vector<Config>* outConfigs);
    Error getDisplayName(Display display, std::string* outName);

    Error getDisplayRequests(Display display, uint32_t* outDisplayRequestMask,
            std::vector<Layer>* outLayers,
            std::vector<uint32_t>* outLayerRequestMasks);

    Error getDisplayType(Display display,
            IComposerClient::DisplayType* outType);
    Error getDozeSupport(Display display, bool* outSupport);
    Error getHdrCapabilities(Display display, std::vector<Hdr>* outTypes,
            float* outMaxLuminance, float* outMaxAverageLuminance,
            float* outMinLuminance);

    Error getReleaseFences(Display display, std::vector<Layer>* outLayers,
            std::vector<int>* outReleaseFences);

    Error presentDisplay(Display display, int* outPresentFence);

    Error setActiveConfig(Display display, Config config);

    /*
     * The composer caches client targets internally.  When target is nullptr,
     * the composer uses slot to look up the client target from its cache.
     * When target is not nullptr, the cache is updated with the new target.
     */
    Error setClientTarget(Display display, uint32_t slot,
            const sp<GraphicBuffer>& target,
            int acquireFence, Dataspace dataspace,
            const std::vector<IComposerClient::Rect>& damage);
    Error setColorMode(Display display, ColorMode mode);
    Error setColorTransform(Display display, const float* matrix,
            ColorTransform hint);
    Error setOutputBuffer(Display display, const native_handle_t* buffer,
            int releaseFence);
    Error setPowerMode(Display display, IComposerClient::PowerMode mode);
    Error setVsyncEnabled(Display display, IComposerClient::Vsync enabled);

    Error setClientTargetSlotCount(Display display);

    Error validateDisplay(Display display, uint32_t* outNumTypes,
            uint32_t* outNumRequests);

    Error presentOrValidateDisplay(Display display, uint32_t* outNumTypes,
                                   uint32_t* outNumRequests,
                                   int* outPresentFence,
                                   uint32_t* state);

    Error setCursorPosition(Display display, Layer layer,
            int32_t x, int32_t y);
    /* see setClientTarget for the purpose of slot */
    Error setLayerBuffer(Display display, Layer layer, uint32_t slot,
            const sp<GraphicBuffer>& buffer, int acquireFence);
    Error setLayerSurfaceDamage(Display display, Layer layer,
            const std::vector<IComposerClient::Rect>& damage);
    Error setLayerBlendMode(Display display, Layer layer,
            IComposerClient::BlendMode mode);
    Error setLayerColor(Display display, Layer layer,
            const IComposerClient::Color& color);
    Error setLayerCompositionType(Display display, Layer layer,
            IComposerClient::Composition type);
    Error setLayerDataspace(Display display, Layer layer,
            Dataspace dataspace);
    Error setLayerDisplayFrame(Display display, Layer layer,
            const IComposerClient::Rect& frame);
    Error setLayerPlaneAlpha(Display display, Layer layer,
            float alpha);
    Error setLayerSidebandStream(Display display, Layer layer,
            const native_handle_t* stream);
    Error setLayerSourceCrop(Display display, Layer layer,
            const IComposerClient::FRect& crop);
    Error setLayerTransform(Display display, Layer layer,
            Transform transform);
    Error setLayerVisibleRegion(Display display, Layer layer,
            const std::vector<IComposerClient::Rect>& visible);
    Error setLayerZOrder(Display display, Layer layer, uint32_t z);
    Error setLayerInfo(Display display, Layer layer, uint32_t type,
                       uint32_t appId);
private:
    class CommandWriter : public CommandWriterBase {
    public:
        CommandWriter(uint32_t initialMaxSize);
        ~CommandWriter() override;

        void setLayerInfo(uint32_t type, uint32_t appId);
        void setClientTargetMetadata(
                const IVrComposerClient::BufferMetadata& metadata);
        void setLayerBufferMetadata(
                const IVrComposerClient::BufferMetadata& metadata);

    private:
        void writeBufferMetadata(
                const IVrComposerClient::BufferMetadata& metadata);
    };

    // Many public functions above simply write a command into the command
    // queue to batch the calls.  validateDisplay and presentDisplay will call
    // this function to execute the command queue.
    Error execute();

    sp<IComposer> mComposer;
    sp<IComposerClient> mClient;

    // 64KiB minus a small space for metadata such as read/write pointers
    static constexpr size_t kWriterInitialSize =
        64 * 1024 / sizeof(uint32_t) - 16;
    CommandWriter mWriter;
    CommandReader mReader;

    // When true, the we attach to the vr_hwcomposer service instead of the
    // hwcomposer. This allows us to redirect surfaces to 3d surfaces in vr.
    const bool mIsUsingVrComposer;
};

} // namespace Hwc2

} // namespace android

#endif // ANDROID_SF_COMPOSER_HAL_H
