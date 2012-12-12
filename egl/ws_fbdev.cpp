#include <ws.h>
#include "fbdev_window.h"
#include <malloc.h>
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

extern "C" int ws_fbdev_IsValidDisplay(EGLNativeDisplayType display)
{
	return display == EGL_DEFAULT_DISPLAY; 
}

extern "C" EGLNativeWindowType ws_fbdev_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	return (EGLNativeWindowType) *(new FbDevNativeWindow());
}
