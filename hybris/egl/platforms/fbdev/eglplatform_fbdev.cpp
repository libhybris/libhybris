#include <ws.h>
#include "fbdev_window.h"
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

extern "C" int fbdevws_IsValidDisplay(EGLNativeDisplayType display)
{
	if (__sync_fetch_and_add(&inited,1)==0)
	{
		hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
		int err = framebuffer_open((hw_module_t *) gralloc, &framebuffer);
		printf("** open framebuffer HAL (%s) format x%x\n", strerror(-err), framebuffer->format);
		err = gralloc_open((const hw_module_t *) gralloc, &alloc);
		printf("** got gralloc %p err:%s\n", gralloc, strerror(-err));
		eglplatformcommon_init(gralloc);
	}

	return display == EGL_DEFAULT_DISPLAY;
}

extern "C" EGLNativeWindowType fbdevws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	assert (inited == 1);
	return (EGLNativeWindowType) *(new FbDevNativeWindow(gralloc, alloc, framebuffer));
}

extern "C" __eglMustCastToProperFunctionPointerType fbdevws_eglGetProcAddress(const char *procname) 
{
	return eglplatformcommon_eglGetProcAddress(procname);
}

extern "C" void fbdevws_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer)
{
	eglplatformcommon_passthroughImageKHR(target, buffer);
}

struct ws_module ws_module_info = {
	fbdevws_IsValidDisplay,
	fbdevws_CreateWindow,
	fbdevws_eglGetProcAddress,
	fbdevws_passthroughImageKHR,
	eglplatformcommon_eglQueryString
};





