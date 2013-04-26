/*******************************************************************************
 **
 ** Copyright (C) 2013 Jolla Ltd.
 ** Contact: Carsten Munk <carsten.munk@jollamobile.com>
 ** All rights reserved.
 **
 ** This file is part of Wayland enablement for libhybris
 **
 ** You may use this file under the terms of the GNU Lesser General
 ** Public License version 2.1 as published by the Free Software Foundation
 ** and appearing in the file license.lgpl included in the packaging
 ** of this file.
 **
 ** This library is free software; you can redistribute it and/or
 ** modify it under the terms of the GNU Lesser General Public
 ** License version 2.1 as published by the Free Software Foundation
 ** and appearing in the file license.lgpl included in the packaging
 ** of this file.
 **
 ** This library is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 ** Lesser General Public License for more details.
 **
 ******************************************************************************/

#ifndef FBDEV_WINDOW_H
#define FBDEV_WINDOW_H

#include "nativewindowbase.h"
#include <linux/fb.h>
#include <android/hardware/gralloc.h>

#include <list>

class FbDevNativeWindowBuffer : public BaseNativeWindowBuffer {
    friend class FbDevNativeWindow;
protected:
    FbDevNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage) ;
   virtual ~FbDevNativeWindowBuffer() ;
protected:
    int busy;
};

class FbDevNativeWindow : public BaseNativeWindow {
public:
    FbDevNativeWindow(gralloc_module_t* gralloc, alloc_device_t* alloc,
         framebuffer_device_t* fbDev);
    ~FbDevNativeWindow();

protected:
    // overloads from BaseNativeWindow
    virtual int setSwapInterval(int interval);
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer);

    virtual int dequeueBuffer(BaseNativeWindowBuffer **buffer);
    virtual int dequeueBuffer(BaseNativeWindowBuffer **buffer, int *fenceFd);

    virtual int queueBuffer(BaseNativeWindowBuffer* buffer);
    virtual int queueBuffer(BaseNativeWindowBuffer *buffer, int fenceFd);

    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer);
    virtual int cancelBuffer(BaseNativeWindowBuffer *buffer, int fenceFd);

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
    unsigned int m_usage;
    unsigned int m_bufFormat;
    std::list<FbDevNativeWindowBuffer*> m_bufList;
    alloc_device_t* m_gralloc;
    framebuffer_device_t* m_fbDev;
};

#endif
// vim: noai:ts=4:sw=4:ss=4:expandtab
