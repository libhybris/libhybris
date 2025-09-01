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

#include <android-config.h>
#include <stdint.h>
#include <cstring>
#include <unistd.h>

#include "server_wlegl_handle.h"
#include "wayland-android-server-protocol.h"

static void
add_fd(struct wl_client *client, struct wl_resource *resource, int32_t fd)
{
	server_wlegl_handle *handle = server_wlegl_handle_from(resource);

	if (handle->fds.size >= handle->num_fds * sizeof(int)) {
		close(fd);
		wl_resource_post_error(handle->resource,
				       ANDROID_WLEGL_HANDLE_ERROR_TOO_MANY_FDS,
				       "too many file descriptors");
		return;
	}

	int *ptr = (int *)wl_array_add(&handle->fds, sizeof(int));
	*ptr = fd;
}

static void
destroy(struct wl_client *client, struct wl_resource *resource)
{
	wl_resource_destroy(resource);
}

static const struct android_wlegl_handle_interface server_handle_impl = {
	add_fd,
	destroy
};

static void
server_wlegl_handle_dtor(struct wl_resource *resource)
{
	server_wlegl_handle *handle = server_wlegl_handle_from(resource);

	int *fd = (int *)handle->fds.data;
	int *end = (int *)((char *)handle->fds.data + handle->fds.size);

	for (; fd < end; ++fd)
		close(*fd);

	wl_array_release(&handle->fds);
	wl_array_release(&handle->ints);

	delete handle;
}

server_wlegl_handle *
server_wlegl_handle_from(struct wl_resource *resource)
{
	if (!resource || !wl_resource_instance_of(resource, &android_wlegl_handle_interface, &server_handle_impl))
		return NULL;
	return static_cast<server_wlegl_handle *>(wl_resource_get_user_data(resource));
}

server_wlegl_handle *
server_wlegl_handle_create(wl_client *client, uint32_t id, int32_t num_fds, wl_array *ints)
{
	server_wlegl_handle *handle = new server_wlegl_handle;

	memset(handle, 0, sizeof(*handle));

	handle->resource = wl_resource_create(client, &android_wlegl_handle_interface, 1, id);
	wl_resource_set_implementation(handle->resource, &server_handle_impl, handle, server_wlegl_handle_dtor);

	wl_array_init(&handle->ints);
	wl_array_init(&handle->fds);

	wl_array_copy(&handle->ints, ints);
	handle->num_fds = num_fds;

	return handle;
}

buffer_handle_t
server_wlegl_handle_to_native(server_wlegl_handle *handle)
{
	if (!handle)
		return NULL;

	native_handle_t *native;
	int numFds = handle->fds.size / sizeof(int);
	int numInts = handle->ints.size / sizeof(int32_t);

	if (numFds != handle->num_fds)
		return NULL;

	native = native_handle_create(numFds, numInts);
	
	memcpy(&native->data[0], handle->fds.data, handle->fds.size);
	memcpy(&native->data[numFds], handle->ints.data, handle->ints.size);
	/* ownership of fds passed to native_handle_t */
	handle->fds.size = 0;

	return (buffer_handle_t) native;
}
