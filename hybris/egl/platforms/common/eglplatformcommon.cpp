/*
 * Copyright (c) 2013-2022 Jolla Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <android-config.h>
#include <ws.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
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

#include "platformcommon.h"
#include "windowbuffer.h"

#include <hybris/gralloc/gralloc.h>

static struct ws_egl_interface *my_egl_interface;

extern "C" void eglplatformcommon_init(struct ws_egl_interface *egl_iface)
{
	my_egl_interface = egl_iface;
}

extern "C" void *hybris_android_egl_dlsym(const char *symbol)
{
	return (*my_egl_interface->android_egl_dlsym)(symbol);
}

extern "C" int hybris_egl_has_mapping(EGLSurface surface)
{
	return (*my_egl_interface->has_mapping)(surface);
}

EGLNativeWindowType hybris_egl_get_mapping(EGLSurface surface)
{
	return (*my_egl_interface->get_mapping)(surface);
}

#ifdef WANT_WAYLAND

extern "C" EGLBoolean eglplatformcommon_eglBindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
{
	server_wlegl_create(display);
	return EGL_TRUE;
}

extern "C" EGLBoolean eglplatformcommon_eglUnbindWaylandDisplayWL(EGLDisplay dpy, struct wl_display *display)
{
	return EGL_TRUE;
}

extern "C" EGLBoolean eglplatformcommon_eglQueryWaylandBufferWL(EGLDisplay dpy,
	struct wl_resource *buffer, EGLint attribute, EGLint *value)
{
	server_wlegl_buffer *buf  = server_wlegl_buffer_from(buffer);
	if (!buf || !buf->buf)
		return EGL_FALSE;

	ANativeWindowBuffer* anwb = (ANativeWindowBuffer *) buf->buf;

	if (attribute == EGL_TEXTURE_FORMAT) {
		switch(anwb->format) {
		case HAL_PIXEL_FORMAT_RGB_565:
			*value = EGL_TEXTURE_RGB;
			break;
		case HAL_PIXEL_FORMAT_RGBA_8888:
		case HAL_PIXEL_FORMAT_BGRA_8888:
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

// Added as part of EGL_HYBRIS_WL_acquire_native_buffer. Buffers are released
// and decRef'ed using eglHybrisReleaseNativeBuffer
extern "C" EGLBoolean eglplatformcommon_eglHybrisAcquireNativeBufferWL(EGLDisplay dpy, struct wl_resource *wlBuffer, EGLClientBuffer *buffer)
 {
     if (!buffer)
         return EGL_FALSE;
    server_wlegl_buffer *buf  = server_wlegl_buffer_from(wlBuffer);
    if (!buf || !buf->buf || !buf->buf->isAllocated()) {
        // We only return the handles from buffers which are allocated server side. This is because some
        // hardware compositors have problems with client-side allocated buffers.
        *buffer = 0;
        return EGL_FALSE;
    }
    ANativeWindowBuffer* anwb = (ANativeWindowBuffer *) buf->buf;
    anwb->common.incRef(&anwb->common);
    *buffer = (EGLClientBuffer *) anwb;
    return EGL_TRUE;
}

#endif

// Added as part of EGL_HYBRIS_native_buffer2
extern "C" EGLBoolean eglplatformcommon_eglHybrisNativeBufferHandle(EGLDisplay dpy, EGLClientBuffer buffer, void **handle)
{
	if (!buffer || !handle)
		return EGL_FALSE;
	ANativeWindowBuffer *anwb = (ANativeWindowBuffer *) buffer;
	*handle = (void *) anwb->handle;
	return EGL_TRUE;
}

extern "C" void eglplatformcommon_eglHybrisGetNativeBufferInfo(EGLClientBuffer buffer, int *num_ints, int *num_fds)
{
	RemoteWindowBuffer *buf = static_cast<RemoteWindowBuffer *>((ANativeWindowBuffer *) buffer);
	*num_ints = buf->handle->numInts;
	*num_fds = buf->handle->numFds;
}

extern "C" void eglplatformcommon_eglHybrisSerializeNativeBuffer(EGLClientBuffer buffer, int *ints, int *fds)
{
	RemoteWindowBuffer *buf = static_cast<RemoteWindowBuffer *>((ANativeWindowBuffer *) buffer);
	memcpy(ints, buf->handle->data + buf->handle->numFds, buf->handle->numInts * sizeof(int));
	memcpy(fds, buf->handle->data, buf->handle->numFds * sizeof(int));
}

extern "C" EGLBoolean eglplatformcommon_eglHybrisCreateRemoteBuffer(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint stride,
                                                                    int num_ints, int *ints, int num_fds, int *fds, EGLClientBuffer *buffer)
{
	native_handle_t *native = native_handle_create(num_fds, num_ints);
	memcpy(&native->data[0], fds, num_fds * sizeof(int));
	memcpy(&native->data[num_fds], ints, num_ints * sizeof(int));

	const native_handle_t* out_handle = NULL;
	int ret = hybris_gralloc_import_buffer(native, &out_handle);
	native_handle_close(native);
	native_handle_delete(native);

	if (ret == 0)
	{
		RemoteWindowBuffer *buf = new RemoteWindowBuffer(width, height, stride, format, usage, (buffer_handle_t)out_handle);
		buf->common.incRef(&buf->common);
		*buffer = (EGLClientBuffer) static_cast<ANativeWindowBuffer *>(buf);
		return EGL_TRUE;
	}
	else
		return EGL_FALSE;
}

extern "C" EGLBoolean eglplatformcommon_eglHybrisCreateNativeBuffer(EGLint width, EGLint height, EGLint usage, EGLint format, EGLint *stride, EGLClientBuffer *buffer)
{
	int ret;
	buffer_handle_t _handle;
	int _stride;

	ret = hybris_gralloc_allocate(width, height, format, usage, &_handle, (uint32_t*)&_stride);

	if (ret == 0)
	{
		RemoteWindowBuffer *buf = new RemoteWindowBuffer(width, height, _stride, format, usage, _handle);
		buf->common.incRef(&buf->common);
		buf->setAllocated(true);
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

	ret = hybris_gralloc_lock(buf->handle, usage, l, t, w, h, vaddr);
	if (ret == 0)
		return EGL_TRUE;
	else
		return EGL_FALSE;
}

extern "C" EGLBoolean eglplatformcommon_eglHybrisUnlockNativeBuffer(EGLClientBuffer buffer)
{
	int ret;
	RemoteWindowBuffer *buf = static_cast<RemoteWindowBuffer *>((ANativeWindowBuffer *) buffer);

	ret = hybris_gralloc_unlock(buf->handle);
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
		server_wlegl_buffer *buf = server_wlegl_buffer_from((struct wl_resource *)*buffer);
		if (!buf || !buf->buf)
			return;

		HYBRIS_TRACE_BEGIN("eglplatformcommon", "Wayland_eglImageKHR", "-resource@%i", wl_resource_get_id((struct wl_resource *)*buffer));
		HYBRIS_TRACE_END("eglplatformcommon", "Wayland_eglImageKHR", "-resource@%i", wl_resource_get_id((struct wl_resource *)*buffer));
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
	if (strcmp(procname, "eglHybrisAcquireNativeBufferWL") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType) eglplatformcommon_eglHybrisAcquireNativeBufferWL;
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
	else
	if (strcmp(procname, "eglHybrisGetNativeBufferInfo") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisGetNativeBufferInfo;
	}
	else
	if (strcmp(procname, "eglHybrisSerializeNativeBuffer") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisSerializeNativeBuffer;
	}
	else
	if (strcmp(procname, "eglHybrisCreateRemoteBuffer") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisCreateRemoteBuffer;
	}
	else
	if (strcmp(procname, "eglHybrisNativeBufferHandle") == 0)
	{
		return (__eglMustCastToProperFunctionPointerType)eglplatformcommon_eglHybrisNativeBufferHandle;
	}
	return NULL;
}

extern "C" const char *eglplatformcommon_eglQueryString(EGLDisplay dpy, EGLint name, const char *(*real_eglQueryString)(EGLDisplay dpy, EGLint name))
{
	const char *ret = (*real_eglQueryString)(dpy, name);
#ifdef WANT_WAYLAND
	if (ret && name == EGL_EXTENSIONS)
	{
		static char eglextensionsbuf[2048];
		snprintf(eglextensionsbuf, 2046, "%s %s", ret,
			"EGL_HYBRIS_native_buffer2 EGL_HYBRIS_WL_acquire_native_buffer EGL_WL_bind_wayland_display"
		);
		ret = eglextensionsbuf;
	}
#endif
	return ret;
}
