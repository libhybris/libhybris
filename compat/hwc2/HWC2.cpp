/*
 * Copyright 2015 The Android Open Source Project
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

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"

// #define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "HWC2"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "HWC2.h"

#include <android/configuration.h>
#include <ui/Fence.h>
#include <ui/FloatRect.h>
#include <ui/GraphicBuffer.h>

#include <algorithm>
#include <cinttypes>
#include <iterator>
#include <set>

#if ANDROID_VERSION_MAJOR >= 13
using aidl::android::hardware::graphics::composer3::Color;
using aidl::android::hardware::graphics::composer3::Composition;
using AidlCapability = aidl::android::hardware::graphics::composer3::Capability;
using aidl::android::hardware::graphics::composer3::DisplayCapability;
#endif

namespace android {

using android::Fence;
using android::FloatRect;
using android::GraphicBuffer;
using android::HdrCapabilities;
using android::HdrMetadata;
using android::Rect;
using android::Region;
using android::sp;
using android::hardware::Return;
using android::hardware::Void;

namespace HWC2 {

using namespace android::hardware::graphics::composer::hal;

namespace Hwc2 = android::Hwc2;

namespace {

inline bool hasMetadataKey(const std::set<Hwc2::PerFrameMetadataKey>& keys,
                           const Hwc2::PerFrameMetadataKey& key) {
    return keys.find(key) != keys.end();
}

} // namespace anonymous

// Device methods

Device::Device(std::unique_ptr<android::Hwc2::Composer> composer) : mComposer(std::move(composer)) {
    loadCapabilities();
}

void Device::registerCallback(HWC2::ComposerCallback& callback) {
    if (mRegisteredCallback) {
        ALOGW("Callback already registered. Ignored extra registration "
                "attempt.");
        return;
    }
    mComposer->registerCallback(callback);
}

// Required by HWC2 device

std::string Device::dump() const
{
    return mComposer->dumpDebugInfo();
}

uint32_t Device::getMaxVirtualDisplayCount() const
{
    return mComposer->getMaxVirtualDisplayCount();
}

Error Device::getDisplayIdentificationData(hal::HWDisplayId hwcDisplayId, uint8_t* outPort,
                                           std::vector<uint8_t>* outData) const {
    auto intError = mComposer->getDisplayIdentificationData(hwcDisplayId, outPort, outData);
    return static_cast<Error>(intError);
}

Error Device::createVirtualDisplay(uint32_t width, uint32_t height,
        hal::PixelFormat* format, Display** outDisplay)
{
    ALOGI("Creating virtual display");

    hal::HWDisplayId displayId = 0;
    auto intError = mComposer->createVirtualDisplay(width, height,
            format, &displayId);
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        return error;
    }

    auto display = std::make_unique<impl::Display>(*mComposer.get(), mCapabilities, displayId,
                                                   hal::DisplayType::VIRTUAL);
    display->setConnected(true);
    *outDisplay = display.get();
    mDisplays.emplace(displayId, std::move(display));
    ALOGI("Created virtual display");
    return Error::NONE;
}

void Device::destroyDisplay(hal::HWDisplayId displayId)
{
    ALOGI("Destroying display %" PRIu64, displayId);
    mDisplays.erase(displayId);
}

void Device::onHotplug(hal::HWDisplayId displayId, hal::Connection connection) {
    if (connection == hal::Connection::CONNECTED) {
        // If we get a hotplug connected event for a display we already have,
        // destroy the display and recreate it. This will force us to requery
        // the display params and recreate all layers on that display.
        auto oldDisplay = getDisplayById(displayId);
        if (oldDisplay != nullptr && oldDisplay->isConnected()) {
            ALOGI("Hotplug connecting an already connected display."
                    " Clearing old display state.");
        }
        mDisplays.erase(displayId);

        auto newDisplay = std::make_unique<impl::Display>(*mComposer.get(), mCapabilities,
                                                          displayId, hal::DisplayType::PHYSICAL);
        newDisplay->setConnected(true);
        mDisplays.emplace(displayId, std::move(newDisplay));
    } else if (connection == hal::Connection::DISCONNECTED) {
        // The display will later be destroyed by a call to
        // destroyDisplay(). For now we just mark it disconnected.
        auto display = getDisplayById(displayId);
        if (display) {
            display->setConnected(false);
        } else {
            ALOGW("Attempted to disconnect unknown display %" PRIu64,
                  displayId);
        }
    }
}

// Other Device methods

Display* Device::getDisplayById(hal::HWDisplayId id) {
    auto iter = mDisplays.find(id);
    return iter == mDisplays.end() ? nullptr : iter->second.get();
}

// Device initialization methods

void Device::loadCapabilities()
{
    static_assert(sizeof(hal::Capability) == sizeof(int32_t),
            "Capability size has changed");
    auto capabilities = mComposer->getCapabilities();
    for (auto capability : capabilities) {
        mCapabilities.emplace(capability);
    }
}

Error Device::flushCommands()
{
    return static_cast<Error>(mComposer->executeCommands());
}

// Display methods
Display::~Display() = default;

Display::Config::Config(Display& display, HWConfigId id)
      : mDisplay(display),
        mId(id),
        mWidth(-1),
        mHeight(-1),
        mVsyncPeriod(-1),
        mDpiX(-1),
        mDpiY(-1) {}

Display::Config::Builder::Builder(Display& display, HWConfigId id)
      : mConfig(new Config(display, id)) {}

float Display::Config::Builder::getDefaultDensity() {
    // Default density is based on TVs: 1080p displays get XHIGH density, lower-
    // resolution displays get TV density. Maybe eventually we'll need to update
    // it for 4k displays, though hopefully those will just report accurate DPI
    // information to begin with. This is also used for virtual displays and
    // older HWC implementations, so be careful about orientation.

    auto longDimension = std::max(mConfig->mWidth, mConfig->mHeight);
    if (longDimension >= 1080) {
        return ACONFIGURATION_DENSITY_XHIGH;
    } else {
        return ACONFIGURATION_DENSITY_TV;
    }
}

namespace impl {

Display::Display(android::Hwc2::Composer& composer,
                 const std::unordered_set<hal::Capability>& capabilities, HWDisplayId id,
                 DisplayType type)
      : mComposer(composer), mCapabilities(capabilities), mId(id), mType(type) {
    ALOGV("Created display %" PRIu64, id);
}

Display::~Display() {
    // Note: The calls to onOwningDisplayDestroyed() are allowed (and expected)
    // to call Display::onLayerDestroyed(). As that call removes entries from
    // mLayers, we do not want to have a for loop directly over it here. Since
    // the end goal is an empty mLayers anyway, we just go ahead and swap an
    // initially empty local container with mLayers, and then enumerate
    // the contents of the local container.
    Layers destroyingLayers;
    std::swap(mLayers, destroyingLayers);
    for (const auto& [_, weakLayer] : destroyingLayers) {
        if (std::shared_ptr layer = weakLayer.lock()) {
            layer->onOwningDisplayDestroyed();
        }
    }

    Error error = Error::NONE;
    const char* msg;
    switch (mType) {
        case DisplayType::PHYSICAL:
            error = setVsyncEnabled(HWC2::Vsync::DISABLE);
            msg = "disable VSYNC for";
            break;

        case DisplayType::VIRTUAL:
            error = static_cast<Error>(mComposer.destroyVirtualDisplay(mId));
            msg = "destroy virtual";
            break;

        case DisplayType::INVALID: // Used in unit tests.
            break;
    }

    ALOGE_IF(error != Error::NONE, "%s: Failed to %s display %" PRIu64 ": %d", __FUNCTION__, msg,
             mId, static_cast<int32_t>(error));

    ALOGV("Destroyed display %" PRIu64, mId);
}

// Required by HWC2 display
Error Display::acceptChanges()
{
    auto intError = mComposer.acceptDisplayChanges(mId);
    return static_cast<Error>(intError);
}

Error Display::createLayer(std::shared_ptr<HWC2::Layer>* outLayer) {
    HWLayerId layerId = 0;
    auto intError = mComposer.createLayer(mId, &layerId);
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        return error;
    }

    auto layer = std::make_shared<impl::Layer>(mComposer, mCapabilities, *this, layerId);
    *outLayer = layer;
    mLayers.emplace(layerId, layer);
    return Error::NONE;
}

void Display::onLayerDestroyed(hal::HWLayerId layerId) {
    mLayers.erase(layerId);
}

Error Display::getActiveConfig(
        std::shared_ptr<const Display::Config>* outConfig) const
{
    ALOGV("[%" PRIu64 "] getActiveConfig", mId);
    HWConfigId configId = 0;
    auto intError = mComposer.getActiveConfig(mId, &configId);
    auto error = static_cast<Error>(intError);

    if (error != Error::NONE) {
        ALOGE("Unable to get active config for mId:[%" PRIu64 "]", mId);
        *outConfig = nullptr;
        return error;
    }

    if (mConfigs.count(configId) != 0) {
        *outConfig = mConfigs.at(configId);
    } else {
        ALOGE("[%" PRIu64 "] getActiveConfig returned unknown config %u", mId,
                configId);
        // Return no error, but the caller needs to check for a null pointer to
        // detect this case
        *outConfig = nullptr;
    }

    return Error::NONE;
}

bool Display::isVsyncPeriodSwitchSupported() const {
    ALOGV("[%" PRIu64 "] isVsyncPeriodSwitchSupported()", mId);

    return mComposer.isSupported(android::Hwc2::Composer::OptionalFeature::RefreshRateSwitching);
}

Error Display::getDisplayVsyncPeriod(nsecs_t* outVsyncPeriod) const {
    ALOGV("[%" PRIu64 "] getDisplayVsyncPeriod", mId);

    Error error;

    if (isVsyncPeriodSwitchSupported()) {
        Hwc2::VsyncPeriodNanos vsyncPeriodNanos = 0;
        auto intError = mComposer.getDisplayVsyncPeriod(mId, &vsyncPeriodNanos);
        error = static_cast<Error>(intError);
        *outVsyncPeriod = static_cast<nsecs_t>(vsyncPeriodNanos);
    } else {
        // Get the default vsync period
        std::shared_ptr<const Display::Config> config;
        error = getActiveConfig(&config);
        if (error != Error::NONE) {
            return error;
        }
        if (!config) {
            // HWC has updated the display modes and hasn't notified us yet.
            return Error::BAD_CONFIG;
        }

        *outVsyncPeriod = config->getVsyncPeriod();
    }

    return error;
}

Error Display::getActiveConfigIndex(int* outIndex) const {
    ALOGV("[%" PRIu64 "] getActiveConfigIndex", mId);
    HWConfigId configId = 0;
    auto intError = mComposer.getActiveConfig(mId, &configId);
    auto error = static_cast<Error>(intError);

    if (error != Error::NONE) {
        ALOGE("Unable to get active config for mId:[%" PRIu64 "]", mId);
        *outIndex = -1;
        return error;
    }

    auto pos = mConfigs.find(configId);
    if (pos != mConfigs.end()) {
        *outIndex = std::distance(mConfigs.begin(), pos);
        ALOGV("[%" PRIu64 "] index = %d", mId, *outIndex);
    } else {
        ALOGE("[%" PRIu64 "] getActiveConfig returned unknown config %u", mId, configId);
        // Return no error, but the caller needs to check for a negative index
        // to detect this case
        *outIndex = -1;
    }

    return Error::NONE;
}

bool Display::hasDisplayIdleTimerCapability() const {
    bool isCapabilitySupported = false;
    return mComposer.hasDisplayIdleTimerCapability(mId, &isCapabilitySupported) == Error::NONE &&
            isCapabilitySupported;
}

#if ANDROID_VERSION_MAJOR >= 13
Error Display::getPhysicalDisplayOrientation(Hwc2::AidlTransform* outTransform) const {
    auto error = mComposer.getPhysicalDisplayOrientation(mId, outTransform);
    return static_cast<Error>(error);
}
#endif

Error Display::getChangedCompositionTypes(std::unordered_map<HWC2::Layer*, Composition>* outTypes) {
    std::vector<Hwc2::Layer> layerIds;
    std::vector<Composition> types;
    auto intError = mComposer.getChangedCompositionTypes(
            mId, &layerIds, &types);
    uint32_t numElements = layerIds.size();
    auto error = static_cast<Error>(intError);
    error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        return error;
    }

    outTypes->clear();
    outTypes->reserve(numElements);
    for (uint32_t element = 0; element < numElements; ++element) {
        auto layer = getLayerById(layerIds[element]);
        if (layer) {
            auto type = types[element];
            ALOGV("getChangedCompositionTypes: adding %" PRIu64 " %s",
                    layer->getId(), to_string(type).c_str());
            outTypes->emplace(layer.get(), type);
        } else {
            ALOGE("getChangedCompositionTypes: invalid layer %" PRIu64 " found"
                    " on display %" PRIu64, layerIds[element], mId);
        }
    }

    return Error::NONE;
}

Error Display::getColorModes(std::vector<ColorMode>* outModes) const
{
    auto intError = mComposer.getColorModes(mId, outModes);
    return static_cast<Error>(intError);
}

int32_t Display::getSupportedPerFrameMetadata() const
{
    int32_t supportedPerFrameMetadata = 0;

    std::vector<Hwc2::PerFrameMetadataKey> tmpKeys = mComposer.getPerFrameMetadataKeys(mId);
    std::set<Hwc2::PerFrameMetadataKey> keys(tmpKeys.begin(), tmpKeys.end());

    // Check whether a specific metadata type is supported. A metadata type is considered
    // supported if and only if all required fields are supported.

    // SMPTE2086
    if (hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::WHITE_POINT_X) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::WHITE_POINT_Y) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::MAX_LUMINANCE) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::MIN_LUMINANCE)) {
        supportedPerFrameMetadata |= HdrMetadata::Type::SMPTE2086;
    }
    // CTA861_3
    if (hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL) &&
        hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL)) {
        supportedPerFrameMetadata |= HdrMetadata::Type::CTA861_3;
    }

    // HDR10PLUS
    if (hasMetadataKey(keys, Hwc2::PerFrameMetadataKey::HDR10_PLUS_SEI)) {
        supportedPerFrameMetadata |= HdrMetadata::Type::HDR10PLUS;
    }

    return supportedPerFrameMetadata;
}

Error Display::getRenderIntents(ColorMode colorMode,
        std::vector<RenderIntent>* outRenderIntents) const
{
    auto intError = mComposer.getRenderIntents(mId, colorMode, outRenderIntents);
    return static_cast<Error>(intError);
}

Error Display::getDataspaceSaturationMatrix(Dataspace dataspace, android::mat4* outMatrix)
{
    auto intError = mComposer.getDataspaceSaturationMatrix(dataspace, outMatrix);
    return static_cast<Error>(intError);
}

std::vector<std::shared_ptr<const Display::Config>> Display::getConfigs() const
{
    std::vector<std::shared_ptr<const Config>> configs;
    for (const auto& element : mConfigs) {
        configs.emplace_back(element.second);
    }
    return configs;
}

Error Display::getName(std::string* outName) const
{
    auto intError = mComposer.getDisplayName(mId, outName);
    return static_cast<Error>(intError);
}

Error Display::getRequests(HWC2::DisplayRequest* outDisplayRequests,
                           std::unordered_map<HWC2::Layer*, LayerRequest>* outLayerRequests) {
    uint32_t intDisplayRequests = 0;
    std::vector<Hwc2::Layer> layerIds;
    std::vector<uint32_t> layerRequests;
    auto intError = mComposer.getDisplayRequests(
            mId, &intDisplayRequests, &layerIds, &layerRequests);
    uint32_t numElements = layerIds.size();
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        return error;
    }

    *outDisplayRequests = static_cast<DisplayRequest>(intDisplayRequests);
    outLayerRequests->clear();
    outLayerRequests->reserve(numElements);
    for (uint32_t element = 0; element < numElements; ++element) {
        auto layer = getLayerById(layerIds[element]);
        if (layer) {
            auto layerRequest =
                    static_cast<LayerRequest>(layerRequests[element]);
            outLayerRequests->emplace(layer.get(), layerRequest);
        } else {
            ALOGE("getRequests: invalid layer %" PRIu64 " found on display %"
                    PRIu64, layerIds[element], mId);
        }
    }

    return Error::NONE;
}

Error Display::getConnectionType(ui::DisplayConnectionType* outType) const {
    if (mType != DisplayType::PHYSICAL) return Error::BAD_DISPLAY;

    using ConnectionType = Hwc2::IComposerClient::DisplayConnectionType;
    ConnectionType connectionType;
    const auto error = static_cast<Error>(mComposer.getDisplayConnectionType(mId, &connectionType));
    if (error != Error::NONE) {
        return error;
    }

    *outType = connectionType == ConnectionType::INTERNAL ? ui::DisplayConnectionType::Internal
                                                          : ui::DisplayConnectionType::External;
    return Error::NONE;
}

bool Display::hasCapability(DisplayCapability capability) const {
    std::scoped_lock lock(mDisplayCapabilitiesMutex);
    if (mDisplayCapabilities) {
        return mDisplayCapabilities->count(capability) > 0;
    }

    ALOGW("Can't query capability %s."
          " Display Capabilities were not queried from HWC yet",
          to_string(capability).c_str());

    return false;
}

Error Display::supportsDoze(bool* outSupport) const {
    *outSupport = hasCapability(DisplayCapability::DOZE);
    return Error::NONE;
}

Error Display::getHdrCapabilities(HdrCapabilities* outCapabilities) const
{
    float maxLuminance = -1.0f;
    float maxAverageLuminance = -1.0f;
    float minLuminance = -1.0f;
    std::vector<Hwc2::Hdr> types;
    auto intError = mComposer.getHdrCapabilities(mId, &types,
            &maxLuminance, &maxAverageLuminance, &minLuminance);
    auto error = static_cast<HWC2::Error>(intError);

    if (error != Error::NONE) {
        return error;
    }

    *outCapabilities = HdrCapabilities(std::move(types),
            maxLuminance, maxAverageLuminance, minLuminance);
    return Error::NONE;
}

Error Display::getDisplayedContentSamplingAttributes(hal::PixelFormat* outFormat,
                                                     Dataspace* outDataspace,
                                                     uint8_t* outComponentMask) const {
    auto intError = mComposer.getDisplayedContentSamplingAttributes(mId, outFormat, outDataspace,
                                                                    outComponentMask);
    return static_cast<Error>(intError);
}

Error Display::setDisplayContentSamplingEnabled(bool enabled, uint8_t componentMask,
                                                uint64_t maxFrames) const {
    auto intError =
            mComposer.setDisplayContentSamplingEnabled(mId, enabled, componentMask, maxFrames);
    return static_cast<Error>(intError);
}

Error Display::getDisplayedContentSample(uint64_t maxFrames, uint64_t timestamp,
                                         android::DisplayedFrameStats* outStats) const {
    auto intError = mComposer.getDisplayedContentSample(mId, maxFrames, timestamp, outStats);
    return static_cast<Error>(intError);
}

Error Display::getReleaseFences(std::unordered_map<HWC2::Layer*, sp<Fence>>* outFences) const {
    std::vector<Hwc2::Layer> layerIds;
    std::vector<int> fenceFds;
    auto intError = mComposer.getReleaseFences(mId, &layerIds, &fenceFds);
    auto error = static_cast<Error>(intError);
    uint32_t numElements = layerIds.size();
    if (error != Error::NONE) {
        return error;
    }

    std::unordered_map<HWC2::Layer*, sp<Fence>> releaseFences;
    releaseFences.reserve(numElements);
    for (uint32_t element = 0; element < numElements; ++element) {
        auto layer = getLayerById(layerIds[element]);
        if (layer) {
            sp<Fence> fence(new Fence(fenceFds[element]));
            releaseFences.emplace(layer.get(), fence);
        } else {
            ALOGE("getReleaseFences: invalid layer %" PRIu64
                    " found on display %" PRIu64, layerIds[element], mId);
            for (; element < numElements; ++element) {
                close(fenceFds[element]);
            }
            return Error::BAD_LAYER;
        }
    }

    *outFences = std::move(releaseFences);
    return Error::NONE;
}

Error Display::present(sp<Fence>* outPresentFence)
{
    int32_t presentFenceFd = -1;
    auto intError = mComposer.presentDisplay(mId, &presentFenceFd);
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        return error;
    }

    *outPresentFence = new Fence(presentFenceFd);
    return Error::NONE;
}

Error Display::setActiveConfigWithConstraints(hal::HWConfigId configId,
                                              const VsyncPeriodChangeConstraints& constraints,
                                              VsyncPeriodChangeTimeline* outTimeline) {
    ALOGV("[%" PRIu64 "] setActiveConfigWithConstraints", mId);

    if (isVsyncPeriodSwitchSupported()) {
        Hwc2::IComposerClient::VsyncPeriodChangeConstraints hwc2Constraints;
        hwc2Constraints.desiredTimeNanos = constraints.desiredTimeNanos;
        hwc2Constraints.seamlessRequired = constraints.seamlessRequired;

        Hwc2::VsyncPeriodChangeTimeline vsyncPeriodChangeTimeline = {};
        auto intError = mComposer.setActiveConfigWithConstraints(mId, configId, hwc2Constraints,
                                                                 &vsyncPeriodChangeTimeline);
        outTimeline->newVsyncAppliedTimeNanos = vsyncPeriodChangeTimeline.newVsyncAppliedTimeNanos;
        outTimeline->refreshRequired = vsyncPeriodChangeTimeline.refreshRequired;
        outTimeline->refreshTimeNanos = vsyncPeriodChangeTimeline.refreshTimeNanos;
        return static_cast<Error>(intError);
    }

    // Use legacy setActiveConfig instead
    ALOGV("fallback to legacy setActiveConfig");
    const auto now = systemTime();
    if (constraints.desiredTimeNanos > now || constraints.seamlessRequired) {
        ALOGE("setActiveConfigWithConstraints received constraints that can't be satisfied");
    }

    auto intError_2_4 = mComposer.setActiveConfig(mId, configId);
    outTimeline->newVsyncAppliedTimeNanos = std::max(now, constraints.desiredTimeNanos);
    outTimeline->refreshRequired = true;
    outTimeline->refreshTimeNanos = now;
    return static_cast<Error>(intError_2_4);
}

Error Display::setActiveConfig(const std::shared_ptr<const Config>& config)
{
    if (config->getDisplayId() != mId) {
        ALOGE("setActiveConfig received config %u for the wrong display %"
                PRIu64 " (expected %" PRIu64 ")", config->getId(),
                config->getDisplayId(), mId);
        return Error::BAD_CONFIG;
    }
    auto intError = mComposer.setActiveConfig(mId, config->getId());
    return static_cast<Error>(intError);
}

Error Display::setClientTarget(uint32_t slot, const sp<GraphicBuffer>& target,
        const sp<Fence>& acquireFence, Dataspace dataspace)
{
    // TODO: Properly encode client target surface damage
    int32_t fenceFd = acquireFence->dup();
    auto intError = mComposer.setClientTarget(mId, slot, target,
            fenceFd, dataspace, std::vector<Hwc2::IComposerClient::Rect>());
    return static_cast<Error>(intError);
}

Error Display::setColorMode(ColorMode mode, RenderIntent renderIntent)
{
    auto intError = mComposer.setColorMode(mId, mode, renderIntent);
    return static_cast<Error>(intError);
}

Error Display::setColorTransform(const android::mat4& matrix) {
    auto intError = mComposer.setColorTransform(mId, matrix.asArray());
    return static_cast<Error>(intError);
}

Error Display::setOutputBuffer(const sp<GraphicBuffer>& buffer,
        const sp<Fence>& releaseFence)
{
    int32_t fenceFd = releaseFence->dup();
    auto handle = buffer->getNativeBuffer()->handle;
    auto intError = mComposer.setOutputBuffer(mId, handle, fenceFd);
    close(fenceFd);
    return static_cast<Error>(intError);
}

Error Display::setPowerMode(PowerMode mode)
{
    auto intMode = static_cast<Hwc2::IComposerClient::PowerMode>(mode);
    auto intError = mComposer.setPowerMode(mId, intMode);

    if (mode == PowerMode::ON) {
        std::call_once(mDisplayCapabilityQueryFlag, [this]() {
            std::vector<DisplayCapability> tmpCapabilities;
            auto error =
                    static_cast<Error>(mComposer.getDisplayCapabilities(mId, &tmpCapabilities));
            if (error == Error::NONE) {
                std::scoped_lock lock(mDisplayCapabilitiesMutex);
                mDisplayCapabilities.emplace();
                for (auto capability : tmpCapabilities) {
                    mDisplayCapabilities->emplace(capability);
                }
            } else if (error == Error::UNSUPPORTED) {
                std::scoped_lock lock(mDisplayCapabilitiesMutex);
                mDisplayCapabilities.emplace();
                if (mCapabilities.count(hal::Capability::SKIP_CLIENT_COLOR_TRANSFORM)) {
                    mDisplayCapabilities->emplace(DisplayCapability::SKIP_CLIENT_COLOR_TRANSFORM);
                }
                bool dozeSupport = false;
                error = static_cast<Error>(mComposer.getDozeSupport(mId, &dozeSupport));
                if (error == Error::NONE && dozeSupport) {
                    mDisplayCapabilities->emplace(DisplayCapability::DOZE);
                }
            }
        });
    }

    return static_cast<Error>(intError);
}

Error Display::setVsyncEnabled(Vsync enabled)
{
    auto intEnabled = static_cast<Hwc2::IComposerClient::Vsync>(enabled);
    auto intError = mComposer.setVsyncEnabled(mId, intEnabled);
    return static_cast<Error>(intError);
}

Error Display::validate(nsecs_t expectedPresentTime, uint32_t* outNumTypes,
                        uint32_t* outNumRequests) {
    uint32_t numTypes = 0;
    uint32_t numRequests = 0;
    auto intError = mComposer.validateDisplay(mId, expectedPresentTime, &numTypes, &numRequests);
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE && !hasChangesError(error)) {
        return error;
    }

    *outNumTypes = numTypes;
    *outNumRequests = numRequests;
    return error;
}

Error Display::presentOrValidate(nsecs_t expectedPresentTime, uint32_t* outNumTypes,
                                 uint32_t* outNumRequests, sp<android::Fence>* outPresentFence,
                                 uint32_t* state) {
    uint32_t numTypes = 0;
    uint32_t numRequests = 0;
    int32_t presentFenceFd = -1;
    auto intError = mComposer.presentOrValidateDisplay(mId, expectedPresentTime, &numTypes,
                                                       &numRequests, &presentFenceFd, state);
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE && !hasChangesError(error)) {
        return error;
    }

    if (*state == 1) {
        *outPresentFence = new Fence(presentFenceFd);
    }

    if (*state == 0) {
        *outNumTypes = numTypes;
        *outNumRequests = numRequests;
    }
    return error;
}

#if ANDROID_VERSION_MAJOR >= 13
ftl::Future<Error> Display::setDisplayBrightness(
        float brightness, float brightnessNits,
        const Hwc2::Composer::DisplayBrightnessOptions& options) {
    return ftl::defer([composer = &mComposer, id = mId, brightness, brightnessNits, options] {
        const auto intError =
                composer->setDisplayBrightness(id, brightness, brightnessNits, options);
        return static_cast<Error>(intError);
    });
}
#else
Error Display::setDisplayBrightness(
        float brightness, float brightnessNits,
        const Hwc2::Composer::DisplayBrightnessOptions& options) {
    const auto intError =
            mComposer.setDisplayBrightness(mId, brightness, brightnessNits, options);
    return static_cast<Error>(intError);
}
#endif

#if ANDROID_VERSION_MAJOR >= 13
Error Display::setBootDisplayConfig(hal::HWConfigId configId) {
    auto intError = mComposer.setBootDisplayConfig(mId, configId);
    return static_cast<Error>(intError);
}

Error Display::clearBootDisplayConfig() {
    auto intError = mComposer.clearBootDisplayConfig(mId);
    return static_cast<Error>(intError);
}

Error Display::getPreferredBootDisplayConfig(hal::HWConfigId* configId) const {
    auto intError = mComposer.getPreferredBootDisplayConfig(mId, configId);
    return static_cast<Error>(intError);
}
#endif

Error Display::setAutoLowLatencyMode(bool on) {
    auto intError = mComposer.setAutoLowLatencyMode(mId, on);
    return static_cast<Error>(intError);
}

Error Display::getSupportedContentTypes(std::vector<ContentType>* outSupportedContentTypes) const {
    std::vector<Hwc2::IComposerClient::ContentType> tmpSupportedContentTypes;
    auto intError = mComposer.getSupportedContentTypes(mId, &tmpSupportedContentTypes);
    for (Hwc2::IComposerClient::ContentType contentType : tmpSupportedContentTypes) {
        outSupportedContentTypes->push_back(static_cast<ContentType>(contentType));
    }
    return static_cast<Error>(intError);
}

Error Display::setContentType(ContentType contentType) {
    auto intError = mComposer.setContentType(mId, contentType);
    return static_cast<Error>(intError);
}

Error Display::getClientTargetProperty(
        Hwc2::ClientTargetProperty*
                outClientTargetProperty) {
    const auto error = mComposer.getClientTargetProperty(mId, outClientTargetProperty);
    return static_cast<Error>(error);
}

#if ANDROID_VERSION_MAJOR >= 13
Error Display::getDisplayDecorationSupport(
        std::optional<aidl::android::hardware::graphics::common::DisplayDecorationSupport>*
                support) {
    const auto error = mComposer.getDisplayDecorationSupport(mId, support);
    return static_cast<Error>(error);
}

Error Display::setIdleTimerEnabled(std::chrono::milliseconds timeout) {
    const auto error = mComposer.setIdleTimerEnabled(mId, timeout);
    return static_cast<Error>(error);
}
#endif

// For use by Device

void Display::setConnected(bool connected) {
    if (!mIsConnected && connected) {
        mComposer.setClientTargetSlotCount(mId);
        if (mType == DisplayType::PHYSICAL) {
            loadConfigs();
        }
    }
    mIsConnected = connected;
}

int32_t Display::getAttribute(HWConfigId configId, Attribute attribute) {
    int32_t value = 0;
    auto intError = mComposer.getDisplayAttribute(mId, configId, attribute, &value);
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        ALOGE("getDisplayAttribute(%" PRIu64 ", %u, %s) failed: %s (%d)", mId,
                configId, to_string(attribute).c_str(),
                to_string(error).c_str(), intError);
        return -1;
    }
    return value;
}

void Display::loadConfig(HWConfigId configId) {
    ALOGV("[%" PRIu64 "] loadConfig(%u)", mId, configId);

    auto config = Config::Builder(*this, configId)
                          .setWidth(getAttribute(configId, hal::Attribute::WIDTH))
                          .setHeight(getAttribute(configId, hal::Attribute::HEIGHT))
                          .setVsyncPeriod(getAttribute(configId, hal::Attribute::VSYNC_PERIOD))
                          .setDpiX(getAttribute(configId, hal::Attribute::DPI_X))
                          .setDpiY(getAttribute(configId, hal::Attribute::DPI_Y))
                          .setConfigGroup(getAttribute(configId, hal::Attribute::CONFIG_GROUP))
                          .build();
    mConfigs.emplace(configId, std::move(config));
}

void Display::loadConfigs()
{
    ALOGV("[%" PRIu64 "] loadConfigs", mId);

    std::vector<HWConfigId> configIds;
    auto intError = mComposer.getDisplayConfigs(mId, &configIds);
    auto error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        ALOGE("[%" PRIu64 "] getDisplayConfigs [2] failed: %s (%d)", mId,
                to_string(error).c_str(), intError);
        return;
    }

    for (auto configId : configIds) {
        loadConfig(configId);
    }
}

// Other Display methods

std::shared_ptr<HWC2::Layer> Display::getLayerById(HWLayerId id) const {
    auto it = mLayers.find(id);
    return it != mLayers.end() ? it->second.lock() : nullptr;
}
} // namespace impl

// Layer methods

namespace {
std::vector<Hwc2::IComposerClient::Rect> convertRegionToHwcRects(const Region& region) {
    size_t rectCount = 0;
    Rect const* rectArray = region.getArray(&rectCount);

    std::vector<Hwc2::IComposerClient::Rect> hwcRects;
    hwcRects.reserve(rectCount);
    for (size_t rect = 0; rect < rectCount; ++rect) {
        hwcRects.push_back({rectArray[rect].left, rectArray[rect].top, rectArray[rect].right,
                            rectArray[rect].bottom});
    }
    return hwcRects;
}
} // namespace

Layer::~Layer() = default;

namespace impl {

Layer::Layer(android::Hwc2::Composer& composer,
             const std::unordered_set<hal::Capability>& capabilities, HWC2::Display& display,
             HWLayerId layerId)
      : mComposer(composer),
        mCapabilities(capabilities),
        mDisplay(&display),
        mId(layerId),
        mColorMatrix(android::mat4()) {
    ALOGV("Created layer %" PRIu64 " on display %" PRIu64, layerId, display.getId());
}

Layer::~Layer()
{
    onOwningDisplayDestroyed();
}

void Layer::onOwningDisplayDestroyed() {
    // Note: onOwningDisplayDestroyed() may be called to perform cleanup by
    // either the Layer dtor or by the Display dtor and must be safe to call
    // from either path. In particular, the call to Display::onLayerDestroyed()
    // is expected to be safe to do,

    if (CC_UNLIKELY(!mDisplay)) {
        return;
    }

    mDisplay->onLayerDestroyed(mId);

    // Note: If the HWC display was actually disconnected, these calls are will
    // return an error. We always make them as there may be other reasons for
    // the HWC2::Display to be destroyed.
    auto intError = mComposer.destroyLayer(mDisplay->getId(), mId);
    auto error = static_cast<Error>(intError);
    ALOGE_IF(error != Error::NONE,
             "destroyLayer(%" PRIu64 ", %" PRIu64 ")"
             " failed: %s (%d)",
             mDisplay->getId(), mId, to_string(error).c_str(), intError);

    mDisplay = nullptr;
}

Error Layer::setCursorPosition(int32_t x, int32_t y)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError = mComposer.setCursorPosition(mDisplay->getId(), mId, x, y);
    return static_cast<Error>(intError);
}

Error Layer::setBuffer(uint32_t slot, const sp<GraphicBuffer>& buffer,
        const sp<Fence>& acquireFence)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (buffer == nullptr && mBufferSlot == slot) {
        return Error::NONE;
    }
    mBufferSlot = slot;

    int32_t fenceFd = acquireFence->dup();
    auto intError = mComposer.setLayerBuffer(mDisplay->getId(), mId, slot, buffer, fenceFd);
    return static_cast<Error>(intError);
}

Error Layer::setSurfaceDamage(const Region& damage)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (damage.isRect() && mDamageRegion.isRect() &&
        (damage.getBounds() == mDamageRegion.getBounds())) {
        return Error::NONE;
    }
    mDamageRegion = damage;

    // We encode default full-screen damage as INVALID_RECT upstream, but as 0
    // rects for HWC
    Hwc2::Error intError = Hwc2::Error::NONE;
    if (damage.isRect() && damage.getBounds() == Rect::INVALID_RECT) {
        intError = mComposer.setLayerSurfaceDamage(mDisplay->getId(), mId,
                                                   std::vector<Hwc2::IComposerClient::Rect>());
    } else {
        const auto hwcRects = convertRegionToHwcRects(damage);
        intError = mComposer.setLayerSurfaceDamage(mDisplay->getId(), mId, hwcRects);
    }

    return static_cast<Error>(intError);
}

Error Layer::setBlendMode(BlendMode mode)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError = mComposer.setLayerBlendMode(mDisplay->getId(), mId, mode);
    return static_cast<Error>(intError);
}

Error Layer::setColor(Color color) {
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError = mComposer.setLayerColor(mDisplay->getId(), mId, color);
    return static_cast<Error>(intError);
}

Error Layer::setCompositionType(Composition type)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError = mComposer.setLayerCompositionType(mDisplay->getId(), mId, type);
    return static_cast<Error>(intError);
}

Error Layer::setDataspace(Dataspace dataspace)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (dataspace == mDataSpace) {
        return Error::NONE;
    }
    mDataSpace = dataspace;
    auto intError = mComposer.setLayerDataspace(mDisplay->getId(), mId, mDataSpace);
    return static_cast<Error>(intError);
}

Error Layer::setPerFrameMetadata(const int32_t supportedPerFrameMetadata,
        const android::HdrMetadata& metadata)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (metadata == mHdrMetadata) {
        return Error::NONE;
    }

    mHdrMetadata = metadata;
    int validTypes = mHdrMetadata.validTypes & supportedPerFrameMetadata;
    std::vector<Hwc2::PerFrameMetadata> perFrameMetadatas;
    if (validTypes & HdrMetadata::SMPTE2086) {
        perFrameMetadatas.insert(perFrameMetadatas.end(),
                                 {{Hwc2::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_X,
                                   mHdrMetadata.smpte2086.displayPrimaryRed.x},
                                  {Hwc2::PerFrameMetadataKey::DISPLAY_RED_PRIMARY_Y,
                                   mHdrMetadata.smpte2086.displayPrimaryRed.y},
                                  {Hwc2::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_X,
                                   mHdrMetadata.smpte2086.displayPrimaryGreen.x},
                                  {Hwc2::PerFrameMetadataKey::DISPLAY_GREEN_PRIMARY_Y,
                                   mHdrMetadata.smpte2086.displayPrimaryGreen.y},
                                  {Hwc2::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_X,
                                   mHdrMetadata.smpte2086.displayPrimaryBlue.x},
                                  {Hwc2::PerFrameMetadataKey::DISPLAY_BLUE_PRIMARY_Y,
                                   mHdrMetadata.smpte2086.displayPrimaryBlue.y},
                                  {Hwc2::PerFrameMetadataKey::WHITE_POINT_X,
                                   mHdrMetadata.smpte2086.whitePoint.x},
                                  {Hwc2::PerFrameMetadataKey::WHITE_POINT_Y,
                                   mHdrMetadata.smpte2086.whitePoint.y},
                                  {Hwc2::PerFrameMetadataKey::MAX_LUMINANCE,
                                   mHdrMetadata.smpte2086.maxLuminance},
                                  {Hwc2::PerFrameMetadataKey::MIN_LUMINANCE,
                                   mHdrMetadata.smpte2086.minLuminance}});
    }

    if (validTypes & HdrMetadata::CTA861_3) {
        perFrameMetadatas.insert(perFrameMetadatas.end(),
                                 {{Hwc2::PerFrameMetadataKey::MAX_CONTENT_LIGHT_LEVEL,
                                   mHdrMetadata.cta8613.maxContentLightLevel},
                                  {Hwc2::PerFrameMetadataKey::MAX_FRAME_AVERAGE_LIGHT_LEVEL,
                                   mHdrMetadata.cta8613.maxFrameAverageLightLevel}});
    }

    Error error = static_cast<Error>(
            mComposer.setLayerPerFrameMetadata(mDisplay->getId(), mId, perFrameMetadatas));

    if (validTypes & HdrMetadata::HDR10PLUS) {
        if (CC_UNLIKELY(mHdrMetadata.hdr10plus.size() == 0)) {
            return Error::BAD_PARAMETER;
        }

        std::vector<Hwc2::PerFrameMetadataBlob> perFrameMetadataBlobs;
        perFrameMetadataBlobs.push_back(
                {Hwc2::PerFrameMetadataKey::HDR10_PLUS_SEI, mHdrMetadata.hdr10plus});
        Error setMetadataBlobsError =
                static_cast<Error>(mComposer.setLayerPerFrameMetadataBlobs(mDisplay->getId(), mId,
                                                                           perFrameMetadataBlobs));
        if (error == Error::NONE) {
            return setMetadataBlobsError;
        }
    }
    return error;
}

Error Layer::setDisplayFrame(const Rect& frame)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    Hwc2::IComposerClient::Rect hwcRect{frame.left, frame.top,
        frame.right, frame.bottom};
    auto intError = mComposer.setLayerDisplayFrame(mDisplay->getId(), mId, hwcRect);
    return static_cast<Error>(intError);
}

Error Layer::setPlaneAlpha(float alpha)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError = mComposer.setLayerPlaneAlpha(mDisplay->getId(), mId, alpha);
    return static_cast<Error>(intError);
}

Error Layer::setSidebandStream(const native_handle_t* stream)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (mCapabilities.count(hal::Capability::SIDEBAND_STREAM) == 0) {
        ALOGE("Attempted to call setSidebandStream without checking that the "
                "device supports sideband streams");
        return Error::UNSUPPORTED;
    }
    auto intError = mComposer.setLayerSidebandStream(mDisplay->getId(), mId, stream);
    return static_cast<Error>(intError);
}

Error Layer::setSourceCrop(const FloatRect& crop)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    Hwc2::IComposerClient::FRect hwcRect{
        crop.left, crop.top, crop.right, crop.bottom};
    auto intError = mComposer.setLayerSourceCrop(mDisplay->getId(), mId, hwcRect);
    return static_cast<Error>(intError);
}

Error Layer::setTransform(Transform transform)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intTransform = static_cast<Hwc2::Transform>(transform);
    auto intError = mComposer.setLayerTransform(mDisplay->getId(), mId, intTransform);
    return static_cast<Error>(intError);
}

Error Layer::setVisibleRegion(const Region& region)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (region.isRect() && mVisibleRegion.isRect() &&
        (region.getBounds() == mVisibleRegion.getBounds())) {
        return Error::NONE;
    }
    mVisibleRegion = region;
    const auto hwcRects = convertRegionToHwcRects(region);
    auto intError = mComposer.setLayerVisibleRegion(mDisplay->getId(), mId, hwcRects);
    return static_cast<Error>(intError);
}

Error Layer::setZOrder(uint32_t z)
{
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError = mComposer.setLayerZOrder(mDisplay->getId(), mId, z);
    return static_cast<Error>(intError);
}

// Composer HAL 2.3
Error Layer::setColorTransform(const android::mat4& matrix) {
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (matrix == mColorMatrix) {
        return Error::NONE;
    }
    auto intError = mComposer.setLayerColorTransform(mDisplay->getId(), mId, matrix.asArray());
    Error error = static_cast<Error>(intError);
    if (error != Error::NONE) {
        return error;
    }
    mColorMatrix = matrix;
    return error;
}

// Composer HAL 2.4
Error Layer::setLayerGenericMetadata(const std::string& name, bool mandatory,
                                     const std::vector<uint8_t>& value) {
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError =
            mComposer.setLayerGenericMetadata(mDisplay->getId(), mId, name, mandatory, value);
    return static_cast<Error>(intError);
}

#if ANDROID_VERSION_MAJOR >= 13
// AIDL HAL
Error Layer::setBrightness(float brightness) {
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    auto intError = mComposer.setLayerBrightness(mDisplay->getId(), mId, brightness);
    return static_cast<Error>(intError);
}

Error Layer::setBlockingRegion(const Region& region) {
    if (CC_UNLIKELY(!mDisplay)) {
        return Error::BAD_DISPLAY;
    }

    if (region.isRect() && mBlockingRegion.isRect() &&
        (region.getBounds() == mBlockingRegion.getBounds())) {
        return Error::NONE;
    }
    mBlockingRegion = region;
    const auto hwcRects = convertRegionToHwcRects(region);
    const auto intError = mComposer.setLayerBlockingRegion(mDisplay->getId(), mId, hwcRects);
    return static_cast<Error>(intError);
}
#endif

} // namespace impl
} // namespace HWC2
} // namespace android

// TODO(b/129481165): remove the #pragma below and fix conversion issues
#pragma clang diagnostic pop // ignored "-Wconversion"
