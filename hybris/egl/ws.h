#ifndef __LIBHYBRIS_WS_H
#define __LIBHYBRIS_WS_H
#include <EGL/egl.h>

struct ws_module {
	int (*IsValidDisplay)(EGLNativeDisplayType display_id);
	EGLNativeWindowType (*CreateWindow)(EGLNativeWindowType win, EGLNativeDisplayType display);
	__eglMustCastToProperFunctionPointerType (*eglGetProcAddress)(const char *procname);
	void (*passthroughImageKHR)(EGLenum *target, EGLClientBuffer *buffer);
	const char *(*eglQueryString)(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));
};

int ws_IsValidDisplay(EGLNativeDisplayType display);
EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display);
__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname);
void ws_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer);
const char *ws_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));

#endif
