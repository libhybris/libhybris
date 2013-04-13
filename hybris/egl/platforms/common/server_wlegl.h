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

#ifndef SERVER_WLEGL_H
#define SERVER_WLEGL_H

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <hardware/gralloc.h>
#include <system/window.h>
extern "C" {

struct wl_display;
struct wl_buffer;

}

struct server_wlegl;

server_wlegl *
server_wlegl_create(struct wl_display *wldpy, gralloc_module_t *gralloc);

void
server_wlegl_destroy(server_wlegl *wlegl);

EGLImageKHR
egl_create_image_wl(EGLDisplay egldisplay,
		    struct wl_buffer *user_buffer,
		    const EGLint *attrib_list);

#endif /* SERVER_WLEGL_H */
