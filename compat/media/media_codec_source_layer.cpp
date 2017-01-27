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

#include <gui/Surface.h>

#include <media/ICrypto.h>

#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>

#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodecSource.h>
#include <media/stagefright/MediaErrors.h>

#include <hybris/media/media_codec_source_layer.h>

#include "media_message_priv.h"
#include "media_buffer_priv.h"
#include "media_meta_data_priv.h"

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__);

struct MediaSourcePrivate : public android::MediaSource
{
public:
    static MediaSourcePrivate* toPrivate(MediaSourceWrapper *source);

    android::status_t start(android::MetaData *params = NULL);
    android::status_t stop();
    android::sp<android::MetaData> getFormat();
    android::status_t read(android::MediaBuffer **buffer, const android::MediaSource::ReadOptions *options = NULL);
    android::status_t pause();

    android::sp<android::MetaData> format;

    MediaSourceStartCallback start_callback;
    void *start_callback_data;
    MediaSourceStopCallback stop_callback;
    void *stop_callback_data;
    MediaSourceReadCallback read_callback;
    void *read_callback_data;
    MediaSourcePauseCallback pause_callback;
    void *pause_callback_data;
};

MediaSourcePrivate* MediaSourcePrivate::toPrivate(MediaSourceWrapper *source)
{
    if (!source)
        return NULL;

    return static_cast<MediaSourcePrivate*>(source);
}

android::status_t MediaSourcePrivate::start(android::MetaData *params)
{
    if(!start_callback)
        return android::ERROR_UNSUPPORTED;

    return start_callback(nullptr, start_callback_data);
}

android::status_t MediaSourcePrivate::stop()
{
    if (!stop_callback)
        return android::ERROR_UNSUPPORTED;

    return stop_callback(stop_callback_data);
}

android::sp<android::MetaData> MediaSourcePrivate::getFormat()
{
    return format;
}

android::status_t MediaSourcePrivate::read(android::MediaBuffer **buffer, const android::MediaSource::ReadOptions *options)
{
    (void) options;

    if (!read_callback)
        return android::ERROR_UNSUPPORTED;

    MediaBufferPrivate *buf = NULL;

    int err = read_callback(reinterpret_cast<MediaBufferWrapper**>(&buf), read_callback_data);
    if (!buf)
        return err;

    *buffer = buf->buffer;

    return err;
}

android::status_t MediaSourcePrivate::pause()
{
    if (!pause_callback)
        return android::ERROR_UNSUPPORTED;

    return pause_callback(pause_callback_data);
}

MediaSourceWrapper* media_source_create(void)
{
    MediaSourcePrivate *d  = new MediaSourcePrivate;
    if (!d)
        return NULL;

    return d;
}

void media_source_release(MediaSourceWrapper *source)
{
    MediaSourcePrivate *d = MediaSourcePrivate::toPrivate(source);
    if (!d)
        return;

    delete d;
}

void media_source_set_format(MediaSourceWrapper *source, MediaMetaDataWrapper *meta)
{
    MediaSourcePrivate *d = MediaSourcePrivate::toPrivate(source);
    if (!d)
        return;

    auto dm = MediaMetaDataPrivate::toPrivate(meta);
    if (!dm)
        return;

    d->format = dm->data;
}

void media_source_set_start_callback(MediaSourceWrapper *source, MediaSourceStartCallback callback, void *user_data)
{
    MediaSourcePrivate *d = MediaSourcePrivate::toPrivate(source);
    if (!d)
        return;

    d->start_callback = callback;
    d->start_callback_data = user_data;
}

void media_source_set_stop_callback(MediaSourceWrapper *source, MediaSourceStopCallback callback, void *user_data)
{
    MediaSourcePrivate *d = MediaSourcePrivate::toPrivate(source);
    if (!d)
        return;

    d->stop_callback = callback;
    d->stop_callback_data = user_data;
}

void media_source_set_read_callback(MediaSourceWrapper *source, MediaSourceReadCallback callback, void *user_data)
{
    MediaSourcePrivate *d = MediaSourcePrivate::toPrivate(source);
    if (!d)
        return;

    d->read_callback = callback;
    d->read_callback_data = user_data;
}

void media_source_set_pause_callback(MediaSourceWrapper *source, MediaSourcePauseCallback callback, void *user_data)
{
    MediaSourcePrivate *d = MediaSourcePrivate::toPrivate(source);
    if (!d)
        return;

    d->pause_callback = callback;
    d->pause_callback_data = user_data;
}

struct MediaCodecSourcePrivate : public android::AHandler
{
public:
    static MediaCodecSourcePrivate* toPrivate(MediaCodecSourceWrapper *source);

    explicit MediaCodecSourcePrivate(void *context);
    virtual ~MediaCodecSourcePrivate();

protected:
    virtual void onMessageReceived(const android::sp<android::AMessage> &msg);

public:
    void *context;
    unsigned int refcount;
    android::sp<android::ALooper> looper;
    android::sp<android::MediaCodecSource> codec;
    android::sp<android::Surface> input_surface;
    android::sp<android::MediaSource> source;
};

MediaCodecSourcePrivate* MediaCodecSourcePrivate::toPrivate(MediaCodecSourceWrapper *source)
{
    if (!source)
        return NULL;

    return static_cast<MediaCodecSourcePrivate*>(source);
}

MediaCodecSourcePrivate::MediaCodecSourcePrivate(void *context) :
    context(context),
    refcount(1)
{
    REPORT_FUNCTION();
}

MediaCodecSourcePrivate::~MediaCodecSourcePrivate()
{
    REPORT_FUNCTION();
}

void MediaCodecSourcePrivate::onMessageReceived(const android::sp<android::AMessage> &msg)
{
    (void) msg;
}

MediaCodecSourceWrapper* media_codec_source_create(MediaMessageWrapper *format, MediaSourceWrapper *source, int flags)
{
    if (!format)
        return NULL;

    MediaCodecSourcePrivate *d = new MediaCodecSourcePrivate(NULL);
    if (!d)
        return NULL;

    d->looper = new android::ALooper();
    d->looper->start();

    d->source = MediaSourcePrivate::toPrivate(source);

    MediaMessagePrivate *dm = MediaMessagePrivate::toPrivate(format);

    ALOGV("Creating media codec source");
#if ANDROID_VERSION_MAJOR>=6
    // We don't use persistent input surface
    d->codec = android::MediaCodecSource::Create(d->looper, dm->msg, d->source, NULL, flags);
#else
    d->codec = android::MediaCodecSource::Create(d->looper, dm->msg, d->source, flags);
#endif

    return d;
}

void media_codec_source_release(MediaCodecSourceWrapper *source)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d)
        return;

    delete d;
}

MediaNativeWindowHandle* media_codec_source_get_native_window_handle(MediaCodecSourceWrapper *source)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d)
        return NULL;

    if (!d->input_surface.get())
        d->input_surface = new android::Surface(d->codec->getGraphicBufferProducer());

    return static_cast<ANativeWindow*>(d->input_surface.get());
}

MediaMetaDataWrapper* media_codec_source_get_format(MediaCodecSourceWrapper *source)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d)
        return NULL;

    return new MediaMetaDataPrivate(d->codec->getFormat());
}

bool media_codec_source_start(MediaCodecSourceWrapper *source)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d)
        return false;

    android::status_t err = d->codec->start(NULL);
    if (err != android::OK)
        return false;

    return true;
}

bool media_codec_source_stop(MediaCodecSourceWrapper *source)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d)
        return false;

    android::status_t err = d->codec->stop();
    if (err != android::OK)
        return false;

    return true;
}

bool media_codec_source_pause(MediaCodecSourceWrapper *source)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d)
        return false;

    android::status_t err = d->codec->pause();
    if (err != android::OK)
        return false;

    return true;
}

bool media_codec_source_read(MediaCodecSourceWrapper *source, MediaBufferWrapper **buffer)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d)
        return false;

    android::MediaBuffer *buff = NULL;
    android::status_t err = d->codec->read(&buff);
    if (err != android::OK)
        return false;

    *buffer = new MediaBufferPrivate(buff);

    return true;
}

bool media_codec_source_request_idr_frame(MediaCodecSourceWrapper *source)
{
    MediaCodecSourcePrivate *d = MediaCodecSourcePrivate::toPrivate(source);
    if (!d || !d->codec.get())
        return false;

    android::status_t err = d->codec->requestIDRFrame();
    if (err != android::OK)
        return false;

    return true;
}
