/*
 * Copyright (C) 2013-2014 Canonical Ltd
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

#ifndef MEDIA_RECORDER_CLIENT_H_
#define MEDIA_RECORDER_CLIENT_H_

#include <hybris/internal/camera_control.h>
#include <hybris/media/media_recorder_layer.h>

#include "media_recorder_observer.h"

#include <media/IMediaRecorder.h>

namespace android {

#if ANDROID_VERSION_MAJOR>=8
struct MediaRecorderBase;
#else
class MediaRecorderBase;
#endif
class Mutex;
class BpMediaRecorderObserver;

#if ANDROID_VERSION_MAJOR >= 7
namespace hardware {
class ICamera;
}
#else
class ICamera;
#endif
/*!
 * \brief The MediaRecorderClient struct wraps the service side of the MediaRecorder class
 */
struct MediaRecorderClient : public BnMediaRecorder
{
public:
    MediaRecorderClient();
    virtual ~MediaRecorderClient();

#if ANDROID_VERSION_MAJOR>=7
    virtual status_t setCamera(const sp<android::hardware::ICamera>& camera,
            const sp<ICameraRecordingProxy>& proxy);
#else
    virtual status_t setCamera(const sp<ICamera>& camera,
            const sp<ICameraRecordingProxy>& proxy);
#endif
    virtual status_t setPreviewSurface(const sp<IGraphicBufferProducer>& surface);
    virtual status_t setVideoSource(int vs);
    virtual status_t setAudioSource(int as);
    virtual status_t setOutputFormat(int of);
    virtual status_t setVideoEncoder(int ve);
    virtual status_t setAudioEncoder(int ae);
#if ANDROID_VERSION_MAJOR<=5
    virtual status_t setOutputFile(const char* path);
#else
#if ANDROID_VERSION_MAJOR>=8
    virtual status_t setInputSurface(const sp<PersistentSurface>& surface);
#else
    virtual status_t setInputSurface(const sp<IGraphicBufferConsumer>& surface);
#endif
#endif
#if ANDROID_VERSION_MAJOR>=8
    virtual status_t setOutputFile(int fd);
#else
    virtual status_t setOutputFile(int fd, int64_t offset, int64_t length);
#endif
    virtual status_t setVideoSize(int width, int height);
    virtual status_t setVideoFrameRate(int frames_per_second);
    virtual status_t setParameters(const String8& params);
    virtual status_t setListener(const sp<IMediaRecorderClient>& listener);
    virtual status_t setClientName(const String16& clientName);
    virtual status_t prepare();
    virtual status_t getMaxAmplitude(int* max);
    virtual status_t start();
    virtual status_t stop();
#if defined(BOARD_HAS_MEDIA_RECORDER_PAUSE) || ANDROID_VERSION_MAJOR>=7
    virtual status_t pause();
#endif
#if defined(BOARD_HAS_MEDIA_RECORDER_RESUME) || ANDROID_VERSION_MAJOR>=7
    virtual status_t resume();
#endif
    virtual status_t reset();
    virtual status_t init();
    virtual status_t close();
    virtual status_t release();
#if ANDROID_VERSION_MAJOR>=8
    virtual status_t dump(int fd, const Vector<String16>& args);
#else
    virtual status_t dump(int fd, const Vector<String16>& args) const;
#endif
    virtual sp<IGraphicBufferProducer> querySurfaceMediaSource();
#if ANDROID_VERSION_MAJOR>=8
    virtual status_t setNextOutputFile(int fd);
    virtual status_t getMetrics(Parcel* reply);
    virtual status_t setInputDevice(audio_port_handle_t deviceId);
    virtual status_t getRoutedDeviceId(audio_port_handle_t* deviceId);
    virtual status_t enableAudioDeviceCallback(bool enabled);
    virtual status_t getActiveMicrophones(
                        std::vector<media::MicrophoneInfo>* activeMicrophones);
#endif
#if ANDROID_VERSION_MAJOR>=10
    virtual status_t setPreferredMicrophoneDirection(audio_microphone_direction_t direction);
    virtual status_t setPreferredMicrophoneFieldDimension(float zoom);
    virtual status_t getPortId(audio_port_handle_t *portId);
#endif
#if ANDROID_VERSION_MAJOR>=11
   virtual status_t setPrivacySensitive(bool privacySensitive);
   virtual status_t isPrivacySensitive(bool *privacySensitive) const;
#endif

private:
    sp<BpMediaRecorderObserver> media_recorder_observer;
    MediaRecorderBase *recorder;
    Mutex recorder_lock;
};

}

#endif
