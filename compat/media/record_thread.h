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

 #ifndef RECORD_THREAD_H
 #define RECORD_THREAD_H

#include <media/AudioBufferProvider.h>
#include <media/AudioSystem.h>
#include <system/audio.h>
#include <utils/threads.h>

namespace android {

class RecordHandle;
class RecordTrack;

class ThreadBase : public Thread
{
public:
    ThreadBase(audio_io_handle_t id);
    virtual ~ThreadBase();

    void exit();

    // see note at declaration of mStandby, mOutDevice and mInDevice
    bool standby() const { return mStandby; }

protected:
    friend class RecordTrack;

    SortedVector < sp<RecordTrack> > mTracks;
    // mActiveTrack has dual roles:  it indicates the current active track, and
    // is used together with mStartStopCond to indicate start()/stop() progress
    sp<RecordTrack> mActiveTrack;
    Condition mStartStopCond;

    // These fields are written and read by thread itself without lock or barrier,
    // and read by other threads without lock or barrier via standby() , outDevice()
    // and inDevice().
    // Because of the absence of a lock or barrier, any other thread that reads
    // these fields must use the information in isolation, or be prepared to deal
    // with possibility that it might be inconsistent with other information.
    bool mStandby;   // Whether thread is currently in standby.
    const audio_io_handle_t mId;

    uint32_t mSampleRate;
    size_t mFrameCount;       // output HAL, direct output, record
    audio_channel_mask_t mChannelMask;
    uint32_t mChannelCount;
    size_t mFrameSize;
    audio_format_t mFormat;

    static const int kNameLength = 16;   // prctl(PR_SET_NAME) limit
    char mName[kNameLength];

    Condition mWaitWorkCV;
    mutable Mutex mLock;
};

//---------- RecordThread -----------//

class RecordThread : public ThreadBase, public AudioBufferProvider
                        // derives from AudioBufferProvider interface for use by resampler
{
public:
    RecordThread(uint32_t sampleRate,
            audio_channel_mask_t channelMask,
            audio_io_handle_t id
            );
    virtual ~RecordThread();

    void destroyTrack_l(const sp<RecordTrack>& track);
    void removeTrack_l(const sp<RecordTrack>& track);

    // Thread virtuals
    virtual bool        threadLoop();
    virtual status_t    readyToRun();

    // RefBase
    virtual void        onFirstRef();

    sp<RecordTrack> createRecordTrack_l(
            uint32_t sampleRate,
            audio_format_t format,
            audio_channel_mask_t channelMask,
            size_t frameCount,
            int sessionId,
            int uid,
            pid_t tid,
            status_t *status);

    status_t start(RecordTrack* recordTrack,
            AudioSystem::sync_event_t event,
            int triggerSession);

    // ask the thread to stop the specified track, and
    // return true if the caller should then do it's part of the stopping process
    bool stop(RecordTrack* recordTrack);

    // AudioBufferProvider interface
    virtual status_t getNextBuffer(AudioBufferProvider::Buffer* buffer, int64_t pts);
    virtual void releaseBuffer(AudioBufferProvider::Buffer* buffer);

    void readInputParameters();

    virtual size_t frameCount() const { return mFrameCount; }
    bool hasFastRecorder() const { return false; }

    class SyncEvent;

    typedef void (*sync_event_callback_t)(const wp<SyncEvent>& event) ;

    class SyncEvent : public RefBase {
    public:
        SyncEvent(AudioSystem::sync_event_t type,
                  int triggerSession,
                  int listenerSession,
                  sync_event_callback_t callBack,
                  void *cookie)
        : mType(type), mTriggerSession(triggerSession), mListenerSession(listenerSession),
          mCallback(callBack), mCookie(cookie)
        {}

        virtual ~SyncEvent() {}

        void trigger() { Mutex::Autolock _l(mLock); if (mCallback) mCallback(this); }
        bool isCancelled() const { Mutex::Autolock _l(mLock); return (mCallback == NULL); }
        void cancel() { Mutex::Autolock _l(mLock); mCallback = NULL; }
        AudioSystem::sync_event_t type() const { return mType; }
        int triggerSession() const { return mTriggerSession; }
        int listenerSession() const { return mListenerSession; }
        void *cookie() const { return mCookie; }

    private:
          const AudioSystem::sync_event_t mType;
          const int mTriggerSession;
          const int mListenerSession;
          sync_event_callback_t mCallback;
          void * const mCookie;
          mutable Mutex mLock;
    };

private:
    // Read in audio data from named pipe
    bool openPipe();
    ssize_t readPipe(void *buffer, size_t size);
    void clearSyncStartEvent();

    Condition mStartStopCond;

    // The named pipe file descriptor
    int m_fifoFd;
    // interleaved stereo pairs of fixed-point signed Q19.12
    int32_t *mRsmpOutBuffer;
    int16_t *mRsmpInBuffer; // [mFrameCount * mChannelCount]
    size_t mRsmpInIndex;
    size_t mBufferSize;    // stream buffer size for read()
    const uint32_t mReqChannelCount;
    const uint32_t mReqSampleRate;
    ssize_t mBytesRead;
    // sync event triggering actual audio capture. Frames read before this event will
    // be dropped and therefore not read by the application.
    sp<SyncEvent> mSyncStartEvent;
    // number of captured frames to drop after the start sync event has been received.
    // when < 0, maximum frames to drop before starting capture even if sync event is
    // not received
    ssize_t mFramestoDrop;
};

} // namespace android

#endif // RECORD_THREAD_H
