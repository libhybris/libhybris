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

// #define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "HWC2"
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "HWC2.h"
#include "ComposerHal.h"

#include <ui/Fence.h>
#include <ui/FloatRect.h>
#include <ui/GraphicBuffer.h>
#include <ui/Region.h>

#include <android/configuration.h>

#include <algorithm>
#include <inttypes.h>

using android::Fence;
using android::FloatRect;
using android::GraphicBuffer;
using android::HdrCapabilities;
using android::Rect;
using android::Region;
using android::sp;
using android::hardware::Return;
using android::hardware::Void;

namespace HWC2 {

namespace Hwc2 = android::Hwc2;

namespace {

class ComposerCallbackBridge : public Hwc2::IComposerCallback {
public:
    ComposerCallbackBridge(ComposerCallback* callback, int32_t sequenceId)
            : mCallback(callback), mSequenceId(sequenceId),
              mHasPrimaryDisplay(false) {}

    Return<void> onHotplug(Hwc2::Display display,
                           IComposerCallback::Connection conn) override
    {
        HWC2::Connection connection = static_cast<HWC2::Connection>(conn);
        if (!mHasPrimaryDisplay) {
            LOG_ALWAYS_FATAL_IF(connection != HWC2::Connection::Connected,
                    "Initial onHotplug callback should be "
                    "primary display connected");
            mHasPrimaryDisplay = true;
            mCallback->onHotplugReceived(mSequenceId, display,
                                         connection, true);
        } else {
            mCallback->onHotplugReceived(mSequenceId, display,
                                         connection, false);
        }
        return Void();
    }

    Return<void> onRefresh(Hwc2::Display display) override
    {
        mCallback->onRefreshReceived(mSequenceId, display);
        return Void();
    }

    Return<void> onVsync(Hwc2::Display display, int64_t timestamp) override
    {
        mCallback->onVsyncReceived(mSequenceId, display, timestamp);
        return Void();
    }

    bool HasPrimaryDisplay() { return mHasPrimaryDisplay; }

private:
    ComposerCallback* mCallback;
    int32_t mSequenceId;
    bool mHasPrimaryDisplay;
};

} // namespace anonymous


// Device methods

Device::Device(bool useVrComposer)
  : mComposer(std::make_unique<Hwc2::Composer>(useVrComposer)),
    mCapabilities(),
    mDisplays(),
    mRegisteredCallback(false)
{
    loadCapabilities();
}

void Device::registerCallback(ComposerCallback* callback, int32_t sequenceId) {
    if (mRegisteredCallback) {
        ALOGW("Callback already registered. Ignored extra registration "
                "attempt.");
        return;
    }
    mRegisteredCallback = true;
    sp<ComposerCallbackBridge> callbackBridge(
            new ComposerCallbackBridge(callback, sequenceId));
    mComposer->registerCallback(callbackBridge);
    LOG_ALWAYS_FATAL_IF(!callbackBridge->HasPrimaryDisplay(),
            "Registered composer callback but didn't get primary display");
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

Error Device::createVirtualDisplay(uint32_t width, uint32_t height,
        android_pixel_format_t* format, Display** outDisplay)
{
    ALOGI("Creating virtual display");

    hwc2_display_t displayId = 0;
    auto intFormat = static_cast<Hwc2::PixelFormat>(*format);
    auto intError = mComposer->createVirtualDisplay(width, height,
            &intFormat, &displayId);
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
        return error;
    }

    auto display = std::make_unique<Display>(
            *mComposer.get(), mCapabilities, displayId, DisplayType::Virtual);
    *outDisplay = display.get();
    *format = static_cast<android_pixel_format_t>(intFormat);
    mDisplays.emplace(displayId, std::move(display));
    ALOGI("Created virtual display");
    return Error::None;
}

void Device::destroyDisplay(hwc2_display_t displayId)
{
    ALOGI("Destroying display %" PRIu64, displayId);
    mDisplays.erase(displayId);
}

void Device::onHotplug(hwc2_display_t displayId, Connection connection) {
    if (connection == Connection::Connected) {
        auto display = getDisplayById(displayId);
        if (display) {
            if (display->isConnected()) {
                ALOGW("Attempt to hotplug connect display %" PRIu64
                        " , which is already connected.", displayId);
            } else {
                display->setConnected(true);
            }
        } else {
            DisplayType displayType;
            auto intError = mComposer->getDisplayType(displayId,
                    reinterpret_cast<Hwc2::IComposerClient::DisplayType *>(
                            &displayType));
            auto error = static_cast<Error>(intError);
            if (error != Error::None) {
                ALOGE("getDisplayType(%" PRIu64 ") failed: %s (%d). "
                        "Aborting hotplug attempt.",
                        displayId, to_string(error).c_str(), intError);
                return;
            }

            auto newDisplay = std::make_unique<Display>(
                    *mComposer.get(), mCapabilities, displayId, displayType);
            mDisplays.emplace(displayId, std::move(newDisplay));
        }
    } else if (connection == Connection::Disconnected) {
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

Display* Device::getDisplayById(hwc2_display_t id) {
    auto iter = mDisplays.find(id);
    return iter == mDisplays.end() ? nullptr : iter->second.get();
}

// Device initialization methods

void Device::loadCapabilities()
{
    static_assert(sizeof(Capability) == sizeof(int32_t),
            "Capability size has changed");
    auto capabilities = mComposer->getCapabilities();
    for (auto capability : capabilities) {
        mCapabilities.emplace(static_cast<Capability>(capability));
    }
}

// Display methods

Display::Display(android::Hwc2::Composer& composer,
                 const std::unordered_set<Capability>& capabilities,
                 hwc2_display_t id, DisplayType type)
  : mComposer(composer),
    mCapabilities(capabilities),
    mId(id),
    mIsConnected(false),
    mType(type)
{
    ALOGV("Created display %" PRIu64, id);
    setConnected(true);
}

Display::~Display() {
    mLayers.clear();

    if (mType == DisplayType::Virtual) {
        ALOGV("Destroying virtual display");
        auto intError = mComposer.destroyVirtualDisplay(mId);
        auto error = static_cast<Error>(intError);
        ALOGE_IF(error != Error::None, "destroyVirtualDisplay(%" PRIu64
                ") failed: %s (%d)", mId, to_string(error).c_str(), intError);
    } else if (mType == DisplayType::Physical) {
        auto error = setVsyncEnabled(HWC2::Vsync::Disable);
        if (error != Error::None) {
            ALOGE("~Display: Failed to disable vsync for display %" PRIu64
                    ": %s (%d)", mId, to_string(error).c_str(),
                    static_cast<int32_t>(error));
        }
    }
}

Display::Config::Config(Display& display, hwc2_config_t id)
  : mDisplay(display),
    mId(id),
    mWidth(-1),
    mHeight(-1),
    mVsyncPeriod(-1),
    mDpiX(-1),
    mDpiY(-1) {}

Display::Config::Builder::Builder(Display& display, hwc2_config_t id)
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

// Required by HWC2 display

Error Display::acceptChanges()
{
    auto intError = mComposer.acceptDisplayChanges(mId);
    return static_cast<Error>(intError);
}

Error Display::createLayer(Layer** outLayer)
{
    if (!outLayer) {
        return Error::BadParameter;
    }
    hwc2_layer_t layerId = 0;
    auto intError = mComposer.createLayer(mId, &layerId);
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
        return error;
    }

    auto layer = std::make_unique<Layer>(
            mComposer, mCapabilities, mId, layerId);
    *outLayer = layer.get();
    mLayers.emplace(layerId, std::move(layer));
    return Error::None;
}

Error Display::destroyLayer(Layer* layer)
{
    if (!layer) {
        return Error::BadParameter;
    }
    mLayers.erase(layer->getId());
    return Error::None;
}

Error Display::getActiveConfig(
        std::shared_ptr<const Display::Config>* outConfig) const
{
    ALOGV("[%" PRIu64 "] getActiveConfig", mId);
    hwc2_config_t configId = 0;
    auto intError = mComposer.getActiveConfig(mId, &configId);
    auto error = static_cast<Error>(intError);

    if (error != Error::None) {
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

    return Error::None;
}

Error Display::getChangedCompositionTypes(
        std::unordered_map<Layer*, Composition>* outTypes)
{
    std::vector<Hwc2::Layer> layerIds;
    std::vector<Hwc2::IComposerClient::Composition> types;
    auto intError = mComposer.getChangedCompositionTypes(
            mId, &layerIds, &types);
    uint32_t numElements = layerIds.size();
    auto error = static_cast<Error>(intError);
    error = static_cast<Error>(intError);
    if (error != Error::None) {
        return error;
    }

    outTypes->clear();
    outTypes->reserve(numElements);
    for (uint32_t element = 0; element < numElements; ++element) {
        auto layer = getLayerById(layerIds[element]);
        if (layer) {
            auto type = static_cast<Composition>(types[element]);
            ALOGV("getChangedCompositionTypes: adding %" PRIu64 " %s",
                    layer->getId(), to_string(type).c_str());
            outTypes->emplace(layer, type);
        } else {
            ALOGE("getChangedCompositionTypes: invalid layer %" PRIu64 " found"
                    " on display %" PRIu64, layerIds[element], mId);
        }
    }

    return Error::None;
}

Error Display::getColorModes(std::vector<android_color_mode_t>* outModes) const
{
    std::vector<Hwc2::ColorMode> modes;
    auto intError = mComposer.getColorModes(mId, &modes);
    uint32_t numModes = modes.size();
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
        return error;
    }

    outModes->resize(numModes);
    for (size_t i = 0; i < numModes; i++) {
        (*outModes)[i] = static_cast<android_color_mode_t>(modes[i]);
    }
    return Error::None;
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
        std::unordered_map<Layer*, LayerRequest>* outLayerRequests)
{
    uint32_t intDisplayRequests;
    std::vector<Hwc2::Layer> layerIds;
    std::vector<uint32_t> layerRequests;
    auto intError = mComposer.getDisplayRequests(
            mId, &intDisplayRequests, &layerIds, &layerRequests);
    uint32_t numElements = layerIds.size();
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
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
            outLayerRequests->emplace(layer, layerRequest);
        } else {
            ALOGE("getRequests: invalid layer %" PRIu64 " found on display %"
                    PRIu64, layerIds[element], mId);
        }
    }

    return Error::None;
}

Error Display::getType(DisplayType* outType) const
{
    *outType = mType;
    return Error::None;
}

Error Display::supportsDoze(bool* outSupport) const
{
    bool intSupport = false;
    auto intError = mComposer.getDozeSupport(mId, &intSupport);
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
        return error;
    }
    *outSupport = static_cast<bool>(intSupport);
    return Error::None;
}

Error Display::getHdrCapabilities(
        std::unique_ptr<HdrCapabilities>* outCapabilities) const
{
    uint32_t numTypes = 0;
    float maxLuminance = -1.0f;
    float maxAverageLuminance = -1.0f;
    float minLuminance = -1.0f;
    std::vector<Hwc2::Hdr> intTypes;
    auto intError = mComposer.getHdrCapabilities(mId, &intTypes,
            &maxLuminance, &maxAverageLuminance, &minLuminance);
    auto error = static_cast<HWC2::Error>(intError);

    std::vector<int32_t> types;
    for (auto type : intTypes) {
        types.push_back(static_cast<int32_t>(type));
    }
    numTypes = types.size();
    if (error != Error::None) {
        return error;
    }

    *outCapabilities = std::make_unique<HdrCapabilities>(std::move(types),
            maxLuminance, maxAverageLuminance, minLuminance);
    return Error::None;
}

Error Display::getReleaseFences(
        std::unordered_map<Layer*, sp<Fence>>* outFences) const
{
    std::vector<Hwc2::Layer> layerIds;
    std::vector<int> fenceFds;
    auto intError = mComposer.getReleaseFences(mId, &layerIds, &fenceFds);
    auto error = static_cast<Error>(intError);
    uint32_t numElements = layerIds.size();
    if (error != Error::None) {
        return error;
    }

    std::unordered_map<Layer*, sp<Fence>> releaseFences;
    releaseFences.reserve(numElements);
    for (uint32_t element = 0; element < numElements; ++element) {
        auto layer = getLayerById(layerIds[element]);
        if (layer) {
            sp<Fence> fence(new Fence(fenceFds[element]));
            releaseFences.emplace(layer, fence);
        } else {
            ALOGE("getReleaseFences: invalid layer %" PRIu64
                    " found on display %" PRIu64, layerIds[element], mId);
            for (; element < numElements; ++element) {
                close(fenceFds[element]);
            }
            return Error::BadLayer;
        }
    }

    *outFences = std::move(releaseFences);
    return Error::None;
}

Error Display::present(sp<Fence>* outPresentFence)
{
    int32_t presentFenceFd = -1;
    auto intError = mComposer.presentDisplay(mId, &presentFenceFd);
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
        return error;
    }

    *outPresentFence = new Fence(presentFenceFd);
    return Error::None;
}

Error Display::setActiveConfig(const std::shared_ptr<const Config>& config)
{
    if (config->getDisplayId() != mId) {
        ALOGE("setActiveConfig received config %u for the wrong display %"
                PRIu64 " (expected %" PRIu64 ")", config->getId(),
                config->getDisplayId(), mId);
        return Error::BadConfig;
    }
    auto intError = mComposer.setActiveConfig(mId, config->getId());
    return static_cast<Error>(intError);
}

Error Display::setClientTarget(uint32_t slot, const sp<GraphicBuffer>& target,
        const sp<Fence>& acquireFence, android_dataspace_t dataspace)
{
    // TODO: Properly encode client target surface damage
    int32_t fenceFd = acquireFence->dup();
    auto intError = mComposer.setClientTarget(mId, slot, target,
            fenceFd, static_cast<Hwc2::Dataspace>(dataspace),
            std::vector<Hwc2::IComposerClient::Rect>());
    return static_cast<Error>(intError);
}

Error Display::setColorMode(android_color_mode_t mode)
{
    auto intError = mComposer.setColorMode(
            mId, static_cast<Hwc2::ColorMode>(mode));
    return static_cast<Error>(intError);
}

Error Display::setColorTransform(const android::mat4& matrix,
        android_color_transform_t hint)
{
    auto intError = mComposer.setColorTransform(mId,
            matrix.asArray(), static_cast<Hwc2::ColorTransform>(hint));
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
    return static_cast<Error>(intError);
}

Error Display::setVsyncEnabled(Vsync enabled)
{
    auto intEnabled = static_cast<Hwc2::IComposerClient::Vsync>(enabled);
    auto intError = mComposer.setVsyncEnabled(mId, intEnabled);
    return static_cast<Error>(intError);
}

Error Display::validate(uint32_t* outNumTypes, uint32_t* outNumRequests)
{
    uint32_t numTypes = 0;
    uint32_t numRequests = 0;
    auto intError = mComposer.validateDisplay(mId, &numTypes, &numRequests);
    auto error = static_cast<Error>(intError);
    if (error != Error::None && error != Error::HasChanges) {
        return error;
    }

    *outNumTypes = numTypes;
    *outNumRequests = numRequests;
    return error;
}

Error Display::presentOrValidate(uint32_t* outNumTypes, uint32_t* outNumRequests,
                                 sp<android::Fence>* outPresentFence, uint32_t* state) {

    uint32_t numTypes = 0;
    uint32_t numRequests = 0;
    int32_t presentFenceFd = -1;
    auto intError = mComposer.presentOrValidateDisplay(
            mId, &numTypes, &numRequests, &presentFenceFd, state);
    auto error = static_cast<Error>(intError);
    if (error != Error::None && error != Error::HasChanges) {
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

void Display::discardCommands()
{
    mComposer.resetCommands();
}

// For use by Device

void Display::setConnected(bool connected) {
    if (!mIsConnected && connected && mType == DisplayType::Physical) {
        mComposer.setClientTargetSlotCount(mId);
        loadConfigs();
    }
    mIsConnected = connected;
}

int32_t Display::getAttribute(hwc2_config_t configId, Attribute attribute)
{
    int32_t value = 0;
    auto intError = mComposer.getDisplayAttribute(mId, configId,
            static_cast<Hwc2::IComposerClient::Attribute>(attribute),
            &value);
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
        ALOGE("getDisplayAttribute(%" PRIu64 ", %u, %s) failed: %s (%d)", mId,
                configId, to_string(attribute).c_str(),
                to_string(error).c_str(), intError);
        return -1;
    }
    return value;
}

void Display::loadConfig(hwc2_config_t configId)
{
    ALOGV("[%" PRIu64 "] loadConfig(%u)", mId, configId);

    auto config = Config::Builder(*this, configId)
            .setWidth(getAttribute(configId, Attribute::Width))
            .setHeight(getAttribute(configId, Attribute::Height))
            .setVsyncPeriod(getAttribute(configId, Attribute::VsyncPeriod))
            .setDpiX(getAttribute(configId, Attribute::DpiX))
            .setDpiY(getAttribute(configId, Attribute::DpiY))
            .build();
    mConfigs.emplace(configId, std::move(config));
}

void Display::loadConfigs()
{
    ALOGV("[%" PRIu64 "] loadConfigs", mId);

    std::vector<Hwc2::Config> configIds;
    auto intError = mComposer.getDisplayConfigs(mId, &configIds);
    auto error = static_cast<Error>(intError);
    if (error != Error::None) {
        ALOGE("[%" PRIu64 "] getDisplayConfigs [2] failed: %s (%d)", mId,
                to_string(error).c_str(), intError);
        return;
    }

    for (auto configId : configIds) {
        loadConfig(configId);
    }
}

// Other Display methods

Layer* Display::getLayerById(hwc2_layer_t id) const
{
    if (mLayers.count(id) == 0) {
        return nullptr;
    }

    return mLayers.at(id).get();
}

// Layer methods

Layer::Layer(android::Hwc2::Composer& composer,
             const std::unordered_set<Capability>& capabilities,
             hwc2_display_t displayId, hwc2_layer_t layerId)
  : mComposer(composer),
    mCapabilities(capabilities),
    mDisplayId(displayId),
    mId(layerId)
{
    ALOGV("Created layer %" PRIu64 " on display %" PRIu64, layerId, displayId);
}

Layer::~Layer()
{
    auto intError = mComposer.destroyLayer(mDisplayId, mId);
    auto error = static_cast<Error>(intError);
    ALOGE_IF(error != Error::None, "destroyLayer(%" PRIu64 ", %" PRIu64 ")"
            " failed: %s (%d)", mDisplayId, mId, to_string(error).c_str(),
            intError);
    if (mLayerDestroyedListener) {
        mLayerDestroyedListener(this);
    }
}

void Layer::setLayerDestroyedListener(std::function<void(Layer*)> listener) {
    LOG_ALWAYS_FATAL_IF(mLayerDestroyedListener && listener,
            "Attempt to set layer destroyed listener multiple times");
    mLayerDestroyedListener = listener;
}

Error Layer::setCursorPosition(int32_t x, int32_t y)
{
    auto intError = mComposer.setCursorPosition(mDisplayId, mId, x, y);
    return static_cast<Error>(intError);
}

Error Layer::setBuffer(uint32_t slot, const sp<GraphicBuffer>& buffer,
        const sp<Fence>& acquireFence)
{
    int32_t fenceFd = acquireFence->dup();
    auto intError = mComposer.setLayerBuffer(mDisplayId, mId, slot, buffer,
                                             fenceFd);
    return static_cast<Error>(intError);
}

Error Layer::setSurfaceDamage(const Region& damage)
{
    // We encode default full-screen damage as INVALID_RECT upstream, but as 0
    // rects for HWC
    Hwc2::Error intError = Hwc2::Error::NONE;
    if (damage.isRect() && damage.getBounds() == Rect::INVALID_RECT) {
        intError = mComposer.setLayerSurfaceDamage(mDisplayId,
                mId, std::vector<Hwc2::IComposerClient::Rect>());
    } else {
        size_t rectCount = 0;
        auto rectArray = damage.getArray(&rectCount);

        std::vector<Hwc2::IComposerClient::Rect> hwcRects;
        for (size_t rect = 0; rect < rectCount; ++rect) {
            hwcRects.push_back({rectArray[rect].left, rectArray[rect].top,
                    rectArray[rect].right, rectArray[rect].bottom});
        }

        intError = mComposer.setLayerSurfaceDamage(mDisplayId, mId, hwcRects);
    }

    return static_cast<Error>(intError);
}

Error Layer::setBlendMode(BlendMode mode)
{
    auto intMode = static_cast<Hwc2::IComposerClient::BlendMode>(mode);
    auto intError = mComposer.setLayerBlendMode(mDisplayId, mId, intMode);
    return static_cast<Error>(intError);
}

Error Layer::setColor(hwc_color_t color)
{
    Hwc2::IComposerClient::Color hwcColor{color.r, color.g, color.b, color.a};
    auto intError = mComposer.setLayerColor(mDisplayId, mId, hwcColor);
    return static_cast<Error>(intError);
}

Error Layer::setCompositionType(Composition type)
{
    auto intType = static_cast<Hwc2::IComposerClient::Composition>(type);
    auto intError = mComposer.setLayerCompositionType(
            mDisplayId, mId, intType);
    return static_cast<Error>(intError);
}

Error Layer::setDataspace(android_dataspace_t dataspace)
{
    if (dataspace == mDataSpace) {
        return Error::None;
    }
    mDataSpace = dataspace;
    auto intDataspace = static_cast<Hwc2::Dataspace>(dataspace);
    auto intError = mComposer.setLayerDataspace(mDisplayId, mId, intDataspace);
    return static_cast<Error>(intError);
}

Error Layer::setDisplayFrame(const Rect& frame)
{
    Hwc2::IComposerClient::Rect hwcRect{frame.left, frame.top,
        frame.right, frame.bottom};
    auto intError = mComposer.setLayerDisplayFrame(mDisplayId, mId, hwcRect);
    return static_cast<Error>(intError);
}

Error Layer::setPlaneAlpha(float alpha)
{
    auto intError = mComposer.setLayerPlaneAlpha(mDisplayId, mId, alpha);
    return static_cast<Error>(intError);
}

Error Layer::setSidebandStream(const native_handle_t* stream)
{
    if (mCapabilities.count(Capability::SidebandStream) == 0) {
        ALOGE("Attempted to call setSidebandStream without checking that the "
                "device supports sideband streams");
        return Error::Unsupported;
    }
    auto intError = mComposer.setLayerSidebandStream(mDisplayId, mId, stream);
    return static_cast<Error>(intError);
}

Error Layer::setSourceCrop(const FloatRect& crop)
{
    Hwc2::IComposerClient::FRect hwcRect{
        crop.left, crop.top, crop.right, crop.bottom};
    auto intError = mComposer.setLayerSourceCrop(mDisplayId, mId, hwcRect);
    return static_cast<Error>(intError);
}

Error Layer::setTransform(Transform transform)
{
    auto intTransform = static_cast<Hwc2::Transform>(transform);
    auto intError = mComposer.setLayerTransform(mDisplayId, mId, intTransform);
    return static_cast<Error>(intError);
}

Error Layer::setVisibleRegion(const Region& region)
{
    size_t rectCount = 0;
    auto rectArray = region.getArray(&rectCount);

    std::vector<Hwc2::IComposerClient::Rect> hwcRects;
    for (size_t rect = 0; rect < rectCount; ++rect) {
        hwcRects.push_back({rectArray[rect].left, rectArray[rect].top,
                rectArray[rect].right, rectArray[rect].bottom});
    }

    auto intError = mComposer.setLayerVisibleRegion(mDisplayId, mId, hwcRects);
    return static_cast<Error>(intError);
}

Error Layer::setZOrder(uint32_t z)
{
    auto intError = mComposer.setLayerZOrder(mDisplayId, mId, z);
    return static_cast<Error>(intError);
}

Error Layer::setInfo(uint32_t type, uint32_t appId)
{
  auto intError = mComposer.setLayerInfo(mDisplayId, mId, type, appId);
  return static_cast<Error>(intError);
}

} // namespace HWC2
