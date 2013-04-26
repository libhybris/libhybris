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

#include "fbdev_window.h"


#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#define DEBUG_FBDEV_WINDOW

#ifdef DEBUG_FBDEV_WINDOW
#define TRACE(fmt, ...) printf("FBDEV TRACE:%s " fmt,__PRETTY_FUNCTION__,##__VA_ARGS__); printf("\n")
#else
#define TRACE(fmt, ...)
#endif

#define FRAMEBUFFER_PARTITIONS 2

static pthread_mutex_t _mutex=PTHREAD_MUTEX_INITIALIZER;


FbDevNativeWindowBuffer::FbDevNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage)
{
    ANativeWindowBuffer::width  = width;
    ANativeWindowBuffer::height = height;
    ANativeWindowBuffer::format = format;
    ANativeWindowBuffer::usage  = usage;
}



FbDevNativeWindowBuffer::~FbDevNativeWindowBuffer()
{
    TRACE("%p",this);
}


////////////////////////////////////////////////////////////////////////////////
FbDevNativeWindow::FbDevNativeWindow(gralloc_module_t* gralloc,
                            alloc_device_t* alloc,
                            framebuffer_device_t* fbDev)
{
    m_gralloc = alloc;
    m_fbDev = fbDev;
    setBufferCount(FRAMEBUFFER_PARTITIONS);
}



FbDevNativeWindow::~FbDevNativeWindow()
{
    TRACE("");
}



int FbDevNativeWindow::setSwapInterval(int interval)
{
    TRACE("");
    m_fbDev->setSwapInterval(m_fbDev, interval);
    return 0;
}



int FbDevNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer)
{
    TRACE("");
    int fenceFd=-1;
    return dequeueBuffer(buffer, &fenceFd);
}
int FbDevNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer, int *fenceFd)
{
    TRACE("");
    FbDevNativeWindowBuffer* backBuf=NULL;

    while (1)
    {
        pthread_mutex_lock(&_mutex);
        std::list<FbDevNativeWindowBuffer*>::iterator it = m_bufList.begin();
        for (; it != m_bufList.end(); it++) if ((*it)->busy == 0) break;
        if (it != m_bufList.end())
        {
            backBuf = *it;
            backBuf->busy = 1;
            *buffer = backBuf;
            *fenceFd = -1;
            pthread_mutex_unlock(&_mutex);
            break;
        }
        pthread_mutex_unlock(&_mutex);
    }

    return NO_ERROR;
}



int FbDevNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer)
{
    return queueBuffer(buffer,-1);
}
int FbDevNativeWindow::queueBuffer(BaseNativeWindowBuffer *buffer, int fenceFd)
{
    TRACE("");
    FbDevNativeWindowBuffer* backBuf = (FbDevNativeWindowBuffer *) buffer;

    backBuf->common.incRef(&backBuf->common);
    pthread_mutex_lock(&_mutex);
    backBuf->busy = 2;
    pthread_mutex_unlock(&_mutex);
    int res = m_fbDev->post(m_fbDev, buffer->handle);
    pthread_mutex_lock(&_mutex);
    backBuf->busy = 0;
    pthread_mutex_unlock(&_mutex);
    //fronted.push_back(backBuf);
    return NO_ERROR;
}



int FbDevNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer)
{
    TRACE("");
    return cancelBuffer(buffer,-1);
}
int FbDevNativeWindow::cancelBuffer(BaseNativeWindowBuffer *buffer, int fenceFd)
{
    TRACE("");
    assert(0);
    return 0;
}



int FbDevNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer)
{
    TRACE("");
    return NO_ERROR;
}



unsigned int FbDevNativeWindow::width() const
{
    unsigned int val = m_fbDev->width;
    TRACE("val=%i", val);
    return val;
}



unsigned int FbDevNativeWindow::height() const
{
    unsigned int val = m_fbDev->height;
    TRACE("val=%i", val);
    return val;
}



unsigned int FbDevNativeWindow::format() const
{
    unsigned int val = m_fbDev->format;
    TRACE("val=%x", val);
    return val;
}



unsigned int FbDevNativeWindow::defaultWidth() const
{
    TRACE("");
    return m_fbDev->width;
}



unsigned int FbDevNativeWindow::defaultHeight() const
{
    TRACE("");
    return m_fbDev->height;
}



unsigned int FbDevNativeWindow::queueLength() const
{
    TRACE("stub");
    return 0;
}



unsigned int FbDevNativeWindow::type() const
{
    TRACE("");
    return NATIVE_WINDOW_FRAMEBUFFER;
}



unsigned int FbDevNativeWindow::transformHint() const
{
    TRACE("stub");
    return 0;
}



int FbDevNativeWindow::setUsage(int usage)
{
    TRACE("usage=x%x", usage);
    m_usage = usage;
    return NO_ERROR;
}



int FbDevNativeWindow::setBuffersFormat(int format)
{
    TRACE("format=x%x", format);
    m_bufFormat = format;
    return NO_ERROR;
}



int FbDevNativeWindow::setBufferCount(int cnt)
{
    for(unsigned int i = 0; i < cnt; i++) {
        FbDevNativeWindowBuffer *fbnb = new FbDevNativeWindowBuffer(width(),
                            height(), m_fbDev->format, GRALLOC_USAGE_HW_FB);

        int err = m_gralloc->alloc(m_gralloc, width(), height(), m_fbDev->format,
                            GRALLOC_USAGE_HW_FB, &fbnb->handle, &fbnb->stride);
        TRACE("buffer %i is at %p (native %p) err=%s handle=%i stride=%i",
                i, fbnb, (ANativeWindowBuffer*)fbnb,
                strerror(-err), fbnb->handle, fbnb->stride);
        assert(err==0);
        m_bufList.push_back(fbnb);
    }
    return NO_ERROR;
}



int FbDevNativeWindow::setBuffersDimensions(int width, int height)
{
    TRACE("size=%ix%i", width, height);
    return NO_ERROR;
}
// vim: noai:ts=4:sw=4:ss=4:expandtab
