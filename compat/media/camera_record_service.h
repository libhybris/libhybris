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
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

 #ifndef CAMERA_RECORD_SERVICE_H
 #define CAMERA_RECORD_SERVICE_H

#include <media/IAudioRecord.h>
#include <binder/IInterface.h>
#include <binder/Parcel.h>

#include <system/audio.h>
#include <binder/IPCThreadState.h>
#include <utils/threads.h>

namespace android {

class RecordThread;

class ICameraRecordService : public IInterface
{
public:
    DECLARE_META_INTERFACE(CameraRecordService);

    static const char* exported_service_name() { return "android.media.ICameraRecordService"; }

    virtual status_t initRecord(
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask) = 0;
    virtual sp<IAudioRecord> openRecord(
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask,
                                size_t frameCount,
                                pid_t tid,
                                int *sessionId,
                                status_t *status) = 0;

};

class BnCameraRecordService : public BnInterface<ICameraRecordService>
{
public:
    BnCameraRecordService();
    virtual ~BnCameraRecordService();

    virtual status_t onTransact(uint32_t code, const Parcel& data,
                                Parcel* reply, uint32_t flags = 0);
};

enum {
    INIT_RECORD = IBinder::FIRST_CALL_TRANSACTION,
    OPEN_RECORD,
};

class BpCameraRecordService : public BpInterface<ICameraRecordService>
{
public:
    BpCameraRecordService(const sp<IBinder>& impl);
    ~BpCameraRecordService();

    virtual status_t initRecord(
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask);
    virtual sp<IAudioRecord> openRecord(
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask,
                                size_t frameCount,
                                pid_t tid,
                                int *sessionId,
                                status_t *status);
};

// ----------------------------------------------------------------------------

class CameraRecordService : public BnCameraRecordService
{
public:
    CameraRecordService();
    virtual ~CameraRecordService();

    static void instantiate();

    uint32_t nextUniqueId();

    virtual status_t initRecord(
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask);
    virtual sp<IAudioRecord> openRecord(
                                uint32_t sampleRate,
                                audio_format_t format,
                                audio_channel_mask_t channelMask,
                                size_t frameCount,
                                pid_t tid,
                                int *sessionId,
                                status_t *status);

private:
    static sp<CameraRecordService>& service_instance();

    sp<RecordThread> mRecordThread;
    volatile int32_t mNextUniqueId;  // updated by android_atomic_inc
    mutable Mutex mLock;

    static sp<CameraRecordService> camera_record_service;
    static Mutex s_lock;
};

} // namespace android

 #endif // CAMERA_RECORD_SERVICE_H
