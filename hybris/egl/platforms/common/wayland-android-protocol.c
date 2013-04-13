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

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface android_wlegl_handle_interface;
extern const struct wl_interface wl_buffer_interface;
extern const struct wl_interface android_wlegl_handle_interface;

static const struct wl_interface *types[] = {
	NULL,
	&android_wlegl_handle_interface,
	NULL,
	NULL,
	&wl_buffer_interface,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	&android_wlegl_handle_interface,
};

static const struct wl_message android_wlegl_requests[] = {
	{ "create_handle", "nia", types + 1 },
	{ "create_buffer", "niiiiio", types + 4 },
};

WL_EXPORT const struct wl_interface android_wlegl_interface = {
	"android_wlegl", 1,
	2, android_wlegl_requests,
	0, NULL,
};

static const struct wl_message android_wlegl_handle_requests[] = {
	{ "add_fd", "h", types + 0 },
	{ "destroy", "", types + 0 },
};

WL_EXPORT const struct wl_interface android_wlegl_handle_interface = {
	"android_wlegl_handle", 1,
	2, android_wlegl_handle_requests,
	0, NULL,
};

