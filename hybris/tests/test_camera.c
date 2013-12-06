/*
 * Copyright (C) 2012 Canonical Ltd
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
 */

#include <hybris/camera/camera_compatibility_layer.h>
#include <hybris/camera/camera_compatibility_layer_capabilities.h>

#include <hybris/input/input_stack_compatibility_layer.h>
#include <hybris/input/input_stack_compatibility_layer_codes_key.h>
#include <hybris/input/input_stack_compatibility_layer_flags_key.h>
#include <hybris/input/input_stack_compatibility_layer_flags_motion.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

int shot_counter = 1;
int32_t current_zoom_level = 1;
bool new_camera_frame_available = true;

static GLuint gProgram;
static GLuint gaPositionHandle, gaTexHandle, gsTextureHandle, gmTexMatrix;

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

	struct CameraControl* cc = (struct CameraControl*) context;
	static int zoom;
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
	sprintf(fn, "/tmp/shot_%d.jpeg", shot_counter);
	int fd = open(fn, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	ssize_t ret = write(fd, data, data_size);
	close(fd);
	shot_counter++;

	struct CameraControl* cc = (struct CameraControl*) context;
	android_camera_start_preview(cc);
}

void size_cb(void* ctx, int width, int height)
{
	printf("Supported size: [%d,%d]\n", width, height);
}

void preview_texture_needs_update_cb(void* ctx)
{
	new_camera_frame_available = true;
}

void on_new_input_event(struct Event* event, void* context)
{
	assert(context);

	if (event->type == KEY_EVENT_TYPE && event->action == ISCL_KEY_EVENT_ACTION_UP) {
		printf("We have got a key event: %d \n", event->details.key.key_code);

		struct CameraControl* cc = (struct CameraControl*) context;

		switch(event->details.key.key_code) {
		case ISCL_KEYCODE_VOLUME_UP:
			printf("\tZooming in now.\n");
			android_camera_start_zoom(cc, current_zoom_level+1);
			break;
		case ISCL_KEYCODE_VOLUME_DOWN:
			printf("\tZooming out now.\n");
			android_camera_start_zoom(cc, current_zoom_level-1);
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

static const char* vertex_shader()
{
	return
		"#extension GL_OES_EGL_image_external : require              \n"
		"attribute vec4 a_position;                                  \n"
		"attribute vec2 a_texCoord;                                  \n"
		"uniform mat4 m_texMatrix;                                   \n"
		"varying vec2 v_texCoord;                                    \n"
		"varying float topDown;                                      \n"
		"void main()                                                 \n"
		"{                                                           \n"
		"   gl_Position = a_position;                                \n"
		"   v_texCoord = a_texCoord;                                 \n"
		// "   v_texCoord = (m_texMatrix * vec4(a_texCoord, 0.0, 1.0)).xy;\n"
		// "   topDown = v_texCoord.y;                                  \n"
		"}                                                           \n";
}

static const char* fragment_shader()
{
	return
		"#extension GL_OES_EGL_image_external : require      \n"
		"precision mediump float;                            \n"
		"varying vec2 v_texCoord;                            \n"
		"uniform samplerExternalOES s_texture;               \n"
		"void main()                                         \n"
		"{                                                   \n"
		"  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
		"}                                                   \n";
}

static GLuint loadShader(GLenum shaderType, const char* pSource) {
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

static GLuint create_program(const char* pVertexSource, const char* pFragmentSource) {
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

int main(int argc, char** argv)
{
	struct CameraControlListener listener;
	memset(&listener, 0, sizeof(listener));
	listener.on_msg_error_cb = error_msg_cb;
	listener.on_msg_shutter_cb = shutter_msg_cb;
	listener.on_msg_focus_cb = autofocus_msg_cb;
	listener.on_msg_zoom_cb = zoom_msg_cb;

	listener.on_data_raw_image_cb = raw_data_cb;
	listener.on_data_compressed_image_cb = jpeg_data_cb;
	listener.on_preview_texture_needs_update_cb = preview_texture_needs_update_cb;
	struct CameraControl* cc = android_camera_connect_to(FRONT_FACING_CAMERA_TYPE,
			&listener);
	if (cc == NULL) {
		printf("Problem connecting to camera");
		return 1;
	}

	listener.context = cc;

	struct AndroidEventListener event_listener;
	event_listener.on_new_event = on_new_input_event;
	event_listener.context = cc;

	struct InputStackConfiguration input_configuration = { false, 25000, 1024, 1024 };

	android_input_stack_initialize(&event_listener, &input_configuration);
	android_input_stack_start();

	android_camera_dump_parameters(cc);
	android_camera_enumerate_supported_picture_sizes(cc, size_cb, NULL);
	android_camera_enumerate_supported_preview_sizes(cc, size_cb, NULL);

	int min_fps, max_fps, current_fps;
	android_camera_get_preview_fps_range(cc, &min_fps, &max_fps);
	printf("Preview fps range: [%d,%d]\n", min_fps, max_fps);
	android_camera_get_preview_fps(cc, &current_fps);
	printf("Current preview fps range: %d\n", current_fps);

	android_camera_set_preview_size(cc, 960, 720);

	int width, height;
	android_camera_get_preview_size(cc, &width, &height);
	printf("Current preview size: [%d,%d]\n", width, height);
	android_camera_get_picture_size(cc, &width, &height);
	printf("Current picture size: [%d,%d]\n", width, height);
	int zoom;
	android_camera_get_max_zoom(cc, &zoom);
	printf("Max zoom: %d \n", zoom);
	android_camera_set_zoom(cc, 10);

	EffectMode effect_mode;
	FlashMode flash_mode;
	WhiteBalanceMode wb_mode;
	SceneMode scene_mode;
	AutoFocusMode af_mode;
	android_camera_get_effect_mode(cc, &effect_mode);
	android_camera_get_flash_mode(cc, &flash_mode);
	android_camera_get_white_balance_mode(cc, &wb_mode);
	android_camera_get_scene_mode(cc, &scene_mode);
	android_camera_get_auto_focus_mode(cc, &af_mode);
	printf("Current effect mode: %d \n", effect_mode);
	printf("Current flash mode: %d \n", flash_mode);
	printf("Current wb mode: %d \n", wb_mode);
	printf("Current scene mode: %d \n", scene_mode);
	printf("Current af mode: %d \n", af_mode);
	FocusRegion fr = { top: -200, left: -200, bottom: 200, right: 200, weight: 300};
	android_camera_set_focus_region(cc, &fr);

	/* EGL Setup */
	EGLConfig ecfg;
	EGLBoolean rv;
	EGLint num_config;
	EGLContext context;
	EGLint attr[] = {
		EGL_BUFFER_SIZE, 32,
		EGL_RENDERABLE_TYPE,
		EGL_OPENGL_ES2_BIT,
		EGL_NONE
	};
	EGLint ctxattr[] = {
		EGL_CONTEXT_CLIENT_VERSION, 2,
		EGL_NONE
	};

	EGLDisplay disp = eglGetDisplay(NULL);
	assert(eglGetError() == EGL_SUCCESS);
	assert(disp != EGL_NO_DISPLAY);

	rv = eglInitialize(disp, 0, 0);
	assert(eglGetError() == EGL_SUCCESS);
	assert(rv == EGL_TRUE);

	eglChooseConfig((EGLDisplay) disp, attr, &ecfg, 1, &num_config);
	assert(eglGetError() == EGL_SUCCESS);
	assert(rv == EGL_TRUE);

	EGLSurface surface = eglCreateWindowSurface((EGLDisplay) disp, ecfg,
			(EGLNativeWindowType) NULL, NULL);
	assert(eglGetError() == EGL_SUCCESS);
	assert(surface != EGL_NO_SURFACE);

	context = eglCreateContext((EGLDisplay) disp, ecfg, EGL_NO_CONTEXT, ctxattr);
	assert(eglGetError() == EGL_SUCCESS);
	assert(context != EGL_NO_CONTEXT);

	assert(eglMakeCurrent((EGLDisplay) disp, surface, surface, context) == EGL_TRUE);

	gProgram = create_program(vertex_shader(), fragment_shader());
	gaPositionHandle = glGetAttribLocation(gProgram, "a_position");
	gaTexHandle = glGetAttribLocation(gProgram, "a_texCoord");
	gsTextureHandle = glGetUniformLocation(gProgram, "s_texture");
	gmTexMatrix = glGetUniformLocation(gProgram, "m_texMatrix");

	GLuint preview_texture_id;
	glGenTextures(1, &preview_texture_id);
	glClearColor(1.0, 0., 0.5, 1.);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(
			GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(
			GL_TEXTURE_EXTERNAL_OES, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	android_camera_set_preview_texture(cc, preview_texture_id);
	android_camera_set_effect_mode(cc, EFFECT_MODE_SEPIA);
	android_camera_set_flash_mode(cc, FLASH_MODE_AUTO);
	android_camera_set_auto_focus_mode(cc, AUTO_FOCUS_MODE_CONTINUOUS_PICTURE);
	android_camera_start_preview(cc);

	GLfloat transformation_matrix[16];
	android_camera_get_preview_texture_transformation(cc, transformation_matrix);
	glUniformMatrix4fv(gmTexMatrix, 1, GL_FALSE, transformation_matrix);

	printf("Started camera preview.\n");

	while(true) {
		/*if (new_camera_frame_available)
		  {
		  printf("Updating texture");
		  new_camera_frame_available = false;
		  }*/
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
		glUseProgram(gProgram);
		// Enable attributes
		glEnableVertexAttribArray(gaPositionHandle);
		glEnableVertexAttribArray(gaTexHandle);
		// Load the vertex position
		glVertexAttribPointer(gaPositionHandle,
				3,
				GL_FLOAT,
				GL_FALSE,
				5 * sizeof(GLfloat),
				vVertices);
		// Load the texture coordinate
		glVertexAttribPointer(gaTexHandle,
				2,
				GL_FLOAT,
				GL_FALSE,
				5 * sizeof(GLfloat),
				vVertices+3);

		glActiveTexture(GL_TEXTURE0);
		// Set the sampler texture unit to 0
		glUniform1i(gsTextureHandle, 0);
		glUniform1i(gmTexMatrix, 0);
		android_camera_update_preview_texture(cc);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);
		glDisableVertexAttribArray(gaPositionHandle);
		glDisableVertexAttribArray(gaTexHandle);

		eglSwapBuffers((EGLDisplay) disp, surface);
	}
}
