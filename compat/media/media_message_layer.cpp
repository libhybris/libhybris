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

#include <hybris/media/media_message_layer.h>

#include "media_message_priv.h"

MediaMessagePrivate* MediaMessagePrivate::toPrivate(MediaMessageWrapper *msg)
{
    if (!msg)
        return NULL;

    return static_cast<MediaMessagePrivate*>(msg);
}

MediaMessagePrivate::MediaMessagePrivate() :
    msg(new android::AMessage)
{
}

MediaMessagePrivate::~MediaMessagePrivate()
{
}

MediaMessageWrapper* media_message_create()
{
    MediaMessagePrivate *d = new MediaMessagePrivate;
    if (!d)
        return NULL;

    return d;
}

void media_message_release(MediaMessageWrapper *msg)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;

    delete d;
}

void media_message_clear(MediaMessageWrapper *msg)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;

    d->msg->clear();
}

const char* media_message_dump(MediaMessageWrapper *msg)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return NULL;

    return d->msg->debugString().c_str();
}

void media_message_set_int32(MediaMessageWrapper *msg, const char *name, int32_t value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;

    d->msg->setInt32(name, value);
}

void media_message_set_int64(MediaMessageWrapper *msg, const char *name, int64_t value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;

    d->msg->setInt64(name, value);
}

void media_message_set_size(MediaMessageWrapper *msg, const char *name, ssize_t value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;

    d->msg->setSize(name, value);
}

void media_message_set_float(MediaMessageWrapper *msg, const char *name, float value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;

    d->msg->setFloat(name, value);
}

void media_message_set_double(MediaMessageWrapper *msg, const char *name, double value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;

    d->msg->setDouble(name, value);
}

void media_message_set_string(MediaMessageWrapper *msg, const char *name, const char *value, ssize_t len)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return;


    d->msg->setString(name, value);
}

bool media_message_find_int32(MediaMessageWrapper *msg, const char *name, int32_t *value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return false;

    return d->msg->findInt32(name, value);
}

bool media_message_find_int64(MediaMessageWrapper *msg, const char *name, int64_t *value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return false;

    int64_t v;
    if (!d->msg->findInt64(name, &v))
        return false;

    if (value)
        *value = v;
    return true;
}

bool media_message_find_size(MediaMessageWrapper *msg, const char *name, size_t *value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return false;

    size_t v;
    if (d->msg->findSize(name, &v))
        return false;

    if (value)
        *value = v;

    return true;
}

bool media_message_find_float(MediaMessageWrapper *msg, const char *name, float *value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return false;

    float v;
    if (!d->msg->findFloat(name, &v))
        return false;

    if (value)
        *value = v;

    return true;
}

bool media_message_find_double(MediaMessageWrapper *msg, const char *name, double *value)
{
    MediaMessagePrivate *d = MediaMessagePrivate::toPrivate(msg);
    if (!d)
        return false;

    double v;
    if (!d->msg->findDouble(name, &v))
        return false;

    if (value)
        *value = v;

    return true;
}
