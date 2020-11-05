/*
 * Copyright (C) 2013 libhybris
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
 */

#include <ws.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <sys/auxv.h>
#include <pthread.h>
#include <string.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static struct ws_module *ws = NULL;
static char ws_name[32] = { 0 };

static EGLBoolean ensureCorrectWs(const char * egl_platform)
{
	return strcmp(egl_platform, ws_name) == 0;
}

/*
 * ws_init() will be called from __eglHybrisGetPlatformDisplayCommon() to
 * load the selected platform. That means the loading of ws is postponed until
 * a display is opened. Ws functions which doesn't depend on having a display
 * opened must cope with not having a ws.
 *
 * The function will return EGL_FALSE if the opened ws is not the requested
 * one. It doesn't make sense to have multiple ws'es opened; Android EGL (and
 * by extension our implementation) allow only one display to be opened.
 */
EGLBoolean ws_init(const char * egl_platform)
{
	if (ws != NULL)
		return ensureCorrectWs(egl_platform);

	pthread_mutex_lock(&mutex);
	if (ws != NULL) {
		pthread_mutex_unlock(&mutex);
		return ensureCorrectWs(egl_platform);
	}

	char ws_lib_path[2048];

	const char *eglplatform_dir = PKGLIBDIR;
	const char *user_eglplatform_dir = getauxval(AT_SECURE)
					   ? NULL
					   : getenv("HYBRIS_EGLPLATFORM_DIR");
	if (user_eglplatform_dir)
		eglplatform_dir = user_eglplatform_dir;

	snprintf(ws_lib_path, 2048, "%s/eglplatform_%s.so", eglplatform_dir, egl_platform);

	void *wsmod = (void *) dlopen(ws_lib_path, RTLD_LAZY);
	if (wsmod==NULL)
	{
		fprintf(stderr, "ERROR: %s\n\t%s\n", ws_lib_path, dlerror());
		assert(0);
	}
	ws = dlsym(wsmod, "ws_module_info");
	assert(ws != NULL);
	ws->init_module(&hybris_egl_interface);

	// Keep the name of chosen ws so that future calls can check.
	strncpy(ws_name, egl_platform, sizeof(ws_name));

	pthread_mutex_unlock(&mutex);
	return EGL_TRUE;
}


struct _EGLDisplay *ws_GetDisplay(EGLNativeDisplayType display)
{
	assert(ws != NULL);
	return ws->GetDisplay(display);
}

void ws_Terminate(struct _EGLDisplay *dpy)
{
	assert(ws != NULL);
	ws->Terminate(dpy);
}

EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, struct _EGLDisplay *display)
{
	assert(ws != NULL);
	return ws->CreateWindow(win, display);
}

void ws_DestroyWindow(EGLNativeWindowType win)
{
	assert(ws != NULL);
	return ws->DestroyWindow(win);
}

__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname)
{
	/*
	 * eglGetProcAddress is special; it must be able to provide address very early.
	 * Thus it cannot just assert() like other functions. Instead, we'll return NULL
	 * if ws is not initialized. The side effect is that ws cannot hook functions
	 * that are loaded early. So far functions that glvnd loads early are core functions
	 * that shouldn't be hooked by ws anyway.
	 */
	if (ws == NULL)
		return NULL;

	return ws->eglGetProcAddress(procname);
}

void ws_passthroughImageKHR(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list)
{
	assert(ws != NULL);
	return ws->passthroughImageKHR(ctx, target, buffer, attrib_list);
}

const char *ws_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name))
{
	if (dpy == EGL_NO_DISPLAY)
		// Querying client strings doesn't depend on having a ws.
		return real_eglQueryString(dpy, name);

	assert(ws != NULL);
	return ws->eglQueryString(dpy, name, real_eglQueryString);
}

void ws_prepareSwap(EGLDisplay dpy, EGLNativeWindowType win, EGLint *damage_rects, EGLint damage_n_rects)
{
	assert(ws != NULL);
	if (ws->prepareSwap)
		ws->prepareSwap(dpy, win, damage_rects, damage_n_rects);
}

void ws_finishSwap(EGLDisplay dpy, EGLNativeWindowType win)
{
	assert(ws != NULL);
	if (ws->finishSwap)
		ws->finishSwap(dpy, win);
}

void ws_setSwapInterval(EGLDisplay dpy, EGLNativeWindowType win, EGLint interval)
{
	assert(ws != NULL);
	if (ws->setSwapInterval)
		ws->setSwapInterval(dpy, win, interval);
}

// vim:ts=4:sw=4:noexpandtab
