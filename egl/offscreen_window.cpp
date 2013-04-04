/*
 * Copyright (c) 2012-2013 Carsten Munk <carsten.munk@gmail.com>
 *               2012-2013 Simon Busch <morphis@gravedo.de>
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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/matroxfb.h> // for FBIO_WAITFORVSYNC
#include <sys/mman.h> //mmap, munmap
#include <errno.h>

#include <android/hardware/gralloc.h>

#include "nativewindowbase.h"
#include "offscreen_window.h"

void printUsage(int usage)
{
	if((usage & GRALLOC_USAGE_SW_READ_NEVER) == GRALLOC_USAGE_SW_READ_NEVER) {
		TRACE("GRALLOC_USAGE_SW_READ_NEVER | ");
	}
	/* buffer is rarely read in software */
	if((usage & GRALLOC_USAGE_SW_READ_RARELY) == GRALLOC_USAGE_SW_READ_RARELY) {
		TRACE("GRALLOC_USAGE_SW_READ_RARELY | ");
	}
	/* buffer is often read in software */
	if((usage & GRALLOC_USAGE_SW_READ_OFTEN) == GRALLOC_USAGE_SW_READ_OFTEN) {
		TRACE("GRALLOC_USAGE_SW_READ_OFTEN | ");
	}
	/* mask for the software read values */
	if((usage & GRALLOC_USAGE_SW_READ_MASK) == GRALLOC_USAGE_SW_READ_MASK) {
		TRACE("GRALLOC_USAGE_SW_READ_MASK | ");
	}
	
	/* buffer is never written in software */
	if((usage & GRALLOC_USAGE_SW_WRITE_NEVER) == GRALLOC_USAGE_SW_WRITE_NEVER) {
		TRACE("GRALLOC_USAGE_SW_WRITE_NEVER | ");
	}
	/* buffer is never written in software */
	if((usage & GRALLOC_USAGE_SW_WRITE_RARELY) == GRALLOC_USAGE_SW_WRITE_RARELY) {
		TRACE("GRALLOC_USAGE_SW_WRITE_RARELY | ");
	}
	/* buffer is never written in software */
	if((usage & GRALLOC_USAGE_SW_WRITE_OFTEN) == GRALLOC_USAGE_SW_WRITE_OFTEN) {
		TRACE("GRALLOC_USAGE_SW_WRITE_OFTEN | ");
	}
	/* mask for the software write values */
	if((usage & GRALLOC_USAGE_SW_WRITE_MASK) == GRALLOC_USAGE_SW_WRITE_MASK) {
		TRACE("GRALLOC_USAGE_SW_WRITE_MASK | ");
	}

	/* buffer will be used as an OpenGL ES texture */
	if((usage & GRALLOC_USAGE_HW_TEXTURE) == GRALLOC_USAGE_HW_TEXTURE) {
		TRACE("GRALLOC_USAGE_HW_TEXTURE | ");
	}
	/* buffer will be used as an OpenGL ES render target */
	if((usage & GRALLOC_USAGE_HW_RENDER) == GRALLOC_USAGE_HW_RENDER) {
		TRACE("GRALLOC_USAGE_HW_RENDER | ");
	}
	/* buffer will be used by the 2D hardware blitter */
	if((usage & GRALLOC_USAGE_HW_2D) == GRALLOC_USAGE_HW_2D) {
		TRACE("GRALLOC_USAGE_HW_2D | ");
	}
	/* buffer will be used by the HWComposer HAL module */
	if((usage & GRALLOC_USAGE_HW_COMPOSER) == GRALLOC_USAGE_HW_COMPOSER) {
		TRACE("GRALLOC_USAGE_HW_COMPOSER | ");
	}
	/* buffer will be used with the framebuffer device */
	if((usage & GRALLOC_USAGE_HW_FB) == GRALLOC_USAGE_HW_FB) {
		TRACE("GRALLOC_USAGE_HW_FB | ");
	}
	/* buffer will be used with the HW video encoder */
	if((usage & GRALLOC_USAGE_HW_VIDEO_ENCODER) == GRALLOC_USAGE_HW_VIDEO_ENCODER) {
		TRACE("GRALLOC_USAGE_HW_VIDEO_ENCODER | ");
	}
	/* mask for the software usage bit-mask */
	if((usage & GRALLOC_USAGE_HW_MASK) == GRALLOC_USAGE_HW_MASK) {
		TRACE("GRALLOC_USAGE_HW_MASK | ");
	}

	/* buffer should be displayed full-screen on an external display when
	 * possible
	 */
	if((usage & GRALLOC_USAGE_EXTERNAL_DISP) == GRALLOC_USAGE_EXTERNAL_DISP) {
		TRACE("GRALLOC_USAGE_EXTERNAL_DISP | ");
	}

	/* Must have a hardware-protected path to external display sink for
	 * this buffer.  If a hardware-protected path is not available, then
	 * either don't composite only this buffer (preferred) to the
	 * external sink, or (less desirable) do not route the entire
	 * composition to the external sink.
	 */
	if((usage & GRALLOC_USAGE_PROTECTED) == GRALLOC_USAGE_PROTECTED) {
		TRACE("GRALLOC_USAGE_PROTECTED | ");
	}

	/* implementation-specific private usage flags */
	if((usage & GRALLOC_USAGE_PRIVATE_0) == GRALLOC_USAGE_PRIVATE_0) {
		TRACE("GRALLOC_USAGE_PRIVATE_0 | ");
	}
	if((usage & GRALLOC_USAGE_PRIVATE_1) == GRALLOC_USAGE_PRIVATE_1) {
		TRACE("GRALLOC_USAGE_PRIVATE_1 | ");
	}
	if((usage & GRALLOC_USAGE_PRIVATE_2) == GRALLOC_USAGE_PRIVATE_2) {
		TRACE("GRALLOC_USAGE_PRIVATE_2 |");
	}
	if((usage & GRALLOC_USAGE_PRIVATE_3) == GRALLOC_USAGE_PRIVATE_3) {
		TRACE("GRALLOC_USAGE_PRIVATE_3 |");
	}
	if((usage & GRALLOC_USAGE_PRIVATE_MASK) == GRALLOC_USAGE_PRIVATE_MASK) {
		TRACE("GRALLOC_USAGE_PRIVATE_MASK |");
	}
	TRACE("\n");
}

OffscreenNativeWindow::OffscreenNativeWindow(unsigned int aWidth, unsigned int aHeight, unsigned int aFormat)
	: m_width(aWidth)
	, m_height(aHeight)
	, m_defaultWidth(aWidth)
	, m_defaultHeight(aHeight)
	, m_format(aFormat)
{
	hw_get_module(GRALLOC_HARDWARE_MODULE_ID, (const hw_module_t**)&m_gralloc);
	m_usage=GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE;
	int err = gralloc_open((hw_module_t*)m_gralloc, &m_alloc);
	TRACE("got alloc %p err:%s\n", m_alloc, strerror(-err));

	for(unsigned int i = 0; i < NUM_BUFFERS; i++)
		m_buffers[i] = 0;

	m_frontbuffer = NUM_BUFFERS-1;
	m_tailbuffer = 0;
}

OffscreenNativeWindow::~OffscreenNativeWindow()
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
}

OffscreenNativeWindowBuffer* OffscreenNativeWindow::getFrontBuffer()
{
	 OffscreenNativeWindowBuffer *buf = m_buffers[m_frontbuffer];
/* int err = m_gralloc->lock(m_gralloc,
			buf->handle, 
			buf->usage,
			0,0, m_width, m_height,
			&buf->vaddr
			);
	TRACE("front buffer lock %s vaddr %p\n", strerror(-err), buf->vaddr);
*/
	return buf;

}

// overloads from BaseNativeWindow
int OffscreenNativeWindow::setSwapInterval(int interval)
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
	return 0;
}

OffscreenNativeWindowBuffer* OffscreenNativeWindow::allocateBuffer()
{
	OffscreenNativeWindowBuffer *buffer = 0;

	buffer = new OffscreenNativeWindowBuffer(width(), height(), m_format, m_usage);

	int usage = buffer->usage;
	usage |= GRALLOC_USAGE_HW_TEXTURE;

	TRACE("alloc usage: ");
	printUsage(usage);

	int err = m_alloc->alloc(m_alloc, width(), height(), m_format,
				usage, &buffer->handle, &buffer->stride);

	return buffer;
}

int OffscreenNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer)
{
	TRACE("%s ===================================\n",__PRETTY_FUNCTION__);

	if(m_buffers[m_tailbuffer] == 0) {
		m_buffers[m_tailbuffer] = allocateBuffer();

		TRACE("buffer %i is at %p (native %p) handle=%i stride=%i\n",
				m_tailbuffer, m_buffers[m_tailbuffer], (ANativeWindowBuffer*) m_buffers[m_tailbuffer],
				m_buffers[m_tailbuffer]->handle, m_buffers[m_tailbuffer]->stride);
	}

	OffscreenNativeWindowBuffer *selectedBuffer = m_buffers[m_tailbuffer];
	if (selectedBuffer->width != m_width || selectedBuffer->height != m_height) {
		TRACE("%s buffer and window size doesn't match: resizing buffer ...\n", __PRETTY_FUNCTION__);
		resizeBuffer(m_tailbuffer, selectedBuffer, m_width, m_height);
	}

	*buffer = selectedBuffer;

	waitForBuffer(selectedBuffer);

	TRACE("dequeued buffer is %i %p\n", m_tailbuffer, selectedBuffer);

	m_tailbuffer++;

	if(m_tailbuffer == NUM_BUFFERS)
		m_tailbuffer = 0;

	return NO_ERROR;
}

int OffscreenNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer)
{
	TRACE("%s ===================\n",__PRETTY_FUNCTION__);

	OffscreenNativeWindowBuffer *buf = static_cast<OffscreenNativeWindowBuffer*>(buffer);
	buf->lock();

	return NO_ERROR;
}

int OffscreenNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer)
{
	OffscreenNativeWindowBuffer* buf = static_cast<OffscreenNativeWindowBuffer*>(buffer);

	buf->unlock();

	m_frontbuffer++;
	if (m_frontbuffer == NUM_BUFFERS)
		m_frontbuffer = 0;

    TRACE("queue buffer front now %i\n", m_frontbuffer);
	postBuffer(buf);

	return NO_ERROR;
}

int OffscreenNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer)
{
	TRACE("%s\n",__PRETTY_FUNCTION__);

	OffscreenNativeWindowBuffer *buf = static_cast<OffscreenNativeWindowBuffer*>(buffer);
	buf->unlock();

	return 0;
}

unsigned int OffscreenNativeWindow::width() const
{
	TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_width);
	return m_width;
}

unsigned int OffscreenNativeWindow::height() const
{
	TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_height);
	return m_height;
}

unsigned int OffscreenNativeWindow::format() const
{
	TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_format);
	return m_format;
}

unsigned int OffscreenNativeWindow::defaultWidth() const
{
	TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_defaultWidth);
	return m_defaultWidth;
}

unsigned int OffscreenNativeWindow::defaultHeight() const
{
	TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_defaultHeight);
	return m_defaultHeight;
}

unsigned int OffscreenNativeWindow::queueLength() const
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
	return 1;
}

unsigned int OffscreenNativeWindow::type() const
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
	return NATIVE_WINDOW_SURFACE_TEXTURE_CLIENT;
}

unsigned int OffscreenNativeWindow::transformHint() const
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
	return 0;
}

int OffscreenNativeWindow::setBuffersFormat(int format)
{
	TRACE("%s format %i\n",__PRETTY_FUNCTION__, format);
	m_format = format;
	return NO_ERROR;
}

int OffscreenNativeWindow::setBuffersDimensions(int width, int height)
{
	TRACE("%s size %ix%i\n",__PRETTY_FUNCTION__, width, height);
	return NO_ERROR;
}

int OffscreenNativeWindow::setUsage(int usage)
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
	printUsage(usage);
	m_usage = usage;
	return NO_ERROR;
}

void OffscreenNativeWindow::resize(unsigned int width, unsigned int height)
{
	TRACE("%s width=%i height=%i\n", __PRETTY_FUNCTION__, width, height);

	m_width = width;
	m_defaultWidth = width;
	m_height = height;
	m_defaultHeight = height;

	for (int n = 0; n < NUM_BUFFERS; n++) {
		OffscreenNativeWindowBuffer *buffer = m_buffers[n];

		if (!buffer) {
			TRACE("%s buffer %i isn't used so we ignore it\n", __PRETTY_FUNCTION__, n);
			continue;
		}

		if (!buffer->locked()) {
			TRACE("%s buffer %i is locked so we ignore it\n", __PRETTY_FUNCTION__, n);
			continue;
		}

		resizeBuffer(n, buffer, width, height);
	}
}

void OffscreenNativeWindow::resizeBuffer(int id, OffscreenNativeWindowBuffer *buffer, unsigned int width,
										 unsigned int height)
{
	if (buffer->handle) {
		TRACE("%s freeing buffer %i ...\n", __PRETTY_FUNCTION__, id);
		m_alloc->free(m_alloc, buffer->handle);
		buffer->handle = 0;
	}

	int usage = buffer->usage;
	usage |= GRALLOC_USAGE_HW_TEXTURE;
	int err = m_alloc->alloc(m_alloc, this->width(), this->height(), m_format,
				usage, &buffer->handle, &buffer->stride);

	buffer->width = width;
	buffer->height = height;
}
