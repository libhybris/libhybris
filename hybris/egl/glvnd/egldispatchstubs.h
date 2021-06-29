#ifndef EGLDISPATCHSTUBS_H
#define EGLDISPATCHSTUBS_H

#include "glvnd/libeglabi.h"

#if defined(__cplusplus)
extern "C" {
#endif

// These variables are all generated along with the dispatch stubs.
extern const int __EGL_DISPATCH_FUNC_COUNT;
extern const char * const __EGL_DISPATCH_FUNC_NAMES[];
extern int __EGL_DISPATCH_FUNC_INDICES[];
extern const __eglMustCastToProperFunctionPointerType __EGL_DISPATCH_FUNCS[];

void __eglInitDispatchStubs(const __EGLapiExports *exportsTable);
void __eglSetDispatchIndex(const char *name, int index);

/**
 * Returns the dispatch function for the given name, or \c NULL if the function
 * isn't supported.
 */
void *__eglDispatchFindDispatchFunction(const char *name);

// Helper functions used by the generated stubs.
__eglMustCastToProperFunctionPointerType __eglDispatchFetchByDisplay(EGLDisplay dpy, int index);
__eglMustCastToProperFunctionPointerType __eglDispatchFetchByDevice(EGLDeviceEXT dpy, int index);
__eglMustCastToProperFunctionPointerType __eglDispatchFetchByCurrent(int index);

#if defined(__cplusplus)
}
#endif

#endif // EGLDISPATCHSTUBS_H
