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
 *              Guenter Schwann <guenter.schwann@canonical.com>
 *              Ricardo Salveti de Araujo <ricardo.salveti@canonical.com>
 */

#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "MediaRecorderCompatibilityLayer"

#include "media_recorder.h"

#include <hybris/internal/camera_control.h>
#include <hybris/media/media_recorder_layer.h>

#include <utils/KeyedVector.h>
#include <utils/Log.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

using namespace android;

/*!
 * \brief The MediaRecorderListenerWrapper class is used to listen to camera events
 * from the MediaRecorder instance
 */
class MediaRecorderListenerWrapper : public android::MediaRecorderListener
{
public:
    MediaRecorderListenerWrapper()
        : error_cb(NULL),
          error_context(NULL),
          read_audio_cb(NULL),
          read_audio_context(NULL)
{
}

    void notify(int msg, int ext1, int ext2)
    {
        ALOGV("\tmsg: %d, ext1: %d, ext2: %d \n", msg, ext1, ext2);

        switch (msg) {
            case android::MEDIA_RECORDER_EVENT_ERROR:
                ALOGV("\tMEDIA_RECORDER_EVENT_ERROR msg\n");
                // TODO: Extend this cb to include the error message
                if (error_cb != NULL)
                    error_cb(error_context);
                else
                    ALOGE("Failed to signal error to app layer, callback not set.");
                break;
            default:
                ALOGV("\tUnknown notification\n");
        }
    }

    void readAudio()
    {
        REPORT_FUNCTION();
        if (read_audio_cb != NULL) {
            read_audio_cb(read_audio_context);
        }
        else
            ALOGW("Failed to call read_audio_cb since it's NULL");
    }

    void setErrorCb(on_recorder_msg_error cb, void *context)
    {
        REPORT_FUNCTION();
        error_cb = cb;
        error_context = context;
    }

    void setReadAudioCb(on_recorder_read_audio cb, void *context)
    {
        REPORT_FUNCTION();
        read_audio_cb = cb;
        read_audio_context = context;
    }

private:
    on_recorder_msg_error error_cb;
    void *error_context;
    on_recorder_read_audio read_audio_cb;
    void *read_audio_context;
};

/*!
 * \brief The MediaRecorderWrapper struct wraps the MediaRecorder class
 */
struct MediaRecorderWrapper : public android::MediaRecorder
{
public:
    MediaRecorderWrapper()
        : MediaRecorder(),
          media_recorder_listener(new MediaRecorderListenerWrapper())
{
}

    ~MediaRecorderWrapper()
    {
        reset();
    }

    void init()
    {
        setListener(media_recorder_listener);
    }

    void setErrorCb(on_recorder_msg_error cb, void *context)
    {
        REPORT_FUNCTION();

        assert(media_recorder_listener != NULL);
        media_recorder_listener->setErrorCb(cb, context);
    }

    void setAudioReadCb(on_recorder_read_audio cb, void *context)
    {
        REPORT_FUNCTION();

        assert(media_recorder_listener != NULL);
        media_recorder_listener->setReadAudioCb(cb, context);
    }

private:
    android::sp<MediaRecorderListenerWrapper> media_recorder_listener;
};

/*!
 * \brief android_recorder_set_error_cb
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param cb The callback function to be called when a recording error occurs
 * \param user context to pass through to the error handler
 */
void android_recorder_set_error_cb(MediaRecorderWrapper *mr, on_recorder_msg_error cb,
        void *context)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return;
    }

    mr->setErrorCb(cb, context);
}

void android_recorder_set_audio_read_cb(MediaRecorderWrapper *mr, on_recorder_read_audio cb,
        void *context)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return;
    }

    mr->setAudioReadCb(cb, context);
}

/*!
 * \brief android_media_new_recorder creates a new MediaRecorder
 * \return New MediaRecorder object, or NULL if the object could not be created.
 */
MediaRecorderWrapper *android_media_new_recorder()
{
    REPORT_FUNCTION();

    MediaRecorderWrapper *mr = new MediaRecorderWrapper;
    if (mr == NULL) {
        ALOGE("Failed to create new MediaRecorder instance.");
        return NULL;
    }

    return mr;
}

/*!
 * \brief android_recorder_initCheck
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_initCheck(MediaRecorderWrapper *mr)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->initCheck();
}

/*!
 * \brief android_recorder_setCamera sets the camera object for recording videos from the camera
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param control Wrapper for the camera (see camera in hybris)
 * \return negative value if an error occured
 */
int android_recorder_setCamera(MediaRecorderWrapper *mr, CameraControl* control)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }
    if (control == NULL) {
        ALOGE("control must not be NULL");
        return android::BAD_VALUE;
    }

    mr->init();

    return mr->setCamera(control->camera->remote(), control->camera->getRecordingProxy());
}

/*!
 * \brief android_recorder_setVideoSource sets the video source.
 * If no video source is set, only audio is recorded.
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param vs The video source. It's either a camera or a gralloc buffer
 * \return negative value if an error occured
 */
int android_recorder_setVideoSource(MediaRecorderWrapper *mr, VideoSource vs)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setVideoSource(static_cast<int>(vs));
}

/*!
 * \brief android_recorder_setAudioSource
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param as The audio source.
 * \return negative value if an error occured
 */
int android_recorder_setAudioSource(MediaRecorderWrapper *mr, AudioSource as)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setAudioSource(static_cast<int>(as));
}

/*!
 * \brief android_recorder_setOutputFormat
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param of The output file container format
 * \return negative value if an error occured
 */
int android_recorder_setOutputFormat(MediaRecorderWrapper *mr, OutputFormat of)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setOutputFormat(static_cast<int>(of));
}

/*!
 * \brief android_recorder_setVideoEncoder
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param ve The video encoder sets the codec type for encoding/recording video
 * \return negative value if an error occured
 */
int android_recorder_setVideoEncoder(MediaRecorderWrapper *mr, VideoEncoder ve)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setVideoEncoder(static_cast<int>(ve));
}

/*!
 * \brief android_recorder_setAudioEncoder
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param ae The audio encoder sets the codec type for encoding/recording audio
 * \return negative value if an error occured
 */
int android_recorder_setAudioEncoder(MediaRecorderWrapper *mr, AudioEncoder ae)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setAudioEncoder(static_cast<int>(ae));
}

/*!
 * \brief android_recorder_setOutputFile sets the output file to the given file descriptor
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param fd File descriptor of an open file, that the stream can be written to
 * \return negative value if an error occured
 */
int android_recorder_setOutputFile(MediaRecorderWrapper *mr, int fd)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setOutputFile(fd, 0, 0);
}

/*!
 * \brief android_recorder_setVideoSize
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param width output width for the video to record
 * \param height output height for the video to record
 * \return negative value if an error occured
 */
int android_recorder_setVideoSize(MediaRecorderWrapper *mr, int width, int height)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setVideoSize(width, height);
}

/*!
 * \brief android_recorder_setVideoFrameRate
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param frames_per_second How many frames per second to record at (e.g. 720p is typically 30)
 * \return negative value if an error occured
 */
int android_recorder_setVideoFrameRate(MediaRecorderWrapper *mr, int frames_per_second)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->setVideoFrameRate(frames_per_second);
}

/*!
 * \brief android_recorder_setParameters sets a parameter. Even those without
 * explicit function.
 * For possible parameter pairs look for examples in StagefrightRecorder::setParameter()
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \param parameters list of parameters. format is "parameter1=value;parameter2=value"
 * \return negative value if an error occured
 */
int android_recorder_setParameters(MediaRecorderWrapper *mr, const char *parameters)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    android::String8 params(parameters);
    return mr->setParameters(params);
}

/*!
 * \brief android_recorder_start starts the recording.
 * The MediaRecorder has to be in state "prepared" (call android_recorder_prepare() first)
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_start(MediaRecorderWrapper *mr)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->start();
}

/*!
 * \brief android_recorder_stop Stops a running recording.
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_stop(MediaRecorderWrapper *mr)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->stop();
}

/*!
 * \brief android_recorder_prepare put the MediaRecorder into state "prepare"
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_prepare(MediaRecorderWrapper *mr)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->prepare();
}

/*!
 * \brief android_recorder_reset resets the MediaRecorder
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_reset(MediaRecorderWrapper *mr)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->reset();
}

/*!
 * \brief android_recorder_close closes the MediaRecorder
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_close(MediaRecorderWrapper *mr)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->close();
}

/*!
 * \brief android_recorder_release releases the MediaRecorder resources
 * This deletes the MediaRecorder instance. So don't use the instance after calling this function.
 * \param mr A MediaRecorder instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_release(MediaRecorderWrapper *mr)
{
    REPORT_FUNCTION();

    if (mr == NULL) {
        ALOGE("mr must not be NULL");
        return android::BAD_VALUE;
    }

    return mr->release();
}
