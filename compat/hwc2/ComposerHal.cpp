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

#undef LOG_TAG
#define LOG_TAG "HwcComposer"

#include <inttypes.h>
#include <log/log.h>
#include <gui/BufferQueue.h>

#include "ComposerHal.h"

namespace android {

using hardware::Return;
using hardware::hidl_vec;
using hardware::hidl_handle;

namespace Hwc2 {

namespace {

class BufferHandle {
public:
    BufferHandle(const native_handle_t* buffer)
    {
        // nullptr is not a valid handle to HIDL
        mHandle = (buffer) ? buffer : native_handle_init(mStorage, 0, 0);
    }

    operator const hidl_handle&() const
    {
        return mHandle;
    }

private:
    NATIVE_HANDLE_DECLARE_STORAGE(mStorage, 0, 0);
    hidl_handle mHandle;
};

class FenceHandle
{
public:
    FenceHandle(int fd, bool owned)
        : mOwned(owned)
    {
        native_handle_t* handle;
        if (fd >= 0) {
            handle = native_handle_init(mStorage, 1, 0);
            handle->data[0] = fd;
        } else {
            // nullptr is not a valid handle to HIDL
            handle = native_handle_init(mStorage, 0, 0);
        }
        mHandle = handle;
    }

    ~FenceHandle()
    {
        if (mOwned) {
            native_handle_close(mHandle);
        }
    }

    operator const hidl_handle&() const
    {
        return mHandle;
    }

private:
    bool mOwned;
    NATIVE_HANDLE_DECLARE_STORAGE(mStorage, 1, 0);
    hidl_handle mHandle;
};

// assume NO_RESOURCES when Status::isOk returns false
constexpr Error kDefaultError = Error::NO_RESOURCES;

template<typename T, typename U>
T unwrapRet(Return<T>& ret, const U& default_val)
{
    return (ret.isOk()) ? static_cast<T>(ret) :
        static_cast<T>(default_val);
}

Error unwrapRet(Return<Error>& ret)
{
    return unwrapRet(ret, kDefaultError);
}

} // anonymous namespace

Composer::CommandWriter::CommandWriter(uint32_t initialMaxSize)
    : CommandWriterBase(initialMaxSize) {}

Composer::CommandWriter::~CommandWriter()
{
}

void Composer::CommandWriter::setLayerInfo(uint32_t type, uint32_t appId)
{
    constexpr uint16_t kSetLayerInfoLength = 2;
    beginCommand(
        static_cast<IComposerClient::Command>(
            IVrComposerClient::VrCommand::SET_LAYER_INFO),
        kSetLayerInfoLength);
    write(type);
    write(appId);
    endCommand();
}

void Composer::CommandWriter::setClientTargetMetadata(
        const IVrComposerClient::BufferMetadata& metadata)
{
    constexpr uint16_t kSetClientTargetMetadataLength = 7;
    beginCommand(
        static_cast<IComposerClient::Command>(
            IVrComposerClient::VrCommand::SET_CLIENT_TARGET_METADATA),
        kSetClientTargetMetadataLength);
    writeBufferMetadata(metadata);
    endCommand();
}

void Composer::CommandWriter::setLayerBufferMetadata(
        const IVrComposerClient::BufferMetadata& metadata)
{
    constexpr uint16_t kSetLayerBufferMetadataLength = 7;
    beginCommand(
        static_cast<IComposerClient::Command>(
            IVrComposerClient::VrCommand::SET_LAYER_BUFFER_METADATA),
        kSetLayerBufferMetadataLength);
    writeBufferMetadata(metadata);
    endCommand();
}

void Composer::CommandWriter::writeBufferMetadata(
        const IVrComposerClient::BufferMetadata& metadata)
{
    write(metadata.width);
    write(metadata.height);
    write(metadata.stride);
    write(metadata.layerCount);
    writeSigned(static_cast<int32_t>(metadata.format));
    write64(metadata.usage);
}

Composer::Composer(bool useVrComposer)
    : mWriter(kWriterInitialSize),
      mIsUsingVrComposer(useVrComposer)
{
    if (mIsUsingVrComposer) {
        mComposer = IComposer::getService("vr");
    } else {
        mComposer = IComposer::getService(); // use default name
    }

    if (mComposer == nullptr) {
        LOG_ALWAYS_FATAL("failed to get hwcomposer service");
    }

    mComposer->createClient(
            [&](const auto& tmpError, const auto& tmpClient)
            {
                if (tmpError == Error::NONE) {
                    mClient = tmpClient;
                }
            });
    if (mClient == nullptr) {
        LOG_ALWAYS_FATAL("failed to create composer client");
    }

    if (mIsUsingVrComposer) {
        sp<IVrComposerClient> vrClient = IVrComposerClient::castFrom(mClient);
        if (vrClient == nullptr) {
            LOG_ALWAYS_FATAL("failed to create vr composer client");
        }
    }
}

std::vector<IComposer::Capability> Composer::getCapabilities()
{
    std::vector<IComposer::Capability> capabilities;
    mComposer->getCapabilities(
            [&](const auto& tmpCapabilities) {
                capabilities = tmpCapabilities;
            });

    return capabilities;
}

std::string Composer::dumpDebugInfo()
{
    std::string info;
    mComposer->dumpDebugInfo([&](const auto& tmpInfo) {
        info = tmpInfo.c_str();
    });

    return info;
}

void Composer::registerCallback(const sp<IComposerCallback>& callback)
{
    auto ret = mClient->registerCallback(callback);
    if (!ret.isOk()) {
        ALOGE("failed to register IComposerCallback");
    }
}

bool Composer::isRemote() {
    return mClient->isRemote();
}

void Composer::resetCommands() {
    mWriter.reset();
}

uint32_t Composer::getMaxVirtualDisplayCount()
{
    auto ret = mClient->getMaxVirtualDisplayCount();
    return unwrapRet(ret, 0);
}

Error Composer::createVirtualDisplay(uint32_t width, uint32_t height,
            PixelFormat* format, Display* outDisplay)
{
    const uint32_t bufferSlotCount = 1;
    Error error = kDefaultError;
    mClient->createVirtualDisplay(width, height, *format, bufferSlotCount,
            [&](const auto& tmpError, const auto& tmpDisplay,
                const auto& tmpFormat) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outDisplay = tmpDisplay;
                *format = tmpFormat;
            });

    return error;
}

Error Composer::destroyVirtualDisplay(Display display)
{
    auto ret = mClient->destroyVirtualDisplay(display);
    return unwrapRet(ret);
}

Error Composer::acceptDisplayChanges(Display display)
{
    mWriter.selectDisplay(display);
    mWriter.acceptDisplayChanges();
    return Error::NONE;
}

Error Composer::createLayer(Display display, Layer* outLayer)
{
    Error error = kDefaultError;
    mClient->createLayer(display, BufferQueue::NUM_BUFFER_SLOTS,
            [&](const auto& tmpError, const auto& tmpLayer) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outLayer = tmpLayer;
            });

    return error;
}

Error Composer::destroyLayer(Display display, Layer layer)
{
    auto ret = mClient->destroyLayer(display, layer);
    return unwrapRet(ret);
}

Error Composer::getActiveConfig(Display display, Config* outConfig)
{
    Error error = kDefaultError;
    mClient->getActiveConfig(display,
            [&](const auto& tmpError, const auto& tmpConfig) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outConfig = tmpConfig;
            });

    return error;
}

Error Composer::getChangedCompositionTypes(Display display,
        std::vector<Layer>* outLayers,
        std::vector<IComposerClient::Composition>* outTypes)
{
    mReader.takeChangedCompositionTypes(display, outLayers, outTypes);
    return Error::NONE;
}

Error Composer::getColorModes(Display display,
        std::vector<ColorMode>* outModes)
{
    Error error = kDefaultError;
    mClient->getColorModes(display,
            [&](const auto& tmpError, const auto& tmpModes) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outModes = tmpModes;
            });

    return error;
}

Error Composer::getDisplayAttribute(Display display, Config config,
        IComposerClient::Attribute attribute, int32_t* outValue)
{
    Error error = kDefaultError;
    mClient->getDisplayAttribute(display, config, attribute,
            [&](const auto& tmpError, const auto& tmpValue) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outValue = tmpValue;
            });

    return error;
}

Error Composer::getDisplayConfigs(Display display,
        std::vector<Config>* outConfigs)
{
    Error error = kDefaultError;
    mClient->getDisplayConfigs(display,
            [&](const auto& tmpError, const auto& tmpConfigs) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outConfigs = tmpConfigs;
            });

    return error;
}

Error Composer::getDisplayName(Display display, std::string* outName)
{
    Error error = kDefaultError;
    mClient->getDisplayName(display,
            [&](const auto& tmpError, const auto& tmpName) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outName = tmpName.c_str();
            });

    return error;
}

Error Composer::getDisplayRequests(Display display,
        uint32_t* outDisplayRequestMask, std::vector<Layer>* outLayers,
        std::vector<uint32_t>* outLayerRequestMasks)
{
    mReader.takeDisplayRequests(display, outDisplayRequestMask,
            outLayers, outLayerRequestMasks);
    return Error::NONE;
}

Error Composer::getDisplayType(Display display,
        IComposerClient::DisplayType* outType)
{
    Error error = kDefaultError;
    mClient->getDisplayType(display,
            [&](const auto& tmpError, const auto& tmpType) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outType = tmpType;
            });

    return error;
}

Error Composer::getDozeSupport(Display display, bool* outSupport)
{
    Error error = kDefaultError;
    mClient->getDozeSupport(display,
            [&](const auto& tmpError, const auto& tmpSupport) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outSupport = tmpSupport;
            });

    return error;
}

Error Composer::getHdrCapabilities(Display display,
        std::vector<Hdr>* outTypes, float* outMaxLuminance,
        float* outMaxAverageLuminance, float* outMinLuminance)
{
    Error error = kDefaultError;
    mClient->getHdrCapabilities(display,
            [&](const auto& tmpError, const auto& tmpTypes,
                const auto& tmpMaxLuminance,
                const auto& tmpMaxAverageLuminance,
                const auto& tmpMinLuminance) {
                error = tmpError;
                if (error != Error::NONE) {
                    return;
                }

                *outTypes = tmpTypes;
                *outMaxLuminance = tmpMaxLuminance;
                *outMaxAverageLuminance = tmpMaxAverageLuminance;
                *outMinLuminance = tmpMinLuminance;
            });

    return error;
}

Error Composer::getReleaseFences(Display display,
        std::vector<Layer>* outLayers, std::vector<int>* outReleaseFences)
{
    mReader.takeReleaseFences(display, outLayers, outReleaseFences);
    return Error::NONE;
}

Error Composer::presentDisplay(Display display, int* outPresentFence)
{
    mWriter.selectDisplay(display);
    mWriter.presentDisplay();

    Error error = execute();
    if (error != Error::NONE) {
        return error;
    }

    mReader.takePresentFence(display, outPresentFence);

    return Error::NONE;
}

Error Composer::setActiveConfig(Display display, Config config)
{
    auto ret = mClient->setActiveConfig(display, config);
    return unwrapRet(ret);
}

Error Composer::setClientTarget(Display display, uint32_t slot,
        const sp<GraphicBuffer>& target,
        int acquireFence, Dataspace dataspace,
        const std::vector<IComposerClient::Rect>& damage)
{
    mWriter.selectDisplay(display);
    if (mIsUsingVrComposer && target.get()) {
        IVrComposerClient::BufferMetadata metadata = {
            .width = target->getWidth(),
            .height = target->getHeight(),
            .stride = target->getStride(),
            .layerCount = target->getLayerCount(),
            .format = static_cast<PixelFormat>(target->getPixelFormat()),
            .usage = target->getUsage(),
        };
        mWriter.setClientTargetMetadata(metadata);
    }

    const native_handle_t* handle = nullptr;
    if (target.get()) {
        handle = target->getNativeBuffer()->handle;
    }

    mWriter.setClientTarget(slot, handle, acquireFence, dataspace, damage);
    return Error::NONE;
}

Error Composer::setColorMode(Display display, ColorMode mode)
{
    auto ret = mClient->setColorMode(display, mode);
    return unwrapRet(ret);
}

Error Composer::setColorTransform(Display display, const float* matrix,
        ColorTransform hint)
{
    mWriter.selectDisplay(display);
    mWriter.setColorTransform(matrix, hint);
    return Error::NONE;
}

Error Composer::setOutputBuffer(Display display, const native_handle_t* buffer,
        int releaseFence)
{
    mWriter.selectDisplay(display);
    mWriter.setOutputBuffer(0, buffer, dup(releaseFence));
    return Error::NONE;
}

Error Composer::setPowerMode(Display display, IComposerClient::PowerMode mode)
{
    auto ret = mClient->setPowerMode(display, mode);
    return unwrapRet(ret);
}

Error Composer::setVsyncEnabled(Display display, IComposerClient::Vsync enabled)
{
    auto ret = mClient->setVsyncEnabled(display, enabled);
    return unwrapRet(ret);
}

Error Composer::setClientTargetSlotCount(Display display)
{
    const uint32_t bufferSlotCount = BufferQueue::NUM_BUFFER_SLOTS;
    auto ret = mClient->setClientTargetSlotCount(display, bufferSlotCount);
    return unwrapRet(ret);
}

Error Composer::validateDisplay(Display display, uint32_t* outNumTypes,
        uint32_t* outNumRequests)
{
    mWriter.selectDisplay(display);
    mWriter.validateDisplay();

    Error error = execute();
    if (error != Error::NONE) {
        return error;
    }

    mReader.hasChanges(display, outNumTypes, outNumRequests);

    return Error::NONE;
}

Error Composer::presentOrValidateDisplay(Display display, uint32_t* outNumTypes,
                               uint32_t* outNumRequests, int* outPresentFence, uint32_t* state) {
   mWriter.selectDisplay(display);
   mWriter.presentOrvalidateDisplay();

   Error error = execute();
   if (error != Error::NONE) {
       return error;
   }

   mReader.takePresentOrValidateStage(display, state);

   if (*state == 1) { // Present succeeded
       mReader.takePresentFence(display, outPresentFence);
   }

   if (*state == 0) { // Validate succeeded.
       mReader.hasChanges(display, outNumTypes, outNumRequests);
   }

   return Error::NONE;
}

Error Composer::setCursorPosition(Display display, Layer layer,
        int32_t x, int32_t y)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerCursorPosition(x, y);
    return Error::NONE;
}

Error Composer::setLayerBuffer(Display display, Layer layer,
        uint32_t slot, const sp<GraphicBuffer>& buffer, int acquireFence)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    if (mIsUsingVrComposer && buffer.get()) {
        IVrComposerClient::BufferMetadata metadata = {
            .width = buffer->getWidth(),
            .height = buffer->getHeight(),
            .stride = buffer->getStride(),
            .layerCount = buffer->getLayerCount(),
            .format = static_cast<PixelFormat>(buffer->getPixelFormat()),
            .usage = buffer->getUsage(),
        };
        mWriter.setLayerBufferMetadata(metadata);
    }

    const native_handle_t* handle = nullptr;
    if (buffer.get()) {
        handle = buffer->getNativeBuffer()->handle;
    }

    mWriter.setLayerBuffer(slot, handle, acquireFence);
    return Error::NONE;
}

Error Composer::setLayerSurfaceDamage(Display display, Layer layer,
        const std::vector<IComposerClient::Rect>& damage)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerSurfaceDamage(damage);
    return Error::NONE;
}

Error Composer::setLayerBlendMode(Display display, Layer layer,
        IComposerClient::BlendMode mode)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerBlendMode(mode);
    return Error::NONE;
}

Error Composer::setLayerColor(Display display, Layer layer,
        const IComposerClient::Color& color)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerColor(color);
    return Error::NONE;
}

Error Composer::setLayerCompositionType(Display display, Layer layer,
        IComposerClient::Composition type)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerCompositionType(type);
    return Error::NONE;
}

Error Composer::setLayerDataspace(Display display, Layer layer,
        Dataspace dataspace)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerDataspace(dataspace);
    return Error::NONE;
}

Error Composer::setLayerDisplayFrame(Display display, Layer layer,
        const IComposerClient::Rect& frame)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerDisplayFrame(frame);
    return Error::NONE;
}

Error Composer::setLayerPlaneAlpha(Display display, Layer layer,
        float alpha)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerPlaneAlpha(alpha);
    return Error::NONE;
}

Error Composer::setLayerSidebandStream(Display display, Layer layer,
        const native_handle_t* stream)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerSidebandStream(stream);
    return Error::NONE;
}

Error Composer::setLayerSourceCrop(Display display, Layer layer,
        const IComposerClient::FRect& crop)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerSourceCrop(crop);
    return Error::NONE;
}

Error Composer::setLayerTransform(Display display, Layer layer,
        Transform transform)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerTransform(transform);
    return Error::NONE;
}

Error Composer::setLayerVisibleRegion(Display display, Layer layer,
        const std::vector<IComposerClient::Rect>& visible)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerVisibleRegion(visible);
    return Error::NONE;
}

Error Composer::setLayerZOrder(Display display, Layer layer, uint32_t z)
{
    mWriter.selectDisplay(display);
    mWriter.selectLayer(layer);
    mWriter.setLayerZOrder(z);
    return Error::NONE;
}

Error Composer::setLayerInfo(Display display, Layer layer, uint32_t type,
                             uint32_t appId)
{
    if (mIsUsingVrComposer) {
        mWriter.selectDisplay(display);
        mWriter.selectLayer(layer);
        mWriter.setLayerInfo(type, appId);
    }
    return Error::NONE;
}

Error Composer::execute()
{
    // prepare input command queue
    bool queueChanged = false;
    uint32_t commandLength = 0;
    hidl_vec<hidl_handle> commandHandles;
    if (!mWriter.writeQueue(&queueChanged, &commandLength, &commandHandles)) {
        mWriter.reset();
        return Error::NO_RESOURCES;
    }

    // set up new input command queue if necessary
    if (queueChanged) {
        auto ret = mClient->setInputCommandQueue(*mWriter.getMQDescriptor());
        auto error = unwrapRet(ret);
        if (error != Error::NONE) {
            mWriter.reset();
            return error;
        }
    }

    Error error = kDefaultError;
    auto ret = mClient->executeCommands(commandLength, commandHandles,
            [&](const auto& tmpError, const auto& tmpOutChanged,
                const auto& tmpOutLength, const auto& tmpOutHandles)
            {
                error = tmpError;

                // set up new output command queue if necessary
                if (error == Error::NONE && tmpOutChanged) {
                    error = kDefaultError;
                    mClient->getOutputCommandQueue(
                            [&](const auto& tmpError,
                                const auto& tmpDescriptor)
                            {
                                error = tmpError;
                                if (error != Error::NONE) {
                                    return;
                                }

                                mReader.setMQDescriptor(tmpDescriptor);
                            });
                }

                if (error != Error::NONE) {
                    return;
                }

                if (mReader.readQueue(tmpOutLength, tmpOutHandles)) {
                    error = mReader.parse();
                    mReader.reset();
                } else {
                    error = Error::NO_RESOURCES;
                }
            });
    // executeCommands can fail because of out-of-fd and we do not want to
    // abort() in that case
    if (!ret.isOk()) {
        ALOGE("executeCommands failed because of %s", ret.description().c_str());
    }

    if (error == Error::NONE) {
        std::vector<CommandReader::CommandError> commandErrors =
            mReader.takeErrors();

        for (const auto& cmdErr : commandErrors) {
            auto command = mWriter.getCommand(cmdErr.location);

            if (command == IComposerClient::Command::VALIDATE_DISPLAY ||
                command == IComposerClient::Command::PRESENT_DISPLAY ||
                command == IComposerClient::Command::PRESENT_OR_VALIDATE_DISPLAY) {
                error = cmdErr.error;
            } else {
                ALOGW("command 0x%x generated error %d",
                        command, cmdErr.error);
            }
        }
    }

    mWriter.reset();

    return error;
}

CommandReader::~CommandReader()
{
    resetData();
}

Error CommandReader::parse()
{
    resetData();

    IComposerClient::Command command;
    uint16_t length = 0;

    while (!isEmpty()) {
        if (!beginCommand(&command, &length)) {
            break;
        }

        bool parsed = false;
        switch (command) {
        case IComposerClient::Command::SELECT_DISPLAY:
            parsed = parseSelectDisplay(length);
            break;
        case IComposerClient::Command::SET_ERROR:
            parsed = parseSetError(length);
            break;
        case IComposerClient::Command::SET_CHANGED_COMPOSITION_TYPES:
            parsed = parseSetChangedCompositionTypes(length);
            break;
        case IComposerClient::Command::SET_DISPLAY_REQUESTS:
            parsed = parseSetDisplayRequests(length);
            break;
        case IComposerClient::Command::SET_PRESENT_FENCE:
            parsed = parseSetPresentFence(length);
            break;
        case IComposerClient::Command::SET_RELEASE_FENCES:
            parsed = parseSetReleaseFences(length);
            break;
        case IComposerClient::Command ::SET_PRESENT_OR_VALIDATE_DISPLAY_RESULT:
            parsed = parseSetPresentOrValidateDisplayResult(length);
            break;
        default:
            parsed = false;
            break;
        }

        endCommand();

        if (!parsed) {
            ALOGE("failed to parse command 0x%x length %" PRIu16,
                    command, length);
            break;
        }
    }

    return isEmpty() ? Error::NONE : Error::NO_RESOURCES;
}

bool CommandReader::parseSelectDisplay(uint16_t length)
{
    if (length != CommandWriterBase::kSelectDisplayLength) {
        return false;
    }

    mCurrentReturnData = &mReturnData[read64()];

    return true;
}

bool CommandReader::parseSetError(uint16_t length)
{
    if (length != CommandWriterBase::kSetErrorLength) {
        return false;
    }

    auto location = read();
    auto error = static_cast<Error>(readSigned());

    mErrors.emplace_back(CommandError{location, error});

    return true;
}

bool CommandReader::parseSetChangedCompositionTypes(uint16_t length)
{
    // (layer id, composition type) pairs
    if (length % 3 != 0 || !mCurrentReturnData) {
        return false;
    }

    uint32_t count = length / 3;
    mCurrentReturnData->changedLayers.reserve(count);
    mCurrentReturnData->compositionTypes.reserve(count);
    while (count > 0) {
        auto layer = read64();
        auto type = static_cast<IComposerClient::Composition>(readSigned());

        mCurrentReturnData->changedLayers.push_back(layer);
        mCurrentReturnData->compositionTypes.push_back(type);

        count--;
    }

    return true;
}

bool CommandReader::parseSetDisplayRequests(uint16_t length)
{
    // display requests followed by (layer id, layer requests) pairs
    if (length % 3 != 1 || !mCurrentReturnData) {
        return false;
    }

    mCurrentReturnData->displayRequests = read();

    uint32_t count = (length - 1) / 3;
    mCurrentReturnData->requestedLayers.reserve(count);
    mCurrentReturnData->requestMasks.reserve(count);
    while (count > 0) {
        auto layer = read64();
        auto layerRequestMask = read();

        mCurrentReturnData->requestedLayers.push_back(layer);
        mCurrentReturnData->requestMasks.push_back(layerRequestMask);

        count--;
    }

    return true;
}

bool CommandReader::parseSetPresentFence(uint16_t length)
{
    if (length != CommandWriterBase::kSetPresentFenceLength ||
            !mCurrentReturnData) {
        return false;
    }

    if (mCurrentReturnData->presentFence >= 0) {
        close(mCurrentReturnData->presentFence);
    }
    mCurrentReturnData->presentFence = readFence();

    return true;
}

bool CommandReader::parseSetReleaseFences(uint16_t length)
{
    // (layer id, release fence index) pairs
    if (length % 3 != 0 || !mCurrentReturnData) {
        return false;
    }

    uint32_t count = length / 3;
    mCurrentReturnData->releasedLayers.reserve(count);
    mCurrentReturnData->releaseFences.reserve(count);
    while (count > 0) {
        auto layer = read64();
        auto fence = readFence();

        mCurrentReturnData->releasedLayers.push_back(layer);
        mCurrentReturnData->releaseFences.push_back(fence);

        count--;
    }

    return true;
}

bool CommandReader::parseSetPresentOrValidateDisplayResult(uint16_t length)
{
    if (length != CommandWriterBase::kPresentOrValidateDisplayResultLength || !mCurrentReturnData) {
        return false;
    }
    mCurrentReturnData->presentOrValidateState = read();
    return true;
}

void CommandReader::resetData()
{
    mErrors.clear();

    for (auto& data : mReturnData) {
        if (data.second.presentFence >= 0) {
            close(data.second.presentFence);
        }
        for (auto fence : data.second.releaseFences) {
            if (fence >= 0) {
                close(fence);
            }
        }
    }

    mReturnData.clear();
    mCurrentReturnData = nullptr;
}

std::vector<CommandReader::CommandError> CommandReader::takeErrors()
{
    return std::move(mErrors);
}

bool CommandReader::hasChanges(Display display,
        uint32_t* outNumChangedCompositionTypes,
        uint32_t* outNumLayerRequestMasks) const
{
    auto found = mReturnData.find(display);
    if (found == mReturnData.end()) {
        *outNumChangedCompositionTypes = 0;
        *outNumLayerRequestMasks = 0;
        return false;
    }

    const ReturnData& data = found->second;

    *outNumChangedCompositionTypes = data.compositionTypes.size();
    *outNumLayerRequestMasks = data.requestMasks.size();

    return !(data.compositionTypes.empty() && data.requestMasks.empty());
}

void CommandReader::takeChangedCompositionTypes(Display display,
        std::vector<Layer>* outLayers,
        std::vector<IComposerClient::Composition>* outTypes)
{
    auto found = mReturnData.find(display);
    if (found == mReturnData.end()) {
        outLayers->clear();
        outTypes->clear();
        return;
    }

    ReturnData& data = found->second;

    *outLayers = std::move(data.changedLayers);
    *outTypes = std::move(data.compositionTypes);
}

void CommandReader::takeDisplayRequests(Display display,
        uint32_t* outDisplayRequestMask, std::vector<Layer>* outLayers,
        std::vector<uint32_t>* outLayerRequestMasks)
{
    auto found = mReturnData.find(display);
    if (found == mReturnData.end()) {
        *outDisplayRequestMask = 0;
        outLayers->clear();
        outLayerRequestMasks->clear();
        return;
    }

    ReturnData& data = found->second;

    *outDisplayRequestMask = data.displayRequests;
    *outLayers = std::move(data.requestedLayers);
    *outLayerRequestMasks = std::move(data.requestMasks);
}

void CommandReader::takeReleaseFences(Display display,
        std::vector<Layer>* outLayers, std::vector<int>* outReleaseFences)
{
    auto found = mReturnData.find(display);
    if (found == mReturnData.end()) {
        outLayers->clear();
        outReleaseFences->clear();
        return;
    }

    ReturnData& data = found->second;

    *outLayers = std::move(data.releasedLayers);
    *outReleaseFences = std::move(data.releaseFences);
}

void CommandReader::takePresentFence(Display display, int* outPresentFence)
{
    auto found = mReturnData.find(display);
    if (found == mReturnData.end()) {
        *outPresentFence = -1;
        return;
    }

    ReturnData& data = found->second;

    *outPresentFence = data.presentFence;
    data.presentFence = -1;
}

void CommandReader::takePresentOrValidateStage(Display display, uint32_t* state) {
    auto found = mReturnData.find(display);
    if (found == mReturnData.end()) {
        *state= -1;
        return;
    }
    ReturnData& data = found->second;
    *state = data.presentOrValidateState;
}

} // namespace Hwc2

} // namespace android
