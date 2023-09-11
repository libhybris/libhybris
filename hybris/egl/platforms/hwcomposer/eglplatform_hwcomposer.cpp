/*
 * Copyright (c) 2013-2022 Jolla Ltd.
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

#include <android-config.h>
#include <ws.h>
#include "hwcomposer_window.h"
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <mutex>
#include <algorithm>
extern "C" {
#include <eglplatformcommon.h>
};

#include "logging.h"

#include <hybris/gralloc/gralloc.h>

static std::vector<HWComposerNativeWindow *> _nativewindows;
static std::mutex _nativewindows_mutex;

extern "C" void hwcomposerws_init_module(struct ws_egl_interface *egl_iface)
{
    hybris_gralloc_initialize(0);
	eglplatformcommon_init(egl_iface);
}

extern "C" _EGLDisplay *hwcomposerws_GetDisplay(EGLNativeDisplayType display)
{
	_EGLDisplay *dpy = 0;
	if (display == EGL_DEFAULT_DISPLAY) {
		dpy = new _EGLDisplay;
	}
	return dpy;
}

extern "C" void hwcomposerws_Terminate(_EGLDisplay *dpy)
{
	delete dpy;
}

extern "C" EGLNativeWindowType hwcomposerws_CreateWindow(EGLNativeWindowType win, _EGLDisplay *display)
{
	HWComposerNativeWindow *window = static_cast<HWComposerNativeWindow *>((ANativeWindow *) win);
	std::lock_guard<std::mutex> lock(_nativewindows_mutex);

	window->common.incRef(&window->common);
	_nativewindows.push_back(window);

	return (EGLNativeWindowType) static_cast<struct ANativeWindow *>(window);
}

extern "C" void hwcomposerws_DestroyWindow(EGLNativeWindowType win)
{
	HWComposerNativeWindow *window = static_cast<HWComposerNativeWindow *>((ANativeWindow *) win);
	std::lock_guard<std::mutex> lock(_nativewindows_mutex);

	std::vector<HWComposerNativeWindow *>::iterator it = std::find(_nativewindows.begin(),
		_nativewindows.end(), window);
	if (it != _nativewindows.end()) {
		window->common.decRef(&window->common);
		_nativewindows.erase(it);
	}
}

extern "C" __eglMustCastToProperFunctionPointerType hwcomposerws_eglGetProcAddress(const char *procname) 
{
	return eglplatformcommon_eglGetProcAddress(procname);
}

extern "C" void hwcomposerws_passthroughImageKHR(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list)
{
	eglplatformcommon_passthroughImageKHR(ctx, target, buffer, attrib_list);
}

struct ws_module ws_module_info = {
	hwcomposerws_init_module,
	hwcomposerws_GetDisplay,
	hwcomposerws_Terminate,
	hwcomposerws_CreateWindow,
	hwcomposerws_DestroyWindow,
	hwcomposerws_eglGetProcAddress,
	hwcomposerws_passthroughImageKHR,
	eglplatformcommon_eglQueryString
};

// vim:ts=4:sw=4:noexpandtab
