#ifndef __LIBHYBRIS_WS_H
#define __LIBHYBRIS_WS_H
#include <EGL/egl.h>
#include <EGL/eglext.h>

struct ws_egl_interface {
	void * (*android_egl_dlsym)(const char *symbol);

	int (*has_mapping)(EGLSurface surface);
	EGLNativeWindowType (*get_mapping)(EGLSurface surface);
};

struct egl_image
{
    // We experience that some client API's may pass invalid pointers for
    // EGLImages and the blob survives this, so we have this token to try to
    // acheive the same.
    int __type_token;
    EGLImageKHR egl_image;
    EGLClientBuffer egl_buffer;
    EGLenum target;
};

/* Used to allocate egl_image with the __type_token safeguard initialized. */
struct egl_image *egl_image_create();

/* Returns 1 if the input image is NULL or a valid pointer with the
   __internal_id set to the right value */
int egl_image_sanitycheck(struct egl_image *image);

/* Defined in egl.c */
extern struct ws_egl_interface hybris_egl_interface;

struct _EGLDisplay {
	EGLDisplay dpy;
};

struct ws_module {
	void (*init_module)(struct ws_egl_interface *egl_interface);

	struct _EGLDisplay *(*GetDisplay)(EGLNativeDisplayType native);
	void (*Terminate)(struct _EGLDisplay *display);
	EGLNativeWindowType (*CreateWindow)(EGLNativeWindowType win, struct _EGLDisplay *display);
	void (*DestroyWindow)(EGLNativeWindowType win);
	__eglMustCastToProperFunctionPointerType (*eglGetProcAddress)(const char *procname);
	void (*passthroughImageKHR)(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list);
	const char *(*eglQueryString)(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));
	void (*prepareSwap)(EGLDisplay dpy, EGLNativeWindowType win, EGLint *damage_rects, EGLint damage_n_rects);
	void (*finishSwap)(EGLDisplay dpy, EGLNativeWindowType win);
	void (*setSwapInterval)(EGLDisplay dpy, EGLNativeWindowType win, EGLint interval);
};

struct _EGLDisplay *ws_GetDisplay(EGLNativeDisplayType native);
void ws_Terminate(struct _EGLDisplay *dpy);
EGLNativeWindowType ws_CreateWindow(EGLNativeWindowType win, struct _EGLDisplay *display);
void ws_DestroyWindow(EGLNativeWindowType win);
__eglMustCastToProperFunctionPointerType ws_eglGetProcAddress(const char *procname);
void ws_passthroughImageKHR(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list);
const char *ws_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name));
void ws_prepareSwap(EGLDisplay dpy, EGLNativeWindowType win, EGLint *damage_rects, EGLint damage_n_rects);
void ws_finishSwap(EGLDisplay dpy, EGLNativeWindowType win);
void ws_setSwapInterval(EGLDisplay dpy, EGLNativeWindowType win, EGLint interval);

#endif
