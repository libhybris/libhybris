#ifndef __EGLPLATFORMCOMMON_H
#define __EGLPLATFORMCOMMON_H
#include <string.h>
#include <android/hardware/gralloc.h>
#include <EGL/egl.h>

void eglplatformcommon_init(gralloc_module_t *gralloc);
__eglMustCastToProperFunctionPointerType eglplatformcommon_eglGetProcAddress(const char *procname);
void eglplatformcommon_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer);
const char *eglplatformcommon_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));
#endif
