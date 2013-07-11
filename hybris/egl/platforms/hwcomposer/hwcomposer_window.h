/*
 * Copyright (C) 2013 libhybris
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FBDEV_WINDOW_H
#define FBDEV_WINDOW_H

#include "nativewindowbase.h"
#include <linux/fb.h>
#include <android/hardware/gralloc.h>

#include <list>


class HWComposerNativeWindowBuffer : public BaseNativeWindowBuffer {
friend class HWComposerNativeWindow;

protected:
    HWComposerNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage) ;
   virtual ~HWComposerNativeWindowBuffer() ;

protected:
    int busy;
    int fenceFd;
};


class HWComposerNativeWindow : public BaseNativeWindow {
public:
    HWComposerNativeWindow(unsigned int width, unsigned int height, unsigned int format);
    ~HWComposerNativeWindow();
    void setup(gralloc_module_t* gralloc, alloc_device_t* alloc);


    void lockFrontBuffer(HWComposerNativeWindowBuffer **buffer);
    void unlockFrontBuffer(HWComposerNativeWindowBuffer *buffer);

protected:
    // overloads from BaseNativeWindow
    virtual int setSwapInterval(int interval);

    virtual int dequeueBuffer(BaseNativeWindowBuffer** buffer, int* fenceFd);
    virtual int queueBuffer(BaseNativeWindowBuffer* buffer, int fenceFd);
    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer, int fenceFd);
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer);

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
    virtual int setBufferCount(int cnt);

private:
    void destroyBuffers();

private:
    framebuffer_device_t* m_fbDev;
    alloc_device_t* m_alloc;
    unsigned int m_usage;
    unsigned int m_bufFormat;
    int m_freeBufs;
    std::list<HWComposerNativeWindowBuffer*> m_bufList;
    HWComposerNativeWindowBuffer* m_frontBuf;

    int m_width;
    int m_height;

};

#endif
// vim: noai:ts=4:sw=4:ss=4:expandtab
