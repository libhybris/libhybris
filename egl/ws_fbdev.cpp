#include <ws.h>
#include "fbdev_window.h"
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

static int inited = 0;
static gralloc_module_t *gralloc; 
static framebuffer_device_t *framebuffer;
static alloc_device_t *alloc;

extern "C" int ws_fbdev_IsValidDisplay(EGLNativeDisplayType display)
{
	if (inited == 0)
	{	
  	    hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t **) &gralloc);
	    int err = framebuffer_open((hw_module_t *) gralloc, &framebuffer);
    	    printf("open framebuffer HAL (%s) format %i", strerror(-err), framebuffer->format);
 
	    err = gralloc_open((const hw_module_t *) gralloc, &alloc);
            printf("got gralloc %p err:%s\n", gralloc, strerror(-err));
	    inited = 1;	
	}
	return display == EGL_DEFAULT_DISPLAY; 
}

extern "C" EGLNativeWindowType ws_fbdev_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	assert (inited == 1);
	return (EGLNativeWindowType) *(new FbDevNativeWindow(gralloc, alloc, framebuffer));
}
