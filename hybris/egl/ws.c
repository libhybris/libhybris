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

static struct ws_module *ws = NULL;

static void _init_ws()
{
	if (ws == NULL)
	{
		char ws_name[2048];
		char *egl_platform;

		egl_platform=getenv("EGL_PLATFORM");
		// Mesa uses EGL_PLATFORM for its own purposes.
		// Add HYBRIS_EGLPLATFORM to avoid the conflicts
		if (egl_platform == NULL)
			egl_platform=getenv("HYBRIS_EGLPLATFORM");

		if (egl_platform == NULL)
			egl_platform = "null";

		snprintf(ws_name, 2048, PKGLIBDIR "eglplatform_%s.so", egl_platform);

		void *wsmod = (void *) dlopen(ws_name, RTLD_LAZY);
		if (wsmod==NULL)
		{
			fprintf(stderr, "ERROR: %s\n\t%s\n", ws_name, dlerror());
			assert(0);
		}
		ws = dlsym(wsmod, "ws_module_info");
		assert(ws != NULL);
	}
}


int ws_IsValidDisplay(EGLNativeDisplayType display)
{
	_init_ws();
	return ws->IsValidDisplay(display);
}

EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	_init_ws();
	return ws->CreateWindow(win, display);
}

__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname)
{
	_init_ws();
	return ws->eglGetProcAddress(procname);
}

void ws_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer)
{
	_init_ws();
	return ws->passthroughImageKHR(target, buffer);
}

const char *ws_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name))
{
	_init_ws();
	return ws->eglQueryString(dpy, name, real_eglQueryString);
}

// vim:ts=4:sw=4:noexpandtab
