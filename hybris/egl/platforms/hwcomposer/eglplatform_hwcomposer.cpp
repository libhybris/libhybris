#include <ws.h>
#include "hwcomposer_window.h"
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
extern "C" {
#include <eglplatformcommon.h>
};

static int inited = 0;
static gralloc_module_t *gralloc = 0;
static framebuffer_device_t *framebuffer = 0;
static alloc_device_t *alloc = 0;

extern "C" int hwcomposerws_IsValidDisplay(EGLNativeDisplayType display)
{
	if (__sync_fetch_and_add(&inited,1)==0)
	{
		int err;
		err = hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
		if (gralloc==NULL) {
			fprintf(stderr, "failed to get gralloc module: (%s)\n",strerror(-err));
			assert(0);
		}

		/* Not fatal on HW composer >= 1.1 */
		err = framebuffer_open((hw_module_t *) gralloc, &framebuffer);
		if (err) {
			fprintf(stderr, "WARNING: failed to open framebuffer: (%s)\n",strerror(-err));
		}
		printf("** framebuffer_open: status=(%s) format=x%x", strerror(-err), framebuffer->format);
	

		err = gralloc_open((const hw_module_t *) gralloc, &alloc);
		if (err) {
			fprintf(stderr, "ERROR: failed to open gralloc: (%s)\n",strerror(-err));
			assert(0);
		}
		printf("** gralloc_open %p status=%s\n", gralloc, strerror(-err));

		framebuffer_close(framebuffer);
		eglplatformcommon_init(gralloc);
	}

	return display == EGL_DEFAULT_DISPLAY;
}

extern "C" EGLNativeWindowType hwcomposerws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	assert (inited == 1);
	assert (win != 0);
	
	HWComposerNativeWindow *window = static_cast<HWComposerNativeWindow *>((ANativeWindow *) win);
	window->setup(gralloc, alloc);

	return win;
}

extern "C" __eglMustCastToProperFunctionPointerType hwcomposerws_eglGetProcAddress(const char *procname) 
{
	return eglplatformcommon_eglGetProcAddress(procname);
}

extern "C" void hwcomposerws_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer)
{
	eglplatformcommon_passthroughImageKHR(target, buffer);
}

struct ws_module ws_module_info = {
	hwcomposerws_IsValidDisplay,
	hwcomposerws_CreateWindow,
	hwcomposerws_eglGetProcAddress,
	hwcomposerws_passthroughImageKHR,
	eglplatformcommon_eglQueryString
};

// vim:ts=4:sw=4:noexpandtab
