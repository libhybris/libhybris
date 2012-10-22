#include "fbdev_window.h"
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/matroxfb.h> // for FBIO_WAITFORVSYNC
#include <sys/mman.h> //mmap, munmap
#include <errno.h>


FbDevNativeWindow::FbDevNativeWindow()
{
    hw_module_t const* pmodule = NULL;
	hw_get_module(GRALLOC_HARDWARE_MODULE_ID, &pmodule);
    int err = framebuffer_open(pmodule, &m_fbDev);
    printf("open framebuffer HAL (%s) format %i", strerror(-err), m_fbDev->format);

    err = gralloc_open(pmodule, &m_gralloc);
    printf("got gralloc %p err:%s\n", m_gralloc, strerror(-err));

    for(unsigned int i = 0; i < FRAMEBUFFER_PARTITIONS; i++) {
        m_buffers[i] = new FbDevNativeWindowBuffer(width(), height(), m_fbDev->format, GRALLOC_USAGE_HW_FB);

        int err = m_gralloc->alloc(m_gralloc,
                        width(), height(), m_fbDev->format,
                        GRALLOC_USAGE_HW_FB,
                        &m_buffers[i]->handle,
                        &m_buffers[i]->stride);
        printf("buffer %i is at %p (native %p) err=%s handle=%i stride=%i\n", 
                i, m_buffers[i], (ANativeWindowBuffer*) m_buffers[i],
                strerror(-err), m_buffers[i]->handle, m_buffers[i]->stride);


    }
    m_frontbuffer = 0;
    m_tailbuffer = 1;


}

FbDevNativeWindow::~FbDevNativeWindow() {
    printf("%s\n",__PRETTY_FUNCTION__);
}

// overloads from BaseNativeWindow
int FbDevNativeWindow::setSwapInterval(int interval) {
    printf("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

int FbDevNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer){
    printf("%s\n",__PRETTY_FUNCTION__);
    *buffer = m_buffers[m_tailbuffer];
    printf("dequeueing buffer %i %p",m_tailbuffer, m_buffers[m_tailbuffer]);
    m_tailbuffer++;
    if(m_tailbuffer == FRAMEBUFFER_PARTITIONS)
        m_tailbuffer = 0;
    return NO_ERROR;
}

int FbDevNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer){
    printf("%s\n",__PRETTY_FUNCTION__);
    return NO_ERROR;
}

int FbDevNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer){
    FbDevNativeWindowBuffer* buf = static_cast<FbDevNativeWindowBuffer*>(buffer);
    m_frontbuffer++;
    if(m_frontbuffer == FRAMEBUFFER_PARTITIONS)
        m_frontbuffer = 0;
    int res = m_fbDev->post(m_fbDev, buffer->handle);
    printf("%s %s\n",__PRETTY_FUNCTION__,strerror(res));
    return NO_ERROR;
}

int FbDevNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer){
    printf("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

unsigned int FbDevNativeWindow::width() const {
    unsigned int val = m_fbDev->width;
    printf("%s value: %i\n",__PRETTY_FUNCTION__, val);
    return val;
}

unsigned int FbDevNativeWindow::height() const {
    unsigned int val = m_fbDev->height;
    printf("%s value: %i\n",__PRETTY_FUNCTION__, val);
    return val;
}

unsigned int FbDevNativeWindow::format() const {
    unsigned int val = m_fbDev->format;
    printf("%s value: %i\n",__PRETTY_FUNCTION__, val);
    return val;
}

unsigned int FbDevNativeWindow::defaultWidth() const {
    printf("%s\n",__PRETTY_FUNCTION__);
    return m_fbDev->width;
}

unsigned int FbDevNativeWindow::defaultHeight() const {
    printf("%s\n",__PRETTY_FUNCTION__);
    return m_fbDev->height;
}

unsigned int FbDevNativeWindow::queueLength() const {
    printf("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

unsigned int FbDevNativeWindow::type() const {
    printf("%s\n",__PRETTY_FUNCTION__);
    return NATIVE_WINDOW_FRAMEBUFFER;
}

unsigned int FbDevNativeWindow::transformHint() const {
    printf("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

int FbDevNativeWindow::setUsage(int usage) {
    printf("%s usage %i\n",__PRETTY_FUNCTION__, usage);
    return NO_ERROR;
}

int FbDevNativeWindow::setBuffersFormat(int format) {
    printf("%s format %i\n",__PRETTY_FUNCTION__, format);
    return NO_ERROR;
}

int FbDevNativeWindow::setBuffersDimensions(int width, int height) {
    printf("%s size %ix%i\n",__PRETTY_FUNCTION__, width, height);
    return NO_ERROR;
}
