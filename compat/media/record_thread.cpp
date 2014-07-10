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
#define LOG_TAG "RecordThread"

#include "record_thread.h"
#include "record_track.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <hybris/media/media_recorder_layer.h>
#include <audio_utils/primitives.h>

#include <utils/Log.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

namespace android {

// don't warn about blocked writes or record buffer overflows more often than this
static const nsecs_t kWarningThrottleNs = seconds(5);

// RecordThread loop sleep time upon application overrun or audio HAL read error
static const int kRecordThreadSleepUs = 5000;

ThreadBase::ThreadBase(audio_io_handle_t id)
    : Thread(false),
      mStandby(false),
      mId(id)
{
}

ThreadBase::~ThreadBase()
{
}

void ThreadBase::exit()
{
}

//---------- RecordThread -----------//

RecordThread::RecordThread(uint32_t sampleRate, audio_channel_mask_t channelMask, audio_io_handle_t id)
    : ThreadBase(id),
      m_fifoFd(-1),
      mRsmpOutBuffer(NULL),
      mRsmpInBuffer(NULL),
      mReqChannelCount(popcount(channelMask)),
      mReqSampleRate(sampleRate),
      mFramestoDrop(0)
{
    REPORT_FUNCTION();

    snprintf(mName, kNameLength, "AudioIn_%X", id);
    readInputParameters();

}

RecordThread::~RecordThread()
{
    REPORT_FUNCTION();

    close(m_fifoFd);
}

void RecordThread::destroyTrack_l(const sp<RecordTrack>& track)
{
    REPORT_FUNCTION();

    track->terminate();
    track->mState = RecordTrack::STOPPED;
    // active tracks are removed by threadLoop()
    if (mActiveTrack != track) {
        removeTrack_l(track);
    }
}

void RecordThread::removeTrack_l(const sp<RecordTrack>& track)
{
    REPORT_FUNCTION();

    mTracks.remove(track);
}

bool RecordThread::threadLoop()
{
    REPORT_FUNCTION();

    AudioBufferProvider::Buffer buffer;
    sp<RecordTrack> activeTrack;
    nsecs_t lastWarning = 0;

    //inputStandBy();
    {
        Mutex::Autolock _l(mLock);
        activeTrack = mActiveTrack;
        //acquireWakeLock_l(activeTrack != 0 ? activeTrack->uid() : -1);
    }

    // used to verify we've read at least once before evaluating how many bytes were read
    bool readOnce = false;

    // start recording
    while (!exitPending()) {

        //processConfigEvents();

        { // scope for mLock
            Mutex::Autolock _l(mLock);
            //checkForNewParameters_l();
            if (mActiveTrack != 0 && activeTrack != mActiveTrack) {
                SortedVector<int> tmp;
                tmp.add(mActiveTrack->uid());
            }
            activeTrack = mActiveTrack;
            if (mActiveTrack == 0) {
                if (exitPending()) {
                    break;
                }

                ALOGV("RecordThread: loop stopping");
                // go to sleep
                mWaitWorkCV.wait(mLock);
                ALOGV("RecordThread: loop starting");
                continue;
            }

            if (mActiveTrack != 0) {
                if (mActiveTrack->isTerminated()) {
                    removeTrack_l(mActiveTrack);
                    mActiveTrack.clear();
                } else if (mActiveTrack->mState == RecordTrack::PAUSING) {
                    mActiveTrack.clear();
                    mStartStopCond.broadcast();
                } else if (mActiveTrack->mState == RecordTrack::RESUMING) {
                    if (mReqChannelCount != mActiveTrack->channelCount()) {
                        mActiveTrack.clear();
                        mStartStopCond.broadcast();
                    } else if (readOnce) {
                        // record start succeeds only if first read from audio input
                        // succeeds
                        if (mBytesRead >= 0) {
                            mActiveTrack->mState = RecordTrack::ACTIVE;
                        } else {
                            mActiveTrack.clear();
                        }
                        mStartStopCond.broadcast();
                    }
                    mStandby = false;
                }
            }
        }

        if (mActiveTrack != 0) {
            if (mActiveTrack->mState != RecordTrack::ACTIVE &&
                mActiveTrack->mState != RecordTrack::RESUMING) {
                usleep(kRecordThreadSleepUs);
                continue;
            }

            buffer.frameCount = mFrameCount;
            status_t status = mActiveTrack->getNextBuffer(&buffer);
            if (status == NO_ERROR) {
                readOnce = true;
                size_t framesOut = buffer.frameCount;
                while (framesOut) {
                    size_t framesIn = mFrameCount - mRsmpInIndex;
                    if (framesIn) {
                        int8_t *src = (int8_t *)mRsmpInBuffer + mRsmpInIndex * mFrameSize;
                        int8_t *dst = buffer.i8 + (buffer.frameCount - framesOut) *
                            mActiveTrack->mFrameSize;
                        if (framesIn > framesOut)
                            framesIn = framesOut;
                        mRsmpInIndex += framesIn;
                        framesOut -= framesIn;
                        if (mChannelCount == mReqChannelCount) {
                            memcpy(dst, src, framesIn * mFrameSize);
                        } else {
                            if (mChannelCount == 1) {
                                upmix_to_stereo_i16_from_mono_i16((int16_t *)dst,
                                        (int16_t *)src, framesIn);
                            } else {
                                downmix_to_mono_i16_from_stereo_i16((int16_t *)dst,
                                        (int16_t *)src, framesIn);
                            }
                        }
                    }
                    if (framesOut && mFrameCount == mRsmpInIndex) {
                        void *readInto;
                        if (framesOut == mFrameCount && mChannelCount == mReqChannelCount) {
                            readInto = buffer.raw;
                            framesOut = 0;
                        } else {
                            readInto = mRsmpInBuffer;
                            mRsmpInIndex = 0;
                        }
                        // Read from the named pipe /dev/socket/micshm
                        mBytesRead = readPipe(readInto, mBufferSize);
                        if (mBytesRead <= 0) {
                            if ((mBytesRead < 0) && (mActiveTrack->mState == RecordTrack::ACTIVE))
                            {
                                ALOGE("Error reading audio input");
                                // Force input into standby so that it tries to
                                // recover at next read attempt
                                usleep(kRecordThreadSleepUs);
                            }
                            mRsmpInIndex = mFrameCount;
                            framesOut = 0;
                            buffer.frameCount = 0;
                        }
                    }
                }
                if (mFramestoDrop == 0) {
                    mActiveTrack->releaseBuffer(&buffer);
                } else {
                    if (mFramestoDrop > 0) {
                        mFramestoDrop -= buffer.frameCount;
                        if (mFramestoDrop <= 0) {
                            clearSyncStartEvent();
                        }
                    } else {
                        mFramestoDrop += buffer.frameCount;
                        if (mFramestoDrop >= 0 || mSyncStartEvent == 0 ||
                                mSyncStartEvent->isCancelled()) {
                            ALOGW("Synced record %s, session %d, trigger session %d",
                                  (mFramestoDrop >= 0) ? "timed out" : "cancelled",
                                  mActiveTrack->sessionId(),
                                  (mSyncStartEvent != 0) ? mSyncStartEvent->triggerSession() : 0);
                            clearSyncStartEvent();
                        }
                    }
                }
            }
            // client isn't retrieving buffers fast enough
            else {
                ALOGW("Client isn't retrieving buffers fast enough!");
                if (!mActiveTrack->setOverflow()) {
                    nsecs_t now = systemTime();
                    if ((now - lastWarning) > kWarningThrottleNs) {
                        ALOGW("RecordThread: buffer overflow");
                        lastWarning = now;
                    }
                }
                // Release the processor for a while before asking for a new buffer.
                // This will give the application more chance to read from the buffer and
                // clear the overflow.
                usleep(kRecordThreadSleepUs);
            }
        }
    }

    {
        Mutex::Autolock _l(mLock);
        for (size_t i = 0; i < mTracks.size(); i++) {
            sp<RecordTrack> track = mTracks[i];
            track->invalidate();
        }
        mActiveTrack.clear();
        mStartStopCond.broadcast();
    }

    ALOGV("RecordThread %p exiting", this);
    return false;
}

status_t RecordThread::readyToRun()
{
    REPORT_FUNCTION();

    return NO_ERROR;
}

void RecordThread::onFirstRef()
{
    REPORT_FUNCTION();

    run(mName, PRIORITY_URGENT_AUDIO);
}

sp<RecordTrack> RecordThread::createRecordTrack_l(
        uint32_t sampleRate,
        audio_format_t format,
        audio_channel_mask_t channelMask,
        size_t frameCount,
        int sessionId,
        int uid,
        pid_t tid,
        status_t *status)
{
    REPORT_FUNCTION();

    sp<RecordTrack> track;
    status_t lStatus;

    { // scope for mLock
        Mutex::Autolock _l(mLock);

        track = new RecordTrack(this, sampleRate,
                      format, channelMask, frameCount, 0 /* sharedBuffer */, sessionId, uid);

        if (track->getCblk() == 0) {
            ALOGE("createRecordTrack_l() no control block");
            lStatus = NO_MEMORY;
            track.clear();
            goto Exit;
        }
        mTracks.add(track);

    }
    lStatus = NO_ERROR;

Exit:
    if (status) {
        *status = lStatus;
    }
    return track;
}

status_t RecordThread::start(RecordTrack* recordTrack,
        AudioSystem::sync_event_t event,
        int triggerSession)
{
    ALOGV("RecordThread::start event %d, triggerSession %d", event, triggerSession);
    sp<ThreadBase> strongMe = this;
    status_t status = NO_ERROR;

    {
        AutoMutex lock(mLock);
        if (mActiveTrack != 0) {
            if (recordTrack != mActiveTrack.get()) {
                status = -EBUSY;
            } else if (mActiveTrack->mState == RecordTrack::PAUSING) {
                mActiveTrack->mState = RecordTrack::ACTIVE;
            }
            return status;
        }

        recordTrack->mState = RecordTrack::IDLE;
        mActiveTrack = recordTrack;

        if (status != NO_ERROR) {
            mActiveTrack.clear();
            clearSyncStartEvent();
            return status;
        }
        mRsmpInIndex = mFrameCount;
        mBytesRead = 0;

        mActiveTrack->mState = RecordTrack::RESUMING;
        // signal thread to start
        ALOGV("Signal record thread");
        mWaitWorkCV.broadcast();
        // do not wait for mStartStopCond if exiting
        if (exitPending()) {
            mActiveTrack.clear();
            status = INVALID_OPERATION;
            goto startError;
        }
        mStartStopCond.wait(mLock);
        if (mActiveTrack == 0) {
            ALOGV("Record failed to start");
            status = BAD_VALUE;
            goto startError;
        }
        ALOGV("Record started OK");
        return status;
    }

startError:
    clearSyncStartEvent();
    close(m_fifoFd);
    return status;
}

void RecordThread::clearSyncStartEvent()
{
    if (mSyncStartEvent != 0) {
        mSyncStartEvent->cancel();
    }
    mSyncStartEvent.clear();
    mFramestoDrop = 0;
}

bool RecordThread::stop(RecordTrack* recordTrack)
{
    REPORT_FUNCTION();

    AutoMutex _l(mLock);
    if (recordTrack != mActiveTrack.get() || recordTrack->mState == RecordTrack::PAUSING) {
        return false;
    }
    recordTrack->mState = RecordTrack::PAUSING;
    // do not wait for mStartStopCond if exiting
    if (exitPending()) {
        return true;
    }
    mStartStopCond.wait(mLock);
    // if we have been restarted, recordTrack == mActiveTrack.get() here
    if (exitPending() || recordTrack != mActiveTrack.get()) {
        ALOGV("Record stopped OK");
        return true;
    }
    return false;
}

status_t RecordThread::getNextBuffer(AudioBufferProvider::Buffer* buffer, int64_t pts)
{
    REPORT_FUNCTION();

    size_t framesReq = buffer->frameCount;
    size_t framesReady = mFrameCount - mRsmpInIndex;
    int channelCount;

    if (framesReady == 0) {
        // Read from the named pipe /dev/socket/micshm
        mBytesRead = readPipe(mRsmpInBuffer, mBufferSize);
        if (mBytesRead <= 0) {
            if ((mBytesRead < 0) && (mActiveTrack->mState == RecordTrack::ACTIVE)) {
                ALOGE("RecordThread::getNextBuffer() Error reading audio input");
                // Force input into standby so that it tries to
                // recover at next read attempt
                usleep(kRecordThreadSleepUs);
            }
            buffer->raw = NULL;
            buffer->frameCount = 0;
            return NOT_ENOUGH_DATA;
        }
        mRsmpInIndex = 0;
        framesReady = mFrameCount;
    }

    if (framesReq > framesReady) {
        framesReq = framesReady;
    }

    if (mChannelCount == 1 && mReqChannelCount == 2) {
        channelCount = 1;
    } else {
        channelCount = 2;
    }
    buffer->raw = mRsmpInBuffer + mRsmpInIndex * channelCount;
    buffer->frameCount = framesReq;
    return NO_ERROR;
}

void RecordThread::releaseBuffer(AudioBufferProvider::Buffer* buffer)
{
    REPORT_FUNCTION();

    mRsmpInIndex += buffer->frameCount;
    buffer->frameCount = 0;
}

void RecordThread::readInputParameters()
{
    REPORT_FUNCTION();

    // TODO: these are all hardcoded for right now, they should be
    // obtained through more dynamic means
    mSampleRate = 48000;
    mChannelMask = 0x10;   // FIXME: where should this come from?
    mChannelCount = popcount(mChannelMask);
    mFormat = AUDIO_FORMAT_PCM_16_BIT;
    mFrameSize = 2;
    mBufferSize = MIC_READ_BUF_SIZE * sizeof(int16_t);
    mFrameCount = mBufferSize / mFrameSize;
    mRsmpInBuffer = new int16_t[mBufferSize];
    mRsmpInIndex = mFrameCount;

    ALOGV("mSampleRate: %d", mSampleRate);
    ALOGV("mChannelMask: %d", mChannelMask);
    ALOGV("mChannelCount: %d", mChannelCount);
    ALOGV("mFormat: %d", mFormat);
    ALOGV("mFrameSize: %d", mFrameSize);
    ALOGV("mBufferSize: %d", mBufferSize);
    ALOGV("mFrameCount: %d", mFrameCount);
    ALOGV("mRsmpInIndex: %d", mRsmpInIndex);
}

bool RecordThread::openPipe()
{
    if (m_fifoFd > 0) {
        ALOGW("/dev/socket/micshm already opened, not opening twice");
        return true;
    }

    // Open read access to the named pipe that lives on the application side
    m_fifoFd = open("/dev/socket/micshm", O_RDONLY); //| O_NONBLOCK);
    if (m_fifoFd < 0) {
        ALOGE("Failed to open named pipe /dev/socket/micshm %s", strerror(errno));
        return false;
    }

    return true;
}

ssize_t RecordThread::readPipe(void *buffer, size_t size)
{
    REPORT_FUNCTION();

    if (buffer == NULL || size == 0)
    {
        ALOGE("Can't read named pipe, buffer is NULL or size is 0");
        return 0;
    }

    if (m_fifoFd < 0) {
        openPipe();
    }

    ssize_t readSize = read(m_fifoFd, buffer, size);
    if (readSize < 0)
    {
        ALOGE("Failed to read in data from named pipe /dev/socket/micshm: %s", strerror(errno));
        readSize = 0;
    }
    else
        ALOGV("Read in %d bytes into buffer", readSize);

    return readSize;
}

} // namespace android
