#include <ws.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <android/hardware/gralloc.h>
#include <stdio.h>
#include <assert.h>
#include "config.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "logging.h"

#ifdef WANT_WAYLAND
#include <wayland-client.h>
#include "server_wlegl.h"
#include "server_wlegl_buffer.h"
#endif

#include "windowbuffer.h"

static gralloc_module_t *my_gralloc = 0;
static alloc_device_t *my_alloc = 0;

extern "C" void eglplatformcommon_init(gralloc_module_t *gralloc, alloc_device_t *allocdevice)
{
	my_gralloc = gralloc;
	my_alloc = allocdevice;
}

extern "C" int hybris_register_buffer_handle(buffer_handle_t handle)
{
	if (!my_gralloc)
		return -1;

	return my_gralloc->registerBuffer(my_gralloc, handle);
}

extern "C" int hybris_unregister_buffer_handle(buffer_handle_t handle)
{
	if (!my_gralloc)
		return -1;

	return my_gralloc->unregisterBuffer(my_gralloc, handle);
}

extern "C" void hybris_dump_buffer_to_file(ANativeWindowBuffer *buf)
{
	static int cnt = 0;
	void *vaddr;
	int ret = my_gralloc->lock(my_gralloc, buf->handle, buf->usage, 0, 0, buf->width, buf->height, &vaddr);
	TRACE("buf:%p gralloc lock returns %i", buf, ret);
	TRACE("buf:%p lock to vaddr %p", buf, vaddr);
	char b[1024];
	int bytes_pp = 0;

	if (buf->format == HAL_PIXEL_FORMAT_RGBA_8888)
		bytes_pp = 4;
	else if (buf->format == HAL_PIXEL_FORMAT_RGB_565)
		bytes_pp = 2;

	snprintf(b, 1020, "vaddr.%p.%p.%i.%is%ix%ix%i", buf, vaddr, cnt, buf->width, buf->stride, buf->height, bytes_pp);
	cnt++;
	int fd = ::open(b, O_WRONLY|O_CREAT, S_IRWXU);

	::write(fd, vaddr, buf->stride * buf->height * bytes_pp);
	::close(fd);
	my_gralloc->unlock(my_gralloc, buf->handle);
}

#ifdef WANT_WAYLAND

extern "C" EGLBoolean eglplatformcommon_eglBindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
{
	assert(my_gralloc != NULL);
	server_wlegl_create(display, my_gralloc);
	return EGL_TRUE;
}

extern "C" EGLBoolean eglplatformcommon_eglUnbindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
{
	return EGL_TRUE;
}

extern "C" EGLBoolean eglplatformcommon_eglQueryWaylandBufferWL(EGLDisplay dpy,
	struct wl_buffer *buffer, EGLint attribute, EGLint *value)
{
	server_wlegl_buffer *buf  = server_wlegl_buffer_from(buffer);
	ANativeWindowBuffer* anwb = (ANativeWindowBuffer *) buf->buf;

	if (attribute == EGL_TEXTURE_FORMAT) {
		switch(anwb->format) {
		case HAL_PIXEL_FORMAT_RGB_565:
			*value = EGL_TEXTURE_RGB;
			break;
		case HAL_PIXEL_FORMAT_RGBA_8888:
			*value = EGL_TEXTURE_RGBA;
			break;
		default:
			*value =  EGL_TEXTURE_EXTERNAL_WL;
		}
		return EGL_TRUE;
	}
	if (attribute == EGL_WIDTH) {
		*value = anwb->width;
		return EGL_TRUE;
	}
	if (attribute == EGL_HEIGHT) {
		*value = anwb->height;
		return EGL_TRUE;
	}
	return EGL_FALSE ;
}

#endif

extern "C" EGLBoolean eglplatformcommon_eglHybrisCreateNativeBuffer(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint *stride, EGLClientBuffer *buffer)
{
	int ret;
	buffer_handle_t _handle;
	int _stride;

	assert(my_gralloc != NULL);
	assert(my_alloc != NULL);

	ret = my_alloc->alloc(my_alloc, width, height, format, usage, &_handle, &_stride);

	if (ret == 0)
	{
		RemoteWindowBuffer *buf = new RemoteWindowBuffer(width, height, _stride, format, usage, _handle, my_gralloc);
		buf->common.incRef(&buf->common);
		*buffer = (EGLClientBuffer) static_cast<ANativeWindowBuffer *>(buf);
		*stride = _stride;
		return EGL_TRUE;
	}
	else
		return EGL_FALSE;
}

extern "C" EGLBoolean eglplatformcommon_eglHybrisLockNativeBuffer(EGLClientBuffer buffer, EGLint usage, EGLint l, EGLint t, EGLint w, EGLint h, void **vaddr)
{
	int ret;
	RemoteWindowBuffer *buf = static_cast<RemoteWindowBuffer *>((ANativeWindowBuffer *) buffer);

	assert(my_gralloc != NULL);

	ret = my_gralloc->lock(my_gralloc, buf->handle, usage, l, t, w, h, vaddr);
	if (ret == 0)
		return EGL_TRUE;
	else
		return EGL_FALSE;
}

extern "C" EGLBoolean eglplatformcommon_eglHybrisUnlockNativeBuffer(EGLClientBuffer buffer)
{
	int ret;
	RemoteWindowBuffer *buf = static_cast<RemoteWindowBuffer *>((ANativeWindowBuffer *) buffer);

	assert(my_gralloc != NULL);

	ret = my_gralloc->unlock(my_gralloc, buf->handle);
	if (ret == 0)
		return EGL_TRUE;
	else
		return EGL_FALSE;
}


extern "C" EGLBoolean eglplatformcommon_eglHybrisReleaseNativeBuffer(EGLClientBuffer buffer)
{
	RemoteWindowBuffer *buf = static_cast<RemoteWindowBuffer *>((ANativeWindowBuffer *) buffer);

	buf->common.decRef(&buf->common);
	return EGL_TRUE;
}



extern "C" void
eglplatformcommon_passthroughImageKHR(EGLContext *ctx, EGLenum *target, EGLClientBuffer *buffer, const EGLint **attrib_list)
{
#ifdef WANT_WAYLAND
	static int debugenvchecked = 0;
	if (*target == EGL_WAYLAND_BUFFER_WL)
	{
		server_wlegl_buffer *buf = server_wlegl_buffer_from((struct wl_buffer *)*buffer);
		HYBRIS_TRACE_BEGIN("eglplatformcommon", "Wayland_eglImageKHR", "-resource@%i", ((struct wl_buffer *)*buffer)->resource.object.id);
		HYBRIS_TRACE_END("eglplatformcommon", "Wayland_eglImageKHR", "-resource@%i", ((struct wl_buffer *)*buffer)->resource.object.id);
		if (debugenvchecked == 0)
		{
			if (getenv("HYBRIS_WAYLAND_KHR_DUMP_BUFFERS") != NULL)
				debugenvchecked = 2;
			else
				debugenvchecked = 1;
		} else if (debugenvchecked == 2)
		{
			hybris_dump_buffer_to_file((ANativeWindowBuffer *) buf->buf);
		}
		*buffer = (EGLClientBuffer) (ANativeWindowBuffer *) buf->buf;
		*target = EGL_NATIVE_BUFFER_ANDROID;
		*ctx = EGL_NO_CONTEXT;
		*attrib_list = NULL;
	}
#endif
}

extern "C" __eglMustCastToProperFunctionPointerType eglplatformcommon_eglGetProcAddress(const char *procname)
{
#ifdef WANT_WAYLAND
	if (strcmp(procname, "eglBindWaylandDisplayWL") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglBindWaylandDisplayWL;
	}
	else
	if (strcmp(procname, "eglUnbindWaylandDisplayWL") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglUnbindWaylandDisplayWL;
	}else
	if (strcmp(procname, "eglQueryWaylandBufferWL") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglQueryWaylandBufferWL;
	}
	else
#endif
	if (strcmp(procname, "eglHybrisCreateNativeBuffer") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisCreateNativeBuffer;
	}
	else
	if (strcmp(procname, "eglHybrisLockNativeBuffer") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisLockNativeBuffer;
	}
	else
	if (strcmp(procname, "eglHybrisUnlockNativeBuffer") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisUnlockNativeBuffer;
	}
	else
	if (strcmp(procname, "eglHybrisReleaseNativeBuffer") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisReleaseNativeBuffer;
	}
	return NULL;
}

extern "C" const char *eglplatformcommon_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name))
{
#ifdef WANT_WAYLAND
	if (name == EGL_EXTENSIONS)
	{
		const char *ret = (*real_eglQueryString)(dpy, name);
		static char eglextensionsbuf[512];
		assert(ret != NULL);
		snprintf(eglextensionsbuf, 510, "%sEGL_HYBRIS_native_buffer %s", ret,
#ifdef WANT_WAYLAND
			"EGL_WL_bind_wayland_display "
#else
			""
#endif
		);
		ret = eglextensionsbuf;
		return ret;
	}
#endif
	return (*real_eglQueryString)(dpy, name);
}
