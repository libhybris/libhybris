/*
 * Copyright (C) 2016 Canonical Ltd
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
 * Authored by: Simon Fels <simon.fels@canonical.com>
 */

#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "MediaCompatibilityLayer"

#include <hybris/media/media_meta_data_layer.h>

#include "media_meta_data_priv.h"

MediaMetaDataPrivate* MediaMetaDataPrivate::toPrivate(MediaMetaDataWrapper *md)
{
    if (!md)
        return NULL;

    return static_cast<MediaMetaDataPrivate*>(md);
}

MediaMetaDataPrivate::MediaMetaDataPrivate() :
#if ANDROID_VERSION_MAJOR>=8
    data(nullptr)
#else
    data(new android::MetaData)
#endif
{
}

#if ANDROID_VERSION_MAJOR>=8
MediaMetaDataPrivate::MediaMetaDataPrivate(android::MediaBufferBase *buffer) :
    data(buffer)
{
}
#endif

MediaMetaDataPrivate::MediaMetaDataPrivate(const android::sp<android::MetaData> &md) :
    data(md)
{
}

MediaMetaDataPrivate::~MediaMetaDataPrivate()
{
}

uint32_t media_meta_data_get_key_id(int key)
{
    switch (key)
    {
    case MEDIA_META_DATA_KEY_TIME:
        return android::kKeyTime;
    case MEDIA_META_DATA_KEY_IS_CODEC_CONFIG:
        return android::kKeyIsCodecConfig;
    case MEDIA_META_DATA_KEY_MIME:
        return android::kKeyMIMEType;
    case MEDIA_META_DATA_KEY_NUM_BUFFERS:
        return android::kKeyNumBuffers;
    case MEDIA_META_DATA_KEY_WIDTH:
        return android::kKeyWidth;
    case MEDIA_META_DATA_KEY_HEIGHT:
        return android::kKeyHeight;
    case MEDIA_META_DATA_KEY_STRIDE:
        return android::kKeyStride;
    case MEDIA_META_DATA_KEY_COLOR_FORMAT:
        return android::kKeyColorFormat;
    case MEDIA_META_DATA_KEY_SLICE_HEIGHT:
        return android::kKeySliceHeight;
    case MEDIA_META_DATA_KEY_FRAMERATE:
        return android::kKeyFrameRate;
    default:
        break;
    }

    return 0;
}

MediaMetaDataWrapper* media_meta_data_create()
{
    MediaMetaDataPrivate *d = new MediaMetaDataPrivate;
    if (!d)
        return NULL;

    return d;
}

void media_meta_data_release(MediaMetaDataWrapper *meta_data)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d)
        return;

    delete d;
}

void media_meta_data_clear(MediaMetaDataWrapper *meta_data)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return;

    d->data->clear();
}

bool media_meta_data_remove(MediaMetaDataWrapper *meta_data, uint32_t key)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    return d->data->remove(key);
}

bool media_meta_data_set_cstring(MediaMetaDataWrapper *meta_data, uint32_t key, const char *value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    android::status_t err = d->data->setCString(key, value);
    return err == android::OK;
}

bool media_meta_data_set_int32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    android::status_t err = d->data->setInt32(key, value);
    return err == android::OK;
}

bool media_meta_data_set_int64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    android::status_t err = d->data->setInt64(key, value);
    return err == android::OK;
}

bool media_meta_data_set_float(MediaMetaDataWrapper *meta_data, uint32_t key, float value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    android::status_t err = d->data->setFloat(key, value);
    return err == android::OK;
}

bool media_meta_data_set_pointer(MediaMetaDataWrapper *meta_data, uint32_t key, void *value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    android::status_t err = d->data->setPointer(key, value);
    return err == android::OK;
}

bool media_meta_data_find_cstring(MediaMetaDataWrapper *meta_data, uint32_t key, const char **value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    const char *v = NULL;
    if (!d->data->findCString(key, &v))
        return false;

    if (value)
        *value = v;

    return true;
}

bool media_meta_data_find_int32(MediaMetaDataWrapper *meta_data, uint32_t key, int32_t *value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    int32_t v;
    if (!d->data->findInt32(key, &v))
        return false;

    if (value)
        *value = v;

    return true;
}

bool media_meta_data_find_int64(MediaMetaDataWrapper *meta_data, uint32_t key, int64_t *value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    int64_t v;
    if (!d->data->findInt64(key, &v))
        return false;

    if (value)
        *value = v;

    return true;
}

bool media_meta_data_find_float(MediaMetaDataWrapper *meta_data, uint32_t key, float *value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    float v;
    if (!d->data->findFloat(key, &v))
        return false;

    if (value)
        *value = v;

    return true;
}

bool media_meta_data_find_pointer(MediaMetaDataWrapper *meta_data, uint32_t key, void **value)
{
    MediaMetaDataPrivate *d = MediaMetaDataPrivate::toPrivate(meta_data);
    if (!d || !d->data.get())
        return false;

    void *v = NULL;
    if (!d->data->findPointer(key, &v))
        return false;

    if (value)
        *value = v;

    return true;
}
