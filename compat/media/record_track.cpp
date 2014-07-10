/*
 * Copyright (C) 2014 Canonical Ltd
 * Copyright 2012, The Android Open Source Project
 * NOTE: Reimplemented starting from Android RecordThread class
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
#define LOG_TAG "RecordTrack"

#include "record_track.h"
#include "record_thread.h"

#include <utils/Atomic.h>
#include <binder/IPCThreadState.h>
#include <binder/MemoryDealer.h>
#include <utils/Log.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

namespace android {

static volatile int32_t nextTrackId = 55;

RecordTrack::RecordTrack(ThreadBase *thread,
            uint32_t sampleRate,
            audio_format_t format,
            audio_channel_mask_t channelMask,
            size_t frameCount,
            const sp<IMemory>& sharedBuffer,
            int sessionId,
            int clientUid)
    : RefBase(),
      mThread(thread),
      mMemoryDealer(new MemoryDealer(1024*1024, "AudioFlinger::Client")),
      mCblk(NULL),
      // mBuffer
      mState(IDLE),
      mSampleRate(sampleRate),
      mFormat(format),
      mChannelMask(channelMask),
      mChannelCount(popcount(channelMask)),
      mFrameSize(audio_is_linear_pcm(format) ?
                mChannelCount * audio_bytes_per_sample(format) : sizeof(int8_t)),
      mSessionId(sessionId),
      mOverflow(false),
      mId(android_atomic_inc(&nextTrackId)),
      mTerminated(false)
{
    REPORT_FUNCTION();

    // This was originally in AudioFlinger's ctor
    getpid_cached = getpid();

    // if the caller is us, trust the specified uid
    if (IPCThreadState::self()->getCallingPid() != getpid_cached || clientUid == -1) {
        int newclientUid = IPCThreadState::self()->getCallingUid();
        if (clientUid != -1 && clientUid != newclientUid) {
            ALOGW("uid %d tried to pass itself off as %d", newclientUid, clientUid);
        }
        clientUid = newclientUid;
    }
    // clientUid contains the uid of the app that is responsible for this track, so we can blame
    // battery usage on it.
    mUid = clientUid;

    ALOGV_IF(sharedBuffer != 0, "sharedBuffer: %p, size: %d", sharedBuffer->pointer(),
            sharedBuffer->size());

    size_t size = sizeof(audio_track_cblk_t);
    size_t bufferSize = (sharedBuffer == 0 ? roundup(frameCount) : frameCount) * mFrameSize;
    if (sharedBuffer == 0) {
        size += bufferSize;
    }
    ALOGD("Creating track with buffers @ %d bytes", bufferSize);

    if (mMemoryDealer != 0) {
        mCblkMemory = mMemoryDealer->allocate(size);
        if (mCblkMemory != 0) {
            mCblk = static_cast<audio_track_cblk_t *>(mCblkMemory->pointer());
            // can't assume mCblk != NULL
        } else {
            ALOGE("not enough memory for AudioTrack size=%u", size);
            mMemoryDealer->dump("AudioTrack");
            return;
        }
    } else {
        // this syntax avoids calling the audio_track_cblk_t constructor twice
        mCblk = (audio_track_cblk_t *) new uint8_t[size];
        // assume mCblk != NULL
    }

    // construct the shared structure in-place.
    if (mCblk != NULL) {
        new(mCblk) audio_track_cblk_t();
        // clear all buffers
        mCblk->frameCount_ = frameCount;
        if (sharedBuffer == 0) {
            mBuffer = (char*)mCblk + sizeof(audio_track_cblk_t);
            memset(mBuffer, 0, bufferSize);
        } else {
            mBuffer = sharedBuffer->pointer();
#if 0
            mCblk->mFlags = CBLK_FORCEREADY;    // FIXME hack, need to fix the track ready logic
#endif
        }
    }

    if (mCblk != NULL) {
        mAudioRecordServerProxy = new AudioRecordServerProxy(mCblk, mBuffer, frameCount, mFrameSize);
        mServerProxy = mAudioRecordServerProxy;
    }
}

RecordTrack::~RecordTrack()
{
    REPORT_FUNCTION();
}

status_t RecordTrack::start(AudioSystem::sync_event_t event, int triggerSession)
{
    REPORT_FUNCTION();

    sp<ThreadBase> thread = mThread.promote();
    if (thread != 0) {
        RecordThread *recordThread = (RecordThread *)thread.get();
        return recordThread->start(this, event, triggerSession);
    } else {
        return BAD_VALUE;
    }
}

void RecordTrack::stop()
{
    REPORT_FUNCTION();

    sp<ThreadBase> thread = mThread.promote();
    if (thread != 0) {
        RecordThread *recordThread = (RecordThread *)thread.get();
        if (recordThread->stop(this)) {
            ALOGV("AudioFlinger used to stop the input here, not stopping with Pulseaudio");
        }
    }
}

status_t RecordTrack::getNextBuffer(AudioBufferProvider::Buffer* buffer, int64_t pts)
{
    REPORT_FUNCTION();

    ServerProxy::Buffer buf;
    buf.mFrameCount = buffer->frameCount;
    status_t status = mServerProxy->obtainBuffer(&buf);
    buffer->frameCount = buf.mFrameCount;
    buffer->raw = buf.mRaw;
    if (buf.mFrameCount == 0) {
        // FIXME also wake futex so that overrun is noticed more quickly
        (void) android_atomic_or(CBLK_OVERRUN, &mCblk->mFlags);
    }
    return status;
}

void RecordTrack::releaseBuffer(AudioBufferProvider::Buffer* buffer)
{
    REPORT_FUNCTION();

    ServerProxy::Buffer buf;
    buf.mFrameCount = buffer->frameCount;
    buf.mRaw = buffer->raw;
    buffer->frameCount = 0;
    buffer->raw = NULL;
    mServerProxy->releaseBuffer(&buf);
}

void RecordTrack::destroy()
{
    REPORT_FUNCTION();

    // see comments at AudioFlinger::PlaybackThread::Track::destroy()
    sp<RecordTrack> keep(this);
    {
        sp<ThreadBase> thread = mThread.promote();
        if (thread != 0) {
            if (mState == ACTIVE || mState == RESUMING) {
                ALOGV("AudioFlinger used to stop the input here, not stopping with Pulseaudio");
            }
            Mutex::Autolock _l(thread->mLock);
            RecordThread *recordThread = (RecordThread *) thread.get();
            recordThread->destroyTrack_l(this);
        }
    }
}

void RecordTrack::invalidate()
{
    REPORT_FUNCTION();
#if 0
    // FIXME should use proxy, and needs work
    audio_track_cblk_t* cblk = mCblk;
    android_atomic_or(CBLK_INVALID, &cblk->mFlags);
    android_atomic_release_store(0x40000000, &cblk->mFutex);
    // client is not in server, so FUTEX_WAKE is needed instead of FUTEX_WAKE_PRIVATE
    (void) __futex_syscall3(&cblk->mFutex, FUTEX_WAKE, INT_MAX);
#endif
}

RecordHandle::RecordHandle(
        const sp<RecordTrack>& recordTrack)
    : BnAudioRecord(),
    mRecordTrack(recordTrack)
{
    REPORT_FUNCTION();
}

RecordHandle::~RecordHandle()
{
    REPORT_FUNCTION();

    stop_nonvirtual();
    mRecordTrack->destroy();
}

sp<IMemory> RecordHandle::getCblk() const
{
    REPORT_FUNCTION();

    return mRecordTrack->getCblk();
}

status_t RecordHandle::start(int /*AudioSystem::sync_event_t*/ event,
        int triggerSession)
        {
    REPORT_FUNCTION();

    return mRecordTrack->start((AudioSystem::sync_event_t)event, triggerSession);
}

void RecordHandle::stop()
{
    REPORT_FUNCTION();

    stop_nonvirtual();
}

void RecordHandle::stop_nonvirtual()
{
    REPORT_FUNCTION();

    mRecordTrack->stop();
}

status_t RecordHandle::onTransact(
    uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags)
{
    REPORT_FUNCTION();

    return BnAudioRecord::onTransact(code, data, reply, flags);
}

} // namespace android
