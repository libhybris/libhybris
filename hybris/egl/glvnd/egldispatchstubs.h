/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

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
