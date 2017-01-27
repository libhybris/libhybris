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

#include <hybris/media/media_buffer_layer.h>

#include "media_buffer_priv.h"
#include "media_meta_data_priv.h"
#include "media_message_priv.h"

MediaBufferPrivate* MediaBufferPrivate::toPrivate(MediaBufferWrapper *buffer)
{
    if (!buffer)
        return NULL;

    return static_cast<MediaBufferPrivate*>(buffer);
}

MediaBufferPrivate::MediaBufferPrivate(android::MediaBuffer *buffer) :
    buffer(buffer),
    return_callback(NULL),
    return_callback_data(NULL)
{
}

MediaBufferPrivate::MediaBufferPrivate() :
    buffer(NULL)
{
}

MediaBufferPrivate::~MediaBufferPrivate()
{
    if (buffer)
        buffer->release();
}

void MediaBufferPrivate::signalBufferReturned(android::MediaBuffer *buffer)
{
    if (buffer != this->buffer) {
        ALOGE("Got called for unknown buffer %p", buffer);
        return;
    }

    if (!return_callback)
        return;

    return_callback(this, return_callback_data);
}

MediaBufferWrapper* media_buffer_create(size_t size)
{
    android::MediaBuffer *mbuf = new android::MediaBuffer(size);
    if (!mbuf)
        return NULL;

    MediaBufferPrivate *d = new MediaBufferPrivate(mbuf);
    if (!d) {
        mbuf->release();
        return NULL;
    }

    return d;
}

void media_buffer_destroy(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d)
        return;

    delete d;
}


void media_buffer_release(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer)
        return;

    d->buffer->release();
}

void media_buffer_ref(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d)
        return;

    d->buffer->add_ref();
}

int media_buffer_get_refcount(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer)
        return 0;

    return d->buffer->refcount();
}

void* media_buffer_get_data(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer)
        return NULL;

    return d->buffer->data();
}

size_t media_buffer_get_size(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer)
        return 0;

    return d->buffer->size();
}

size_t media_buffer_get_range_offset(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer)
        return 0;

    return d->buffer->range_offset();
}

size_t media_buffer_get_range_length(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer)
        return 0;

    return d->buffer->range_length();
}

MediaMetaDataWrapper* media_buffer_get_meta_data(MediaBufferWrapper *buffer)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer)
        return NULL;

    return new MediaMetaDataPrivate(d->buffer->meta_data());
}

void media_buffer_set_return_callback(MediaBufferWrapper *buffer,
    MediaBufferReturnCallback callback, void *user_data)
{
    MediaBufferPrivate *d = MediaBufferPrivate::toPrivate(buffer);
    if (!d)
        return;

    d->return_callback = callback;
    d->return_callback_data = user_data;

    if (d->return_callback)
        d->buffer->setObserver(d);
    else
        d->buffer->setObserver(NULL);
}

MediaABufferPrivate* MediaABufferPrivate::toPrivate(MediaABufferWrapper *buffer)
{
    if (!buffer)
        return NULL;

    return static_cast<MediaABufferPrivate*>(buffer);
}

MediaABufferPrivate::MediaABufferPrivate()
{
}

MediaABufferPrivate::MediaABufferPrivate(android::sp<android::ABuffer> buffer) :
    buffer(buffer)
{
}

MediaABufferWrapper* media_abuffer_create(size_t capacity)
{
    MediaABufferPrivate *d = new MediaABufferPrivate;
    if (!d)
        return NULL;

    d->buffer = new android::ABuffer(capacity);
    if (!d->buffer.get()) {
        delete d;
        return NULL;
    }

    return d;
}

MediaABufferWrapper* media_abuffer_create_with_data(uint8_t *data, size_t size)
{
    MediaABufferPrivate *d = new MediaABufferPrivate;
    if (!d)
        return NULL;

    d->buffer = new android::ABuffer(data, size);
    if (!d->buffer.get()) {
        delete d;
        return NULL;
    }

    return d;
}

void media_abuffer_set_range(MediaABufferWrapper *buffer, size_t offset, size_t size)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return;

    d->buffer->setRange(offset, size);
}

void media_abuffer_set_media_buffer_base(MediaABufferWrapper *buffer, MediaBufferWrapper *mbuf)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return;

#if ANDROID_VERSION_MAJOR>=5
    android::MediaBuffer *media_buffer = NULL;

    if (mbuf != NULL)
        media_buffer = MediaBufferPrivate::toPrivate(mbuf)->buffer;

    d->buffer->setMediaBufferBase(media_buffer);
#else
    return;
#endif
}

MediaBufferWrapper* media_abuffer_get_media_buffer_base(MediaABufferWrapper *buffer)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return NULL;

#if ANDROID_VERSION_MAJOR>= 5

    android::MediaBufferBase *mbufb = d->buffer->getMediaBufferBase();
    if (mbufb == NULL)
        return NULL;

    MediaBufferPrivate *mbuf = new MediaBufferPrivate;
    mbuf->buffer = (android::MediaBuffer*) mbufb;

    return mbuf;
#else
    return NULL;
#endif
}

void* media_abuffer_get_data(MediaABufferWrapper *buffer)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return NULL;

    return d->buffer->data();
}

size_t media_abuffer_get_size(MediaABufferWrapper *buffer)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return 0;

    return d->buffer->size();
}

size_t media_abuffer_get_range_offset(MediaABufferWrapper *buffer)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return 0;

    return d->buffer->offset();
}

size_t media_abuffer_get_capacity(MediaABufferWrapper *buffer)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return 0;

    return d->buffer->capacity();
}

MediaMessageWrapper* media_abuffer_get_meta(MediaABufferWrapper *buffer)
{
    MediaABufferPrivate *d = MediaABufferPrivate::toPrivate(buffer);
    if (!d || !d->buffer.get())
        return NULL;

    MediaMessagePrivate *msg = new MediaMessagePrivate;
    if (!msg)
        return NULL;

    msg->msg = d->buffer->meta();

    return msg;
}
