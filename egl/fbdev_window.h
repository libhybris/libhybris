#ifndef FBDEV_WINDOW_H
#define FBDEV_WINDOW_H
#include "nativewindowbase.h"
#include <linux/fb.h>
#include <android/hardware/gralloc.h>
#define FRAMEBUFFER_PARTITIONS 2

class FbDevNativeWindowBuffer : public BaseNativeWindowBuffer
{
    friend class FbDevNativeWindow;
    protected:
    FbDevNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage) {
        // Base members
        ANativeWindowBuffer::width = width;
        ANativeWindowBuffer::height = height;
        ANativeWindowBuffer::format = format;
        ANativeWindowBuffer::usage = usage;
    };
};

class FbDevNativeWindow : public BaseNativeWindow
{
public:
    FbDevNativeWindow(gralloc_module_t* gralloc, alloc_device_t* alloc, framebuffer_device_t* fbDev);
    ~FbDevNativeWindow();
protected:
    // overloads from BaseNativeWindow
    virtual int setSwapInterval(int interval);
    virtual int dequeueBuffer(BaseNativeWindowBuffer **buffer);
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer);
    virtual int queueBuffer(BaseNativeWindowBuffer* buffer);
    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer);
    virtual unsigned int type() const;
    virtual unsigned int width() const;
    virtual unsigned int height() const;
    virtual unsigned int format() const;
    virtual unsigned int defaultWidth() const;
    virtual unsigned int defaultHeight() const;
    virtual unsigned int queueLength() const;
    virtual unsigned int transformHint() const;
    // perform calls
    virtual int setUsage(int usage);
    virtual int setBuffersFormat(int format);
    virtual int setBuffersDimensions(int width, int height);
private:
    unsigned int m_frontbuffer;
    unsigned int m_tailbuffer;
    FbDevNativeWindowBuffer* m_buffers[FRAMEBUFFER_PARTITIONS];
    alloc_device_t* m_gralloc;
    framebuffer_device_t* m_fbDev;
};

#endif
