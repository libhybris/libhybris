#ifndef __LIBHYBRIS_WS_H
#define __LIBHYBRIS_WS_H
#include <EGL/egl.h>

struct ws_module {
	int (*IsValidDisplay)(EGLNativeDisplayType display_id);
	EGLNativeWindowType (*CreateWindow)(EGLNativeWindowType win, EGLNativeDisplayType display);
	void (*DestroyWindow)(EGLNativeWindowType win);
	__eglMustCastToProperFunctionPointerType (*eglGetProcAddress)(const char *procname);
	void (*passthroughImageKHR)(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list);
	const char *(*eglQueryString)(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));
};

int ws_IsValidDisplay(EGLNativeDisplayType display);
EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display);
void ws_DestroyWindow(EGLNativeWindowType win);
__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname);
void ws_passthroughImageKHR(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list);
const char *ws_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));

#endif
