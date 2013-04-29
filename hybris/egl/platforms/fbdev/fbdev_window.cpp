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

//#define DEBUG

#include "fbdev_window.h"


#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>

#define FRAMEBUFFER_PARTITIONS 2

static pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;


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
    TRACE("%s: %p\n", __PRETTY_FUNCTION__, this);
}


////////////////////////////////////////////////////////////////////////////////
FbDevNativeWindow::FbDevNativeWindow(gralloc_module_t* gralloc,
                            alloc_device_t* alloc,
                            framebuffer_device_t* fbDev)
{
    m_gralloc = alloc;
    m_fbDev = fbDev;
    m_freeBufs = 0;
    setBufferCount(FRAMEBUFFER_PARTITIONS);
}



FbDevNativeWindow::~FbDevNativeWindow()
{
    TRACE("%s\n",__PRETTY_FUNCTION__);

    std::list<FbDevNativeWindowBuffer*>::iterator it = m_bufList.begin();
    for (;it!=m_bufList.end()
         ;++it)
    {
        FbDevNativeWindowBuffer* fbnb = *it;
        m_gralloc->free(m_gralloc, fbnb);
        delete fbnb;
    }
}



int FbDevNativeWindow::setSwapInterval(int interval)
{
    TRACE("%s: interval=%i\n", __PRETTY_FUNCTION__, interval);
    m_fbDev->setSwapInterval(m_fbDev, interval);
    return 0;
}



int FbDevNativeWindow::dequeueBuffer(BaseNativeWindowBuffer** buffer, int *fenceFd)
{
    TRACE("%s\n",__PRETTY_FUNCTION__);
    FbDevNativeWindowBuffer* backBuf=NULL;

    pthread_mutex_lock(&_mutex);

    while (m_freeBufs==0)
        pthread_cond_wait(&_cond, &_mutex);

    std::list<FbDevNativeWindowBuffer*>::iterator it = m_bufList.begin();
    for (; it != m_bufList.end(); it++) if ((*it)->busy == 0) break;
    if (it != m_bufList.end())
    {
        backBuf = *it;
        backBuf->busy = 1;
        *buffer = backBuf;
        *fenceFd = -1;
        m_freeBufs--;
        pthread_mutex_unlock(&_mutex);
    }

    pthread_mutex_unlock(&_mutex);

    return NO_ERROR;
}



int FbDevNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer, int fenceFd)
{
    TRACE("%s\n",__PRETTY_FUNCTION__);
    FbDevNativeWindowBuffer* backBuf = (FbDevNativeWindowBuffer*) buffer;

    backBuf->common.incRef(&backBuf->common);

    pthread_mutex_lock(&_mutex);
    backBuf->busy = 2;
    pthread_mutex_unlock(&_mutex);

    int rv = m_fbDev->post(m_fbDev, buffer->handle);

    pthread_mutex_lock(&_mutex);
    backBuf->busy = 0;
    m_freeBufs++;
    pthread_cond_signal(&_cond);
    pthread_mutex_unlock(&_mutex);

    return rv;
}



int FbDevNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer, int fenceFd)
{
    TRACE("%s: WARN: stub\n", __PRETTY_FUNCTION__);
    return 0;
}



int FbDevNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer)
{
    TRACE("%s: WARN: stub\n", __PRETTY_FUNCTION__);
    return NO_ERROR;
}



unsigned int FbDevNativeWindow::width() const
{
    unsigned int rv = m_fbDev->width;
    TRACE("%s: width=%i\n", __PRETTY_FUNCTION__, rv);
    return rv;
}



unsigned int FbDevNativeWindow::height() const
{
    unsigned int rv = m_fbDev->height;
    TRACE("%s: height=%i\n", __PRETTY_FUNCTION__, rv);
    return rv;
}



unsigned int FbDevNativeWindow::format() const
{
    unsigned int rv = m_fbDev->format;
    TRACE("%s: format=%x\n", __PRETTY_FUNCTION__, rv);
    return rv;
}



unsigned int FbDevNativeWindow::defaultWidth() const
{
    unsigned int rv = m_fbDev->width;
    TRACE("%s: width=%i\n", __PRETTY_FUNCTION__, rv);
    return rv;
}



unsigned int FbDevNativeWindow::defaultHeight() const
{
    unsigned int rv = m_fbDev->height;
    TRACE("%s: height=%i\n", __PRETTY_FUNCTION__, rv);
    return rv;
}



unsigned int FbDevNativeWindow::queueLength() const
{
    TRACE("%s: WARN: stub\n", __PRETTY_FUNCTION__);
    return 0;
}



unsigned int FbDevNativeWindow::type() const
{
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return NATIVE_WINDOW_FRAMEBUFFER;
}



unsigned int FbDevNativeWindow::transformHint() const
{
    TRACE("%s: WARN: stub\n", __PRETTY_FUNCTION__);
    return 0;
}



int FbDevNativeWindow::setUsage(int usage)
{
    TRACE("%s: usage=x%x\n", __PRETTY_FUNCTION__, usage);
    m_usage = usage;
    return NO_ERROR;
}



int FbDevNativeWindow::setBuffersFormat(int format)
{
    TRACE("%s: format=x%x\n", __PRETTY_FUNCTION__, format);
    m_bufFormat = format;
    return NO_ERROR;
}



int FbDevNativeWindow::setBufferCount(int cnt)
{
    for(unsigned int i = 0; i < cnt; i++)
    {
        FbDevNativeWindowBuffer *fbnb = new FbDevNativeWindowBuffer(width(),
                            height(), m_fbDev->format, GRALLOC_USAGE_HW_FB);

        int err = m_gralloc->alloc(m_gralloc, width(), height(), m_fbDev->format,
                            GRALLOC_USAGE_HW_FB, &fbnb->handle, &fbnb->stride);
        TRACE("buffer %i is at %p (native %p) err=%s handle=%i stride=%i\n",
                i, fbnb, (ANativeWindowBuffer*)fbnb,
                strerror(-err), fbnb->handle, fbnb->stride);

        assert(err==0);
        m_bufList.push_back(fbnb);
    }
    return NO_ERROR;
}



int FbDevNativeWindow::setBuffersDimensions(int width, int height)
{
    TRACE("%s: WARN: stub. size=%ix%i\n", __PRETTY_FUNCTION__, width, height);
    return NO_ERROR;
}
// vim: noai:ts=4:sw=4:ss=4:expandtab
