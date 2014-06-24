/*
 * Copyright (C) 2013-2014 Canonical Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *		http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Thomas Voß <thomas.voss@canonical.com>
 *				Ricardo Salveti de Araujo <ricardo.salveti@canonical.com>
 *				Jim Hodapp <jim.hodapp@canonical.com>
 */

#include <hybris/camera/camera_compatibility_layer.h>
#include <hybris/camera/camera_compatibility_layer_capabilities.h>
#include <hybris/media/media_recorder_layer.h>

#include <hybris/input/input_stack_compatibility_layer.h>
#include <hybris/input/input_stack_compatibility_layer_codes_key.h>
#include <hybris/input/input_stack_compatibility_layer_flags_key.h>
#include <hybris/input/input_stack_compatibility_layer_flags_motion.h>

#include <hybris/surface_flinger/surface_flinger_compatibility_layer.h>

#include <gui/ISurfaceComposer.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

int shot_counter = 1;
int32_t current_zoom_level = 1;
bool new_camera_frame_available = true;
static CameraControl* camera_control = NULL;
int camera_width = 0, camera_height = 0;
static MediaRecorderWrapper *recorder = NULL;
bool recording = false;

EffectMode next_effect()
{
	static EffectMode current_effect = EFFECT_MODE_NONE;

	EffectMode next = current_effect;

	switch (current_effect) {
	case EFFECT_MODE_NONE:
		next = EFFECT_MODE_MONO;
		break;
	case EFFECT_MODE_MONO:
		next = EFFECT_MODE_NEGATIVE;
		break;
	case EFFECT_MODE_NEGATIVE:
		next = EFFECT_MODE_SOLARIZE;
		break;
	case EFFECT_MODE_SOLARIZE:
		next = EFFECT_MODE_SEPIA;
		break;
	case EFFECT_MODE_SEPIA:
		next = EFFECT_MODE_POSTERIZE;
		break;
	case EFFECT_MODE_POSTERIZE:
		next = EFFECT_MODE_WHITEBOARD;
		break;
	case EFFECT_MODE_WHITEBOARD:
		next = EFFECT_MODE_BLACKBOARD;
		break;
	case EFFECT_MODE_BLACKBOARD:
		next = EFFECT_MODE_AQUA;
		break;
	case EFFECT_MODE_AQUA:
		next = EFFECT_MODE_NONE;
		break;
	}

	current_effect = next;
	return next;
}

void error_msg_cb(void* context)
{
	printf("%s \n", __PRETTY_FUNCTION__);
}

void shutter_msg_cb(void* context)
{
	printf("%s \n", __PRETTY_FUNCTION__);
}

void zoom_msg_cb(void* context, int32_t new_zoom_level)
{
	printf("%s \n", __PRETTY_FUNCTION__);

	CameraControl* cc = static_cast<CameraControl*>(context);
	static int zoom;
	android_camera_get_current_zoom(cc, &zoom);
	printf("\t Current zoom: %d\n", zoom);
	current_zoom_level = new_zoom_level;
}

void autofocus_msg_cb(void* context)
{
	printf("%s \n", __PRETTY_FUNCTION__);
}

void raw_data_cb(void* data, uint32_t data_size, void* context)
{
	printf("%s: %d \n", __PRETTY_FUNCTION__, data_size);
}

void jpeg_data_cb(void* data, uint32_t data_size, void* context)
{
	printf("%s: %d \n", __PRETTY_FUNCTION__, data_size);

	char fn[256];
	sprintf(fn, "/cache/shot_%d.jpeg", shot_counter);
	int fd = open(fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);
	write(fd, data, data_size);
	close(fd);
	shot_counter++;

	CameraControl* cc = static_cast<CameraControl*>(context);
	android_camera_start_preview(cc);
}

void size_cb(void* ctx, int width, int height)
{
	printf("Supported size: [%d,%d]\n", width, height);
	if (width == 1024 && height == 768) {
		camera_width = 1024;
		camera_height = 768;
	}
}

struct RenderData
{
	static const char* vertex_shader()
	{
		return
			"#extension GL_OES_EGL_image_external : require				 \n"
			"attribute vec4 a_position;									 \n"
			"attribute vec2 a_texCoord;									 \n"
			"uniform mat4 m_texMatrix;									 \n"
			"varying vec2 v_texCoord;									 \n"
			"varying float topDown;										 \n"
			"void main()												 \n"
			"{															 \n"
			"	gl_Position = a_position;								 \n"
			"	v_texCoord = a_texCoord;								 \n"
			//				  "   v_texCoord = (m_texMatrix * vec4(a_texCoord, 0.0, 1.0)).xy;\n"
			//"   topDown = v_texCoord.y;								   \n"
			"}															 \n";
	}

	static const char* fragment_shader()
	{
		return
			"#extension GL_OES_EGL_image_external : require		 \n"
			"precision mediump float;							 \n"
			"varying vec2 v_texCoord;							 \n"
			"uniform samplerExternalOES s_texture;				 \n"
			"void main()										 \n"
			"{													 \n"
			"  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
			"}													 \n";
	}

	static GLuint loadShader(GLenum shaderType, const char* pSource)
	{
		GLuint shader = glCreateShader(shaderType);

		if (shader) {
			glShaderSource(shader, 1, &pSource, NULL);
			glCompileShader(shader);
			GLint compiled = 0;
			glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

			if (!compiled) {
				GLint infoLen = 0;
				glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
				if (infoLen) {
					char* buf = (char*) malloc(infoLen);
					if (buf) {
						glGetShaderInfoLog(shader, infoLen, NULL, buf);
						fprintf(stderr, "Could not compile shader %d:\n%s\n",
								shaderType, buf);
						free(buf);
					}
					glDeleteShader(shader);
					shader = 0;
				}
			}
		} else {
			printf("Error, during shader creation: %i\n", glGetError());
		}

		return shader;
	}

	static GLuint create_program(const char* pVertexSource, const char* pFragmentSource)
	{
		GLuint vertexShader = loadShader(GL_VERTEX_SHADER, pVertexSource);
		if (!vertexShader) {
			printf("vertex shader not compiled\n");
			return 0;
		}

		GLuint pixelShader = loadShader(GL_FRAGMENT_SHADER, pFragmentSource);
		if (!pixelShader) {
			printf("frag shader not compiled\n");
			return 0;
		}

		GLuint program = glCreateProgram();
		if (program) {
			glAttachShader(program, vertexShader);
			glAttachShader(program, pixelShader);
			glLinkProgram(program);
			GLint linkStatus = GL_FALSE;
			glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);

			if (linkStatus != GL_TRUE) {
				GLint bufLength = 0;
				glGetProgramiv(program, GL_INFO_LOG_LENGTH, &bufLength);
				if (bufLength) {
					char* buf = (char*) malloc(bufLength);
					if (buf) {
						glGetProgramInfoLog(program, bufLength, NULL, buf);
						fprintf(stderr, "Could not link program:\n%s\n", buf);
						free(buf);
					}
				}
				glDeleteProgram(program);
				program = 0;
			}
		}

		return program;
	}

	RenderData() : program_object(create_program(vertex_shader(), fragment_shader()))
	{
		position_loc = glGetAttribLocation(program_object, "a_position");
		tex_coord_loc = glGetAttribLocation(program_object, "a_texCoord");
		sampler_loc = glGetUniformLocation(program_object, "s_texture");
		matrix_loc = glGetUniformLocation(program_object, "m_texMatrix");
	}

	// Handle to a program object
	GLuint program_object;
	// Attribute locations
	GLint  position_loc;
	GLint  tex_coord_loc;
	// Sampler location
	GLint sampler_loc;
	// Matrix location
	GLint matrix_loc;
};


static RenderData render_data;
static EGLDisplay disp;
static EGLSurface surface;

void preview_texture_needs_update_cb(void* ctx)
{
	ALOGD("Updating preview texture");
	new_camera_frame_available = true;
	static GLfloat vVertices[] = { 0.0f, 0.0f, 0.0f, // Position 0
		0.0f, 0.0f, // TexCoord 0
		0.0f, 1.0f, 0.0f, // Position 1
		0.0f, 1.0f, // TexCoord 1
		1.0f, 1.0f, 0.0f, // Position 2
		1.0f, 1.0f, // TexCoord 2
		1.0f, 0.0f, 0.0f, // Position 3
		1.0f, 0.0f // TexCoord 3
	};

	GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

	// Set the viewport
	// Clear the color buffer
	glClear(GL_COLOR_BUFFER_BIT);
	// Use the program object
	glUseProgram(render_data.program_object);
	// Enable attributes
	glEnableVertexAttribArray(render_data.position_loc);
	glEnableVertexAttribArray(render_data.tex_coord_loc);
	// Load the vertex position
	glVertexAttribPointer(render_data.position_loc,
			3,
			GL_FLOAT,
			GL_FALSE,
			5 * sizeof(GLfloat),
			vVertices);
	// Load the texture coordinate
	glVertexAttribPointer(render_data.tex_coord_loc,
			2,
			GL_FLOAT,
			GL_FALSE,
			5 * sizeof(GLfloat),
			vVertices+3);

	glActiveTexture(GL_TEXTURE0);
	// Set the sampler texture unit to 0
	glUniform1i(render_data.sampler_loc, 0);
	glUniform1i(render_data.matrix_loc, 0);
	ALOGD("Updating the preview texture");
	if (camera_control != NULL)
		android_camera_update_preview_texture(camera_control);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
	glDisableVertexAttribArray(render_data.position_loc);
	glDisableVertexAttribArray(render_data.tex_coord_loc);

	eglSwapBuffers(disp, surface);
}

static void errorCB(void *context)
{
	ALOGE("Error while recording.");
}

static MediaRecorderWrapper *start_video_recording(CameraControl *camera_control)
{
	int ret = 0;
	struct MediaRecorderWrapper *recorder = android_media_new_recorder();
	android_recorder_set_error_cb(recorder, &errorCB, NULL);

	ALOGD("Unlocking camera");
	android_camera_unlock(camera_control);
	if (recorder == NULL)
		ALOGW("recorder is NULL: %d", __LINE__);

	ret = android_recorder_setCamera(recorder, camera_control);
	if (ret < 0) {
		ALOGE("android_recorder_setCamera() failed");
		return NULL;
	}
	ret = android_recorder_setAudioSource(recorder, ANDROID_AUDIO_SOURCE_CAMCORDER);
	if (ret < 0) {
		ALOGE("android_recorder_setAudioSource() failed");
		return NULL;
	}
	ret = android_recorder_setVideoSource(recorder, ANDROID_VIDEO_SOURCE_CAMERA);
	if (ret < 0) {
		ALOGE("android_recorder_setVideoSource() failed");
		return NULL;
	}
	ret = android_recorder_setOutputFormat(recorder, ANDROID_OUTPUT_FORMAT_MPEG_4);
	if (ret < 0) {
		ALOGE("android_recorder_setOutputFormat() failed");
		return NULL;
	}
	ret = android_recorder_setAudioEncoder(recorder, ANDROID_AUDIO_ENCODER_AAC);
	if (ret < 0) {
		ALOGE("android_recorder_setAudioEncoder() failed");
		return NULL;
	}
	ret = android_recorder_setVideoEncoder(recorder, ANDROID_VIDEO_ENCODER_H264);
	if (ret < 0) {
		ALOGE("android_recorder_setVideoEncoder() failed");
		return NULL;
	}
	int fd = -1;
	char *out_file = "/cache/test_recording.mp4";
	fd = open(out_file, O_WRONLY | O_CREAT,
			  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd < 0) {
		ALOGE("Couldn't open output video file for recording: %s", out_file);
		return NULL;
	}
	ret = android_recorder_setOutputFile(recorder, fd);
	if (ret < 0) {
		ALOGE("android_recorder_setOutputFile() failed");
		return NULL;
	}
	ret = android_recorder_setVideoSize(recorder, camera_width, camera_height);
	if (ret < 0) {
		ALOGE("android_recorder_setVideoSize() failed");
		return NULL;
	}
	ret = android_recorder_setVideoFrameRate(recorder, 30);
	if (ret < 0) {
		ALOGE("android_recorder_setVideoFrameRate() failed");
		return NULL;
	}
	android_recorder_setParameters(recorder, "video-param-encoding-bitrate=5505024"); // 7*1024*768
	android_recorder_setParameters(recorder, "audio-param-encoding-bitrate=48000");
	android_recorder_setParameters(recorder, "audio-param-number-of-channels=2");
	android_recorder_setParameters(recorder, "audio-param-sampling-rate=96000");
	android_recorder_setParameters(recorder, "video-param-rotation-angle-degrees=90");
	ALOGD("Preparing video recording");
	ret = android_recorder_prepare(recorder);
	if (ret < 0) {
		ALOGE("android_recorder_prepare() failed");
		return NULL;
	}
	ALOGD("Starting video recording");
	ret = android_recorder_start(recorder);
	if (ret < 0) {
		ALOGE("android_recorder_start() failed");
		return NULL;
	}

	return recorder;
}

static void stop_video_recording(MediaRecorderWrapper *recorder)
{
	if (recording) {
		ALOGD("Stopping video recording");
		android_recorder_stop(recorder);
		android_recorder_reset(recorder);
		android_recorder_release(recorder);
		ALOGD("Stopped video recording");
	}
}

void on_new_input_event(Event* event, void* context)
{
	assert(context);

	if (event->type == KEY_EVENT_TYPE && event->action == ISCL_KEY_EVENT_ACTION_UP) {
		printf("We got a key event: %d \n", event->details.key.key_code);

		CameraControl* cc = static_cast<CameraControl*>(context);

		switch(event->details.key.key_code) {
		case ISCL_KEYCODE_VOLUME_UP:
			printf("Starting video recording to /cache/test_recording.mp4\n");
			start_video_recording(cc);
			recording = true;
			break;
		case ISCL_KEYCODE_VOLUME_DOWN:
			printf("Stopping video recording\n");
			stop_video_recording(recorder);
			recording = false;
			break;
		case ISCL_KEYCODE_POWER:
			printf("\tTaking a photo now.\n");
			android_camera_take_snapshot(cc);
			break;
		case ISCL_KEYCODE_HEADSETHOOK:
			printf("\tSwitching effect.\n");
			android_camera_set_effect_mode(cc, next_effect());
		}
	} else if (event->type == MOTION_EVENT_TYPE &&
			event->details.motion.pointer_count == 1) {
		if ((event->action & ISCL_MOTION_EVENT_ACTION_MASK) == ISCL_MOTION_EVENT_ACTION_UP) {
			printf("\tMotion event(Action up): (%f, %f) \n",
					event->details.motion.pointer_coordinates[0].x,
					event->details.motion.pointer_coordinates[0].y);
		}

		if ((event->action & ISCL_MOTION_EVENT_ACTION_MASK) == ISCL_MOTION_EVENT_ACTION_DOWN) {
			printf("\tMotion event(Action down): (%f, %f) \n",
					event->details.motion.pointer_coordinates[0].x,
					event->details.motion.pointer_coordinates[0].y);
		}
	}
}

struct ClientWithSurface
{
	SfClient* client;
	SfSurface* surface;
};

ClientWithSurface client_with_surface(bool setup_surface_with_egl)
{
	ClientWithSurface cs = ClientWithSurface();

	cs.client = sf_client_create();

	if (!cs.client) {
		printf("Problem creating client ... aborting now.");
		return cs;
	}

	static const size_t primary_display = 0;

	SfSurfaceCreationParameters params = {
		0,
		0,
		(int) sf_get_display_width(primary_display),
		(int) sf_get_display_height(primary_display),
		-1, //PIXEL_FORMAT_RGBA_8888,
		15000,
		0.5f,
		setup_surface_with_egl, // Do not associate surface with egl, will be done by camera HAL
		"CameraCompatLayerTestSurface"
	};

	cs.surface = sf_surface_create(cs.client, &params);

	if (!cs.surface) {
		printf("Problem creating surface ... aborting now.");
		return cs;
	}

	sf_surface_make_current(cs.surface);

	return cs;
}

#define PRINT_GLERROR() printf("GL error@%d: %x\n", __LINE__, glGetError());

int main(int argc, char** argv)
{
	CameraControlListener listener;
	memset(&listener, 0, sizeof(listener));
	listener.on_msg_error_cb = error_msg_cb;
	listener.on_msg_shutter_cb = shutter_msg_cb;
	listener.on_msg_focus_cb = autofocus_msg_cb;
	listener.on_msg_zoom_cb = zoom_msg_cb;

	listener.on_data_raw_image_cb = raw_data_cb;
	listener.on_data_compressed_image_cb = jpeg_data_cb;
	listener.on_preview_texture_needs_update_cb = preview_texture_needs_update_cb;
	camera_control = android_camera_connect_to(BACK_FACING_CAMERA_TYPE,
			&listener);

	if (camera_control == NULL) {
		printf("Problem connecting to camera");
		return 1;
	}
	listener.context = camera_control;

	AndroidEventListener event_listener;
	event_listener.on_new_event = on_new_input_event;
	event_listener.context = camera_control;

	InputStackConfiguration input_configuration = {
		enable_touch_point_visualization : true,
		default_layer_for_touch_point_visualization : 10000,
		input_area_width : 1024,
		input_area_height : 1024
	};

	android_input_stack_initialize(&event_listener, &input_configuration);
	android_input_stack_start();

	android_camera_enumerate_supported_picture_sizes(camera_control, size_cb, NULL);
	if (camera_width == 0 && camera_height == 0) {
		camera_width = 320;
		camera_height = 240;
	}
	android_camera_set_picture_size(camera_control, camera_width, camera_height);

	AutoFocusMode af_mode;
	android_camera_get_auto_focus_mode(camera_control, &af_mode);
	printf("Current af mode: %d \n", af_mode);

	int zoom;
	android_camera_set_zoom(camera_control, 0);
	android_camera_get_max_zoom(camera_control, &zoom);
	printf("Max zoom: %d \n", zoom);

	android_camera_enumerate_supported_video_sizes(camera_control, size_cb, NULL);
	android_camera_enumerate_supported_preview_sizes(camera_control, size_cb, NULL);
	android_camera_set_preview_size(camera_control, camera_width, camera_height);

	int min_fps, max_fps, current_fps;
	android_camera_get_preview_fps_range(camera_control, &min_fps, &max_fps);
	printf("Preview fps range: [%d,%d]\n", min_fps, max_fps);
	android_camera_get_preview_fps(camera_control, &current_fps);
	printf("Current preview fps range: %d\n", current_fps);

#if 0
	android_camera_dump_parameters(camera_control);

	android_camera_set_display_orientation(camera_control, 90);

	int width, height;
	android_camera_get_preview_size(camera_control, &width, &height);
	printf("Current preview size: [%d,%d]\n", width, height);
	android_camera_get_picture_size(camera_control, &width, &height);
	printf("Current picture size: [%d,%d]\n", width, height);
	android_camera_get_current_zoom(camera_control, &zoom);
	printf("Current zoom: %d \n", zoom);

	EffectMode effect_mode;
	FlashMode flash_mode;
	WhiteBalanceMode wb_mode;
	//SceneMode scene_mode;
	CameraPixelFormat pixel_format;
	android_camera_get_effect_mode(camera_control, &effect_mode);
	printf("Current effect mode: %d \n", effect_mode);
	android_camera_get_flash_mode(camera_control, &flash_mode);
	printf("Current flash mode: %d \n", flash_mode);
	android_camera_get_white_balance_mode(camera_control, &wb_mode);
	ALOGD("Current wb mode: %d \n", wb_mode);
#if 0
	// Disabled, causes the test app to crash
	android_camera_get_scene_mode(camera_control, &scene_mode);
	printf("Current scene mode: %d \n", scene_mode);
#endif
	android_camera_get_preview_format(camera_control, &pixel_format);
	printf("Current preview pixel format: %d \n", pixel_format);
	//android_camera_set_focus_region(camera_control, -200, -200, 200, 200, 300);
#endif

	printf("Creating client with surface");
	ClientWithSurface cs = client_with_surface(true /* Associate surface with egl. */);

	if (!cs.surface) {
		printf("Problem acquiring surface for preview");
		return 1;
	}
	printf("Finished creating client with surface\n");

	disp = sf_client_get_egl_display(cs.client);
	surface = sf_surface_get_egl_surface(cs.surface);

	GLuint preview_texture_id;
	printf("Getting a texture id\n");
	glGenTextures(1, &preview_texture_id);
	glClearColor(1.0, 0., 0.5, 1.);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(
			GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(
			GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	printf("About to set preview texture\n");
	android_camera_set_preview_texture(camera_control, preview_texture_id);
#if 0
	android_camera_set_effect_mode(camera_control, EFFECT_MODE_SEPIA);
	android_camera_set_flash_mode(camera_control, FLASH_MODE_AUTO);
	android_camera_set_auto_focus_mode(camera_control, AUTO_FOCUS_MODE_CONTINUOUS_PICTURE);
#endif
	android_camera_start_preview(camera_control);

	GLfloat transformation_matrix[16];
	android_camera_get_preview_texture_transformation(camera_control, transformation_matrix);
	glUniformMatrix4fv(render_data.matrix_loc, 1, GL_FALSE, transformation_matrix);

	printf("Started camera preview.\n");

	while (1) {
		usleep(50);
	}

	stop_video_recording(recorder);
	android_camera_stop_preview(camera_control);
	android_camera_disconnect(camera_control);
}
