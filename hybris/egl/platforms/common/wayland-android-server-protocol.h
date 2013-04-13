/* 
 * Copyright © 2012 Collabora, Ltd.
 * 
 * Permission to use, copy, modify, distribute, and sell this
 * software and its documentation for any purpose is hereby granted
 * without fee, provided that the above copyright notice appear in
 * all copies and that both that copyright notice and this permission
 * notice appear in supporting documentation, and that the name of
 * the copyright holders not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 * 
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS
 * SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS, IN NO EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
 * ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 */

#ifndef ANDROID_SERVER_PROTOCOL_H
#define ANDROID_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;
struct wl_resource;

struct android_wlegl;
struct android_wlegl_handle;

extern const struct wl_interface android_wlegl_interface;
extern const struct wl_interface android_wlegl_handle_interface;

#ifndef ANDROID_WLEGL_ERROR_ENUM
#define ANDROID_WLEGL_ERROR_ENUM
enum android_wlegl_error {
	ANDROID_WLEGL_ERROR_BAD_HANDLE = 0,
	ANDROID_WLEGL_ERROR_BAD_VALUE = 1,
};
#endif /* ANDROID_WLEGL_ERROR_ENUM */

/**
 * android_wlegl - Android EGL graphics buffer support
 * @create_handle: Create an Android native_handle_t object
 * @create_buffer: Create a wl_buffer from the native handle
 *
 * Interface used in the Android wrapper libEGL to share graphics buffers
 * between the server and the client.
 */
struct android_wlegl_interface {
	/**
	 * create_handle - Create an Android native_handle_t object
	 * @id: (none)
	 * @num_fds: (none)
	 * @ints: an array of int32_t
	 *
	 * This creator method initialises the native_handle_t object
	 * with everything except the file descriptors, which have to be
	 * submitted separately.
	 */
	void (*create_handle)(struct wl_client *client,
			      struct wl_resource *resource,
			      uint32_t id,
			      int32_t num_fds,
			      struct wl_array *ints);
	/**
	 * create_buffer - Create a wl_buffer from the native handle
	 * @id: (none)
	 * @width: (none)
	 * @height: (none)
	 * @stride: (none)
	 * @format: (none)
	 * @usage: (none)
	 * @native_handle: (none)
	 *
	 * Pass the Android native_handle_t to the server and attach it
	 * to the new wl_buffer object.
	 *
	 * The android_wlegl_handle object must be destroyed immediately
	 * after this request.
	 */
	void (*create_buffer)(struct wl_client *client,
			      struct wl_resource *resource,
			      uint32_t id,
			      int32_t width,
			      int32_t height,
			      int32_t stride,
			      int32_t format,
			      int32_t usage,
			      struct wl_resource *native_handle);
};

#ifndef ANDROID_WLEGL_HANDLE_ERROR_ENUM
#define ANDROID_WLEGL_HANDLE_ERROR_ENUM
enum android_wlegl_handle_error {
	ANDROID_WLEGL_HANDLE_ERROR_TOO_MANY_FDS = 0,
};
#endif /* ANDROID_WLEGL_HANDLE_ERROR_ENUM */

/**
 * android_wlegl_handle - An Android native_handle_t object
 * @add_fd: (none)
 * @destroy: (none)
 *
 * The Android native_handle_t is a semi-opaque object, that contains an
 * EGL implementation specific number of int32 values and file descriptors.
 *
 * We cannot send a variable size array of file descriptors over the
 * Wayland protocol, so we send them one by one.
 */
struct android_wlegl_handle_interface {
	/**
	 * add_fd - (none)
	 * @fd: (none)
	 */
	void (*add_fd)(struct wl_client *client,
		       struct wl_resource *resource,
		       int32_t fd);
	/**
	 * destroy - (none)
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
};

#ifdef  __cplusplus
}
#endif

#endif
