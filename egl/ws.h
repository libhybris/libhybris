#ifndef __LIBHYBRIS_WS_H
#define __LIBHYBRIS_WS_H
#include <EGL/egl.h>

int ws_IsValidDisplay(EGLNativeDisplayType display);
EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, EGLNativeDisplayType display);
__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname);
void ws_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer);
#endif
