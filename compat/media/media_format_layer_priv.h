/*
 * Copyright (C) 2013 Canonical Ltd
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
 *
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

#ifndef MEDIA_FORMAT_LAYER_PRIV_H_
#define MEDIA_FORMAT_LAYER_PRIV_H_

#include <stddef.h>
#include <unistd.h>

#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/ABuffer.h>

#include <utils/RefBase.h>

struct _MediaFormat : public android::RefBase
{
    _MediaFormat()
      : duration_us(0),
        width(0),
        height(0),
        max_input_size(0),
        csd(NULL),
        stride(0),
        slice_height(0),
        color_format(0),
        crop_left(0),
        crop_right(0),
        crop_top(0),
        crop_bottom(0),
        refcount(1)
    {
    }

    android::AString mime;
    int64_t duration_us;
    int32_t width;
    int32_t height;
    int32_t max_input_size;
    android::AString csd_key_name;
    android::sp<android::ABuffer> csd;

    int32_t stride;
    int32_t slice_height;
    int32_t color_format;
    int32_t crop_left;
    int32_t crop_right;
    int32_t crop_top;
    int32_t crop_bottom;

    unsigned int refcount;
};

#endif // MEDIA_FORMAT_LAYER_PRIV_H_
