/*
 * Copyright 2020 The Android Open Source Project
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

#if ANDROID_VERSION_MAJOR < 9
#include <android/hardware/graphics/common/1.0/types.h>
#else
#include <android/hardware/graphics/common/1.1/types.h>
#endif

#if ANDROID_VERSION_MAJOR < 9
#include <android/hardware/graphics/composer/2.1/IComposer.h>
#include <android/hardware/graphics/composer/2.1/IComposerClient.h>
#elif ANDROID_VERSION_MAJOR < 10
#include <android/hardware/graphics/composer/2.2/IComposer.h>
#include <android/hardware/graphics/composer/2.2/IComposerClient.h>
#elif ANDROID_VERSION_MAJOR < 11
#include <android/hardware/graphics/composer/2.3/IComposer.h>
#include <android/hardware/graphics/composer/2.3/IComposerClient.h>
#else
#include <android/hardware/graphics/composer/2.4/IComposer.h>
#include <android/hardware/graphics/composer/2.4/IComposerClient.h>
#endif

#if ANDROID_VERSION_MAJOR >= 13
#include <aidl/android/hardware/graphics/composer3/Composition.h>
#include <aidl/android/hardware/graphics/composer3/DisplayCapability.h>
#endif

#define ERROR_HAS_CHANGES 5

namespace android {
namespace hardware::graphics::composer::hal {

namespace types = android::hardware::graphics::common;
namespace V2_1 = android::hardware::graphics::composer::V2_1;
#if ANDROID_VERSION_MAJOR >= 9
namespace V2_2 = android::hardware::graphics::composer::V2_2;
#endif
#if ANDROID_VERSION_MAJOR >= 10
namespace V2_3 = android::hardware::graphics::composer::V2_3;
#endif
#if ANDROID_VERSION_MAJOR >= 11
namespace V2_4 = android::hardware::graphics::composer::V2_4;
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
#else
using types::V1_1::RenderIntent;
using types::V1_2::ColorMode;
using types::V1_2::Dataspace;
using types::V1_2::Hdr;
using types::V1_2::PixelFormat;
#endif

using V2_1::Error;
#if ANDROID_VERSION_MAJOR < 9
using V2_1::IComposer;
using V2_1::IComposerCallback;
using V2_1::IComposerClient;
#elif ANDROID_VERSION_MAJOR < 10
using V2_2::IComposer;
using V2_2::IComposerCallback;
using V2_2::IComposerClient;
#elif ANDROID_VERSION_MAJOR < 11
using V2_3::IComposer;
using V2_3::IComposerCallback;
using V2_3::IComposerClient;
#else
using V2_4::IComposer;
using V2_4::IComposerCallback;
using V2_4::IComposerClient;
using V2_4::VsyncPeriodChangeTimeline;
#endif

using Attribute = IComposerClient::Attribute;
using BlendMode = IComposerClient::BlendMode;
using Connection = IComposerCallback::Connection;
using DisplayRequest = IComposerClient::DisplayRequest;
using DisplayType = IComposerClient::DisplayType;
using HWConfigId = V2_1::Config;
using HWDisplayId = V2_1::Display;
using HWError = V2_1::Error;
using HWLayerId = V2_1::Layer;
using LayerRequest = IComposerClient::LayerRequest;
using PowerMode = IComposerClient::PowerMode;
using Vsync = IComposerClient::Vsync;

#if ANDROID_VERSION_MAJOR >= 9
using PerFrameMetadata = IComposerClient::PerFrameMetadata;
using PerFrameMetadataKey = IComposerClient::PerFrameMetadataKey;
#endif
#if ANDROID_VERSION_MAJOR >= 10
using PerFrameMetadataBlob = IComposerClient::PerFrameMetadataBlob;
#endif
#if ANDROID_VERSION_MAJOR >= 11
using ContentType = IComposerClient::ContentType;
using LayerGenericMetadataKey = IComposerClient::LayerGenericMetadataKey;
using VsyncPeriodChangeConstraints = IComposerClient::VsyncPeriodChangeConstraints;
#endif

#if ANDROID_VERSION_MAJOR < 13
using Capability = IComposer::Capability;
#if ANDROID_VERSION_MAJOR >= 11
using ClientTargetProperty = IComposerClient::ClientTargetProperty;
#endif
using Color = IComposerClient::Color;
using Composition = IComposerClient::Composition;
#if ANDROID_VERSION_MAJOR >= 10
using DisplayCapability = IComposerClient::DisplayCapability;
#endif
#else
namespace V3_0 = ::aidl::android::hardware::graphics::composer3;
using V3_0::Capability;
using ClientTargetProperty = V3_0::ClientTargetPropertyWithBrightness;
using V3_0::Color;
using V3_0::Composition;
using V3_0::DisplayCapability;
#endif

} // namespace hardware::graphics::composer::hal

inline bool hasChangesError(hardware::graphics::composer::hal::Error error) {
    return ERROR_HAS_CHANGES == static_cast<int32_t>(error);
}

inline std::string to_string(hardware::graphics::composer::hal::Attribute attribute) {
    switch (attribute) {
        case hardware::graphics::composer::hal::Attribute::INVALID:
            return "Invalid";
        case hardware::graphics::composer::hal::Attribute::WIDTH:
            return "Width";
        case hardware::graphics::composer::hal::Attribute::HEIGHT:
            return "Height";
        case hardware::graphics::composer::hal::Attribute::VSYNC_PERIOD:
            return "VsyncPeriod";
        case hardware::graphics::composer::hal::Attribute::DPI_X:
            return "DpiX";
        case hardware::graphics::composer::hal::Attribute::DPI_Y:
            return "DpiY";
        default:
            return "Unknown";
    }
}

inline std::string to_string(
        hardware::graphics::composer::hal::Composition composition) {
    switch (composition) {
        case hardware::graphics::composer::hal::Composition::INVALID:
            return "Invalid";
        case hardware::graphics::composer::hal::Composition::CLIENT:
            return "Client";
        case hardware::graphics::composer::hal::Composition::DEVICE:
            return "Device";
        case hardware::graphics::composer::hal::Composition::SOLID_COLOR:
            return "SolidColor";
        case hardware::graphics::composer::hal::Composition::CURSOR:
            return "Cursor";
        case hardware::graphics::composer::hal::Composition::SIDEBAND:
            return "Sideband";
#if ANDROID_VERSION_MAJOR >= 13
        case hardware::graphics::composer::hal::Composition::DISPLAY_DECORATION:
            return "DisplayDecoration";
#endif
        default:
            return "Unknown";
    }
}

#if ANDROID_VERSION_MAJOR >= 10
inline std::string to_string(
        hardware::graphics::composer::hal::DisplayCapability displayCapability) {
    switch (displayCapability) {
        case hardware::graphics::composer::hal::DisplayCapability::INVALID:
            return "Invalid";
        case hardware::graphics::composer::hal::DisplayCapability::
                SKIP_CLIENT_COLOR_TRANSFORM:
            return "SkipColorTransform";
        case hardware::graphics::composer::hal::DisplayCapability::DOZE:
            return "Doze";
        case hardware::graphics::composer::hal::DisplayCapability::BRIGHTNESS:
            return "Brightness";
#if ANDROID_VERSION_MAJOR >= 11
        case hardware::graphics::composer::hal::DisplayCapability::PROTECTED_CONTENTS:
            return "ProtectedContents";
        case hardware::graphics::composer::hal::DisplayCapability::AUTO_LOW_LATENCY_MODE:
            return "AutoLowLatencyMode";
#endif
#if ANDROID_VERSION_MAJOR >= 13
        case hardware::graphics::composer::hal::DisplayCapability::SUSPEND:
            return "Suspend";
        case hardware::graphics::composer::hal::DisplayCapability::DISPLAY_IDLE_TIMER:
            return "DisplayIdleTimer";
#endif
        default:
            return "Unknown";
    }
}
#endif

#if ANDROID_VERSION_MAJOR >= 11
inline std::string to_string(hardware::graphics::composer::hal::V2_4::Error error) {
    // 5 is reserved for historical reason, during validation 5 means has changes.
    if (ERROR_HAS_CHANGES == static_cast<int32_t>(error)) {
        return "HasChanges";
    }
    switch (error) {
        case hardware::graphics::composer::hal::V2_4::Error::NONE:
            return "None";
        case hardware::graphics::composer::hal::V2_4::Error::BAD_CONFIG:
            return "BadConfig";
        case hardware::graphics::composer::hal::V2_4::Error::BAD_DISPLAY:
            return "BadDisplay";
        case hardware::graphics::composer::hal::V2_4::Error::BAD_LAYER:
            return "BadLayer";
        case hardware::graphics::composer::hal::V2_4::Error::BAD_PARAMETER:
            return "BadParameter";
        case hardware::graphics::composer::hal::V2_4::Error::NO_RESOURCES:
            return "NoResources";
        case hardware::graphics::composer::hal::V2_4::Error::NOT_VALIDATED:
            return "NotValidated";
        case hardware::graphics::composer::hal::V2_4::Error::UNSUPPORTED:
            return "Unsupported";
        case hardware::graphics::composer::hal::V2_4::Error::SEAMLESS_NOT_ALLOWED:
            return "SeamlessNotAllowed";
        case hardware::graphics::composer::hal::V2_4::Error::SEAMLESS_NOT_POSSIBLE:
            return "SeamlessNotPossible";
        default:
            return "Unknown";
    }
}
#endif

inline std::string to_string(hardware::graphics::composer::hal::Error error) {
    // 5 is reserved for historical reason, during validation 5 means has changes.
    if (ERROR_HAS_CHANGES == static_cast<int32_t>(error)) {
        return "HasChanges";
    }
    switch (error) {
        case hardware::graphics::composer::hal::Error::NONE:
            return "None";
        case hardware::graphics::composer::hal::Error::BAD_CONFIG:
            return "BadConfig";
        case hardware::graphics::composer::hal::Error::BAD_DISPLAY:
            return "BadDisplay";
        case hardware::graphics::composer::hal::Error::BAD_LAYER:
            return "BadLayer";
        case hardware::graphics::composer::hal::Error::BAD_PARAMETER:
            return "BadParameter";
        case hardware::graphics::composer::hal::Error::NO_RESOURCES:
            return "NoResources";
        case hardware::graphics::composer::hal::Error::NOT_VALIDATED:
            return "NotValidated";
        case hardware::graphics::composer::hal::Error::UNSUPPORTED:
            return "Unsupported";
        default:
            return "Unknown";
    }
}

inline std::string to_string(hardware::graphics::composer::hal::PowerMode mode) {
    switch (mode) {
        case hardware::graphics::composer::hal::PowerMode::OFF:
            return "Off";
        case hardware::graphics::composer::hal::PowerMode::DOZE:
            return "Doze";
        case hardware::graphics::composer::hal::PowerMode::ON:
            return "On";
        case hardware::graphics::composer::hal::PowerMode::DOZE_SUSPEND:
            return "DozeSuspend";
#if ANDROID_VERSION_MAJOR >= 9
        case hardware::graphics::composer::hal::PowerMode::ON_SUSPEND:
            return "OnSuspend";
#endif
        default:
            return "Unknown";
    }
}

inline std::string to_string(hardware::graphics::composer::hal::Vsync vsync) {
    switch (vsync) {
        case hardware::graphics::composer::hal::Vsync::ENABLE:
            return "Enable";
        case hardware::graphics::composer::hal::Vsync::DISABLE:
            return "Disable";
        default:
            return "Unknown";
    }
}

} // namespace android
