#include <ws.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <android/hardware/gralloc.h>
#include <assert.h>

#ifdef WANT_WAYLAND
#include <wayland-client.h>
#include "server_wlegl.h"
#include "server_wlegl_buffer.h"
#endif

static gralloc_module_t *my_gralloc = 0;

void eglplatformcommon_init(gralloc_module_t *gralloc)
{
	my_gralloc = gralloc;
}

#ifdef WANT_WAYLAND

EGLBoolean eglplatformcommon_eglBindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
{
	assert(my_gralloc != NULL);
	server_wlegl_create(display, my_gralloc);
}

EGLBoolean eglplatformcommon_eglUnbindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
{
}
#endif


	void
eglplatformcommon_passthroughImageKHR(EGLenum *target, EGLClientBuffer *buffer)
{
#ifdef WANT_WAYLAND
	if (*target == EGL_WAYLAND_BUFFER_WL)
	{
		server_wlegl_buffer *buf = server_wlegl_buffer_from((struct wl_buffer *)*buffer);
		*buffer = (EGLClientBuffer) (ANativeWindowBuffer *) buf->buf;
		*target = EGL_NATIVE_BUFFER_ANDROID;		
	}	
#endif
}

__eglMustCastToProperFunctionPointerType eglplatformcommon_eglGetProcAddress(const char *procname) 
{
#ifdef WANT_WAYLAND
	if (strcmp(procname, "eglBindWaylandDisplayWL") == 0)
	{
		return eglBindWaylandDisplayWL;	
	}
	else
	if (strcmp(procname, "eglUnbindWaylandDisplayWL") == 0)
	{
		return eglUnbindWaylandDisplayWL;
	}
#endif
	return NULL;
}

const char *eglplatformcommon_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name))
{
#ifdef WANT_WAYLAND
	if (name == EGL_EXTENSIONS)
	{
		const char *ret = (*real_eglQueryString)(dpy, name);
		assert(ret != NULL)
			strcat(ret, "EGL_WL_bind_wayland_display ");
		return ret;
	}
#endif
	return (*real_eglQueryString(dpy, name));
}
