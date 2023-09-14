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
#define ATRACE_TAG ATRACE_TAG_GRAPHICS

#include "AidlComposerHal.h"
#include "HidlComposerHal.h"

namespace android::Hwc2 {

Composer::~Composer() = default;

std::unique_ptr<Composer> Composer::create(const std::string& serviceName) {
    if (AidlComposer::isDeclared(serviceName)) {
        return std::make_unique<AidlComposer>(serviceName);
    }

    return std::make_unique<HidlComposer>(serviceName);
}

} // namespace android::Hwc2
