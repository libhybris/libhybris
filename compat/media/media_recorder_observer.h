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

#ifndef MEDIA_RECORDER_OBSERVER_H_
#define MEDIA_RECORDER_OBSERVER_H_

#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <hybris/media/media_recorder_layer.h>

namespace android {

enum {
    RECORDING_STARTED = IBinder::FIRST_CALL_TRANSACTION,
    RECORDING_STOPPED,
};

class IMediaRecorderObserver: public IInterface
{
public:
    DECLARE_META_INTERFACE(MediaRecorderObserver);

    static const char* exported_service_name() { return "android.media.IMediaRecorderObserver"; }

    virtual void recordingStarted(void) = 0;
    virtual void recordingStopped(void) = 0;
};

class BnMediaRecorderObserver: public BnInterface<IMediaRecorderObserver>
{
public:
    virtual status_t onTransact( uint32_t code,
                                 const Parcel& data,
                                 Parcel* reply,
                                 uint32_t flags = 0);
};

class BpMediaRecorderObserver: public BpInterface<IMediaRecorderObserver>
{
public:
    BpMediaRecorderObserver(const sp<IBinder>& impl)
        : BpInterface<IMediaRecorderObserver>(impl)
    {
    }

    virtual void recordingStarted()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorderObserver::getInterfaceDescriptor());
        remote()->transact(RECORDING_STARTED, data, &reply);
        return;
    }

    virtual void recordingStopped()
    {
        Parcel data, reply;
        data.writeInterfaceToken(IMediaRecorderObserver::getInterfaceDescriptor());
        remote()->transact(RECORDING_STOPPED, data, &reply);
        return;
    }
};

class MediaRecorderObserver : public BnMediaRecorderObserver
{
public:
    MediaRecorderObserver();
    ~MediaRecorderObserver() = default;

    virtual void recordingStarted(void);
    virtual void recordingStopped(void);

    virtual void setRecordingSignalCb(media_recording_started_cb cb, void *context);

private:
    media_recording_started_cb media_recording_started;
    void *cb_context;
};

}; // namespace android

#endif
