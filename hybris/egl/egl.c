/*
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

/* EGL function pointers */
#define EGL_EGLEXT_PROTOTYPES
/* For RTLD_DEFAULT */
#define _GNU_SOURCE
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdlib.h>
#include <malloc.h>
#include "ws.h"
#include "helper.h"
#include <assert.h>


#include <hybris/common/binding.h>
#include <string.h>

#include <system/window.h>
#include "logging.h"

static void *_libegl = NULL;
static void *_libgles = NULL;
static void *_hybris_libgles1 = NULL;
static void *_hybris_libgles2 = NULL;
static int _egl_context_client_version = 1;

static EGLint  (*_eglGetError)(void) = NULL;

static EGLDisplay  (*_eglGetDisplay)(EGLNativeDisplayType display_id) = NULL;
static EGLBoolean  (*_eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor) = NULL;
static EGLBoolean  (*_eglTerminate)(EGLDisplay dpy) = NULL;

static const char *  (*_eglQueryString)(EGLDisplay dpy, EGLint name) = NULL;

static EGLBoolean  (*_eglGetConfigs)(EGLDisplay dpy, EGLConfig *configs,
		EGLint config_size, EGLint *num_config) = NULL;
static EGLBoolean  (*_eglChooseConfig)(EGLDisplay dpy, const EGLint *attrib_list,
		EGLConfig *configs, EGLint config_size,
		EGLint *num_config) = NULL;
static EGLBoolean  (*_eglGetConfigAttrib)(EGLDisplay dpy, EGLConfig config,
		EGLint attribute, EGLint *value) = NULL;

static EGLSurface  (*_eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config,
		EGLNativeWindowType win,
		const EGLint *attrib_list) = NULL;
static EGLSurface  (*_eglCreatePbufferSurface)(EGLDisplay dpy, EGLConfig config,
		const EGLint *attrib_list) = NULL;
static EGLSurface  (*_eglCreatePixmapSurface)(EGLDisplay dpy, EGLConfig config,
		EGLNativePixmapType pixmap,
		const EGLint *attrib_list) = NULL;
static EGLBoolean  (*_eglDestroySurface)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLBoolean  (*_eglQuerySurface)(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint *value) = NULL;

static EGLBoolean  (*_eglBindAPI)(EGLenum api) = NULL;
static EGLenum  (*_eglQueryAPI)(void) = NULL;

static EGLBoolean  (*_eglWaitClient)(void) = NULL;

static EGLBoolean  (*_eglReleaseThread)(void) = NULL;

static EGLSurface  (*_eglCreatePbufferFromClientBuffer)(
		EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
		EGLConfig config, const EGLint *attrib_list) = NULL;

static EGLBoolean  (*_eglSurfaceAttrib)(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint value) = NULL;
static EGLBoolean  (*_eglBindTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer) = NULL;
static EGLBoolean  (*_eglReleaseTexImage)(EGLDisplay dpy, EGLSurface surface, EGLint buffer) = NULL;


static EGLBoolean  (*_eglSwapInterval)(EGLDisplay dpy, EGLint interval) = NULL;


static EGLContext  (*_eglCreateContext)(EGLDisplay dpy, EGLConfig config,
		EGLContext share_context,
		const EGLint *attrib_list) = NULL;
static EGLBoolean  (*_eglDestroyContext)(EGLDisplay dpy, EGLContext ctx) = NULL;
static EGLBoolean  (*_eglMakeCurrent)(EGLDisplay dpy, EGLSurface draw,
		EGLSurface read, EGLContext ctx) = NULL;

static EGLContext  (*_eglGetCurrentContext)(void) = NULL;
static EGLSurface  (*_eglGetCurrentSurface)(EGLint readdraw) = NULL;
static EGLDisplay  (*_eglGetCurrentDisplay)(void) = NULL;
static EGLBoolean  (*_eglQueryContext)(EGLDisplay dpy, EGLContext ctx,
		EGLint attribute, EGLint *value) = NULL;

static EGLBoolean  (*_eglWaitGL)(void) = NULL;
static EGLBoolean  (*_eglWaitNative)(EGLint engine) = NULL;
static EGLBoolean  (*_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLBoolean  (*_eglCopyBuffers)(EGLDisplay dpy, EGLSurface surface,
		EGLNativePixmapType target) = NULL;


static EGLImageKHR (*_eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) = NULL;
static EGLBoolean (*_eglDestroyImageKHR) (EGLDisplay dpy, EGLImageKHR image) = NULL;

static void (*_glEGLImageTargetTexture2DOES) (GLenum target, GLeglImageOES image) = NULL;

static __eglMustCastToProperFunctionPointerType (*_eglGetProcAddress)(const char *procname) = NULL;

static void _init_androidegl()
{
	_libegl = (void *) android_dlopen(getenv("LIBEGL") ? getenv("LIBEGL") : "libEGL.so", RTLD_LAZY);
	_libgles = (void *) android_dlopen(getenv("LIBGLESV2") ? getenv("LIBGLESV2") : "libGLESv2.so", RTLD_LAZY);
}

static void * _android_egl_dlsym(const char *symbol)
{
	if (_libegl == NULL)
		_init_androidegl();

	return android_dlsym(_libegl, symbol);
}

struct ws_egl_interface hybris_egl_interface = {
	_android_egl_dlsym,
	egl_helper_has_mapping,
	egl_helper_get_mapping,
};

#define EGL_DLSYM(fptr, sym) do { if (_libegl == NULL) { _init_androidegl(); }; if (*(fptr) == NULL) { *(fptr) = (void *) android_dlsym(_libegl, sym); } } while (0)
#define GLESv2_DLSYM(fptr, sym) do { if (_libgles == NULL) { _init_androidegl(); }; if (*(fptr) == NULL) { *(fptr) = (void *) android_dlsym(_libgles, sym); } } while (0)

EGLint eglGetError(void)
{
	EGL_DLSYM(&_eglGetError, "eglGetError");
	return (*_eglGetError)();
}

#define _EGL_MAX_DISPLAYS 100

struct _EGLDisplay *_displayMappings[_EGL_MAX_DISPLAYS];

void _addMapping(struct _EGLDisplay *display_id)
{
	int i;
	for (i = 0; i < _EGL_MAX_DISPLAYS; i++)
	{
		if (_displayMappings[i] == NULL)
		{
			_displayMappings[i] = display_id;
			return;
		}
	}
}

struct _EGLDisplay *hybris_egl_display_get_mapping(EGLDisplay display)
{
	int i;
	for (i = 0; i < _EGL_MAX_DISPLAYS; i++)
	{
		if (_displayMappings[i])
		{
			if (_displayMappings[i]->dpy == display)
			{
				return _displayMappings[i];
			}

		}
	}
	return EGL_NO_DISPLAY;
}

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
	EGL_DLSYM(&_eglGetDisplay, "eglGetDisplay");
	EGLNativeDisplayType real_display;

	real_display = (*_eglGetDisplay)(EGL_DEFAULT_DISPLAY);
	if (real_display == EGL_NO_DISPLAY)
	{
		return EGL_NO_DISPLAY;
	}

	struct _EGLDisplay *dpy = hybris_egl_display_get_mapping(real_display);
	if (!dpy) {
		dpy = ws_GetDisplay(display_id);
		if (!dpy) {
			return EGL_NO_DISPLAY;
		}
		dpy->dpy = real_display;
		_addMapping(dpy);
	}

	return real_display;
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	EGL_DLSYM(&_eglInitialize, "eglInitialize");
	return (*_eglInitialize)(dpy, major, minor);
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
	EGL_DLSYM(&_eglTerminate, "eglTerminate");

	struct _EGLDisplay *display = hybris_egl_display_get_mapping(dpy);
	ws_Terminate(display);
	return (*_eglTerminate)(dpy);
}

const char * eglQueryString(EGLDisplay dpy, EGLint name)
{
	EGL_DLSYM(&_eglQueryString, "eglQueryString");
	return ws_eglQueryString(dpy, name, _eglQueryString);
}

EGLBoolean eglGetConfigs(EGLDisplay dpy, EGLConfig *configs,
		EGLint config_size, EGLint *num_config)
{
	EGL_DLSYM(&_eglGetConfigs, "eglGetConfigs");
	return (*_eglGetConfigs)(dpy, configs, config_size, num_config);
}

EGLBoolean eglChooseConfig(EGLDisplay dpy, const EGLint *attrib_list,
		EGLConfig *configs, EGLint config_size,
		EGLint *num_config)
{
	EGL_DLSYM(&_eglChooseConfig, "eglChooseConfig");
	return (*_eglChooseConfig)(dpy, attrib_list,
			configs, config_size,
			num_config);
}

EGLBoolean eglGetConfigAttrib(EGLDisplay dpy, EGLConfig config,
		EGLint attribute, EGLint *value)
{
	EGL_DLSYM(&_eglGetConfigAttrib, "eglGetConfigAttrib");
	return (*_eglGetConfigAttrib)(dpy, config,
			attribute, value);
}

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
		EGLNativeWindowType win,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreateWindowSurface, "eglCreateWindowSurface");

	HYBRIS_TRACE_BEGIN("hybris-egl", "eglCreateWindowSurface", "");
	struct _EGLDisplay *display = hybris_egl_display_get_mapping(dpy);
	win = ws_CreateWindow(win, display);

	assert(((struct ANativeWindowBuffer *) win)->common.magic == ANDROID_NATIVE_WINDOW_MAGIC);

	HYBRIS_TRACE_BEGIN("native-egl", "eglCreateWindowSurface", "");
	EGLSurface result = (*_eglCreateWindowSurface)(dpy, config, win, attrib_list);
	HYBRIS_TRACE_END("native-egl", "eglCreateWindowSurface", "");
	egl_helper_push_mapping(result, win);

	HYBRIS_TRACE_END("hybris-egl", "eglCreateWindowSurface", "");
	return result;
}

EGLSurface eglCreatePbufferSurface(EGLDisplay dpy, EGLConfig config,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreatePbufferSurface, "eglCreatePbufferSurface");
	return (*_eglCreatePbufferSurface)(dpy, config, attrib_list);
}

EGLSurface eglCreatePixmapSurface(EGLDisplay dpy, EGLConfig config,
		EGLNativePixmapType pixmap,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreatePixmapSurface, "eglCreatePixmapSurface");
	return (*_eglCreatePixmapSurface)(dpy, config, pixmap, attrib_list);
}

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	EGL_DLSYM(&_eglDestroySurface, "eglDestroySurface");
	EGLBoolean result = (*_eglDestroySurface)(dpy, surface);

	/**
         * If the surface was created via eglCreateWindowSurface, we must
         * notify the ws about surface destruction for clean-up.
	 **/
	if (egl_helper_has_mapping(surface)) {
	    ws_DestroyWindow(egl_helper_pop_mapping(surface));
	}

	return result;
}

EGLBoolean eglQuerySurface(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint *value)
{
	EGL_DLSYM(&_eglQuerySurface, "eglQuerySurface");
	return (*_eglQuerySurface)(dpy, surface, attribute, value);
}


EGLBoolean eglBindAPI(EGLenum api)
{
	EGL_DLSYM(&_eglBindAPI, "eglBindAPI");
	return (*_eglBindAPI)(api);
}

EGLenum eglQueryAPI(void)
{
	EGL_DLSYM(&_eglQueryAPI, "eglQueryAPI");
	return (*_eglQueryAPI)();
}

EGLBoolean eglWaitClient(void)
{
	EGL_DLSYM(&_eglWaitClient, "eglWaitClient");
	return (*_eglWaitClient)();
}

EGLBoolean eglReleaseThread(void)
{
	EGL_DLSYM(&_eglReleaseThread, "eglReleaseThread");
	return (*_eglReleaseThread)();
}

EGLSurface eglCreatePbufferFromClientBuffer(
		EGLDisplay dpy, EGLenum buftype, EGLClientBuffer buffer,
		EGLConfig config, const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreatePbufferFromClientBuffer, "eglCreatePbufferFromClientBuffer");
	return (*_eglCreatePbufferFromClientBuffer)(dpy, buftype, buffer, config, attrib_list);
}

EGLBoolean eglSurfaceAttrib(EGLDisplay dpy, EGLSurface surface,
		EGLint attribute, EGLint value)
{
	EGL_DLSYM(&_eglSurfaceAttrib, "eglSurfaceAttrib");
	return (*_eglSurfaceAttrib)(dpy, surface, attribute, value);
}

EGLBoolean eglBindTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	EGL_DLSYM(&_eglBindTexImage, "eglBindTexImage");
	return (*_eglBindTexImage)(dpy, surface, buffer);
}

EGLBoolean eglReleaseTexImage(EGLDisplay dpy, EGLSurface surface, EGLint buffer)
{
	EGL_DLSYM(&_eglReleaseTexImage, "eglReleaseTexImage");
	return (*_eglReleaseTexImage)(dpy, surface, buffer);
}

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	EGLBoolean ret;
	EGLSurface surface;
	EGLNativeWindowType win;
	HYBRIS_TRACE_BEGIN("hybris-egl", "eglSwapInterval", "=%d", interval);

	/* Some egl implementations don't pass through the setSwapInterval
	 * call.  Since we may support various swap intervals internally, we'll
	 * call it anyway and then give the wrapped egl implementation a chance
	 * to chage it. */
	EGL_DLSYM(&_eglGetCurrentSurface, "eglGetCurrentSurface");
	surface = (*_eglGetCurrentSurface)(EGL_DRAW);
	if (egl_helper_has_mapping(surface))
	    ws_setSwapInterval(dpy, egl_helper_get_mapping(surface), interval);

	HYBRIS_TRACE_BEGIN("native-egl", "eglSwapInterval", "=%d", interval);
	EGL_DLSYM(&_eglSwapInterval, "eglSwapInterval");
	ret = (*_eglSwapInterval)(dpy, interval);
	HYBRIS_TRACE_END("native-egl", "eglSwapInterval", "");
	HYBRIS_TRACE_END("hybris-egl", "eglSwapInterval", "");
	return ret;
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
		EGLContext share_context,
		const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreateContext, "eglCreateContext");

	EGLint *p = attrib_list;
	while (p != NULL && *p != EGL_NONE) {
		if (*p == EGL_CONTEXT_CLIENT_VERSION) {
			_egl_context_client_version = p[1];
		}
		p += 2;
	}

	return (*_eglCreateContext)(dpy, config, share_context, attrib_list);
}

EGLBoolean eglDestroyContext(EGLDisplay dpy, EGLContext ctx)
{
	EGL_DLSYM(&_eglDestroyContext, "eglDestroyContext");
	return (*_eglDestroyContext)(dpy, ctx);
}

EGLBoolean eglMakeCurrent(EGLDisplay dpy, EGLSurface draw,
		EGLSurface read, EGLContext ctx)
{
	EGL_DLSYM(&_eglMakeCurrent, "eglMakeCurrent");
	return (*_eglMakeCurrent)(dpy, draw, read, ctx);
}

EGLContext eglGetCurrentContext(void)
{
	EGL_DLSYM(&_eglGetCurrentContext, "eglGetCurrentContext");
	return (*_eglGetCurrentContext)();
}

EGLSurface eglGetCurrentSurface(EGLint readdraw)
{
	EGL_DLSYM(&_eglGetCurrentSurface, "eglGetCurrentSurface");
	return (*_eglGetCurrentSurface)(readdraw);
}

EGLDisplay eglGetCurrentDisplay(void)
{
	EGL_DLSYM(&_eglGetCurrentDisplay, "eglGetCurrentDisplay");
	return (*_eglGetCurrentDisplay)();
}

EGLBoolean eglQueryContext(EGLDisplay dpy, EGLContext ctx,
		EGLint attribute, EGLint *value)
{
	EGL_DLSYM(&_eglQueryContext, "eglQueryContext");
	return (*_eglQueryContext)(dpy, ctx, attribute, value);
}

EGLBoolean eglWaitGL(void)
{
	EGL_DLSYM(&_eglWaitGL, "eglWaitGL");
	return (*_eglWaitGL)();
}

EGLBoolean eglWaitNative(EGLint engine)
{
	EGL_DLSYM(&_eglWaitNative, "eglWaitNative");
	return (*_eglWaitNative)(engine);
}

EGLBoolean _my_eglSwapBuffersWithDamageEXT(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects)
{
	EGLNativeWindowType win;
	EGLBoolean ret;
	HYBRIS_TRACE_BEGIN("hybris-egl", "eglSwapBuffersWithDamageEXT", "");
	EGL_DLSYM(&_eglSwapBuffers, "eglSwapBuffers");

	if (egl_helper_has_mapping(surface)) {
		win = egl_helper_get_mapping(surface);
		ws_prepareSwap(dpy, win, rects, n_rects);
		ret = (*_eglSwapBuffers)(dpy, surface);
		ws_finishSwap(dpy, win);
	} else {
		ret = (*_eglSwapBuffers)(dpy, surface);
	}
	HYBRIS_TRACE_END("hybris-egl", "eglSwapBuffersWithDamageEXT", "");
	return ret;
}

EGLBoolean eglSwapBuffers(EGLDisplay dpy, EGLSurface surface)
{
	EGLBoolean ret;
	HYBRIS_TRACE_BEGIN("hybris-egl", "eglSwapBuffers", "");
	ret = _my_eglSwapBuffersWithDamageEXT(dpy, surface, NULL, 0);
	HYBRIS_TRACE_END("hybris-egl", "eglSwapBuffers", "");
	return ret;
}

EGLBoolean eglCopyBuffers(EGLDisplay dpy, EGLSurface surface,
		EGLNativePixmapType target)
{
	EGL_DLSYM(&_eglCopyBuffers, "eglCopyBuffers");
	return (*_eglCopyBuffers)(dpy, surface, target);
}


static EGLImageKHR _my_eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
	EGL_DLSYM(&_eglCreateImageKHR, "eglCreateImageKHR");
	EGLContext newctx = ctx;
	EGLenum newtarget = target;
	EGLClientBuffer newbuffer = buffer;
	const EGLint *newattrib_list = attrib_list;

	ws_passthroughImageKHR(&newctx, &newtarget, &newbuffer, &newattrib_list);

	EGLImageKHR eik = (*_eglCreateImageKHR)(dpy, newctx, newtarget, newbuffer, newattrib_list);

	if (eik == EGL_NO_IMAGE_KHR) {
		return EGL_NO_IMAGE_KHR;
	}

	struct egl_image *image;
	image = malloc(sizeof *image);
	image->egl_image = eik;
	image->egl_buffer = buffer;
	image->target = target;

	return (EGLImageKHR)image;
}

static void _my_glEGLImageTargetTexture2DOES(GLenum target, GLeglImageOES image)
{
	GLESv2_DLSYM(&_glEGLImageTargetTexture2DOES, "glEGLImageTargetTexture2DOES");
	struct egl_image *img = image;
	(*_glEGLImageTargetTexture2DOES)(target, img ? img->egl_image : NULL);
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname)
{
	EGL_DLSYM(&_eglGetProcAddress, "eglGetProcAddress");
	if (strcmp(procname, "eglCreateImageKHR") == 0)
	{
		return _my_eglCreateImageKHR;
	}
	else if (strcmp(procname, "eglDestroyImageKHR") == 0)
	{
		return eglDestroyImageKHR;
	}
	else if (strcmp(procname, "eglSwapBuffersWithDamageEXT") == 0)
	{
		return _my_eglSwapBuffersWithDamageEXT;
	}
	else if (strcmp(procname, "glEGLImageTargetTexture2DOES") == 0)
	{
		return _my_glEGLImageTargetTexture2DOES;
	}

	__eglMustCastToProperFunctionPointerType ret = NULL;

	switch (_egl_context_client_version) {
		case 1:  // OpenGL ES 1.x API
			if (_hybris_libgles1 == NULL) {
				_hybris_libgles1 = (void *) dlopen(getenv("HYBRIS_LIBGLESV1") ?: "libGLESv1_CM.so.1", RTLD_LAZY);
			}
			ret = _hybris_libgles1 ? dlsym(_hybris_libgles1, procname) : NULL;
			break;
		case 2:  // OpenGL ES 2.0 API
			if (_hybris_libgles2 == NULL) {
				_hybris_libgles2 = (void *) dlopen(getenv("HYBRIS_LIBGLESV2") ?: "libGLESv2.so.2", RTLD_LAZY);
			}
			ret = _hybris_libgles2 ? dlsym(_hybris_libgles2, procname) : NULL;
			break;
		case 3:  // OpenGL ES 3.x API
			// TODO: Load from libGLESv3.so once we have OpenGL ES 3.0/3.1 support
			break;
		default:
			HYBRIS_WARN("Unknown EGL context client version: %d", _egl_context_client_version);
			break;
	}

	if (ret == NULL) {
		ret = ws_eglGetProcAddress(procname);
	}

	if (ret == NULL) {
		ret = (*_eglGetProcAddress)(procname);
	}

	return ret;
}

EGLBoolean eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
	EGL_DLSYM(&_eglDestroyImageKHR, "eglDestroyImageKHR");
	struct egl_image *img = image;
	EGLBoolean ret = (*_eglDestroyImageKHR)(dpy, img ? img->egl_image : NULL);
	if (ret == EGL_TRUE) {
		free(img);
		return EGL_TRUE;
	}
	return ret;
}

// vim:ts=4:sw=4:noexpandtab
