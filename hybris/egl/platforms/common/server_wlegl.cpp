/*
 * Copyright © 2012 Collabora, Ltd.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the copyright holders not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  The copyright holders make
 * no representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cstring>

#include <EGL/egl.h>
#include <EGL/eglext.h>

extern "C" {
#include <cutils/native_handle.h>
#include <hardware/gralloc.h>
}

#include <wayland-server.h>
#include "wayland-android-server-protocol.h"
#include "server_wlegl_private.h"
#include "server_wlegl_handle.h"
#include "server_wlegl_buffer.h"

static inline server_wlegl *
server_wlegl_from(struct wl_resource *resource)
{
	return reinterpret_cast<server_wlegl *>(resource->data);
}

static void
server_wlegl_create_handle(struct wl_client *client,
			   struct wl_resource *resource,
			   uint32_t id,
			   int32_t num_fds,
			   struct wl_array *ints)
{
	server_wlegl *wlegl = server_wlegl_from(resource);
	server_wlegl_handle *handle;

	if (num_fds < 0) {
		wl_resource_post_error(resource,
				       ANDROID_WLEGL_ERROR_BAD_VALUE,
				       "num_fds is negative: %d", num_fds);
		return;
	}

	handle = server_wlegl_handle_create(id);
	wl_array_copy(&handle->ints, ints);
	handle->num_fds = num_fds;
	wl_client_add_resource(client, &handle->resource);
}

static void
server_wlegl_create_buffer(struct wl_client *client,
			   struct wl_resource *resource,
			   uint32_t id,
			   int32_t width,
			   int32_t height,
			   int32_t stride,
			   int32_t format,
			   int32_t usage,
			   struct wl_resource *hnd)
{
	server_wlegl *wlegl = server_wlegl_from(resource);
	server_wlegl_handle *handle = server_wlegl_handle_from(hnd);
	server_wlegl_buffer *buffer;
	buffer_handle_t native;

	if (width < 1 || height < 1) {
		wl_resource_post_error(resource,
				       ANDROID_WLEGL_ERROR_BAD_VALUE,
				       "bad width (%d) or height (%d)",
				       width, height);
		return;
	}

	native = server_wlegl_handle_to_native(handle);
	if (!native) {
		wl_resource_post_error(resource,
				       ANDROID_WLEGL_ERROR_BAD_HANDLE,
				       "fd count mismatch");
		return;
	}

	buffer = server_wlegl_buffer_create(id, width, height, stride,
					    format, usage, native, wlegl);
	if (!buffer) {
		native_handle_close((native_handle_t *)native);
		native_handle_delete((native_handle_t *)native);
		wl_resource_post_error(resource,
				       ANDROID_WLEGL_ERROR_BAD_HANDLE,
				       "invalid native handle");
		return;
	}

	wl_client_add_resource(client, &buffer->base.resource);
}

static const struct android_wlegl_interface server_wlegl_impl = {
	server_wlegl_create_handle,
	server_wlegl_create_buffer,
};

static void
server_wlegl_bind(struct wl_client *client, void *data,
		  uint32_t version, uint32_t id)
{
	server_wlegl *wlegl = reinterpret_cast<server_wlegl *>(data);
	struct wl_resource *resource;

	resource = wl_client_add_object(client, &android_wlegl_interface,
					&server_wlegl_impl, id, wlegl);
}

server_wlegl *
server_wlegl_create(struct wl_display *display, gralloc_module_t *gralloc)
{
	struct server_wlegl *wlegl;
	int ret;

	wlegl = new server_wlegl;

	wlegl->display = display;
	wlegl->global = wl_display_add_global(display,
					      &android_wlegl_interface,
					      wlegl, server_wlegl_bind);
	wlegl->gralloc = (const gralloc_module_t *)gralloc;

	return wlegl;
}

void
server_wlegl_destroy(server_wlegl *wlegl)
{
	/* FIXME: server_wlegl_buffer objects may exist */

	/* no way to release wlegl->gralloc */

	/* FIXME: remove global_ */

	/* Better to leak than expose dtor segfaults, the server
	 * supposedly exits soon after. */
	//LOGW("server_wlegl object leaked on UnbindWaylandDisplayWL");
	/* delete wlegl; */
}

