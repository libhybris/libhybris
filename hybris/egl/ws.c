

#include <ws.h>
#include <stdlib.h>
#include <dlfcn.h>

#define WS_NULL 0
#define WS_FBDEV 1

static void * (*_androidCreateDisplaySurface)();
static void *_libui = NULL;

static void _init_androidui()
{
       _libui = (void *) android_dlopen("/system/lib/libui.so", RTLD_LAZY);
}

#define UI_DLSYM(fptr, sym) do { if (_libui == NULL) { _init_androidui(); }; if (*(fptr) == NULL) { *(fptr) = (void *) android_dlsym(_libui, sym); } } while (0) 

EGLNativeWindowType android_createDisplaySurface()
{
	UI_DLSYM(&_androidCreateDisplaySurface, "android_createDisplaySurface");
	return (EGLNativeWindowType) (*_androidCreateDisplaySurface)();
}


int _whatWS()
{
	char *egl_platform = getenv("EGL_PLATFORM");
	if (egl_platform == NULL)
		return WS_NULL;
	if (strcmp(egl_platform, "null") == 0)
	{
		return WS_NULL;
	}
	else if (strcmp(egl_platform, "fbdev") == 0)
	{
		return WS_FBDEV;
	}

	return -1;
}

int ws_IsValidDisplay(EGLNativeDisplayType display)
{
	switch (_whatWS())
	{
		case WS_NULL: return 1;
		case WS_FBDEV: return ws_fbdev_IsValidDisplay(display);
		default: ;
	}	
	return -1;
}

EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display)
{
	switch (_whatWS())
	{
		case WS_NULL: if (win == 0) { return android_createDisplaySurface(); } else { return win; } 
		case WS_FBDEV: return ws_fbdev_CreateWindow(win, display);
		default: ;
	}	
	return -1;
}

__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname) 
{
	return NULL;
}

void ws_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer)
{
}

