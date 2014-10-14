/*
 * Copyright (C) 2014 Canonical Ltd
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
 * Authored by: Ricardo Mendoza <ricardo.mendoza@canonical.com>
 */

// Uncomment to enabe verbose debug output
//#define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "MediaRecorderObserver"

#include "media_recorder_observer.h"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <binder/ProcessState.h>

#include <binder/IServiceManager.h>

#include <utils/Vector.h>
#include <utils/Log.h>
#include <utils/RefBase.h>

namespace android {

IMPLEMENT_META_INTERFACE(MediaRecorderObserver, "android.media.IMediaRecorderObserver");

status_t BnMediaRecorderObserver::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    switch (code) {
        case RECORDING_STARTED: {
            CHECK_INTERFACE(IMediaRecorderObserver, data, reply);
            recordingStarted();

            return NO_ERROR;
        } break;
        case RECORDING_STOPPED: {
            CHECK_INTERFACE(IMediaRecorderObserver, data, reply);
            recordingStopped();

            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

MediaRecorderObserver::MediaRecorderObserver()
{
    defaultServiceManager()->addService(
        String16(IMediaRecorderObserver::exported_service_name()), this);

    ProcessState::self()->startThreadPool();
}

void MediaRecorderObserver::recordingStarted()
{
    if (media_recording_started != nullptr)
        media_recording_started(true, cb_context);
}

void MediaRecorderObserver::recordingStopped()
{
    if (media_recording_started != nullptr)
        media_recording_started(false, cb_context);
}

void MediaRecorderObserver::setRecordingSignalCb(media_recording_started_cb cb, void *context)
{
    if (cb != NULL) {
        cb_context = context;
        media_recording_started = cb;
    }
}

};

// C API

struct MediaRecorderObserver {
    MediaRecorderObserver(android::MediaRecorderObserver *observer)
        : impl(observer)
    {
    }

    android::MediaRecorderObserver* impl;
};

MediaRecorderObserver* android_media_recorder_observer_new()
{
    MediaRecorderObserver *p = new MediaRecorderObserver(new android::MediaRecorderObserver);

    if (p == NULL) {
        ALOGE("Failed to create new MediaRecorderObserver instance.");
        return NULL;
    }

    return p;
}

void android_media_recorder_observer_set_cb(MediaRecorderObserver *observer, media_recording_started_cb cb, void *context)
{
    if (observer == NULL)
        return;

    auto p = observer->impl;

    p->setRecordingSignalCb(cb, context);
}
