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

#ifndef ANDROID_SF_HWC2_H
#define ANDROID_SF_HWC2_H

#define HWC2_INCLUDE_STRINGIFICATION
#define HWC2_USE_CPP11
#include <hardware/hwcomposer2.h>
#undef HWC2_INCLUDE_STRINGIFICATION
#undef HWC2_USE_CPP11

#include <ui/HdrCapabilities.h>
#include <math/mat4.h>

#include <utils/Log.h>
#include <utils/StrongPointer.h>
#include <utils/Timers.h>

#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <map>

namespace android {
    class Fence;
    class FloatRect;
    class GraphicBuffer;
    class Rect;
    class Region;
    namespace Hwc2 {
        class Composer;
    }
}

namespace HWC2 {

class Display;
class Layer;

// Implement this interface to receive hardware composer events.
//
// These callback functions will generally be called on a hwbinder thread, but
// when first registering the callback the onHotplugReceived() function will
// immediately be called on the thread calling registerCallback().
//
// All calls receive a sequenceId, which will be the value that was supplied to
// HWC2::Device::registerCallback(). It's used to help differentiate callbacks
// from different hardware composer instances.
class ComposerCallback {
 public:
    virtual void onHotplugReceived(int32_t sequenceId, hwc2_display_t display,
                                   Connection connection,
                                   bool primaryDisplay) = 0;
    virtual void onRefreshReceived(int32_t sequenceId,
                                   hwc2_display_t display) = 0;
    virtual void onVsyncReceived(int32_t sequenceId, hwc2_display_t display,
                                 int64_t timestamp) = 0;
    virtual ~ComposerCallback() = default;
};

// C++ Wrapper around hwc2_device_t. Load all functions pointers
// and handle callback registration.
class Device
{
public:
    // useVrComposer is passed to the composer HAL. When true, the composer HAL
    // will use the vr composer service, otherwise it uses the real hardware
    // composer.
    Device(bool useVrComposer);

    void registerCallback(ComposerCallback* callback, int32_t sequenceId);

    // Required by HWC2

    std::string dump() const;

    const std::unordered_set<Capability>& getCapabilities() const {
        return mCapabilities;
    };

    uint32_t getMaxVirtualDisplayCount() const;
    Error createVirtualDisplay(uint32_t width, uint32_t height,
            android_pixel_format_t* format, Display** outDisplay);
    void destroyDisplay(hwc2_display_t displayId);

    void onHotplug(hwc2_display_t displayId, Connection connection);

    // Other Device methods

    Display* getDisplayById(hwc2_display_t id);

    android::Hwc2::Composer* getComposer() { return mComposer.get(); }

private:
    // Initialization methods

    void loadCapabilities();

    // Member variables
    std::unique_ptr<android::Hwc2::Composer> mComposer;
    std::unordered_set<Capability> mCapabilities;
    std::unordered_map<hwc2_display_t, std::unique_ptr<Display>> mDisplays;
    bool mRegisteredCallback;
};

// Convenience C++ class to access hwc2_device_t Display functions directly.
class Display
{
public:
    Display(android::Hwc2::Composer& composer,
            const std::unordered_set<Capability>& capabilities,
            hwc2_display_t id, DisplayType type);
    ~Display();

    class Config
    {
    public:
        class Builder
        {
        public:
            Builder(Display& display, hwc2_config_t id);

            std::shared_ptr<const Config> build() {
                return std::const_pointer_cast<const Config>(
                        std::move(mConfig));
            }

            Builder& setWidth(int32_t width) {
                mConfig->mWidth = width;
                return *this;
            }
            Builder& setHeight(int32_t height) {
                mConfig->mHeight = height;
                return *this;
            }
            Builder& setVsyncPeriod(int32_t vsyncPeriod) {
                mConfig->mVsyncPeriod = vsyncPeriod;
                return *this;
            }
            Builder& setDpiX(int32_t dpiX) {
                if (dpiX == -1) {
                    mConfig->mDpiX = getDefaultDensity();
                } else {
                    mConfig->mDpiX = dpiX / 1000.0f;
                }
                return *this;
            }
            Builder& setDpiY(int32_t dpiY) {
                if (dpiY == -1) {
                    mConfig->mDpiY = getDefaultDensity();
                } else {
                    mConfig->mDpiY = dpiY / 1000.0f;
                }
                return *this;
            }

        private:
            float getDefaultDensity();
            std::shared_ptr<Config> mConfig;
        };

        hwc2_display_t getDisplayId() const { return mDisplay.getId(); }
        hwc2_config_t getId() const { return mId; }

        int32_t getWidth() const { return mWidth; }
        int32_t getHeight() const { return mHeight; }
        nsecs_t getVsyncPeriod() const { return mVsyncPeriod; }
        float getDpiX() const { return mDpiX; }
        float getDpiY() const { return mDpiY; }

    private:
        Config(Display& display, hwc2_config_t id);

        Display& mDisplay;
        hwc2_config_t mId;

        int32_t mWidth;
        int32_t mHeight;
        nsecs_t mVsyncPeriod;
        float mDpiX;
        float mDpiY;
    };

    // Required by HWC2

    [[clang::warn_unused_result]] Error acceptChanges();
    [[clang::warn_unused_result]] Error createLayer(Layer** outLayer);
    [[clang::warn_unused_result]] Error destroyLayer(Layer* layer);
    [[clang::warn_unused_result]] Error getActiveConfig(
            std::shared_ptr<const Config>* outConfig) const;
    [[clang::warn_unused_result]] Error getChangedCompositionTypes(
            std::unordered_map<Layer*, Composition>* outTypes);
    [[clang::warn_unused_result]] Error getColorModes(
            std::vector<android_color_mode_t>* outModes) const;

    // Doesn't call into the HWC2 device, so no errors are possible
    std::vector<std::shared_ptr<const Config>> getConfigs() const;

    [[clang::warn_unused_result]] Error getName(std::string* outName) const;
    [[clang::warn_unused_result]] Error getRequests(
            DisplayRequest* outDisplayRequests,
            std::unordered_map<Layer*, LayerRequest>* outLayerRequests);
    [[clang::warn_unused_result]] Error getType(DisplayType* outType) const;
    [[clang::warn_unused_result]] Error supportsDoze(bool* outSupport) const;
    [[clang::warn_unused_result]] Error getHdrCapabilities(
            std::unique_ptr<android::HdrCapabilities>* outCapabilities) const;
    [[clang::warn_unused_result]] Error getReleaseFences(
            std::unordered_map<Layer*,
                    android::sp<android::Fence>>* outFences) const;
    [[clang::warn_unused_result]] Error present(
            android::sp<android::Fence>* outPresentFence);
    [[clang::warn_unused_result]] Error setActiveConfig(
            const std::shared_ptr<const Config>& config);
    [[clang::warn_unused_result]] Error setClientTarget(
            uint32_t slot, const android::sp<android::GraphicBuffer>& target,
            const android::sp<android::Fence>& acquireFence,
            android_dataspace_t dataspace);
    [[clang::warn_unused_result]] Error setColorMode(android_color_mode_t mode);
    [[clang::warn_unused_result]] Error setColorTransform(
            const android::mat4& matrix, android_color_transform_t hint);
    [[clang::warn_unused_result]] Error setOutputBuffer(
            const android::sp<android::GraphicBuffer>& buffer,
            const android::sp<android::Fence>& releaseFence);
    [[clang::warn_unused_result]] Error setPowerMode(PowerMode mode);
    [[clang::warn_unused_result]] Error setVsyncEnabled(Vsync enabled);
    [[clang::warn_unused_result]] Error validate(uint32_t* outNumTypes,
            uint32_t* outNumRequests);
    [[clang::warn_unused_result]] Error presentOrValidate(uint32_t* outNumTypes,
                                                 uint32_t* outNumRequests,
                                                          android::sp<android::Fence>* outPresentFence, uint32_t* state);

    // Most methods in this class write a command to a command buffer.  The
    // command buffer is implicitly submitted in validate, present, and
    // presentOrValidate.  This method provides a way to discard the commands,
    // which can be used to discard stale commands.
    void discardCommands();

    // Other Display methods

    hwc2_display_t getId() const { return mId; }
    bool isConnected() const { return mIsConnected; }
    void setConnected(bool connected);  // For use by Device only

private:
    int32_t getAttribute(hwc2_config_t configId, Attribute attribute);
    void loadConfig(hwc2_config_t configId);
    void loadConfigs();

    // This may fail (and return a null pointer) if no layer with this ID exists
    // on this display
    Layer* getLayerById(hwc2_layer_t id) const;

    // Member variables

    // These are references to data owned by HWC2::Device, which will outlive
    // this HWC2::Display, so these references are guaranteed to be valid for
    // the lifetime of this object.
    android::Hwc2::Composer& mComposer;
    const std::unordered_set<Capability>& mCapabilities;

    hwc2_display_t mId;
    bool mIsConnected;
    DisplayType mType;
    std::unordered_map<hwc2_layer_t, std::unique_ptr<Layer>> mLayers;
    // The ordering in this map matters, for getConfigs(), when it is
    // converted to a vector
    std::map<hwc2_config_t, std::shared_ptr<const Config>> mConfigs;
};

// Convenience C++ class to access hwc2_device_t Layer functions directly.
class Layer
{
public:
    Layer(android::Hwc2::Composer& composer,
          const std::unordered_set<Capability>& capabilities,
          hwc2_display_t displayId, hwc2_layer_t layerId);
    ~Layer();

    hwc2_layer_t getId() const { return mId; }

    // Register a listener to be notified when the layer is destroyed. When the
    // listener function is called, the Layer will be in the process of being
    // destroyed, so it's not safe to call methods on it.
    void setLayerDestroyedListener(std::function<void(Layer*)> listener);

    [[clang::warn_unused_result]] Error setCursorPosition(int32_t x, int32_t y);
    [[clang::warn_unused_result]] Error setBuffer(uint32_t slot,
            const android::sp<android::GraphicBuffer>& buffer,
            const android::sp<android::Fence>& acquireFence);
    [[clang::warn_unused_result]] Error setSurfaceDamage(
            const android::Region& damage);

    [[clang::warn_unused_result]] Error setBlendMode(BlendMode mode);
    [[clang::warn_unused_result]] Error setColor(hwc_color_t color);
    [[clang::warn_unused_result]] Error setCompositionType(Composition type);
    [[clang::warn_unused_result]] Error setDataspace(
            android_dataspace_t dataspace);
    [[clang::warn_unused_result]] Error setDisplayFrame(
            const android::Rect& frame);
    [[clang::warn_unused_result]] Error setPlaneAlpha(float alpha);
    [[clang::warn_unused_result]] Error setSidebandStream(
            const native_handle_t* stream);
    [[clang::warn_unused_result]] Error setSourceCrop(
            const android::FloatRect& crop);
    [[clang::warn_unused_result]] Error setTransform(Transform transform);
    [[clang::warn_unused_result]] Error setVisibleRegion(
            const android::Region& region);
    [[clang::warn_unused_result]] Error setZOrder(uint32_t z);
    [[clang::warn_unused_result]] Error setInfo(uint32_t type, uint32_t appId);

private:
    // These are references to data owned by HWC2::Device, which will outlive
    // this HWC2::Layer, so these references are guaranteed to be valid for
    // the lifetime of this object.
    android::Hwc2::Composer& mComposer;
    const std::unordered_set<Capability>& mCapabilities;

    hwc2_display_t mDisplayId;
    hwc2_layer_t mId;
    android_dataspace mDataSpace = HAL_DATASPACE_UNKNOWN;
    std::function<void(Layer*)> mLayerDestroyedListener;
};

} // namespace HWC2

#endif // ANDROID_SF_HWC2_H
