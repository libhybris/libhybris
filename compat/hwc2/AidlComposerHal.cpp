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

#undef LOG_TAG
#define LOG_TAG "HwcComposer"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "AidlComposerHal.h"

#include <android-base/file.h>
#include <android/binder_ibinder_platform.h>
#include <android/binder_manager.h>
#include <log/log.h>
#include <utils/Trace.h>

#include <aidl/android/hardware/graphics/composer3/BnComposerCallback.h>

#include <algorithm>
#include <cinttypes>

#include "HWC2.h"

namespace android {

using hardware::hidl_handle;
using hardware::hidl_vec;
using hardware::Return;

using aidl::android::hardware::graphics::composer3::BnComposerCallback;
using aidl::android::hardware::graphics::composer3::Capability;
using aidl::android::hardware::graphics::composer3::ClientTargetPropertyWithBrightness;
using aidl::android::hardware::graphics::composer3::PowerMode;
using aidl::android::hardware::graphics::composer3::VirtualDisplay;

using aidl::android::hardware::graphics::composer3::CommandResultPayload;

using AidlColorMode = aidl::android::hardware::graphics::composer3::ColorMode;
using AidlContentType = aidl::android::hardware::graphics::composer3::ContentType;
using AidlDisplayIdentification =
        aidl::android::hardware::graphics::composer3::DisplayIdentification;
using AidlDisplayContentSample = aidl::android::hardware::graphics::composer3::DisplayContentSample;
using AidlDisplayAttribute = aidl::android::hardware::graphics::composer3::DisplayAttribute;
using AidlDisplayCapability = aidl::android::hardware::graphics::composer3::DisplayCapability;
using AidlHdrCapabilities = aidl::android::hardware::graphics::composer3::HdrCapabilities;
using AidlPerFrameMetadata = aidl::android::hardware::graphics::composer3::PerFrameMetadata;
using AidlPerFrameMetadataKey = aidl::android::hardware::graphics::composer3::PerFrameMetadataKey;
using AidlPerFrameMetadataBlob = aidl::android::hardware::graphics::composer3::PerFrameMetadataBlob;
using AidlRenderIntent = aidl::android::hardware::graphics::composer3::RenderIntent;
using AidlVsyncPeriodChangeConstraints =
        aidl::android::hardware::graphics::composer3::VsyncPeriodChangeConstraints;
using AidlVsyncPeriodChangeTimeline =
        aidl::android::hardware::graphics::composer3::VsyncPeriodChangeTimeline;
using AidlDisplayContentSamplingAttributes =
        aidl::android::hardware::graphics::composer3::DisplayContentSamplingAttributes;
using AidlFormatColorComponent = aidl::android::hardware::graphics::composer3::FormatColorComponent;
using AidlDisplayConnectionType =
        aidl::android::hardware::graphics::composer3::DisplayConnectionType;

using AidlColorTransform = aidl::android::hardware::graphics::common::ColorTransform;
using AidlDataspace = aidl::android::hardware::graphics::common::Dataspace;
using AidlFRect = aidl::android::hardware::graphics::common::FRect;
using AidlRect = aidl::android::hardware::graphics::common::Rect;
using AidlTransform = aidl::android::hardware::graphics::common::Transform;

namespace Hwc2 {

namespace {

template <typename To, typename From>
To translate(From x) {
    return static_cast<To>(x);
}

template <typename To, typename From>
std::vector<To> translate(const std::vector<From>& in) {
    std::vector<To> out;
    out.reserve(in.size());
    std::transform(in.begin(), in.end(), std::back_inserter(out),
                   [](From x) { return translate<To>(x); });
    return out;
}

template <>
AidlRect translate(IComposerClient::Rect x) {
    return AidlRect{
            .left = x.left,
            .top = x.top,
            .right = x.right,
            .bottom = x.bottom,
    };
}

template <>
AidlFRect translate(IComposerClient::FRect x) {
    return AidlFRect{
            .left = x.left,
            .top = x.top,
            .right = x.right,
            .bottom = x.bottom,
    };
}

template <>
AidlPerFrameMetadataBlob translate(IComposerClient::PerFrameMetadataBlob x) {
    AidlPerFrameMetadataBlob blob;
    blob.key = translate<AidlPerFrameMetadataKey>(x.key),
    std::copy(x.blob.begin(), x.blob.end(), std::inserter(blob.blob, blob.blob.end()));
    return blob;
}

template <>
AidlPerFrameMetadata translate(IComposerClient::PerFrameMetadata x) {
    return AidlPerFrameMetadata{
            .key = translate<AidlPerFrameMetadataKey>(x.key),
            .value = x.value,
    };
}

template <>
DisplayedFrameStats translate(AidlDisplayContentSample x) {
    return DisplayedFrameStats{
            .numFrames = static_cast<uint64_t>(x.frameCount),
            .component_0_sample = translate<uint64_t>(x.sampleComponent0),
            .component_1_sample = translate<uint64_t>(x.sampleComponent1),
            .component_2_sample = translate<uint64_t>(x.sampleComponent2),
            .component_3_sample = translate<uint64_t>(x.sampleComponent3),
    };
}

template <>
AidlVsyncPeriodChangeConstraints translate(IComposerClient::VsyncPeriodChangeConstraints x) {
    return AidlVsyncPeriodChangeConstraints{
            .desiredTimeNanos = x.desiredTimeNanos,
            .seamlessRequired = x.seamlessRequired,
    };
}

template <>
VsyncPeriodChangeTimeline translate(AidlVsyncPeriodChangeTimeline x) {
    return VsyncPeriodChangeTimeline{
            .newVsyncAppliedTimeNanos = x.newVsyncAppliedTimeNanos,
            .refreshRequired = x.refreshRequired,
            .refreshTimeNanos = x.refreshTimeNanos,
    };
}
mat4 makeMat4(std::vector<float> in) {
    return mat4(static_cast<const float*>(in.data()));
}

} // namespace

class AidlIComposerCallbackWrapper : public BnComposerCallback {
public:
    AidlIComposerCallbackWrapper(HWC2::ComposerCallback& callback) : mCallback(callback) {}

    ::ndk::ScopedAStatus onHotplug(int64_t in_display, bool in_connected) override {
        const auto connection = in_connected ? V2_4::IComposerCallback::Connection::CONNECTED
                                             : V2_4::IComposerCallback::Connection::DISCONNECTED;
        mCallback.onComposerHalHotplug(translate<Display>(in_display), connection);
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onRefresh(int64_t in_display) override {
        mCallback.onComposerHalRefresh(translate<Display>(in_display));
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onSeamlessPossible(int64_t in_display) override {
        mCallback.onComposerHalSeamlessPossible(translate<Display>(in_display));
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onVsync(int64_t in_display, int64_t in_timestamp,
                                 int32_t in_vsyncPeriodNanos) override {
        mCallback.onComposerHalVsync(translate<Display>(in_display), in_timestamp,
                                     static_cast<uint32_t>(in_vsyncPeriodNanos));
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onVsyncPeriodTimingChanged(
            int64_t in_display, const AidlVsyncPeriodChangeTimeline& in_updatedTimeline) override {
        mCallback.onComposerHalVsyncPeriodTimingChanged(translate<Display>(in_display),
                                                        translate<V2_4::VsyncPeriodChangeTimeline>(
                                                                in_updatedTimeline));
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus onVsyncIdle(int64_t in_display) override {
        mCallback.onComposerHalVsyncIdle(translate<Display>(in_display));
        return ::ndk::ScopedAStatus::ok();
    }

private:
    HWC2::ComposerCallback& mCallback;
};

std::string AidlComposer::instance(const std::string& serviceName) {
    return std::string(AidlIComposer::descriptor) + "/" + serviceName;
}

bool AidlComposer::isDeclared(const std::string& serviceName) {
    return AServiceManager_isDeclared(instance(serviceName).c_str());
}

AidlComposer::AidlComposer(const std::string& serviceName) {
    // This only waits if the service is actually declared
    mAidlComposer = AidlIComposer::fromBinder(
            ndk::SpAIBinder(AServiceManager_waitForService(instance(serviceName).c_str())));
    if (!mAidlComposer) {
        LOG_ALWAYS_FATAL("Failed to get AIDL composer service");
        return;
    }

    if (!mAidlComposer->createClient(&mAidlComposerClient).isOk()) {
        LOG_ALWAYS_FATAL("Can't create AidlComposerClient, fallback to HIDL");
        return;
    }

    ALOGI("Loaded AIDL composer3 HAL service");
}

AidlComposer::~AidlComposer() = default;

bool AidlComposer::isSupported(OptionalFeature feature) const {
    switch (feature) {
        case OptionalFeature::RefreshRateSwitching:
        case OptionalFeature::ExpectedPresentTime:
        case OptionalFeature::DisplayBrightnessCommand:
        case OptionalFeature::KernelIdleTimer:
        case OptionalFeature::PhysicalDisplayOrientation:
            return true;
    }
}

std::vector<Capability> AidlComposer::getCapabilities() {
    std::vector<Capability> capabilities;
    const auto status = mAidlComposer->getCapabilities(&capabilities);
    if (!status.isOk()) {
        ALOGE("getCapabilities failed %s", status.getDescription().c_str());
        return {};
    }
    return capabilities;
}

std::string AidlComposer::dumpDebugInfo() {
    int pipefds[2];
    int result = pipe(pipefds);
    if (result < 0) {
        ALOGE("dumpDebugInfo: pipe failed: %s", strerror(errno));
        return {};
    }

    std::string str;
    const auto status = mAidlComposer->dump(pipefds[1], /*args*/ nullptr, /*numArgs*/ 0);
    // Close the write-end of the pipe to make sure that when reading from the
    // read-end we will get eof instead of blocking forever
    close(pipefds[1]);

    if (status == STATUS_OK) {
        base::ReadFdToString(pipefds[0], &str);
    } else {
        ALOGE("dumpDebugInfo: dump failed: %d", status);
    }

    close(pipefds[0]);
    return str;
}

void AidlComposer::registerCallback(HWC2::ComposerCallback& callback) {
    if (mAidlComposerCallback) {
        ALOGE("Callback already registered");
    }

    mAidlComposerCallback = ndk::SharedRefBase::make<AidlIComposerCallbackWrapper>(callback);
    AIBinder_setMinSchedulerPolicy(mAidlComposerCallback->asBinder().get(), SCHED_FIFO, 2);

    const auto status = mAidlComposerClient->registerCallback(mAidlComposerCallback);
    if (!status.isOk()) {
        ALOGE("registerCallback failed %s", status.getDescription().c_str());
    }
}

void AidlComposer::resetCommands() {
    mWriter.reset();
}

Error AidlComposer::executeCommands() {
    return execute();
}

uint32_t AidlComposer::getMaxVirtualDisplayCount() {
    int32_t count = 0;
    const auto status = mAidlComposerClient->getMaxVirtualDisplayCount(&count);
    if (!status.isOk()) {
        ALOGE("getMaxVirtualDisplayCount failed %s", status.getDescription().c_str());
        return 0;
    }
    return static_cast<uint32_t>(count);
}

Error AidlComposer::createVirtualDisplay(uint32_t width, uint32_t height, PixelFormat* format,
                                         Display* outDisplay) {
    using AidlPixelFormat = aidl::android::hardware::graphics::common::PixelFormat;
    const int32_t bufferSlotCount = 1;
    VirtualDisplay virtualDisplay;
    const auto status =
            mAidlComposerClient->createVirtualDisplay(static_cast<int32_t>(width),
                                                      static_cast<int32_t>(height),
                                                      static_cast<AidlPixelFormat>(*format),
                                                      bufferSlotCount, &virtualDisplay);

    if (!status.isOk()) {
        ALOGE("createVirtualDisplay failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }

    *outDisplay = translate<Display>(virtualDisplay.display);
    *format = static_cast<PixelFormat>(virtualDisplay.format);
    return Error::NONE;
}

Error AidlComposer::destroyVirtualDisplay(Display display) {
    const auto status = mAidlComposerClient->destroyVirtualDisplay(translate<int64_t>(display));
    if (!status.isOk()) {
        ALOGE("destroyVirtualDisplay failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::acceptDisplayChanges(Display display) {
    mWriter.acceptDisplayChanges(translate<int64_t>(display));
    return Error::NONE;
}

Error AidlComposer::createLayer(Display display, Layer* outLayer) {
    int64_t layer;
    const auto status = mAidlComposerClient->createLayer(translate<int64_t>(display),
                                                         kMaxLayerBufferCount, &layer);
    if (!status.isOk()) {
        ALOGE("createLayer failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }

    *outLayer = translate<Layer>(layer);
    return Error::NONE;
}

Error AidlComposer::destroyLayer(Display display, Layer layer) {
    const auto status = mAidlComposerClient->destroyLayer(translate<int64_t>(display),
                                                          translate<int64_t>(layer));
    if (!status.isOk()) {
        ALOGE("destroyLayer failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::getActiveConfig(Display display, Config* outConfig) {
    int32_t config;
    const auto status = mAidlComposerClient->getActiveConfig(translate<int64_t>(display), &config);
    if (!status.isOk()) {
        ALOGE("getActiveConfig failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outConfig = translate<Config>(config);
    return Error::NONE;
}

Error AidlComposer::getChangedCompositionTypes(
        Display display, std::vector<Layer>* outLayers,
        std::vector<aidl::android::hardware::graphics::composer3::Composition>* outTypes) {
    const auto changedLayers = mReader.takeChangedCompositionTypes(translate<int64_t>(display));
    outLayers->reserve(changedLayers.size());
    outTypes->reserve(changedLayers.size());

    for (const auto& layer : changedLayers) {
        outLayers->emplace_back(translate<Layer>(layer.layer));
        outTypes->emplace_back(layer.composition);
    }
    return Error::NONE;
}

Error AidlComposer::getColorModes(Display display, std::vector<ColorMode>* outModes) {
    std::vector<AidlColorMode> modes;
    const auto status = mAidlComposerClient->getColorModes(translate<int64_t>(display), &modes);
    if (!status.isOk()) {
        ALOGE("getColorModes failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outModes = translate<ColorMode>(modes);
    return Error::NONE;
}

Error AidlComposer::getDisplayAttribute(Display display, Config config,
                                        IComposerClient::Attribute attribute, int32_t* outValue) {
    const auto status =
            mAidlComposerClient->getDisplayAttribute(translate<int64_t>(display),
                                                     translate<int32_t>(config),
                                                     static_cast<AidlDisplayAttribute>(attribute),
                                                     outValue);
    if (!status.isOk()) {
        ALOGE("getDisplayAttribute failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::getDisplayConfigs(Display display, std::vector<Config>* outConfigs) {
    std::vector<int32_t> configs;
    const auto status =
            mAidlComposerClient->getDisplayConfigs(translate<int64_t>(display), &configs);
    if (!status.isOk()) {
        ALOGE("getDisplayConfigs failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outConfigs = translate<Config>(configs);
    return Error::NONE;
}

Error AidlComposer::getDisplayName(Display display, std::string* outName) {
    const auto status = mAidlComposerClient->getDisplayName(translate<int64_t>(display), outName);
    if (!status.isOk()) {
        ALOGE("getDisplayName failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::getDisplayRequests(Display display, uint32_t* outDisplayRequestMask,
                                       std::vector<Layer>* outLayers,
                                       std::vector<uint32_t>* outLayerRequestMasks) {
    const auto displayRequests = mReader.takeDisplayRequests(translate<int64_t>(display));
    *outDisplayRequestMask = translate<uint32_t>(displayRequests.mask);
    outLayers->reserve(displayRequests.layerRequests.size());
    outLayerRequestMasks->reserve(displayRequests.layerRequests.size());

    for (const auto& layer : displayRequests.layerRequests) {
        outLayers->emplace_back(translate<Layer>(layer.layer));
        outLayerRequestMasks->emplace_back(translate<uint32_t>(layer.mask));
    }
    return Error::NONE;
}

Error AidlComposer::getDozeSupport(Display display, bool* outSupport) {
    std::vector<AidlDisplayCapability> capabilities;
    const auto status =
            mAidlComposerClient->getDisplayCapabilities(translate<int64_t>(display), &capabilities);
    if (!status.isOk()) {
        ALOGE("getDisplayCapabilities failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outSupport = std::find(capabilities.begin(), capabilities.end(),
                            AidlDisplayCapability::DOZE) != capabilities.end();
    return Error::NONE;
}

Error AidlComposer::hasDisplayIdleTimerCapability(Display display, bool* outSupport) {
    std::vector<AidlDisplayCapability> capabilities;
    const auto status =
            mAidlComposerClient->getDisplayCapabilities(translate<int64_t>(display), &capabilities);
    if (!status.isOk()) {
        ALOGE("getDisplayCapabilities failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outSupport = std::find(capabilities.begin(), capabilities.end(),
                            AidlDisplayCapability::DISPLAY_IDLE_TIMER) != capabilities.end();
    return Error::NONE;
}

Error AidlComposer::getHdrCapabilities(Display display, std::vector<Hdr>* outTypes,
                                       float* outMaxLuminance, float* outMaxAverageLuminance,
                                       float* outMinLuminance) {
    AidlHdrCapabilities capabilities;
    const auto status =
            mAidlComposerClient->getHdrCapabilities(translate<int64_t>(display), &capabilities);
    if (!status.isOk()) {
        ALOGE("getHdrCapabilities failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }

    *outTypes = translate<Hdr>(capabilities.types);
    *outMaxLuminance = capabilities.maxLuminance;
    *outMaxAverageLuminance = capabilities.maxAverageLuminance;
    *outMinLuminance = capabilities.minLuminance;
    return Error::NONE;
}

Error AidlComposer::getReleaseFences(Display display, std::vector<Layer>* outLayers,
                                     std::vector<int>* outReleaseFences) {
    auto fences = mReader.takeReleaseFences(translate<int64_t>(display));
    outLayers->reserve(fences.size());
    outReleaseFences->reserve(fences.size());

    for (auto& fence : fences) {
        outLayers->emplace_back(translate<Layer>(fence.layer));
        // take ownership
        const int fenceOwner = fence.fence.get();
        *fence.fence.getR() = -1;
        outReleaseFences->emplace_back(fenceOwner);
    }
    return Error::NONE;
}

Error AidlComposer::presentDisplay(Display display, int* outPresentFence) {
    ATRACE_NAME("HwcPresentDisplay");
    mWriter.presentDisplay(translate<int64_t>(display));

    Error error = execute();
    if (error != Error::NONE) {
        return error;
    }

    auto fence = mReader.takePresentFence(translate<int64_t>(display));
    // take ownership
    *outPresentFence = fence.get();
    *fence.getR() = -1;
    return Error::NONE;
}

Error AidlComposer::setActiveConfig(Display display, Config config) {
    const auto status = mAidlComposerClient->setActiveConfig(translate<int64_t>(display),
                                                             translate<int32_t>(config));
    if (!status.isOk()) {
        ALOGE("setActiveConfig failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::setClientTarget(Display display, uint32_t slot, const sp<GraphicBuffer>& target,
                                    int acquireFence, Dataspace dataspace,
                                    const std::vector<IComposerClient::Rect>& damage) {
    const native_handle_t* handle = nullptr;
    if (target.get()) {
        handle = target->getNativeBuffer()->handle;
    }

    mWriter.setClientTarget(translate<int64_t>(display), slot, handle, acquireFence,
                            translate<aidl::android::hardware::graphics::common::Dataspace>(
                                    dataspace),
                            translate<AidlRect>(damage));
    return Error::NONE;
}

Error AidlComposer::setColorMode(Display display, ColorMode mode, RenderIntent renderIntent) {
    const auto status =
            mAidlComposerClient->setColorMode(translate<int64_t>(display),
                                              translate<AidlColorMode>(mode),
                                              translate<AidlRenderIntent>(renderIntent));
    if (!status.isOk()) {
        ALOGE("setColorMode failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::setColorTransform(Display display, const float* matrix) {
    mWriter.setColorTransform(translate<int64_t>(display), matrix);
    return Error::NONE;
}

Error AidlComposer::setOutputBuffer(Display display, const native_handle_t* buffer,
                                    int releaseFence) {
    mWriter.setOutputBuffer(translate<int64_t>(display), 0, buffer, dup(releaseFence));
    return Error::NONE;
}

Error AidlComposer::setPowerMode(Display display, IComposerClient::PowerMode mode) {
    const auto status = mAidlComposerClient->setPowerMode(translate<int64_t>(display),
                                                          translate<PowerMode>(mode));
    if (!status.isOk()) {
        ALOGE("setPowerMode failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::setVsyncEnabled(Display display, IComposerClient::Vsync enabled) {
    const bool enableVsync = enabled == IComposerClient::Vsync::ENABLE;
    const auto status =
            mAidlComposerClient->setVsyncEnabled(translate<int64_t>(display), enableVsync);
    if (!status.isOk()) {
        ALOGE("setVsyncEnabled failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::setClientTargetSlotCount(Display display) {
    const int32_t bufferSlotCount = BufferQueue::NUM_BUFFER_SLOTS;
    const auto status = mAidlComposerClient->setClientTargetSlotCount(translate<int64_t>(display),
                                                                      bufferSlotCount);
    if (!status.isOk()) {
        ALOGE("setClientTargetSlotCount failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::validateDisplay(Display display, nsecs_t expectedPresentTime,
                                    uint32_t* outNumTypes, uint32_t* outNumRequests) {
    ATRACE_NAME("HwcValidateDisplay");
    mWriter.validateDisplay(translate<int64_t>(display),
                            ClockMonotonicTimestamp{expectedPresentTime});

    Error error = execute();
    if (error != Error::NONE) {
        return error;
    }

    mReader.hasChanges(translate<int64_t>(display), outNumTypes, outNumRequests);

    return Error::NONE;
}

Error AidlComposer::presentOrValidateDisplay(Display display, nsecs_t expectedPresentTime,
                                             uint32_t* outNumTypes, uint32_t* outNumRequests,
                                             int* outPresentFence, uint32_t* state) {
    ATRACE_NAME("HwcPresentOrValidateDisplay");
    mWriter.presentOrvalidateDisplay(translate<int64_t>(display),
                                     ClockMonotonicTimestamp{expectedPresentTime});

    Error error = execute();
    if (error != Error::NONE) {
        return error;
    }

    const auto result = mReader.takePresentOrValidateStage(translate<int64_t>(display));
    if (!result.has_value()) {
        *state = translate<uint32_t>(-1);
        return Error::NO_RESOURCES;
    }

    *state = translate<uint32_t>(*result);

    if (*result == PresentOrValidate::Result::Presented) {
        auto fence = mReader.takePresentFence(translate<int64_t>(display));
        // take ownership
        *outPresentFence = fence.get();
        *fence.getR() = -1;
    }

    if (*result == PresentOrValidate::Result::Validated) {
        mReader.hasChanges(translate<int64_t>(display), outNumTypes, outNumRequests);
    }

    return Error::NONE;
}

Error AidlComposer::setCursorPosition(Display display, Layer layer, int32_t x, int32_t y) {
    mWriter.setLayerCursorPosition(translate<int64_t>(display), translate<int64_t>(layer), x, y);
    return Error::NONE;
}

Error AidlComposer::setLayerBuffer(Display display, Layer layer, uint32_t slot,
                                   const sp<GraphicBuffer>& buffer, int acquireFence) {
    const native_handle_t* handle = nullptr;
    if (buffer.get()) {
        handle = buffer->getNativeBuffer()->handle;
    }

    mWriter.setLayerBuffer(translate<int64_t>(display), translate<int64_t>(layer), slot, handle,
                           acquireFence);
    return Error::NONE;
}

Error AidlComposer::setLayerSurfaceDamage(Display display, Layer layer,
                                          const std::vector<IComposerClient::Rect>& damage) {
    mWriter.setLayerSurfaceDamage(translate<int64_t>(display), translate<int64_t>(layer),
                                  translate<AidlRect>(damage));
    return Error::NONE;
}

Error AidlComposer::setLayerBlendMode(Display display, Layer layer,
                                      IComposerClient::BlendMode mode) {
    mWriter.setLayerBlendMode(translate<int64_t>(display), translate<int64_t>(layer),
                              translate<BlendMode>(mode));
    return Error::NONE;
}

Error AidlComposer::setLayerColor(Display display, Layer layer, const Color& color) {
    mWriter.setLayerColor(translate<int64_t>(display), translate<int64_t>(layer), color);
    return Error::NONE;
}

Error AidlComposer::setLayerCompositionType(
        Display display, Layer layer,
        aidl::android::hardware::graphics::composer3::Composition type) {
    mWriter.setLayerCompositionType(translate<int64_t>(display), translate<int64_t>(layer), type);
    return Error::NONE;
}

Error AidlComposer::setLayerDataspace(Display display, Layer layer, Dataspace dataspace) {
    mWriter.setLayerDataspace(translate<int64_t>(display), translate<int64_t>(layer),
                              translate<AidlDataspace>(dataspace));
    return Error::NONE;
}

Error AidlComposer::setLayerDisplayFrame(Display display, Layer layer,
                                         const IComposerClient::Rect& frame) {
    mWriter.setLayerDisplayFrame(translate<int64_t>(display), translate<int64_t>(layer),
                                 translate<AidlRect>(frame));
    return Error::NONE;
}

Error AidlComposer::setLayerPlaneAlpha(Display display, Layer layer, float alpha) {
    mWriter.setLayerPlaneAlpha(translate<int64_t>(display), translate<int64_t>(layer), alpha);
    return Error::NONE;
}

Error AidlComposer::setLayerSidebandStream(Display display, Layer layer,
                                           const native_handle_t* stream) {
    mWriter.setLayerSidebandStream(translate<int64_t>(display), translate<int64_t>(layer), stream);
    return Error::NONE;
}

Error AidlComposer::setLayerSourceCrop(Display display, Layer layer,
                                       const IComposerClient::FRect& crop) {
    mWriter.setLayerSourceCrop(translate<int64_t>(display), translate<int64_t>(layer),
                               translate<AidlFRect>(crop));
    return Error::NONE;
}

Error AidlComposer::setLayerTransform(Display display, Layer layer, Transform transform) {
    mWriter.setLayerTransform(translate<int64_t>(display), translate<int64_t>(layer),
                              translate<AidlTransform>(transform));
    return Error::NONE;
}

Error AidlComposer::setLayerVisibleRegion(Display display, Layer layer,
                                          const std::vector<IComposerClient::Rect>& visible) {
    mWriter.setLayerVisibleRegion(translate<int64_t>(display), translate<int64_t>(layer),
                                  translate<AidlRect>(visible));
    return Error::NONE;
}

Error AidlComposer::setLayerZOrder(Display display, Layer layer, uint32_t z) {
    mWriter.setLayerZOrder(translate<int64_t>(display), translate<int64_t>(layer), z);
    return Error::NONE;
}

Error AidlComposer::execute() {
    const auto& commands = mWriter.getPendingCommands();
    if (commands.empty()) {
        mWriter.reset();
        return Error::NONE;
    }

    { // scope for results
        std::vector<CommandResultPayload> results;
        auto status = mAidlComposerClient->executeCommands(commands, &results);
        if (!status.isOk()) {
            ALOGE("executeCommands failed %s", status.getDescription().c_str());
            return static_cast<Error>(status.getServiceSpecificError());
        }

        mReader.parse(std::move(results));
    }
    const auto commandErrors = mReader.takeErrors();
    Error error = Error::NONE;
    for (const auto& cmdErr : commandErrors) {
        const auto index = static_cast<size_t>(cmdErr.commandIndex);
        if (index < 0 || index >= commands.size()) {
            ALOGE("invalid command index %zu", index);
            return Error::BAD_PARAMETER;
        }

        const auto& command = commands[index];
        if (command.validateDisplay || command.presentDisplay || command.presentOrValidateDisplay) {
            error = translate<Error>(cmdErr.errorCode);
        } else {
            ALOGW("command '%s' generated error %" PRId32, command.toString().c_str(),
                  cmdErr.errorCode);
        }
    }

    mWriter.reset();

    return error;
}

Error AidlComposer::setLayerPerFrameMetadata(
        Display display, Layer layer,
        const std::vector<IComposerClient::PerFrameMetadata>& perFrameMetadatas) {
    mWriter.setLayerPerFrameMetadata(translate<int64_t>(display), translate<int64_t>(layer),
                                     translate<AidlPerFrameMetadata>(perFrameMetadatas));
    return Error::NONE;
}

std::vector<IComposerClient::PerFrameMetadataKey> AidlComposer::getPerFrameMetadataKeys(
        Display display) {
    std::vector<AidlPerFrameMetadataKey> keys;
    const auto status =
            mAidlComposerClient->getPerFrameMetadataKeys(translate<int64_t>(display), &keys);
    if (!status.isOk()) {
        ALOGE("getPerFrameMetadataKeys failed %s", status.getDescription().c_str());
        return {};
    }
    return translate<IComposerClient::PerFrameMetadataKey>(keys);
}

Error AidlComposer::getRenderIntents(Display display, ColorMode colorMode,
                                     std::vector<RenderIntent>* outRenderIntents) {
    std::vector<AidlRenderIntent> renderIntents;
    const auto status = mAidlComposerClient->getRenderIntents(translate<int64_t>(display),
                                                              translate<AidlColorMode>(colorMode),
                                                              &renderIntents);
    if (!status.isOk()) {
        ALOGE("getRenderIntents failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outRenderIntents = translate<RenderIntent>(renderIntents);
    return Error::NONE;
}

Error AidlComposer::getDataspaceSaturationMatrix(Dataspace dataspace, mat4* outMatrix) {
    std::vector<float> matrix;
    const auto status =
            mAidlComposerClient->getDataspaceSaturationMatrix(translate<AidlDataspace>(dataspace),
                                                              &matrix);
    if (!status.isOk()) {
        ALOGE("getDataspaceSaturationMatrix failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outMatrix = makeMat4(matrix);
    return Error::NONE;
}

Error AidlComposer::getDisplayIdentificationData(Display display, uint8_t* outPort,
                                                 std::vector<uint8_t>* outData) {
    AidlDisplayIdentification displayIdentification;
    const auto status =
            mAidlComposerClient->getDisplayIdentificationData(translate<int64_t>(display),
                                                              &displayIdentification);
    if (!status.isOk()) {
        ALOGE("getDisplayIdentificationData failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }

    *outPort = static_cast<uint8_t>(displayIdentification.port);
    *outData = displayIdentification.data;

    return Error::NONE;
}

Error AidlComposer::setLayerColorTransform(Display display, Layer layer, const float* matrix) {
    mWriter.setLayerColorTransform(translate<int64_t>(display), translate<int64_t>(layer), matrix);
    return Error::NONE;
}

Error AidlComposer::getDisplayedContentSamplingAttributes(Display display, PixelFormat* outFormat,
                                                          Dataspace* outDataspace,
                                                          uint8_t* outComponentMask) {
    if (!outFormat || !outDataspace || !outComponentMask) {
        return Error::BAD_PARAMETER;
    }

    AidlDisplayContentSamplingAttributes attributes;
    const auto status =
            mAidlComposerClient->getDisplayedContentSamplingAttributes(translate<int64_t>(display),
                                                                       &attributes);
    if (!status.isOk()) {
        ALOGE("getDisplayedContentSamplingAttributes failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }

    *outFormat = translate<PixelFormat>(attributes.format);
    *outDataspace = translate<Dataspace>(attributes.dataspace);
    *outComponentMask = static_cast<uint8_t>(attributes.componentMask);
    return Error::NONE;
}

Error AidlComposer::setDisplayContentSamplingEnabled(Display display, bool enabled,
                                                     uint8_t componentMask, uint64_t maxFrames) {
    const auto status =
            mAidlComposerClient
                    ->setDisplayedContentSamplingEnabled(translate<int64_t>(display), enabled,
                                                         static_cast<AidlFormatColorComponent>(
                                                                 componentMask),
                                                         static_cast<int64_t>(maxFrames));
    if (!status.isOk()) {
        ALOGE("setDisplayedContentSamplingEnabled failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::getDisplayedContentSample(Display display, uint64_t maxFrames,
                                              uint64_t timestamp, DisplayedFrameStats* outStats) {
    if (!outStats) {
        return Error::BAD_PARAMETER;
    }

    AidlDisplayContentSample sample;
    const auto status =
            mAidlComposerClient->getDisplayedContentSample(translate<int64_t>(display),
                                                           static_cast<int64_t>(maxFrames),
                                                           static_cast<int64_t>(timestamp),
                                                           &sample);
    if (!status.isOk()) {
        ALOGE("getDisplayedContentSample failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *outStats = translate<DisplayedFrameStats>(sample);
    return Error::NONE;
}

Error AidlComposer::setLayerPerFrameMetadataBlobs(
        Display display, Layer layer,
        const std::vector<IComposerClient::PerFrameMetadataBlob>& metadata) {
    mWriter.setLayerPerFrameMetadataBlobs(translate<int64_t>(display), translate<int64_t>(layer),
                                          translate<AidlPerFrameMetadataBlob>(metadata));
    return Error::NONE;
}

Error AidlComposer::setDisplayBrightness(Display display, float brightness, float brightnessNits,
                                         const DisplayBrightnessOptions& options) {
    mWriter.setDisplayBrightness(translate<int64_t>(display), brightness, brightnessNits);

    if (options.applyImmediately) {
        return execute();
    }

    return Error::NONE;
}

Error AidlComposer::getDisplayCapabilities(Display display,
                                           std::vector<AidlDisplayCapability>* outCapabilities) {
    const auto status = mAidlComposerClient->getDisplayCapabilities(translate<int64_t>(display),
                                                                    outCapabilities);
    if (!status.isOk()) {
        ALOGE("getDisplayCapabilities failed %s", status.getDescription().c_str());
        outCapabilities->clear();
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

V2_4::Error AidlComposer::getDisplayConnectionType(
        Display display, IComposerClient::DisplayConnectionType* outType) {
    AidlDisplayConnectionType type;
    const auto status =
            mAidlComposerClient->getDisplayConnectionType(translate<int64_t>(display), &type);
    if (!status.isOk()) {
        ALOGE("getDisplayConnectionType failed %s", status.getDescription().c_str());
        return static_cast<V2_4::Error>(status.getServiceSpecificError());
    }
    *outType = translate<IComposerClient::DisplayConnectionType>(type);
    return V2_4::Error::NONE;
}

V2_4::Error AidlComposer::getDisplayVsyncPeriod(Display display, VsyncPeriodNanos* outVsyncPeriod) {
    int32_t vsyncPeriod;
    const auto status =
            mAidlComposerClient->getDisplayVsyncPeriod(translate<int64_t>(display), &vsyncPeriod);
    if (!status.isOk()) {
        ALOGE("getDisplayVsyncPeriod failed %s", status.getDescription().c_str());
        return static_cast<V2_4::Error>(status.getServiceSpecificError());
    }
    *outVsyncPeriod = translate<VsyncPeriodNanos>(vsyncPeriod);
    return V2_4::Error::NONE;
}

V2_4::Error AidlComposer::setActiveConfigWithConstraints(
        Display display, Config config,
        const IComposerClient::VsyncPeriodChangeConstraints& vsyncPeriodChangeConstraints,
        VsyncPeriodChangeTimeline* outTimeline) {
    AidlVsyncPeriodChangeTimeline timeline;
    const auto status =
            mAidlComposerClient
                    ->setActiveConfigWithConstraints(translate<int64_t>(display),
                                                     translate<int32_t>(config),
                                                     translate<AidlVsyncPeriodChangeConstraints>(
                                                             vsyncPeriodChangeConstraints),
                                                     &timeline);
    if (!status.isOk()) {
        ALOGE("setActiveConfigWithConstraints failed %s", status.getDescription().c_str());
        return static_cast<V2_4::Error>(status.getServiceSpecificError());
    }
    *outTimeline = translate<VsyncPeriodChangeTimeline>(timeline);
    return V2_4::Error::NONE;
}

V2_4::Error AidlComposer::setAutoLowLatencyMode(Display display, bool on) {
    const auto status = mAidlComposerClient->setAutoLowLatencyMode(translate<int64_t>(display), on);
    if (!status.isOk()) {
        ALOGE("setAutoLowLatencyMode failed %s", status.getDescription().c_str());
        return static_cast<V2_4::Error>(status.getServiceSpecificError());
    }
    return V2_4::Error::NONE;
}

V2_4::Error AidlComposer::getSupportedContentTypes(
        Display displayId, std::vector<IComposerClient::ContentType>* outSupportedContentTypes) {
    std::vector<AidlContentType> types;
    const auto status =
            mAidlComposerClient->getSupportedContentTypes(translate<int64_t>(displayId), &types);
    if (!status.isOk()) {
        ALOGE("getSupportedContentTypes failed %s", status.getDescription().c_str());
        return static_cast<V2_4::Error>(status.getServiceSpecificError());
    }
    *outSupportedContentTypes = translate<IComposerClient::ContentType>(types);
    return V2_4::Error::NONE;
}

V2_4::Error AidlComposer::setContentType(Display display,
                                         IComposerClient::ContentType contentType) {
    const auto status =
            mAidlComposerClient->setContentType(translate<int64_t>(display),
                                                translate<AidlContentType>(contentType));
    if (!status.isOk()) {
        ALOGE("setContentType failed %s", status.getDescription().c_str());
        return static_cast<V2_4::Error>(status.getServiceSpecificError());
    }
    return V2_4::Error::NONE;
}

V2_4::Error AidlComposer::setLayerGenericMetadata(Display, Layer, const std::string&, bool,
                                                  const std::vector<uint8_t>&) {
    // There are no users for this API. See b/209691612.
    return V2_4::Error::UNSUPPORTED;
}

V2_4::Error AidlComposer::getLayerGenericMetadataKeys(
        std::vector<IComposerClient::LayerGenericMetadataKey>*) {
    // There are no users for this API. See b/209691612.
    return V2_4::Error::UNSUPPORTED;
}

Error AidlComposer::setBootDisplayConfig(Display display, Config config) {
    const auto status = mAidlComposerClient->setBootDisplayConfig(translate<int64_t>(display),
                                                                  translate<int32_t>(config));
    if (!status.isOk()) {
        ALOGE("setBootDisplayConfig failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::clearBootDisplayConfig(Display display) {
    const auto status = mAidlComposerClient->clearBootDisplayConfig(translate<int64_t>(display));
    if (!status.isOk()) {
        ALOGE("clearBootDisplayConfig failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::getPreferredBootDisplayConfig(Display display, Config* config) {
    int32_t displayConfig;
    const auto status =
            mAidlComposerClient->getPreferredBootDisplayConfig(translate<int64_t>(display),
                                                               &displayConfig);
    if (!status.isOk()) {
        ALOGE("getPreferredBootDisplayConfig failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    *config = translate<uint32_t>(displayConfig);
    return Error::NONE;
}

Error AidlComposer::getClientTargetProperty(
        Display display, ClientTargetPropertyWithBrightness* outClientTargetProperty) {
    *outClientTargetProperty = mReader.takeClientTargetProperty(translate<int64_t>(display));
    return Error::NONE;
}

Error AidlComposer::setLayerBrightness(Display display, Layer layer, float brightness) {
    mWriter.setLayerBrightness(translate<int64_t>(display), translate<int64_t>(layer), brightness);
    return Error::NONE;
}

Error AidlComposer::setLayerBlockingRegion(Display display, Layer layer,
                                           const std::vector<IComposerClient::Rect>& blocking) {
    mWriter.setLayerBlockingRegion(translate<int64_t>(display), translate<int64_t>(layer),
                                   translate<AidlRect>(blocking));
    return Error::NONE;
}

Error AidlComposer::getDisplayDecorationSupport(Display display,
                                                std::optional<DisplayDecorationSupport>* support) {
    const auto status =
            mAidlComposerClient->getDisplayDecorationSupport(translate<int64_t>(display), support);
    if (!status.isOk()) {
        ALOGE("getDisplayDecorationSupport failed %s", status.getDescription().c_str());
        support->reset();
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::setIdleTimerEnabled(Display displayId, std::chrono::milliseconds timeout) {
    const auto status =
            mAidlComposerClient->setIdleTimerEnabled(translate<int64_t>(displayId),
                                                     translate<int32_t>(timeout.count()));
    if (!status.isOk()) {
        ALOGE("setIdleTimerEnabled failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

Error AidlComposer::getPhysicalDisplayOrientation(Display displayId,
                                                  AidlTransform* outDisplayOrientation) {
    const auto status =
            mAidlComposerClient->getDisplayPhysicalOrientation(translate<int64_t>(displayId),
                                                               outDisplayOrientation);
    if (!status.isOk()) {
        ALOGE("getPhysicalDisplayOrientation failed %s", status.getDescription().c_str());
        return static_cast<Error>(status.getServiceSpecificError());
    }
    return Error::NONE;
}

} // namespace Hwc2
} // namespace android
