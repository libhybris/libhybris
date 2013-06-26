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

#include "hwcomposer_window.h"
#include "logging.h"

#include <errno.h>
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include <android/android-version.h>
extern "C" {
#include <android/sync/sync.h>
};
 
static pthread_cond_t _cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;


HWComposerNativeWindowBuffer::HWComposerNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage)
{
    ANativeWindowBuffer::width  = width;
    ANativeWindowBuffer::height = height;
    ANativeWindowBuffer::format = format;
    ANativeWindowBuffer::usage  = usage;
    fenceFd = -1;
    busy = 0;
    TRACE("width=%d height=%d format=x%x usage=x%x this=%p",
        width, height, format, usage, this);
}



HWComposerNativeWindowBuffer::~HWComposerNativeWindowBuffer()
{
    TRACE("%p", this);
}


////////////////////////////////////////////////////////////////////////////////
HWComposerNativeWindow::HWComposerNativeWindow(unsigned int width, unsigned int height, unsigned int format)
{
    m_alloc = NULL;
    m_width = width;
    m_height = height;
    m_bufFormat = format;
    m_usage = GRALLOC_USAGE_HW_COMPOSER;
    m_frontBuf = NULL;
}

void HWComposerNativeWindow::setup(gralloc_module_t* gralloc, alloc_device_t* alloc)
{
    m_alloc = alloc;
    setBufferCount(2);
}

HWComposerNativeWindow::~HWComposerNativeWindow()
{
    destroyBuffers();
}



void HWComposerNativeWindow::destroyBuffers()
{
    TRACE("");

    std::list<HWComposerNativeWindowBuffer*>::iterator it = m_bufList.begin();
    for (; it!=m_bufList.end(); ++it)
    {
        HWComposerNativeWindowBuffer* fbnb = *it;
        assert(fbnb->busy == 0);

        m_alloc->free(m_alloc, fbnb->handle);

        fbnb->common.decRef(&fbnb->common);
        assert(fbnb->common.decRef==NULL);
    }
    m_bufList.clear();
    m_freeBufs = 0;
    m_frontBuf = NULL;
}




/*
 * Set the swap interval for this surface.
 *
 * Returns 0 on success or -errno on error.
 */
int HWComposerNativeWindow::setSwapInterval(int interval)
{
    TRACE("interval=%i WARN STUB", interval);
    return 0;
}


/*
 * Hook called by EGL to acquire a buffer. This call may block if no
 * buffers are available.
 *
 * The window holds a reference to the buffer between dequeueBuffer and
 * either queueBuffer or cancelBuffer, so clients only need their own
 * reference if they might use the buffer after queueing or canceling it.
 * Holding a reference to a buffer after queueing or canceling it is only
 * allowed if a specific buffer count has been set.
 *
 * The libsync fence file descriptor returned in the int pointed to by the
 * fenceFd argument will refer to the fence that must signal before the
 * dequeued buffer may be written to.  A value of -1 indicates that the
 * caller may access the buffer immediately without waiting on a fence.  If
 * a valid file descriptor is returned (i.e. any value except -1) then the
 * caller is responsible for closing the file descriptor.
 *
 * Returns 0 on success or -errno on error.
 */
int HWComposerNativeWindow::dequeueBuffer(BaseNativeWindowBuffer** buffer, int *fenceFd)
{
    TRACE("%u", pthread_self());
    HWComposerNativeWindowBuffer* fbnb=NULL;

    pthread_mutex_lock(&_mutex);

    while (m_freeBufs==0)
    {
        pthread_cond_wait(&_cond, &_mutex);
    }

    while (1)
    {
        std::list<HWComposerNativeWindowBuffer*>::iterator it = m_bufList.begin();
        for (; it != m_bufList.end(); ++it)
        {
            if (*it==m_frontBuf)
                continue;
            if ((*it)->busy==0) break;
        }
        if (it == m_bufList.end())
        {
            // have to wait once again 
            pthread_cond_wait(&_cond, &_mutex);
            continue;
        }

        fbnb = *it;
        break;
    }
    assert(fbnb!=NULL);
    fbnb->busy = 1;
    //fbnb->common.incRef(&fbnb->common);
    m_freeBufs--;

    *buffer = fbnb;
    assert(*buffer == static_cast<ANativeWindowBuffer *>(fbnb));
    *fenceFd = -1;

    TRACE("%u DONE --> %p", pthread_self(), fbnb);
    pthread_mutex_unlock(&_mutex);

    return 0;
}

/*
 * Hook called by EGL when modifications to the render buffer are done.
 * This unlocks and post the buffer.
 *
 * The window holds a reference to the buffer between dequeueBuffer and
 * either queueBuffer or cancelBuffer, so clients only need their own
 * reference if they might use the buffer after queueing or canceling it.
 * Holding a reference to a buffer after queueing or canceling it is only
 * allowed if a specific buffer count has been set.
 *
 * The fenceFd argument specifies a libsync fence file descriptor for a
 * fence that must signal before the buffer can be accessed.  If the buffer
 * can be accessed immediately then a value of -1 should be used.  The
 * caller must not use the file descriptor after it is passed to
 * queueBuffer, and the ANativeWindow implementation is responsible for
 * closing it.
 *
 * Returns 0 on success or -errno on error.
 */
int HWComposerNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer, int fenceFd)
{
    TRACE("%u %d", pthread_self(), fenceFd);
    HWComposerNativeWindowBuffer* fbnb = (HWComposerNativeWindowBuffer*) buffer;

    assert(static_cast<HWComposerNativeWindowBuffer *>(buffer) == fbnb);
    fbnb->fenceFd = fenceFd;

    pthread_mutex_lock(&_mutex);

    /* Front buffer hasn't yet been picked up for posting */
    while (m_frontBuf && m_frontBuf->busy >= 2)
    {
           pthread_cond_wait(&_cond, &_mutex);
    }

    assert(fbnb->busy==1);
    fbnb->busy = 2;
    m_frontBuf = fbnb;

    m_freeBufs++;

    sync_wait(fenceFd, -1);
    ::close(fenceFd);    

    pthread_cond_signal(&_cond);

    TRACE("%u %p %p",pthread_self(), m_frontBuf, fbnb);
    pthread_mutex_unlock(&_mutex);

    return 0;
}

void HWComposerNativeWindow::lockFrontBuffer(HWComposerNativeWindowBuffer **buffer)
{
    TRACE("");
    HWComposerNativeWindowBuffer *buf;
    pthread_mutex_lock(&_mutex);
    
    while (!m_frontBuf)
    {
           pthread_cond_wait(&_cond, &_mutex);
    }
    
    assert(m_frontBuf->busy == 2);
    
    m_frontBuf->busy = 3;
    buf = m_frontBuf;
    pthread_mutex_unlock(&_mutex);

   *buffer = buf;
    return;

}

void HWComposerNativeWindow::unlockFrontBuffer(HWComposerNativeWindowBuffer *buffer)
{
    TRACE("");
    pthread_mutex_lock(&_mutex);
    
    assert(buffer == m_frontBuf);
    m_frontBuf->busy = 0; 

    pthread_cond_signal(&_cond);
    pthread_mutex_unlock(&_mutex);
    
    return;
}

/*
 * Hook used to cancel a buffer that has been dequeued.
 * No synchronization is performed between dequeue() and cancel(), so
 * either external synchronization is needed, or these functions must be
 * called from the same thread.
 *
 * The window holds a reference to the buffer between dequeueBuffer and
 * either queueBuffer or cancelBuffer, so clients only need their own
 * reference if they might use the buffer after queueing or canceling it.
 * Holding a reference to a buffer after queueing or canceling it is only
 * allowed if a specific buffer count has been set.
 *
 * The fenceFd argument specifies a libsync fence file decsriptor for a
 * fence that must signal before the buffer can be accessed.  If the buffer
 * can be accessed immediately then a value of -1 should be used.
 *
 * Note that if the client has not waited on the fence that was returned
 * from dequeueBuffer, that same fence should be passed to cancelBuffer to
 * ensure that future uses of the buffer are preceded by a wait on that
 * fence.  The caller must not use the file descriptor after it is passed
 * to cancelBuffer, and the ANativeWindow implementation is responsible for
 * closing it.
 *
 * Returns 0 on success or -errno on error.
 */
int HWComposerNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer, int fenceFd)
{
    TRACE("");
    HWComposerNativeWindowBuffer* fbnb = (HWComposerNativeWindowBuffer*)buffer;
    fbnb->common.decRef(&fbnb->common);
    return 0;
}



int HWComposerNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer)
{
    TRACE("%u STUB", pthread_self());
    return NO_ERROR;
}


/*
 * see NATIVE_WINDOW_FORMAT
 */
unsigned int HWComposerNativeWindow::width() const
{
    unsigned int rv = m_width;
    TRACE("width=%i", rv);
    return rv;
}


/*
 * see NATIVE_WINDOW_HEIGHT
 */
unsigned int HWComposerNativeWindow::height() const
{
    unsigned int rv = m_height;
    TRACE("height=%i", rv);
    return rv;
}


/*
 * see NATIVE_WINDOW_FORMAT
 */
unsigned int HWComposerNativeWindow::format() const
{
    unsigned int rv = m_bufFormat;
    TRACE("format=x%x", rv);
    return rv;
}


/*
 * Default width and height of ANativeWindow buffers, these are the
 * dimensions of the window buffers irrespective of the
 * NATIVE_WINDOW_SET_BUFFERS_DIMENSIONS call and match the native window
 * size unless overridden by NATIVE_WINDOW_SET_BUFFERS_USER_DIMENSIONS.
 */
/*
 * see NATIVE_WINDOW_DEFAULT_HEIGHT
 */
unsigned int HWComposerNativeWindow::defaultHeight() const
{
    unsigned int rv = m_height;
    TRACE("height=%i", rv);
    return rv;
}


/*
 * see BaseNativeWindow::_query(NATIVE_WINDOW_DEFAULT_WIDTH)
 */
unsigned int HWComposerNativeWindow::defaultWidth() const
{
    unsigned int rv = m_width;
    TRACE("width=%i", rv);
    return rv;
}


/*
 * see NATIVE_WINDOW_QUEUES_TO_WINDOW_COMPOSER
 */
unsigned int HWComposerNativeWindow::queueLength() const
{
    TRACE("");
    return 0;
}


/*
 * see NATIVE_WINDOW_CONCRETE_TYPE
 */
unsigned int HWComposerNativeWindow::type() const
{
    TRACE("");
    return NATIVE_WINDOW_FRAMEBUFFER;
}


/*
 * see NATIVE_WINDOW_TRANSFORM_HINT
 */
unsigned int HWComposerNativeWindow::transformHint() const
{
    TRACE("");
    return 0;
}



/*
 *  native_window_set_usage(..., usage)
 *  Sets the intended usage flags for the next buffers
 *  acquired with (*lockBuffer)() and on.
 *  By default (if this function is never called), a usage of
 *      GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE
 *  is assumed.
 *  Calling this function will usually cause following buffers to be
 *  reallocated.
 */
int HWComposerNativeWindow::setUsage(int usage)
{
    int need_realloc = (m_usage != usage);
    TRACE("usage=x%x realloc=%d", usage, need_realloc);
    m_usage = usage;
    if (need_realloc)
        this->setBufferCount(m_bufList.size());

    return NO_ERROR;
}


/*
 * native_window_set_buffers_format(..., int format)
 * All buffers dequeued after this call will have the format specified.
 *
 * If the specified format is 0, the default buffer format will be used.
 */
int HWComposerNativeWindow::setBuffersFormat(int format)
{
    int need_realloc = (format != m_bufFormat);
    TRACE("format=x%x realloc=%d", format, need_realloc);
    m_bufFormat = format;
    if (need_realloc)
        this->setBufferCount(m_bufList.size());
    return NO_ERROR;
}


/*
 * native_window_set_buffer_count(..., count)
 * Sets the number of buffers associated with this native window.
 */
int HWComposerNativeWindow::setBufferCount(int cnt)
{
    TRACE("cnt=%d", cnt);
    int err=NO_ERROR;
    pthread_mutex_lock(&_mutex);

    destroyBuffers();

    for(unsigned int i = 0; i < cnt; i++)
    {
        HWComposerNativeWindowBuffer *fbnb = new HWComposerNativeWindowBuffer(
                            m_width, m_height, m_bufFormat,
                            m_usage|GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_FB);

        fbnb->common.incRef(&fbnb->common);

        err = m_alloc->alloc(m_alloc,
                            m_width, m_height, m_bufFormat,
                            m_usage|GRALLOC_USAGE_HW_COMPOSER|GRALLOC_USAGE_HW_FB,
                            &fbnb->handle, &fbnb->stride);

        TRACE("buffer %i is at %p (native %p) err=%s handle=%i stride=%i",
                i, fbnb, (ANativeWindowBuffer*)fbnb,
                strerror(-err), fbnb->handle, fbnb->stride);

        if (err)
        {
            fbnb->common.decRef(&fbnb->common);
            fprintf(stderr,"WARNING: %s: allocated only %d buffers out of %d\n", __PRETTY_FUNCTION__, m_freeBufs, cnt);
            break;
        }

        m_freeBufs++;
        m_bufList.push_back(fbnb);
    }
    pthread_mutex_unlock(&_mutex);

    return err;
}

/*
 * native_window_set_buffers_dimensions(..., int w, int h)
 * All buffers dequeued after this call will have the dimensions specified.
 * In particular, all buffers will have a fixed-size, independent from the
 * native-window size. They will be scaled according to the scaling mode
 * (see native_window_set_scaling_mode) upon window composition.
 *
 * If w and h are 0, the normal behavior is restored. That is, dequeued buffers
 * following this call will be sized to match the window's size.
 *
 * Calling this function will reset the window crop to a NULL value, which
 * disables cropping of the buffers.
 */
int HWComposerNativeWindow::setBuffersDimensions(int width, int height)
{
    TRACE("WARN: stub. size=%ix%i", width, height);
    return NO_ERROR;
}
// vim: noai:ts=4:sw=4:ss=4:expandtab
