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

#ifndef MEDIA_META_DATA_PRIV_H_
#define MEDIA_META_DATA_PRIV_H_

#include <media/stagefright/MetaData.h>
#if ANDROID_VERSION_MAJOR>=8
#include <media/MediaBufferHolder.h>
#endif

struct MediaMetaDataPrivate
{
public:
    static MediaMetaDataPrivate* toPrivate(MediaMetaDataWrapper *md);

#if ANDROID_VERSION_MAJOR>=8
    struct MetaDataPtr {
        MetaDataPtr(android::MediaBufferBase *buf = nullptr):
            buffer(buf)
        {
            if (buffer) {
                buffer->add_ref();
                data = &buffer->meta_data();
            } else {
                data = new android::MetaDataBase;
            }
        }

        MetaDataPtr(const android::sp<android::MetaData> &md):
            buffer(nullptr),
            data(new android::MetaDataBase(*md))
        {
        }

        ~MetaDataPtr() {
            if (buffer) {
                buffer->release();
                buffer = nullptr;
                data = nullptr;
            } else {
                delete data;
            }
        }

        inline android::MetaDataBase &operator*() const { return *data; }
        inline android::MetaDataBase *operator->() const { return data; }
        inline android::MetaDataBase *get() const { return data; }
        inline operator android::MetaData *() { return new android::MetaData(*data); }

    private:
        android::MediaBufferBase *buffer;
        android::MetaDataBase *data;
    };
#endif

    MediaMetaDataPrivate();
#if ANDROID_VERSION_MAJOR>=8
    MediaMetaDataPrivate(android::MediaBufferBase *buffer);
#endif
    MediaMetaDataPrivate(const android::sp<android::MetaData> &md);
    ~MediaMetaDataPrivate();

#if ANDROID_VERSION_MAJOR>=8
    MetaDataPtr data;
#else
    android::sp<android::MetaData> data;
#endif
};

#endif
