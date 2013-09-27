/*
 * Copyright (C) 2013 Canonical Ltd
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

#include <hybris/internal/camera_control.h>
#include <hybris/media/recorder_compatibility_layer.h>

#include <camera/Camera.h>
#include <camera/ICamera.h>
#include <media/mediarecorder.h>

//#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "MediaRecorderCompatibilityLayer"
#include <utils/KeyedVector.h>
#include <utils/Log.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

/*!
 * \brief The MediaRecorderListenerWrapper class is used to listen to events
 * from the MediaRecorder
 */
class MediaRecorderListenerWrapper : public android::MediaRecorderListener
{
	public:
		MediaRecorderListenerWrapper()
			: error_cb(NULL),
			error_context(NULL)
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

		void setErrorCb(on_recorder_msg_error cb, void *context)
		{
			REPORT_FUNCTION();
			error_cb = cb;
			error_context = context;
		}

	private:
		on_recorder_msg_error error_cb;
		void *error_context;
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
			setListener(media_recorder_listener);
		}

		~MediaRecorderWrapper()
		{
			reset();
		}

		void setErrorCb(on_recorder_msg_error cb, void *context)
		{
			REPORT_FUNCTION();

			assert(media_recorder_listener != NULL);
			media_recorder_listener->setErrorCb(cb, context);
		}

	private:
		android::sp<MediaRecorderListenerWrapper> media_recorder_listener;
};


using namespace android;

/*!
 * \brief android_recorder_set_error_cb
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param cb The callback function to be called when a recording error occurs
 * \param context
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

/*!
 * \brief android_media_new_recorder creates a new MediaRecorder
 * \return New MediaRecorder object, or NULL if the object could not be created.
 */
MediaRecorderWrapper *android_media_new_recorder()
{
	REPORT_FUNCTION();

	MediaRecorderWrapper *mr = new MediaRecorderWrapper;
	if (mr == NULL) {
		ALOGE("Failed to create new MediaRecorderWrapper instance.");
		return NULL;
	}

	return mr;
}

/*!
 * \brief android_recorder_initCheck
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_initCheck(MediaRecorderWrapper *mr)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->initCheck();
}

/*!
 * \brief android_recorder_setCamera sets the camera object for recording videos
 * from the camera
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param control Wrapper for the camera (see camera in hybris)
 * \return negative value if an error occured
 */
int android_recorder_setCamera(MediaRecorderWrapper *mr, CameraControl* control)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}
	if (control == NULL) {
		ALOGE("control must not be NULL");
		return BAD_VALUE;
	}

	return mr->setCamera(control->camera->remote(), control->camera->getRecordingProxy());
}

/*!
 * \brief android_recorder_setVideoSource sets the video source.
 * If no video source is set, only audio is recorded.
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param vs The video source. It's either the camera of gralloc buffer
 * \return negative value if an error occured
 */
int android_recorder_setVideoSource(MediaRecorderWrapper *mr, VideoSource vs)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setVideoSource(static_cast<int>(vs));
}

/*!
 * \brief android_recorder_setAudioSource
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param as The audio source.
 * \return negative value if an error occured
 */
int android_recorder_setAudioSource(MediaRecorderWrapper *mr, AudioSource as)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setAudioSource(static_cast<int>(as));
}

/*!
 * \brief android_recorder_setOutputFormat
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param of The output file format
 * \return negative value if an error occured
 */
int android_recorder_setOutputFormat(MediaRecorderWrapper *mr, OutputFormat of)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setOutputFormat(static_cast<int>(of));
}

/*!
 * \brief android_recorder_setVideoEncoder
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param ve The video encoder sets the codec for the video
 * \return negative value if an error occured
 */
int android_recorder_setVideoEncoder(MediaRecorderWrapper *mr, VideoEncoder ve)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setVideoEncoder(static_cast<int>(ve));
}

/*!
 * \brief android_recorder_setAudioEncoder
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param ae The audio encoder sets the codec for the audio
 * \return negative value if an error occured
 */
int android_recorder_setAudioEncoder(MediaRecorderWrapper *mr, AudioEncoder ae)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setAudioEncoder(static_cast<int>(ae));
}

/*!
 * \brief android_recorder_setOutputFile sets the output file to the given file descriptor
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param fd File descriptor of an open file, that the stream can be written to
 * \return negative value if an error occured
 */
int android_recorder_setOutputFile(MediaRecorderWrapper *mr, int fd)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setOutputFile(fd, 0, 0);
}

/*!
 * \brief android_recorder_setVideoSize
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param width width for the video to record
 * \param height height for the video to record
 * \return negative value if an error occured
 */
int android_recorder_setVideoSize(MediaRecorderWrapper *mr, int width, int height)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setVideoSize(width, height);
}

/*!
 * \brief android_recorder_setVideoFrameRate
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param frames_per_second Frames per second has typical values for a movie clip in 720p is 30
 * \return negative value if an error occured
 */
int android_recorder_setVideoFrameRate(MediaRecorderWrapper *mr, int frames_per_second)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->setVideoFrameRate(frames_per_second);
}

/*!
 * \brief android_recorder_setParameters sets a parameter. Even those without
 * explicit function.
 * For possible parameters look for example in StagefrightRecorder::setParameter()
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \param parameters list of parameters. format is "parameter1=value;parameter2=value"
 * \return negative value if an error occured
 */
int android_recorder_setParameters(MediaRecorderWrapper *mr, const char* parameters)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	String8 params(parameters);
	return mr->setParameters(params);
}

/*!
 * \brief android_recorder_start starts the recording.
 * The MediaRecorder has to be in state "prepared"
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_start(MediaRecorderWrapper *mr)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->start();
}

/*!
 * \brief android_recorder_stop Stops a running recording.
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_stop(MediaRecorderWrapper *mr)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->stop();
}

/*!
 * \brief android_recorder_prepare put the MediaRecorder into state "prepare"
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_prepare(MediaRecorderWrapper *mr)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->prepare();
}

/*!
 * \brief android_recorder_reset resets the MediaRecorder
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_reset(MediaRecorderWrapper *mr)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->reset();
}

/*!
 * \brief android_recorder_close closes the MediaRecorder
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_close(MediaRecorderWrapper *mr)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->close();
}

/*!
 * \brief android_recorder_release releases the MediaRecorder resources
 * This deletes the object. So don't use it after calling this function.
 * \param mr A MediaRecorderWrapper instance, created by calling android_media_new_recorder()
 * \return negative value if an error occured
 */
int android_recorder_release(MediaRecorderWrapper *mr)
{
	REPORT_FUNCTION();

	if (mr == NULL) {
		ALOGE("mr must not be NULL");
		return BAD_VALUE;
	}

	return mr->release();
}
