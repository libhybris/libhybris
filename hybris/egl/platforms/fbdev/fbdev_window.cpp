#include "fbdev_window.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/matroxfb.h> // for FBIO_WAITFORVSYNC
#include <sys/mman.h> //mmap, munmap
#include <errno.h>
#include <assert.h>

#define TRACE(fmt, ...) 

FbDevNativeWindow::FbDevNativeWindow(gralloc_module_t* gralloc, alloc_device_t* alloc, framebuffer_device_t* fbDev)
{
    m_gralloc = alloc;
    m_fbDev = fbDev;

    for(unsigned int i = 0; i < FRAMEBUFFER_PARTITIONS; i++) {
        m_buffers[i] = new FbDevNativeWindowBuffer(width(), height(), m_fbDev->format, GRALLOC_USAGE_HW_FB);

        int err = m_gralloc->alloc(m_gralloc,
                        width(), height(), m_fbDev->format,
                        GRALLOC_USAGE_HW_FB,
                        &m_buffers[i]->handle,
                        &m_buffers[i]->stride);
        TRACE("buffer %i is at %p (native %p) err=%s handle=%i stride=%i\n", 
                i, m_buffers[i], (ANativeWindowBuffer*) m_buffers[i],
                strerror(-err), m_buffers[i]->handle, m_buffers[i]->stride);


    }
    m_frontbuffer = 0;
    m_tailbuffer = 1;
}

FbDevNativeWindow::~FbDevNativeWindow() {
    TRACE("%s\n",__PRETTY_FUNCTION__);
}

// overloads from BaseNativeWindow
int FbDevNativeWindow::setSwapInterval(int interval) {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

int FbDevNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer){
    TRACE("%s\n",__PRETTY_FUNCTION__);
    *buffer = m_buffers[m_tailbuffer];
    TRACE("dequeueing buffer %i %p",m_tailbuffer, m_buffers[m_tailbuffer]);
    m_tailbuffer++;
    if(m_tailbuffer == FRAMEBUFFER_PARTITIONS)
        m_tailbuffer = 0;
    return NO_ERROR;
}

int FbDevNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer){
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return NO_ERROR;
}

int FbDevNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer){
    FbDevNativeWindowBuffer* buf = static_cast<FbDevNativeWindowBuffer*>(buffer);
    m_frontbuffer++;
    if(m_frontbuffer == FRAMEBUFFER_PARTITIONS)
        m_frontbuffer = 0;
    int res = m_fbDev->post(m_fbDev, buffer->handle);
    TRACE("%s %s\n",__PRETTY_FUNCTION__,strerror(res));
    return NO_ERROR;
}

int FbDevNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer){
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

unsigned int FbDevNativeWindow::width() const {
    unsigned int val = m_fbDev->width;
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, val);
    return val;
}

unsigned int FbDevNativeWindow::height() const {
    unsigned int val = m_fbDev->height;
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, val);
    return val;
}

unsigned int FbDevNativeWindow::format() const {
    unsigned int val = m_fbDev->format;
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, val);
    return val;
}

unsigned int FbDevNativeWindow::defaultWidth() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return m_fbDev->width;
}

unsigned int FbDevNativeWindow::defaultHeight() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return m_fbDev->height;
}

unsigned int FbDevNativeWindow::queueLength() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

unsigned int FbDevNativeWindow::type() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return NATIVE_WINDOW_FRAMEBUFFER;
}

unsigned int FbDevNativeWindow::transformHint() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

int FbDevNativeWindow::setUsage(int usage) {
    TRACE("%s usage %i\n",__PRETTY_FUNCTION__, usage);
    return NO_ERROR;
}

int FbDevNativeWindow::setBuffersFormat(int format) {
    TRACE("%s format %i\n",__PRETTY_FUNCTION__, format);
    return NO_ERROR;
}

int FbDevNativeWindow::setBuffersDimensions(int width, int height) {
    TRACE("%s size %ix%i\n",__PRETTY_FUNCTION__, width, height);
    return NO_ERROR;
}
