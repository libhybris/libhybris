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

// #define LOG_NDEBUG 0
#define LOG_TAG "MediaRecorderClient"

#include "media_recorder_client.h"

#include <libmediaplayerservice/StagefrightRecorder.h>
#include <binder/IServiceManager.h>


#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

using namespace android;

MediaRecorderClient::MediaRecorderClient()
{
    REPORT_FUNCTION();

    sp<IServiceManager> service_manager = defaultServiceManager();
    sp<IBinder> service = service_manager->getService(
        String16(IMediaRecorderObserver::exported_service_name()));

    media_recorder_observer = new BpMediaRecorderObserver(service);

#if ANDROID_VERSION_MAJOR>=6
    // TODO: do we need to get valid package here?
    const String16 opPackageName("ubuntu");
    recorder = new android::StagefrightRecorder(opPackageName);
#else
    recorder = new android::StagefrightRecorder;
#endif
}

MediaRecorderClient::~MediaRecorderClient()
{
    REPORT_FUNCTION();
    release();
}
#if ANDROID_VERSION_MAJOR >= 7
status_t MediaRecorderClient::setCamera(const sp<android::hardware::ICamera>& camera,
        const sp<ICameraRecordingProxy>& proxy)
#else
status_t MediaRecorderClient::setCamera(const sp<android::ICamera>& camera,
        const sp<ICameraRecordingProxy>& proxy)
#endif
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setCamera(camera, proxy);
}

status_t MediaRecorderClient::setPreviewSurface(const android::sp<android::IGraphicBufferProducer>& surface)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setPreviewSurface(surface);
}

status_t MediaRecorderClient::setVideoSource(int vs)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL)     {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setVideoSource((android::video_source)vs);
}

status_t MediaRecorderClient::setAudioSource(int as)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL)  {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setAudioSource((audio_source_t)as);
}

status_t MediaRecorderClient::setOutputFormat(int of)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setOutputFormat((android::output_format)of);
}

status_t MediaRecorderClient::setVideoEncoder(int ve)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setVideoEncoder((android::video_encoder)ve);
}

status_t MediaRecorderClient::setAudioEncoder(int ae)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setAudioEncoder((android::audio_encoder)ae);
}

#if ANDROID_VERSION_MAJOR<=5
status_t MediaRecorderClient::setOutputFile(const char* path)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setOutputFile(path);
}
#else
#if ANDROID_VERSION_MAJOR>=8
status_t MediaRecorderClient::setInputSurface(const sp<PersistentSurface>& surface)
#else
status_t MediaRecorderClient::setInputSurface(const sp<IGraphicBufferConsumer>& surface)
#endif
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setInputSurface(surface);
}
#endif

#if ANDROID_VERSION_MAJOR>=8
status_t MediaRecorderClient::setOutputFile(int fd)
#else
status_t MediaRecorderClient::setOutputFile(int fd, int64_t offset, int64_t length)
#endif
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
#if ANDROID_VERSION_MAJOR>=8
    return recorder->setOutputFile(fd);
#else
    return recorder->setOutputFile(fd, offset, length);
#endif
}

status_t MediaRecorderClient::setVideoSize(int width, int height)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setVideoSize(width, height);
}

status_t MediaRecorderClient::setVideoFrameRate(int frames_per_second)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setVideoFrameRate(frames_per_second);
}

status_t MediaRecorderClient::setParameters(const android::String8& params)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setParameters(params);
}

status_t MediaRecorderClient::setListener(const android::sp<android::IMediaRecorderClient>& listener)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setListener(listener);
}

status_t MediaRecorderClient::setClientName(const android::String16& clientName)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->setClientName(clientName);
}

status_t MediaRecorderClient::prepare()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->prepare();
}

status_t MediaRecorderClient::getMaxAmplitude(int* max)
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->getMaxAmplitude(max);
}

status_t MediaRecorderClient::start()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }

    if (media_recorder_observer != NULL)
        media_recorder_observer->recordingStarted();

    return recorder->start();
}

status_t MediaRecorderClient::stop()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }

    if (media_recorder_observer != NULL)
        media_recorder_observer->recordingStopped();

    return recorder->stop();
}

#ifdef BOARD_HAS_MEDIA_RECORDER_PAUSE
status_t MediaRecorderClient::pause()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->pause();

}
#endif

#ifdef BOARD_HAS_MEDIA_RECORDER_RESUME
status_t MediaRecorderClient::resume()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->resume();
}
#endif

status_t MediaRecorderClient::reset()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->reset();
}

status_t MediaRecorderClient::init()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->init();
}

status_t MediaRecorderClient::close()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder must not be NULL");
        return NO_INIT;
    }
    return recorder->close();
}

status_t MediaRecorderClient::release()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        delete recorder;
        recorder = NULL;
    }
    return NO_ERROR;
}

#if ANDROID_VERSION_MAJOR>=8
status_t MediaRecorderClient::dump(int fd, const Vector<String16>& args)
#else
status_t MediaRecorderClient::dump(int fd, const Vector<String16>& args) const
#endif
{
    REPORT_FUNCTION();
    if (recorder != NULL) {
        return recorder->dump(fd, args);
    }
    return android::OK;
}

sp<IGraphicBufferProducer> MediaRecorderClient::querySurfaceMediaSource()
{
    REPORT_FUNCTION();
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder is not initialized");
        return NULL;
    }
    return recorder->querySurfaceMediaSource();
}

#if ANDROID_VERSION_MAJOR>=8
status_t MediaRecorderClient::setNextOutputFile(int fd)
{
    REPORT_FUNCTION();
    ALOGV("setNextOutputFile(%d)", fd);
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return recorder->setNextOutputFile(fd);
}

status_t MediaRecorderClient::getMetrics(Parcel* reply)
{
    REPORT_FUNCTION();
    ALOGV("MediaRecorderClient::getMetrics");
    Mutex::Autolock lock(recorder_lock);
    if (recorder == NULL) {
        ALOGE("recorder is not initialized");
        return NO_INIT;
    }
    return recorder->getMetrics(reply);
}

status_t MediaRecorderClient::setInputDevice(audio_port_handle_t deviceId)
{
    REPORT_FUNCTION();
    ALOGV("setInputDevice(%d)", deviceId);
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->setInputDevice(deviceId);
    }
    return NO_INIT;
}

status_t MediaRecorderClient::getRoutedDeviceId(audio_port_handle_t* deviceId)
{
    REPORT_FUNCTION();
    ALOGV("getRoutedDeviceId");
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->getRoutedDeviceId(deviceId);
    }
    return NO_INIT;
}

status_t MediaRecorderClient::enableAudioDeviceCallback(bool enabled)
{
    REPORT_FUNCTION();
    ALOGV("enableDeviceCallback: %d", enabled);
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->enableAudioDeviceCallback(enabled);
    }
    return NO_INIT;
}

status_t MediaRecorderClient::getActiveMicrophones(
        std::vector<media::MicrophoneInfo>* activeMicrophones)
{
    REPORT_FUNCTION();
    ALOGV("getActiveMicrophones");
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->getActiveMicrophones(activeMicrophones);
    }
    return NO_INIT;
}
#endif

#if ANDROID_VERSION_MAJOR>=10
status_t MediaRecorderClient::setPreferredMicrophoneDirection(audio_microphone_direction_t direction)
{
    REPORT_FUNCTION();
    ALOGV("setPreferredMicrophoneDirection(%d)", direction);
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->setPreferredMicrophoneDirection(direction);
    }
    return NO_INIT;
}

status_t MediaRecorderClient::setPreferredMicrophoneFieldDimension(float zoom)
{
    REPORT_FUNCTION();
    ALOGV("setPreferredMicrophoneFieldDimension(%f)", zoom);
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->setPreferredMicrophoneFieldDimension(zoom);
    }
    return NO_INIT;
}

status_t MediaRecorderClient::getPortId(audio_port_handle_t *portId)
{
    REPORT_FUNCTION();
    ALOGV("getPortId");
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->getPortId(portId);
    }
    return NO_INIT;
}
#endif

#if ANDROID_VERSION_MAJOR>=11
status_t MediaRecorderClient::setPrivacySensitive(bool privacySensitive)
{
    REPORT_FUNCTION();
    ALOGV("setPrivacySensitive(%d)", privacySensitive);
    Mutex::Autolock lock(recorder_lock);
    if (recorder != NULL) {
        return recorder->setPrivacySensitive(privacySensitive);
    }
    return NO_INIT;
}
status_t MediaRecorderClient::isPrivacySensitive(bool *privacySensitive) const
{
    REPORT_FUNCTION();
    ALOGV("isPrivacySensitive");
    if (recorder != NULL) {
        return recorder->isPrivacySensitive(privacySensitive);
    }
    return NO_INIT;
}
#endif
