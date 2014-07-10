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

#define LOG_NDEBUG 0
#define LOG_TAG "ICameraRecordService"

#include "camera_record_service.h"
#include "record_thread.h"
#include "record_track.h"

#include <binder/IServiceManager.h>

#include <utils/Log.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

namespace android {

// IDecodingServiceSession

BpCameraRecordService::BpCameraRecordService(const sp<IBinder>& impl)
    : BpInterface<ICameraRecordService>(impl)
{
    REPORT_FUNCTION();
}

BpCameraRecordService::~BpCameraRecordService()
{
    REPORT_FUNCTION();
}

status_t BpCameraRecordService::initRecord(
        uint32_t sampleRate,
        audio_format_t format,
        audio_channel_mask_t channelMask)
{
    REPORT_FUNCTION();

    Parcel data, reply;
    data.writeInterfaceToken(ICameraRecordService::getInterfaceDescriptor());
    data.writeInt32(sampleRate);
    data.writeInt32(format);
    data.writeInt32(channelMask);
    return remote()->transact(OPEN_RECORD, data, &reply);
}

sp<IAudioRecord> BpCameraRecordService::openRecord(uint32_t sampleRate,
                            audio_format_t format,
                            audio_channel_mask_t channelMask,
                            size_t frameCount,
                            pid_t tid,
                            int *sessionId,
                            status_t *status)
{
    REPORT_FUNCTION();

    Parcel data, reply;
    sp<IAudioRecord> record;
    data.writeInterfaceToken(ICameraRecordService::getInterfaceDescriptor());
    data.writeInt32(sampleRate);
    data.writeInt32(format);
    data.writeInt32(channelMask);
    data.writeInt32(frameCount);
    data.writeInt32((int32_t) tid);
    int lSessionId = 0;
    if (sessionId != 0)
        lSessionId = *sessionId;
    data.writeInt32(lSessionId);

    status_t lStatus = remote()->transact(OPEN_RECORD, data, &reply);
    if (lStatus != NO_ERROR)
        ALOGE("openRecord error: %s", strerror(-lStatus));
    else {
        lStatus = reply.readInt32();
        record = interface_cast<IAudioRecord>(reply.readStrongBinder());
        if (lStatus == NO_ERROR) {
            if (record == 0) {
                ALOGE("openRecord should have returned an IAudioRecord instance");
                lStatus = UNKNOWN_ERROR;
            }
        } else {
            if (record != 0) {
                ALOGE("openRecord returned an IAudioRecord instance but with status %d", lStatus);
                record.clear();
            }
        }
    }
    if (status)
        *status = lStatus;

    return record;
}

// ----------------------------------------------------------------------------

IMPLEMENT_META_INTERFACE(CameraRecordService, "android.media.ICameraRecordService");

BnCameraRecordService::BnCameraRecordService()
{
    REPORT_FUNCTION();
}

BnCameraRecordService::~BnCameraRecordService()
{
    REPORT_FUNCTION();
}

status_t BnCameraRecordService::onTransact(uint32_t code, const Parcel& data,
            Parcel* reply, uint32_t flags)
{
    REPORT_FUNCTION();

    switch (code) {
        case INIT_RECORD: {
            CHECK_INTERFACE(ICameraRecordService, data, reply);
            uint32_t sampleRate = data.readInt32();
            audio_format_t format = (audio_format_t) data.readInt32();
            audio_channel_mask_t channelMask = data.readInt32();
            reply->writeInt32(initRecord(sampleRate, format, channelMask));
            return NO_ERROR;
        } break;
        case OPEN_RECORD: {
            CHECK_INTERFACE(ICameraRecordService, data, reply);
            uint32_t sampleRate = data.readInt32();
            audio_format_t format = (audio_format_t) data.readInt32();
            audio_channel_mask_t channelMask = data.readInt32();
            size_t frameCount = data.readInt32();
            pid_t tid = (pid_t) data.readInt32();
            int sessionId = data.readInt32();
            status_t status;
            sp<IAudioRecord> record = openRecord(sampleRate, format, channelMask,
                frameCount, tid, &sessionId, &status);
            LOG_ALWAYS_FATAL_IF((record != 0) != (status == NO_ERROR));

            reply->writeInt32(sessionId);
            reply->writeInt32(status);
            reply->writeStrongBinder(record->asBinder());
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }

    return NO_ERROR;
}

// ----------------------------------------------------------------------------

sp<CameraRecordService> CameraRecordService::camera_record_service;
Mutex CameraRecordService::s_lock;

CameraRecordService::CameraRecordService()
    : mNextUniqueId(1)
{
    REPORT_FUNCTION();
}

CameraRecordService::~CameraRecordService()
{
    REPORT_FUNCTION();
}

void CameraRecordService::instantiate()
{
    REPORT_FUNCTION();

    defaultServiceManager()->addService(
            String16(ICameraRecordService::exported_service_name()), service_instance());
    ALOGV("Added Binder service '%s' to ServiceManager", ICameraRecordService::exported_service_name());
}

uint32_t CameraRecordService::nextUniqueId()
{
    REPORT_FUNCTION();
    return android_atomic_inc(&mNextUniqueId);
}

status_t CameraRecordService::initRecord(
        uint32_t sampleRate,
        audio_format_t format,
        audio_channel_mask_t channelMask)
{
    REPORT_FUNCTION();

    Mutex::Autolock _l(mLock);
    audio_io_handle_t id = nextUniqueId();

    mRecordThread = new RecordThread(sampleRate,
                              channelMask,
                              id);
    if (mRecordThread == NULL) {
        ALOGE("Failed to instantiate a new RecordThread, audio recording will not function");
        return UNKNOWN_ERROR;
    }

    return NO_ERROR;
}

sp<IAudioRecord> CameraRecordService::openRecord(uint32_t sampleRate,
                            audio_format_t format,
                            audio_channel_mask_t channelMask,
                            size_t frameCount,
                            pid_t tid,
                            int *sessionId,
                            status_t *status)
{
    REPORT_FUNCTION();

    status_t lStatus;
    sp<RecordTrack> recordTrack;
    sp<RecordHandle> recordHandle;
    size_t inFrameCount = 0;
    int lSessionId = 0;

    if (mRecordThread == NULL) {
        lStatus = UNKNOWN_ERROR;
        ALOGE("mRecordThread is NULL, call initRecord() first");
        goto Exit;
    }

    if (format != AUDIO_FORMAT_PCM_16_BIT) {
        ALOGE("openRecord() invalid format %d", format);
        lStatus = BAD_VALUE;
        goto Exit;
    }

    { // scope for mLock
        Mutex::Autolock _l(mLock);
        // If no audio session id is provided, create one here
        if (sessionId != NULL && *sessionId != AUDIO_SESSION_OUTPUT_MIX) {
            lSessionId = *sessionId;
        } else {
            lSessionId = nextUniqueId();
            if (sessionId != NULL) {
                *sessionId = lSessionId;
            }
        }
        // create new record track.
        // The record track uses one track in mHardwareMixerThread by convention.
        // TODO: the uid should be passed in as a parameter to openRecord
        recordTrack = mRecordThread->createRecordTrack_l(sampleRate, format, channelMask,
                                                  frameCount, lSessionId,
                                                  IPCThreadState::self()->getCallingUid(),
                                                  tid, &lStatus);
        LOG_ALWAYS_FATAL_IF((recordTrack != 0) != (lStatus == NO_ERROR));
    }

    if (lStatus != NO_ERROR) {
        recordTrack.clear();
        goto Exit;
    }

    // return to handle to client
    recordHandle = new RecordHandle(recordTrack);
    lStatus = NO_ERROR;

Exit:
    if (status) {
        *status = lStatus;
    }
    return recordHandle;
}

sp<CameraRecordService>& CameraRecordService::service_instance()
{
    REPORT_FUNCTION();

    Mutex::Autolock _l(s_lock);
    if (camera_record_service == NULL)
    {
        ALOGD("Creating new static instance of CameraRecordService");
        camera_record_service = new CameraRecordService();
    }

    return camera_record_service;
}

} // namespace android
