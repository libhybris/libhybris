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

#ifndef WINDOWBUFFER_H
#define WINDOWBUFFER_H

#include <cutils/native_handle.h>
#include <string.h>
#include <system/window.h>
#include <hardware/gralloc.h>
#include "nativewindowbase.h"

class RemoteWindowBuffer : public BaseNativeWindowBuffer
{
	public:
		RemoteWindowBuffer(unsigned int width,
				unsigned int height,
				unsigned int stride,
				unsigned int format,
				unsigned int usage,
				buffer_handle_t handle,
				const gralloc_module_t *gralloc	
				) {
			// Base members
			ANativeWindowBuffer::width = width;
			ANativeWindowBuffer::height = height;
			ANativeWindowBuffer::format = format;
			ANativeWindowBuffer::usage = usage;
			ANativeWindowBuffer::stride = stride;
			ANativeWindowBuffer::handle = handle;
			this->m_gralloc = gralloc;
		};
		~RemoteWindowBuffer();
	private:
		const gralloc_module_t *m_gralloc;
};
#endif /* WINDOWBUFFER_H */
