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

#ifndef RECORD_TRACK_H
#define RECORD_TRACK_H

#include <limits.h>
#include <stdint.h>
#include <sys/types.h>

#include <media/AudioSystem.h>
#include <private/media/AudioTrackShared.h>
#include <media/ExtendedAudioBufferProvider.h>
#include <media/IAudioRecord.h>
#include <binder/IMemory.h>
#include <utils/RefBase.h>
#include <system/audio.h>

namespace android {

class MemoryDealer;
class RecordThread;
class ThreadBase;

class RecordTrack : public ExtendedAudioBufferProvider, public RefBase
{
public:
    enum track_state {
        IDLE,
        FLUSHED,
        STOPPED,
        // next 2 states are currently used for fast tracks
        // and offloaded tracks only
        STOPPING_1,     // waiting for first underrun
        STOPPING_2,     // waiting for presentation complete
        RESUMING,
        ACTIVE,
        PAUSING,
        PAUSED
    };

    RecordTrack(ThreadBase *thread,
            uint32_t sampleRate,
            audio_format_t format,
            audio_channel_mask_t channelMask,
            size_t frameCount,
            const sp<IMemory>& sharedBuffer,
            int sessionId,
            int clientUid);
    virtual ~RecordTrack();

    virtual status_t start(AudioSystem::sync_event_t event, int triggerSession);
    virtual void stop();

    sp<IMemory> getCblk() const { return mCblkMemory; }
    audio_track_cblk_t* cblk() const { return mCblk; }
    int uid() const { return mUid; }

    // AudioBufferProvider interface
    virtual status_t getNextBuffer(AudioBufferProvider::Buffer* buffer, int64_t pts = kInvalidPTS);
    virtual void releaseBuffer(AudioBufferProvider::Buffer* buffer);

    audio_format_t format() const { return mFormat; }

    uint32_t channelCount() const { return mChannelCount; }

    audio_channel_mask_t channelMask() const { return mChannelMask; }

    virtual uint32_t sampleRate() const { return mSampleRate; }

    // Return a pointer to the start of a contiguous slice of the track buffer.
    // Parameter 'offset' is the requested start position, expressed in
    // monotonically increasing frame units relative to the track epoch.
    // Parameter 'frames' is the requested length, also in frame units.
    // Always returns non-NULL.  It is the caller's responsibility to
    // verify that this will be successful; the result of calling this
    // function with invalid 'offset' or 'frames' is undefined.
    void* getBuffer(uint32_t offset, uint32_t frames) const;

    // ExtendedAudioBufferProvider interface is only needed for Track,
    // but putting it in TrackBase avoids the complexity of virtual inheritance
    virtual size_t  framesReady() const { return SIZE_MAX; }

    int sessionId() const { return mSessionId; }

    bool isTerminated() const {
        return mTerminated;
    }

    void terminate() {
        mTerminated = true;
    }

    void destroy();
    void invalidate();
    // clear the buffer overflow flag
    void clearOverflow() { mOverflow = false; }
    // set the buffer overflow flag and return previous value
    bool setOverflow() { bool tmp = mOverflow; mOverflow = true;
                         return tmp; }

protected:
    friend class RecordThread;

    RecordTrack(const RecordTrack&);
    RecordTrack& operator = (const RecordTrack&);

    const wp<ThreadBase> mThread;
    // The heap that cblk points to
    sp<MemoryDealer> mMemoryDealer;
    sp<IMemory> mCblkMemory;
    audio_track_cblk_t* mCblk;
    void* mBuffer;                  // start of track buffer, typically in shared memory
                                    // except for OutputTrack when it is in local memory
    // we don't really need a lock for these
    track_state mState;
    const uint32_t mSampleRate;     // initial sample rate only; for tracks which
                                    // support dynamic rates, the current value is in control block
    const audio_format_t mFormat;
    const audio_channel_mask_t mChannelMask;
    const uint32_t mChannelCount;
    const size_t mFrameSize;        // AudioFlinger's view of frame size in shared memory,
                                    // where for AudioTrack (but not AudioRecord),
                                    // 8-bit PCM samples are stored as 16-bit
    const int mSessionId;
    int mUid;
    bool mOverflow;  // overflow on most recent attempt to fill client buffer
    AudioRecordServerProxy* mAudioRecordServerProxy;
    ServerProxy* mServerProxy;
    const int mId;
    bool mTerminated;

    pid_t getpid_cached;
};

// Server side of the client's IAudioRecord
class RecordHandle : public android::BnAudioRecord {
public:
    RecordHandle(const sp<RecordTrack>& recordTrack);
    virtual             ~RecordHandle();
    virtual sp<IMemory> getCblk() const;
    virtual status_t    start(int /*AudioSystem::sync_event_t*/ event, int triggerSession);
    virtual void        stop();
    virtual status_t onTransact(uint32_t code, const Parcel& data, Parcel* reply, uint32_t flags);
private:
    const sp<RecordTrack> mRecordTrack;

    // for use from destructor
    void                stop_nonvirtual();
};

} // namespace android

#endif // RECORD_TRACK_H
