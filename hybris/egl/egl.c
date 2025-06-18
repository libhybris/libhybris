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

#include "config.h"

#include <android-config.h>

/* EGL function pointers */
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <dlfcn.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include "ws.h"
#include "helper.h"
#include <assert.h>


#include <hybris/common/binding.h>
#include <stdlib.h>
#include <string.h>

#include <system/window.h>
#include "logging.h"

static void *egl_handle = NULL;
static void *glesv2_handle = NULL;
static void *_hybris_libgles1 = NULL;
static void *_hybris_libgles2 = NULL;
static int _egl_context_client_version = 1;

static EGLint      (*_eglGetError)(void) = NULL;

static EGLDisplay  (*_eglGetDisplay)(EGLNativeDisplayType display_id) = NULL;
static EGLBoolean  (*_eglInitialize)(EGLDisplay dpy, EGLint *major, EGLint *minor) = NULL;
static EGLBoolean  (*_eglTerminate)(EGLDisplay dpy) = NULL;

static const char *  (*_eglQueryString)(EGLDisplay dpy, EGLint name) = NULL;

static EGLSurface  (*_eglCreateWindowSurface)(EGLDisplay dpy, EGLConfig config,
		EGLNativeWindowType win,
		const EGLint *attrib_list) = NULL;
static EGLBoolean  (*_eglDestroySurface)(EGLDisplay dpy, EGLSurface surface) = NULL;

static EGLBoolean  (*_eglSwapInterval)(EGLDisplay dpy, EGLint interval) = NULL;


static EGLContext  (*_eglCreateContext)(EGLDisplay dpy, EGLConfig config,
		EGLContext share_context,
		const EGLint *attrib_list) = NULL;

static EGLSurface  (*_eglGetCurrentSurface)(EGLint readdraw) = NULL;

static EGLBoolean  (*_eglSwapBuffers)(EGLDisplay dpy, EGLSurface surface) = NULL;
static EGLBoolean  (*_eglSwapBuffersWithDamageKHR)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects) = NULL;
static EGLBoolean  (*_eglSwapBuffersWithDamageEXT)(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects) = NULL;


static EGLImageKHR (*_eglCreateImageKHR)(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list) = NULL;
static EGLBoolean (*_eglDestroyImageKHR) (EGLDisplay dpy, EGLImageKHR image) = NULL;

static void (*_glEGLImageTargetTexture2DOES) (GLenum target, GLeglImageOES image) = NULL;

static __eglMustCastToProperFunctionPointerType (*_eglGetProcAddress)(const char *procname) = NULL;

static void _init_androidegl()
{
	egl_handle = (void *) android_dlopen(getenv("LIBEGL") ? getenv("LIBEGL") : "libEGL.so", RTLD_LAZY);
	glesv2_handle = (void *) android_dlopen(getenv("LIBGLESV2") ? getenv("LIBGLESV2") : "libGLESv2.so", RTLD_LAZY);
}

static inline void hybris_egl_initialize()
{
	_init_androidegl();
}

static inline void hybris_glesv2_initialize()
{
	_init_androidegl();
}

static void * _android_egl_dlsym(const char *symbol)
{
	if (egl_handle == NULL)
		_init_androidegl();

	return android_dlsym(egl_handle, symbol);
}

struct ws_egl_interface hybris_egl_interface = {
	_android_egl_dlsym,
	egl_helper_has_mapping,
	egl_helper_get_mapping,
};

static __thread EGLint __eglHybrisError = EGL_SUCCESS;

void __eglHybrisSetError(EGLint error)
{
	__eglHybrisError = error;
}

EGLint eglGetError(void)
{
	/*
	 * EGL requires that eglGetError() reports only the "most recent" error
	 * and clear the error afterward. Because we don't hook every function,
	 * both us and Android can have a separated error and we won't know what
	 * is the most recent. But whatever error we report, we have to clear both
	 * error. So we report our error over Android's because it's easier this
	 * way.
	 */
	HYBRIS_DLSYSM(egl, &_eglGetError, "eglGetError");

	EGLint androidError = _eglGetError();

	if (__eglHybrisError != EGL_SUCCESS) {
		EGLint ourError = __eglHybrisError;
		__eglHybrisError = EGL_SUCCESS;
		return ourError;
	}

	return androidError;
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

void hybris_egl_display_release_mappings(void)
{
	int i;
	for (i = 0; i < _EGL_MAX_DISPLAYS; i++)
	{
		if (_displayMappings[i])
		{
			ws_releaseDisplay(_displayMappings[i]);
			_displayMappings[i] = NULL;
		}
	}
}

static const char * _defaultEglPlatform()
{
	char *egl_platform;

	// Mesa uses EGL_PLATFORM for its own purposes.
	// Add HYBRIS_EGLPLATFORM to avoid the conflicts
	egl_platform = getenv("HYBRIS_EGLPLATFORM");

	if (egl_platform == NULL)
		egl_platform = getenv("EGL_PLATFORM");

	// The env variables may be defined yet empty
	if (egl_platform == NULL || strcmp(egl_platform, "") == 0)
		egl_platform = DEFAULT_EGL_PLATFORM;

	return egl_platform;
}

#ifndef WANT_GLVND
static
#endif
EGLDisplay __eglHybrisGetPlatformDisplayCommon(EGLenum platform,
        void *display_id, const EGLAttrib *attrib_list)
{
	// We have nothing to do with attrib_list at the moment. Silence the unused
	// variable warning.
	(void) attrib_list;

	HYBRIS_DLSYSM(egl, &_eglGetDisplay, "eglGetDisplay");

	if (!_eglGetDisplay) {
		__eglHybrisSetError(EGL_NOT_INITIALIZED);
		return EGL_NO_DISPLAY;
	}

	const char * hybris_ws;
	switch (platform) {
		case EGL_NONE:
			hybris_ws = _defaultEglPlatform();
			break;

		case EGL_PLATFORM_ANDROID_KHR:
			// "null" ws passthrough everything, which essentially means
			// the Android platform. Not to be confused with NULL (0) value.
			hybris_ws = "null";
			break;

#ifdef WANT_WAYLAND
		case EGL_PLATFORM_WAYLAND_KHR:
			hybris_ws = "wayland";
			break;
#endif

		default:
			__eglHybrisSetError(EGL_BAD_PARAMETER);
			return EGL_NO_DISPLAY;
	}

	if (ws_init(hybris_ws) == EGL_FALSE) { // Other ws already loaded.
		__eglHybrisSetError(EGL_BAD_PARAMETER);
		return EGL_NO_DISPLAY;
	}

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

EGLDisplay eglGetDisplay(EGLNativeDisplayType display_id)
{
	return __eglHybrisGetPlatformDisplayCommon(EGL_NONE, display_id, NULL);
}

EGLDisplay eglGetPlatformDisplay(EGLenum platform,
        void *display_id, const EGLAttrib *attrib_list)
{
	if (platform == EGL_NONE) {
		__eglHybrisSetError(EGL_BAD_PARAMETER);
		return EGL_NO_DISPLAY;
	}

	return __eglHybrisGetPlatformDisplayCommon(platform, display_id, attrib_list);
}

EGLBoolean eglInitialize(EGLDisplay dpy, EGLint *major, EGLint *minor)
{
	HYBRIS_DLSYSM(egl, &_eglInitialize, "eglInitialize");
	EGLBoolean ret = _eglInitialize(dpy, major, minor);
	if (ret) {
		struct _EGLDisplay *display = hybris_egl_display_get_mapping(dpy);
		ws_eglInitialized(display);
	}
	return ret;
}

EGLBoolean eglTerminate(EGLDisplay dpy)
{
	HYBRIS_DLSYSM(egl, &_eglTerminate, "eglTerminate");

	struct _EGLDisplay *display = hybris_egl_display_get_mapping(dpy);
	ws_Terminate(display);
	return (*_eglTerminate)(dpy);
}

const char * eglQueryString(EGLDisplay dpy, EGLint name)
{
	HYBRIS_DLSYSM(egl, &_eglQueryString, "eglQueryString");
	return ws_eglQueryString(dpy, name, _eglQueryString);
}

HYBRIS_IMPLEMENT_FUNCTION4(egl, EGLBoolean, eglGetConfigs, EGLDisplay, EGLConfig *, EGLint, EGLint *);
HYBRIS_IMPLEMENT_FUNCTION5(egl, EGLBoolean, eglChooseConfig, EGLDisplay, const EGLint *, EGLConfig *, EGLint, EGLint *);
HYBRIS_IMPLEMENT_FUNCTION4(egl, EGLBoolean, eglGetConfigAttrib, EGLDisplay, EGLConfig, EGLint, EGLint *);

EGLSurface eglCreateWindowSurface(EGLDisplay dpy, EGLConfig config,
		EGLNativeWindowType win,
		const EGLint *attrib_list)
{
	HYBRIS_DLSYSM(egl, &_eglCreateWindowSurface, "eglCreateWindowSurface");

	HYBRIS_TRACE_BEGIN("hybris-egl", "eglCreateWindowSurface", "");
	struct _EGLDisplay *display = hybris_egl_display_get_mapping(dpy);
	win = ws_CreateWindow(win, display);

	assert(((struct ANativeWindow *) win)->common.magic == ANDROID_NATIVE_WINDOW_MAGIC);

	HYBRIS_TRACE_BEGIN("native-egl", "eglCreateWindowSurface", "");
	EGLSurface result = (*_eglCreateWindowSurface)(dpy, config, win, attrib_list);

	HYBRIS_TRACE_END("native-egl", "eglCreateWindowSurface", "");

	if (result != EGL_NO_SURFACE)
		egl_helper_push_mapping(result, win);

	HYBRIS_TRACE_END("hybris-egl", "eglCreateWindowSurface", "");
	return result;
}
EGLSurface eglCreatePlatformWindowSurface(EGLDisplay dpy, EGLConfig config,
		void *native_window, const EGLAttrib *attrib_list)
{
	return eglCreateWindowSurface(dpy, config, (uintptr_t) native_window, (const EGLint *) attrib_list);
}

static EGLSurface _my_eglCreatePlatformWindowSurfaceEXT(EGLDisplay dpy, EGLConfig config,
		void *native_window, const EGLint *attrib_list)
{
	/*
	 * TODO: Convert type of parameters here if semantics for native_window
	 * differs from EGLNativeWindowType. Both Android(-based) platform and
	 * Wayland platform doesn't have this problem (they both accept a pointer).
	 * However, a patch for X11 exists, and for X11 platform you pass a pointer
	 * to Window in this function, but the Window itself (which is an XID) as
	 * EGLNativeWindowType. The patch probably have to patch this function.
	 */

	return eglCreateWindowSurface(dpy, config, (uintptr_t) native_window, attrib_list);
}

HYBRIS_IMPLEMENT_FUNCTION3(egl, EGLSurface, eglCreatePbufferSurface, EGLDisplay, EGLConfig, const EGLint *);
HYBRIS_IMPLEMENT_FUNCTION4(egl, EGLSurface, eglCreatePixmapSurface, EGLDisplay, EGLConfig, EGLNativePixmapType, const EGLint *);

EGLBoolean eglDestroySurface(EGLDisplay dpy, EGLSurface surface)
{
	HYBRIS_DLSYSM(egl, &_eglDestroySurface, "eglDestroySurface");
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

HYBRIS_IMPLEMENT_FUNCTION4(egl, EGLBoolean, eglQuerySurface, EGLDisplay, EGLSurface, EGLint, EGLint *);
HYBRIS_IMPLEMENT_FUNCTION1(egl, EGLBoolean, eglBindAPI, EGLenum);
HYBRIS_IMPLEMENT_FUNCTION0(egl, EGLenum, eglQueryAPI);
HYBRIS_IMPLEMENT_FUNCTION0(egl, EGLBoolean, eglWaitClient);
HYBRIS_IMPLEMENT_FUNCTION0(egl, EGLBoolean, eglReleaseThread);
HYBRIS_IMPLEMENT_FUNCTION5(egl, EGLSurface, eglCreatePbufferFromClientBuffer, EGLDisplay, EGLenum, EGLClientBuffer, EGLConfig, const EGLint *);
HYBRIS_IMPLEMENT_FUNCTION4(egl, EGLBoolean, eglSurfaceAttrib, EGLDisplay, EGLSurface, EGLint, EGLint);
HYBRIS_IMPLEMENT_FUNCTION3(egl, EGLBoolean, eglBindTexImage, EGLDisplay, EGLSurface, EGLint);
HYBRIS_IMPLEMENT_FUNCTION3(egl, EGLBoolean, eglReleaseTexImage, EGLDisplay, EGLSurface, EGLint);

EGLBoolean eglSwapInterval(EGLDisplay dpy, EGLint interval)
{
	EGLBoolean ret;
	EGLSurface surface;
	HYBRIS_TRACE_BEGIN("hybris-egl", "eglSwapInterval", "=%d", interval);

	/* Some egl implementations don't pass through the setSwapInterval
	 * call.  Since we may support various swap intervals internally, we'll
	 * call it anyway and then give the wrapped egl implementation a chance
	 * to chage it. */
	HYBRIS_DLSYSM(egl, &_eglGetCurrentSurface, "eglGetCurrentSurface");
	surface = (*_eglGetCurrentSurface)(EGL_DRAW);
	if (egl_helper_has_mapping(surface))
	    ws_setSwapInterval(dpy, egl_helper_get_mapping(surface), interval);

	HYBRIS_TRACE_BEGIN("native-egl", "eglSwapInterval", "=%d", interval);
	HYBRIS_DLSYSM(egl, &_eglSwapInterval, "eglSwapInterval");
	ret = (*_eglSwapInterval)(dpy, interval);
	HYBRIS_TRACE_END("native-egl", "eglSwapInterval", "");
	HYBRIS_TRACE_END("hybris-egl", "eglSwapInterval", "");
	return ret;
}

EGLContext eglCreateContext(EGLDisplay dpy, EGLConfig config,
		EGLContext share_context,
		const EGLint *attrib_list)
{
	HYBRIS_DLSYSM(egl, &_eglCreateContext, "eglCreateContext");

	const EGLint *p = attrib_list;
	while (p != NULL && *p != EGL_NONE) {
		if (*p == EGL_CONTEXT_CLIENT_VERSION) {
			_egl_context_client_version = p[1];
		}
		p += 2;
	}

	return (*_eglCreateContext)(dpy, config, share_context, attrib_list);
}

HYBRIS_IMPLEMENT_FUNCTION2(egl, EGLBoolean, eglDestroyContext, EGLDisplay, EGLContext);
HYBRIS_IMPLEMENT_FUNCTION4(egl, EGLBoolean, eglMakeCurrent, EGLDisplay, EGLSurface, EGLSurface, EGLContext);
HYBRIS_IMPLEMENT_FUNCTION0(egl, EGLContext, eglGetCurrentContext);
HYBRIS_IMPLEMENT_FUNCTION1(egl, EGLSurface, eglGetCurrentSurface, EGLint);
HYBRIS_IMPLEMENT_FUNCTION0(egl, EGLDisplay, eglGetCurrentDisplay);
HYBRIS_IMPLEMENT_FUNCTION4(egl, EGLBoolean, eglQueryContext, EGLDisplay, EGLContext, EGLint, EGLint *);
HYBRIS_IMPLEMENT_FUNCTION0(egl, EGLBoolean, eglWaitGL);
HYBRIS_IMPLEMENT_FUNCTION1(egl, EGLBoolean, eglWaitNative, EGLint);

EGLBoolean _my_eglSwapBuffersWithDamageEXT(EGLDisplay dpy, EGLSurface surface, EGLint *rects, EGLint n_rects)
{
	EGLNativeWindowType win = 0;
	EGLBoolean ret;
	HYBRIS_TRACE_BEGIN("hybris-egl", "eglSwapBuffersWithDamageEXT", "");
	HYBRIS_DLSYSM(egl, &_eglSwapBuffers, "eglSwapBuffers");
	HYBRIS_DLSYSM(egl, &_eglSwapBuffersWithDamageKHR, "eglSwapBuffersWithDamageKHR");
	HYBRIS_DLSYSM(egl, &_eglSwapBuffersWithDamageEXT, "eglSwapBuffersWithDamageEXT");

	if (egl_helper_has_mapping(surface)) {
		win = egl_helper_get_mapping(surface);
	}

	if (win) {
		ws_prepareSwap(dpy, win, rects, n_rects);
	}

	if (_eglSwapBuffersWithDamageKHR) {
		ret = (*_eglSwapBuffersWithDamageKHR)(dpy, surface, rects, n_rects);
	} else if (_eglSwapBuffersWithDamageEXT) {
		ret = (*_eglSwapBuffersWithDamageEXT)(dpy, surface, rects, n_rects);
	} else {
		ret = (*_eglSwapBuffers)(dpy, surface);
	}

	if (win) {
		ws_finishSwap(dpy, win);
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

HYBRIS_IMPLEMENT_FUNCTION3(egl, EGLBoolean, eglCopyBuffers, EGLDisplay, EGLSurface, EGLNativePixmapType);


static EGLImageKHR _my_eglCreateImageKHR(EGLDisplay dpy, EGLContext ctx, EGLenum target, EGLClientBuffer buffer, const EGLint *attrib_list)
{
	HYBRIS_DLSYSM(egl, &_eglCreateImageKHR, "eglCreateImageKHR");
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
	HYBRIS_DLSYSM(glesv2, &_glEGLImageTargetTexture2DOES, "glEGLImageTargetTexture2DOES");
	struct egl_image *img = image;
	(*_glEGLImageTargetTexture2DOES)(target, img ? img->egl_image : NULL);
}

EGLBoolean _my_eglDestroyImageKHR(EGLDisplay dpy, EGLImageKHR image)
{
	HYBRIS_DLSYSM(egl, &_eglDestroyImageKHR, "eglDestroyImageKHR");
	struct egl_image *img = image;
	EGLBoolean ret = (*_eglDestroyImageKHR)(dpy, img ? img->egl_image : NULL);
	if (ret == EGL_TRUE) {
		free(img);
		return EGL_TRUE;
	}
	return ret;
}

struct FuncNamePair {
	const char * name;
	__eglMustCastToProperFunctionPointerType func;
};

#define OVERRIDE_SAMENAME(function) { .name = #function, .func = (__eglMustCastToProperFunctionPointerType) function }
#define OVERRIDE_MY(function) { .name = #function, .func = (__eglMustCastToProperFunctionPointerType) _my_ ## function }
#define OVERRIDE_TO(function_from, function) { .name = #function_from, .func = (__eglMustCastToProperFunctionPointerType) function }

static struct FuncNamePair _eglHybrisOverrideFunctions[] = {
	OVERRIDE_MY(eglCreateImageKHR),
	OVERRIDE_MY(eglSwapBuffersWithDamageEXT),
	OVERRIDE_TO(eglSwapBuffersWithDamageKHR, _my_eglSwapBuffersWithDamageEXT),
	OVERRIDE_MY(glEGLImageTargetTexture2DOES),
	OVERRIDE_MY(eglDestroyImageKHR),
	OVERRIDE_SAMENAME(eglGetError),
	OVERRIDE_SAMENAME(eglGetDisplay),
	OVERRIDE_SAMENAME(eglGetPlatformDisplay),
	OVERRIDE_SAMENAME(eglInitialize),
	OVERRIDE_SAMENAME(eglTerminate),
	OVERRIDE_SAMENAME(eglCreateWindowSurface),
	OVERRIDE_SAMENAME(eglCreatePlatformWindowSurface),
	OVERRIDE_SAMENAME(eglDestroySurface),
	OVERRIDE_SAMENAME(eglSwapInterval),
	OVERRIDE_SAMENAME(eglCreateContext),
	OVERRIDE_SAMENAME(eglSwapBuffers),
	OVERRIDE_SAMENAME(eglGetProcAddress),
	/*
	 * EGL_EXT_platform_base, in case Android EGL or glvnd advertise its
	 * support.
	 * 
	 * For glvnd, it will advertise EGL_EXT_platform_base when one of
	 * the vendor libraries support the extension. And most of the time, we're
	 * installed together with Mesa, which support the extension, so we're
	 * forced to support it or some client (e.g. Weston nested) will not work
	 * properly (see [1]).
	 * 
	 * Nonetheless, we should wrap these functions anyway, as we have to wrap
	 * both eglGetDisplay() and eglCreateWindowSurface().
	 *
	 * Note that the signature of eglGetPlatformDisplayEXT() isn't exactly the
	 * same as eglGetPlatformDisplay(); the EXT one uses EGLint for attrib_list
	 * but non-EXT one uses EGLAttrib (introduced in EGL 1.5). However, because
	 * we don't use attrib_list (at the moment), we can simply ignore that.
	 * 
	 * eglCreatePlatformWindowSurfaceEXT() can have different schematic from
	 * non-platform version (see the function's body, and also [1]). I don't
	 * know what's up with eglCreatePlatformPixmapSurfaceEXT(); it seems to
	 * require cooperation from WS, but it seems to be implemented by only the
	 * standard Hybris wrapper.
	 *
	 * [1] https://gitlab.freedesktop.org/glvnd/libglvnd/-/issues/214
	 */
	OVERRIDE_TO(eglGetPlatformDisplayEXT, eglGetPlatformDisplay),
	OVERRIDE_MY(eglCreatePlatformWindowSurfaceEXT),
	OVERRIDE_TO(eglCreatePlatformPixmapSurfaceEXT, eglCreatePixmapSurface),
};
static EGLBoolean _eglHybrisOverrideFunctions_sorted = EGL_FALSE;

#undef OVERRIDE_SANENAME
#undef OVERRIDE_MY
#undef OVERRIDE_TO

static int compare_sort(const void * a, const void * b)
{
	const struct FuncNamePair *f_a = a, *f_b = b;
	return strcmp(f_a->name, f_b->name);
}

static int compare_search(const void * key, const void * item)
{
	const struct FuncNamePair *f_item = item;
	return strcmp(key, f_item->name);
}

__eglMustCastToProperFunctionPointerType eglGetProcAddress(const char *procname)
{
	HYBRIS_DLSYSM(egl, &_eglGetProcAddress, "eglGetProcAddress");

	if (!_eglHybrisOverrideFunctions_sorted) {
		_eglHybrisOverrideFunctions_sorted = EGL_TRUE;
		qsort(
			_eglHybrisOverrideFunctions,
			sizeof(_eglHybrisOverrideFunctions) / sizeof(_eglHybrisOverrideFunctions[0]),
			sizeof(struct FuncNamePair),
			compare_sort
		);
	}

	struct FuncNamePair *result = bsearch(
		procname,
		_eglHybrisOverrideFunctions,
		sizeof(_eglHybrisOverrideFunctions) / sizeof(_eglHybrisOverrideFunctions[0]),
		sizeof(struct FuncNamePair),
		compare_search
	);
	if (result)
		return result->func;

	__eglMustCastToProperFunctionPointerType ret = NULL;

	switch (_egl_context_client_version) {
		case 1:  // OpenGL ES 1.x API
			if (_hybris_libgles1 == NULL) {
				_hybris_libgles1 = (void *) dlopen(
					getenv("HYBRIS_LIBGLESV1") ?: "libGLESv1_CM" GL_LIB_SUFFIX ".so.1",
					RTLD_LOCAL | RTLD_LAZY);
			}
			ret = _hybris_libgles1 ? dlsym(_hybris_libgles1, procname) : NULL;
			break;
		case 2:  // OpenGL ES 2.0 API
		case 3:  // OpenGL ES 3.x API, backwards compatible with OpenGL ES 2.0 so we implement in same library
			if (_hybris_libgles2 == NULL) {
				_hybris_libgles2 = (void *) dlopen(
					getenv("HYBRIS_LIBGLESV2") ?: "libGLESv2" GL_LIB_SUFFIX ".so.2",
					RTLD_LOCAL | RTLD_LAZY);
			}
			ret = _hybris_libgles2 ? dlsym(_hybris_libgles2, procname) : NULL;
			break;
		default:
			HYBRIS_WARN("Unknown EGL context client version: %d", _egl_context_client_version);
			break;
	}

	if (ret == NULL) {
		ret = ws_eglGetProcAddress(procname);
	}

	if (ret == NULL && _eglGetProcAddress != NULL) {
		ret = (*_eglGetProcAddress)(procname);
	}

	return ret;
}

// vim:ts=4:sw=4:noexpandtab
