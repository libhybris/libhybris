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


class FbDevNativeWindowBuffer : public BaseNativeWindowBuffer {
friend class FbDevNativeWindow;

protected:
    FbDevNativeWindowBuffer(alloc_device_t* alloc,
                            unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage) ;
   virtual ~FbDevNativeWindowBuffer() ;

protected:
    int busy;
    int status;
    alloc_device_t* m_alloc;
};


class FbDevNativeWindow : public BaseNativeWindow {
public:
    FbDevNativeWindow(gralloc_module_t* gralloc, alloc_device_t* alloc,
         framebuffer_device_t* fbDev);
    ~FbDevNativeWindow();

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
    std::list<FbDevNativeWindowBuffer*> m_bufList;
    FbDevNativeWindowBuffer* m_frontBuf;
};

#endif
// vim: noai:ts=4:sw=4:ss=4:expandtab
