#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/matroxfb.h> // for FBIO_WAITFORVSYNC
#include <sys/mman.h> //mmap, munmap
#include <errno.h>

#include <hardware/gralloc.h>

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

	for(unsigned int i = 0; i < NUM_BUFFERS; i++) {
		m_buffers[i] = 0;
	}
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

int OffscreenNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer)
{
	TRACE("%s ===================================\n",__PRETTY_FUNCTION__);
	if(m_buffers[m_tailbuffer] == 0) {
		m_buffers[m_tailbuffer] = new OffscreenNativeWindowBuffer(width(), height(), m_format, m_usage);
		int usage = m_buffers[m_tailbuffer]->usage;
		usage |= GRALLOC_USAGE_HW_TEXTURE;
		TRACE("alloc usage: ");
		printUsage(usage);
		int err = m_alloc->alloc(m_alloc,
						width(), height(), m_format,
						usage,
						&m_buffers[m_tailbuffer]->handle,
						&m_buffers[m_tailbuffer]->stride);
		TRACE("buffer %i is at %p (native %p) err=%s handle=%i stride=%i\n", 
				m_tailbuffer, m_buffers[m_tailbuffer], (ANativeWindowBuffer*) m_buffers[m_tailbuffer],
				strerror(-err), m_buffers[m_tailbuffer]->handle, m_buffers[m_tailbuffer]->stride);
	}
	*buffer = m_buffers[m_tailbuffer];
	TRACE("dequeued buffer is %i %p\n",m_tailbuffer, m_buffers[m_tailbuffer]);
	m_tailbuffer++;
	if(m_tailbuffer == NUM_BUFFERS)
		m_tailbuffer = 0;
	return NO_ERROR;
}

int OffscreenNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer)
{
	TRACE("%s ===================\n",__PRETTY_FUNCTION__);
/*    OffscreenNativeWindowBuffer *buf = static_cast<OffscreenNativeWindowBuffer*>(buffer);
	int usage = buf->usage | GRALLOC_USAGE_SW_READ_RARELY;
	printf("lock usage: ");
	printUsage(usage);
	int err = m_gralloc->lock(m_gralloc,
			buf->handle, 
			usage,
			0,0, m_width, m_height,
			&buf->vaddr
			);
	TRACE("lock %s vaddr %p\n", strerror(-err), buf->vaddr);
	return err;*/
	return NO_ERROR;
}

int OffscreenNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer)
{
    OffscreenNativeWindowBuffer* buf = static_cast<OffscreenNativeWindowBuffer*>(buffer);

	m_frontbuffer++;
	if (m_frontbuffer == NUM_BUFFERS)
		m_frontbuffer = 0;

    TRACE("queue buffer front now %i", m_frontbuffer);
	postBuffer(buf);

	return NO_ERROR;
}

int OffscreenNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer)
{
	TRACE("%s\n",__PRETTY_FUNCTION__);
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
