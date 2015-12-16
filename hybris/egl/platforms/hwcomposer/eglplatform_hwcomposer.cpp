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
extern "C" {
#include <eglplatformcommon.h>
};

#include "logging.h"

static gralloc_module_t *gralloc = 0;
static alloc_device_t *alloc = 0;
static HWComposerNativeWindow *_nativewindow = NULL;

extern "C" void hwcomposerws_init_module(struct ws_egl_interface *egl_iface)
{
	int err;
	err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
	if (gralloc==NULL) {
		fprintf(stderr, "failed to get gralloc module: (%s)\n",strerror(-err));
		assert(0);
	}

	err = gralloc_open((const hw_module_t *) gralloc, &alloc);
	if (err) {
		fprintf(stderr, "ERROR: failed to open gralloc: (%s)\n",strerror(-err));
		assert(0);
	}
	TRACE("** gralloc_open %p status=%s", gralloc, strerror(-err));
	eglplatformcommon_init(egl_iface, gralloc, alloc);
}

extern "C" _EGLDisplay *hwcomposerws_GetDisplay(EGLNativeDisplayType display)
{
	assert (gralloc != NULL);
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
	assert (gralloc != NULL);
	assert (_nativewindow == NULL);

	HWComposerNativeWindow *window = static_cast<HWComposerNativeWindow *>((ANativeWindow *) win);
	window->setup(gralloc, alloc);
	_nativewindow = window;
	_nativewindow->common.incRef(&_nativewindow->common);
	return (EGLNativeWindowType) static_cast<struct ANativeWindow *>(_nativewindow);
}

extern "C" void hwcomposerws_DestroyWindow(EGLNativeWindowType win)
{
	assert (_nativewindow != NULL);
	assert (static_cast<HWComposerNativeWindow *>((struct ANativeWindow *)win) == _nativewindow);

	_nativewindow->common.decRef(&_nativewindow->common);
	/* We are done with it, refcounting will delete the window when appropriate */
	_nativewindow = NULL;
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
