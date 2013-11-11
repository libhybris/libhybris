/****************************************************************************************
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
 ****************************************************************************************/


#include "wayland_window.h"
#include "wayland-egl-priv.h"
#include <assert.h>
#include <stdlib.h>
#include <errno.h>

#include "logging.h"
#include <android/android-version.h>
#include <eglhybris.h>

#if ANDROID_VERSION_MAJOR>=4 && ANDROID_VERSION_MINOR>=2
extern "C" {
#include <android/sync/sync.h>
}
#endif



void WaylandNativeWindowBuffer::wlbuffer_from_native_handle(struct android_wlegl *android_wlegl)
{
    struct wl_array ints;
    int *ints_data;
    struct android_wlegl_handle *wlegl_handle;

    wl_array_init(&ints);
    ints_data = (int*) wl_array_add(&ints, handle->numInts*sizeof(int));
    memcpy(ints_data, handle->data + handle->numFds, handle->numInts*sizeof(int));

    wlegl_handle = android_wlegl_create_handle(android_wlegl, handle->numFds, &ints);

    wl_array_release(&ints);

    for (int i = 0; i < handle->numFds; i++) {
        android_wlegl_handle_add_fd(wlegl_handle, handle->data[i]);
    }

    wlbuffer = android_wlegl_create_buffer(android_wlegl,
            width, height, stride,
            format, usage, wlegl_handle);

    android_wlegl_handle_destroy(wlegl_handle);
}

void WaylandNativeWindow::resize_callback(struct wl_egl_window *egl_window, void *)
{
    TRACE("%dx%d",egl_window->width,egl_window->height);
    native_window_set_buffers_dimensions(
        (WaylandNativeWindow*)egl_window->nativewindow,
        egl_window->width,egl_window->height);
}

void WaylandNativeWindow::lock()
{
    pthread_mutex_lock(&this->mutex);
}

void WaylandNativeWindow::unlock()
{
    pthread_mutex_unlock(&this->mutex);
}

    void
WaylandNativeWindow::registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
        const char *interface, uint32_t version)
{
    WaylandNativeWindow *nw = static_cast<WaylandNativeWindow *>(data);

    if (strcmp(interface, "android_wlegl") == 0) {
        nw->m_android_wlegl = static_cast<struct android_wlegl *>(wl_registry_bind(registry, name, &android_wlegl_interface, 1));
    }
}

static const struct wl_registry_listener registry_listener = {
    WaylandNativeWindow::registry_handle_global
};


    void
WaylandNativeWindow::sync_callback(void *data, struct wl_callback *callback, uint32_t serial)
{
    int *done = static_cast<int *>(data);

    *done = 1;
    wl_callback_destroy(callback);
}

static const struct wl_callback_listener sync_listener = {
    WaylandNativeWindow::sync_callback
};

    int
WaylandNativeWindow::wayland_roundtrip(WaylandNativeWindow *display)
{
    struct wl_callback *callback;
    int done = 0, ret = 0;
    wl_display_dispatch_queue_pending(display->m_display, display->wl_queue);

    callback = wl_display_sync(display->m_display);
    wl_callback_add_listener(callback, &sync_listener, &done);
    wl_proxy_set_queue((struct wl_proxy *) callback, display->wl_queue);
    while (ret == 0 && !done)
        ret = wl_display_dispatch_queue(display->m_display, display->wl_queue);

    return ret;
}

static void check_fatal_error(struct wl_display *display)
{
    int error = wl_display_get_error(display);

    if (error == 0)
        return;

    fprintf(stderr, "Wayland display got fatal error %i: %s\n", error, strerror(error));

    if (errno != 0)
        fprintf(stderr, "Additionally, errno was set to %i: %s\n", errno, strerror(errno));

    fprintf(stderr, "The display is now unusable, aborting.\n");
    abort();
}

    static void
wayland_frame_callback(void *data, struct wl_callback *callback, uint32_t time)
{
    WaylandNativeWindow *surface = static_cast<WaylandNativeWindow *>(data);
    surface->frame();
    wl_callback_destroy(callback);
}

static const struct wl_callback_listener frame_listener = {
    wayland_frame_callback
};

WaylandNativeWindow::WaylandNativeWindow(struct wl_egl_window *window, struct wl_display *display, const gralloc_module_t* gralloc, alloc_device_t* alloc_device)
{
    int wayland_ok;

    HYBRIS_TRACE_BEGIN("wayland-platform", "create_window", "");
    this->m_window = window;
    this->m_window->nativewindow = (void *) this;
    this->m_display = display;
    this->m_width = window->width;
    this->m_height = window->height;
    this->m_defaultWidth = window->width;
    this->m_defaultHeight = window->height;
    this->m_window->resize_callback = resize_callback;
    this->m_format = 1;
    this->wl_queue = wl_display_create_queue(display);
    this->frame_callback = NULL;
    this->registry = wl_display_get_registry(display);
    wl_proxy_set_queue((struct wl_proxy *) this->registry,
            this->wl_queue);
    wl_registry_add_listener(this->registry, &registry_listener, this);

    wayland_ok = wayland_roundtrip(this);
    assert(wayland_ok >= 0);
    assert(this->m_android_wlegl != NULL);

    this->m_gralloc = gralloc;
    this->m_alloc = alloc_device;

    m_usage=GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE;
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond, NULL);
    m_freeBufs = 0;
    setBufferCount(3);
    HYBRIS_TRACE_END("wayland-platform", "create_window", "");
}

WaylandNativeWindow::~WaylandNativeWindow()
{
    std::list<WaylandNativeWindowBuffer *>::iterator it = m_bufList.begin();
    for (; it != m_bufList.end(); it++)
    {
        WaylandNativeWindowBuffer* buf=*it;
        if (buf->wlbuffer)
            wl_buffer_destroy(buf->wlbuffer);
        buf->wlbuffer = NULL;
        buf->common.decRef(&buf->common);
    }
    if (frame_callback)
        wl_callback_destroy(frame_callback);
    wl_registry_destroy(registry);
    wl_event_queue_destroy(wl_queue);
    android_wlegl_destroy(m_android_wlegl);
}

void WaylandNativeWindow::frame() {
    HYBRIS_TRACE_BEGIN("wayland-platform", "frame_event", "");

    this->frame_callback = NULL;

    HYBRIS_TRACE_END("wayland-platform", "frame_event", "");
}


// overloads from BaseNativeWindow
int WaylandNativeWindow::setSwapInterval(int interval) {
    TRACE("interval:%i", interval);
    return 0;
}

    static void
wl_buffer_release(void *data, struct wl_buffer *buffer)
{
    WaylandNativeWindow *win = static_cast<WaylandNativeWindow *>(data);
    win->releaseBuffer(buffer);
}

static struct wl_buffer_listener wl_buffer_listener = {
    wl_buffer_release
};

void WaylandNativeWindow::releaseBuffer(struct wl_buffer *buffer)
{
    lock();
    std::list<WaylandNativeWindowBuffer *>::iterator it = posted.begin();

    for (; it != posted.end(); it++)
    {
        if ((*it)->wlbuffer == buffer)
            break;
    }

    if (it != posted.end())
    {
        WaylandNativeWindowBuffer* pwnb = *it;
        posted.erase(it);
        TRACE("released posted buffer: %p", buffer);
        pwnb->busy = 0;
        pthread_cond_signal(&cond);
        unlock();
        return;
    }

    it = fronted.begin();

    for (; it != fronted.end(); it++)
    {
        if ((*it)->wlbuffer == buffer)
            break;
    }
    assert(it != fronted.end());



    WaylandNativeWindowBuffer* wnb = *it;
    fronted.erase(it);
    HYBRIS_TRACE_COUNTER("wayland-platform", "fronted.size", "%i", fronted.size());

    for (it = m_bufList.begin(); it != m_bufList.end(); it++)
    {
        if ((*it) == wnb)
            break;
    }
    assert(it != m_bufList.end());
    HYBRIS_TRACE_BEGIN("wayland-platform", "releaseBuffer", "-%p", wnb);
    wnb->busy = 0;

    ++m_freeBufs;
    HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);
    for (it = m_bufList.begin(); it != m_bufList.end(); it++)
    {  
        (*it)->youngest = 0;
    }
    wnb->youngest = 1; 


    pthread_cond_signal(&cond);
    HYBRIS_TRACE_END("wayland-platform", "releaseBuffer", "-%p", wnb);
    unlock();
}


int WaylandNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer, int *fenceFd){
    HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer", "");

    WaylandNativeWindowBuffer *wnb=NULL;
    TRACE("%p", buffer);

    lock();
    HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer_wait_for_buffer", "");

    HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);

    while (m_freeBufs==0) {
        HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);

        pthread_cond_wait(&cond,&mutex);
    }
    std::list<WaylandNativeWindowBuffer *>::iterator it = m_bufList.begin();
    for (; it != m_bufList.end(); it++)
    {
         if ((*it)->busy)
             continue;
         if ((*it)->youngest == 1)
             continue;
         break;
    }

    if (it==m_bufList.end()) {
        HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer_worst_case_scenario", "");
        HYBRIS_TRACE_END("wayland-platform", "dequeueBuffer_worst_case_scenario", "");

        it = m_bufList.begin();
        for (; it != m_bufList.end() && (*it)->busy; it++)
        {}
        
    }
    if (it==m_bufList.end()) {
        unlock();
        HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer_no_free_buffers", "");
        HYBRIS_TRACE_END("wayland-platform", "dequeueBuffer_no_free_buffers", "");
        TRACE("%p: no free buffers", buffer);
        return NO_ERROR;
    }

    wnb = *it;
    assert(wnb!=NULL);
    HYBRIS_TRACE_END("wayland-platform", "dequeueBuffer_wait_for_buffer", "");

    /* If the buffer doesn't match the window anymore, re-allocate */
    if (wnb->width != m_window->width || wnb->height != m_window->height
        || wnb->format != m_format || wnb->usage != m_usage)
    {
        TRACE("wnb:%p,win:%p %i,%i %i,%i x%x,x%x x%x,x%x",
            wnb,m_window,
            wnb->width,m_window->width, wnb->height,m_window->height,
            wnb->format,m_format, wnb->usage,m_usage);
        destroyBuffer(wnb);
        m_bufList.erase(it);
        wnb = addBuffer();
    }

    wnb->busy = 1;
    *buffer = wnb;
    --m_freeBufs;

    HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);
    HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer_gotBuffer", "-%p", wnb);
    HYBRIS_TRACE_END("wayland-platform", "dequeueBuffer_gotBuffer", "-%p", wnb);
    HYBRIS_TRACE_END("wayland-platform", "dequeueBuffer_wait_for_buffer", "");

    unlock();
    return NO_ERROR;
}

int WaylandNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer){
    WaylandNativeWindowBuffer *wnb = (WaylandNativeWindowBuffer*) buffer;
    HYBRIS_TRACE_BEGIN("wayland-platform", "lockBuffer", "-%p", wnb);
    HYBRIS_TRACE_END("wayland-platform", "lockBuffer", "-%p", wnb);
    return NO_ERROR;
}

int WaylandNativeWindow::postBuffer(ANativeWindowBuffer* buffer)
{
    TRACE("");
    WaylandNativeWindowBuffer *wnb = NULL;

    lock();
    std::list<WaylandNativeWindowBuffer *>::iterator it = post_registered.begin();
    for (; it != post_registered.end(); it++)
    {
        if ((*it)->other == buffer)
        {
            wnb = (*it);
            break;
        }
    }
    unlock();
    if (!wnb)
    {
        wnb = new WaylandNativeWindowBuffer(buffer);

        wnb->common.incRef(&wnb->common);
        buffer->common.incRef(&buffer->common);
    }

    int ret = 0;

    lock();
    wnb->busy = 1;
    unlock();
    /* XXX locking/something is a bit fishy here */
    while (this->frame_callback && ret != -1) {
        ret = wl_display_dispatch_queue(m_display, this->wl_queue);
    }

    if (ret < 0) {
        TRACE("wl_display_dispatch_queue returned an error:%i", ret);
        check_fatal_error(m_display);
        return ret;
    }

    lock();
    this->frame_callback = wl_surface_frame(m_window->surface);
    wl_callback_add_listener(this->frame_callback, &frame_listener, this);
    wl_proxy_set_queue((struct wl_proxy *) this->frame_callback, this->wl_queue);

    if (wnb->wlbuffer == NULL)
    {
        wnb->wlbuffer_from_native_handle(m_android_wlegl);
        TRACE("%p add listener with %p inside", wnb, wnb->wlbuffer);
        wl_buffer_add_listener(wnb->wlbuffer, &wl_buffer_listener, this);
        wl_proxy_set_queue((struct wl_proxy *) wnb->wlbuffer, this->wl_queue);
        post_registered.push_back(wnb);
    }
    TRACE("%p DAMAGE AREA: %dx%d", wnb, wnb->width, wnb->height);
    wl_surface_attach(m_window->surface, wnb->wlbuffer, 0, 0);
    wl_surface_damage(m_window->surface, 0, 0, wnb->width, wnb->height);
    wl_surface_commit(m_window->surface);
    //--m_freeBufs;
    //pthread_cond_signal(&cond);
    posted.push_back(wnb);
    unlock();

    return NO_ERROR;
}

static int debugenvchecked = 0;

int WaylandNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer, int fenceFd)
{
    WaylandNativeWindowBuffer *wnb = (WaylandNativeWindowBuffer*) buffer;
    int ret = 0;

    HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer", "-%p", wnb);
    lock();
    wnb->busy = 1;
    unlock();
    /* XXX locking/something is a bit fishy here */
    HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer_wait_for_frame_callback", "-%p", wnb);

    while (this->frame_callback && ret != -1) {
        ret = wl_display_dispatch_queue(m_display, this->wl_queue);
    }


    if (ret < 0) {
        TRACE("wl_display_dispatch_queue returned an error");
        HYBRIS_TRACE_END("wayland-platform", "queueBuffer_wait_for_frame_callback", "-%p", wnb);
        check_fatal_error(m_display);
        return ret;
    }

    HYBRIS_TRACE_END("wayland-platform", "queueBuffer_wait_for_frame_callback", "-%p", wnb);


    lock();

    if (debugenvchecked == 0)
    {
        if (getenv("HYBRIS_WAYLAND_DUMP_BUFFERS") != NULL)
            debugenvchecked = 2;
        else
            debugenvchecked = 1;
    }
    if (debugenvchecked == 2)
    {
        HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer_dumping_buffer", "-%p", wnb);
        hybris_dump_buffer_to_file(wnb->getNativeBuffer());
        HYBRIS_TRACE_END("wayland-platform", "queueBuffer_dumping_buffer", "-%p", wnb);

    }

#if ANDROID_VERSION_MAJOR>=4 && ANDROID_VERSION_MINOR>=2
    HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer_waiting_for_fence", "-%p", wnb);
    sync_wait(fenceFd, -1);
    close(fenceFd);
    HYBRIS_TRACE_END("wayland-platform", "queueBuffer_waiting_for_fence", "-%p", wnb);
#endif

    this->frame_callback = wl_surface_frame(m_window->surface);
    wl_callback_add_listener(this->frame_callback, &frame_listener, this);
    wl_proxy_set_queue((struct wl_proxy *) this->frame_callback, this->wl_queue);

    if (wnb->wlbuffer == NULL)
    {
        wnb->wlbuffer_from_native_handle(m_android_wlegl);
        TRACE("%p add listener with %p inside", wnb, wnb->wlbuffer);
        wl_buffer_add_listener(wnb->wlbuffer, &wl_buffer_listener, this);
        wl_proxy_set_queue((struct wl_proxy *) wnb->wlbuffer, this->wl_queue);
    }
    TRACE("%p DAMAGE AREA: %dx%d", wnb, wnb->width, wnb->height);
    HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer_attachdamagecommit", "-resource@%i", wl_proxy_get_id((struct wl_proxy *) wnb->wlbuffer));

    wl_surface_attach(m_window->surface, wnb->wlbuffer, 0, 0);
    wl_surface_damage(m_window->surface, 0, 0, wnb->width, wnb->height);
    wl_surface_commit(m_window->surface);
    wl_display_flush(m_display);
    HYBRIS_TRACE_END("wayland-platform", "queueBuffer_attachdamagecommit", "-resource@%i", wl_proxy_get_id((struct wl_proxy *) wnb->wlbuffer));

    //--m_freeBufs;
    //pthread_cond_signal(&cond);
    fronted.push_back(wnb);
    HYBRIS_TRACE_COUNTER("wayland-platform", "fronted.size", "%i", fronted.size());

    if (fronted.size() == m_bufList.size())
    {
        HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer_wait_for_nonfronted_buffer", "-%p", wnb);

        /* We have fronted all our buffers, let's wait for one of them to be free */
        do {
            unlock();
            ret = wl_display_dispatch_queue(m_display, this->wl_queue);
            lock();
            if (ret == -1)
            {
                check_fatal_error(m_display);
                break;
            }
            HYBRIS_TRACE_COUNTER("wayland-platform", "fronted.size", "%i", fronted.size());

            if (fronted.size() != m_bufList.size())
                break;
        } while (1);
        HYBRIS_TRACE_END("wayland-platform", "queueBuffer_wait_for_nonfronted_buffer", "-%p", wnb);
    }
    HYBRIS_TRACE_END("wayland-platform", "queueBuffer", "-%p", wnb);
    unlock();

    return NO_ERROR;
}

int WaylandNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer, int fenceFd){
    std::list<WaylandNativeWindowBuffer *>::iterator it;
    WaylandNativeWindowBuffer *wnb = (WaylandNativeWindowBuffer*) buffer;

    lock();
    HYBRIS_TRACE_BEGIN("wayland-platform", "cancelBuffer", "-%p", wnb);

    /* Check first that it really is our buffer */
    for (it = m_bufList.begin(); it != m_bufList.end(); it++)
    {
        if ((*it) == wnb)
            break;
    }
    assert(it != m_bufList.end());

    wnb->busy = 0;
    ++m_freeBufs;
    HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);

    for (it = m_bufList.begin(); it != m_bufList.end(); it++)
    {
        (*it)->youngest = 0;
    }
    wnb->youngest = 1;

    pthread_cond_signal(&cond);

    HYBRIS_TRACE_END("wayland-platform", "cancelBuffer", "-%p", wnb);
    unlock();
    return 0;
}

unsigned int WaylandNativeWindow::width() const {
    TRACE("value:%i", m_width);
    return m_width;
}

unsigned int WaylandNativeWindow::height() const {
    TRACE("value:%i", m_height);
    return m_height;
}

unsigned int WaylandNativeWindow::format() const {
    TRACE("value:%i", m_format);
    return m_format;
}

unsigned int WaylandNativeWindow::defaultWidth() const {
    TRACE("value:%i", m_defaultWidth);
    return m_defaultWidth;
}

unsigned int WaylandNativeWindow::defaultHeight() const {
    TRACE("value:%i", m_defaultHeight);
    return m_defaultHeight;
}

unsigned int WaylandNativeWindow::queueLength() const {
    TRACE("WARN: stub");
    return 1;
}

unsigned int WaylandNativeWindow::type() const {
    TRACE("");
#if ANDROID_VERSION_MAJOR>=4 && ANDROID_VERSION_MINOR>=3
    /* https://android.googlesource.com/platform/system/core/+/bcfa910611b42018db580b3459101c564f802552%5E!/ */
    return NATIVE_WINDOW_SURFACE;
#else
    return NATIVE_WINDOW_SURFACE_TEXTURE_CLIENT;
#endif
}

unsigned int WaylandNativeWindow::transformHint() const {
    TRACE("WARN: stub");
    return 0;
}

int WaylandNativeWindow::setBuffersFormat(int format) {
    if (format != m_format)
    {
        TRACE("old-format:x%x new-format:x%x", m_format, format);
        m_format = format;
        /* Buffers will be re-allocated when dequeued */
    } else {
        TRACE("format:x%x", format);
    }
    return NO_ERROR;
}

void WaylandNativeWindow::destroyBuffer(WaylandNativeWindowBuffer* wnb)
{
    TRACE("wnb:%p", wnb);

    assert(wnb != NULL);
    if (wnb->wlbuffer)
        wl_buffer_destroy(wnb->wlbuffer);
    wnb->wlbuffer = NULL;
    wnb->common.decRef(&wnb->common);
    m_freeBufs--;
}

void WaylandNativeWindow::destroyBuffers()
{
    TRACE("");

    std::list<WaylandNativeWindowBuffer*>::iterator it = m_bufList.begin();
    for (; it!=m_bufList.end(); ++it)
    {
        destroyBuffer(*it);
    }
    m_bufList.clear();
    m_freeBufs = 0;
}

WaylandNativeWindowBuffer *WaylandNativeWindow::addBuffer() {

    WaylandNativeWindowBuffer *wnb = new WaylandNativeWindowBuffer(m_alloc, m_width, m_height, m_format, m_usage);
    m_bufList.push_back(wnb);
    wnb->common.incRef(&wnb->common);
    ++m_freeBufs;

    TRACE("wnb:%p width:%i height:%i format:x%x usage:x%x",
         wnb, wnb->width, wnb->height, wnb->format, wnb->usage);

    return wnb;
}


int WaylandNativeWindow::setBufferCount(int cnt) {
    int start = 0;

    TRACE("cnt:%d", cnt);

    if (m_bufList.size() == cnt)
        return NO_ERROR;

    lock();

    if (m_bufList.size() > cnt) {
        /* Decreasing buffer count, remove from beginning */
        std::list<WaylandNativeWindowBuffer*>::iterator it = m_bufList.begin();
        for (int i = 0; i <= m_bufList.size() - cnt; i++ )
        {
            destroyBuffer(*it);
            ++it;
            m_bufList.pop_front();
        }

    } else {
        /* Increasing buffer count, start from current size */
        for (int i = m_bufList.size(); i < cnt; i++)
            WaylandNativeWindowBuffer *unused = addBuffer();

    }

    unlock();

    return NO_ERROR;
}




int WaylandNativeWindow::setBuffersDimensions(int width, int height) {
    if (m_width != width || m_height != height)
    {
        TRACE("old-size:%ix%i new-size:%ix%i", m_width, m_height, width, height);
        m_width = m_defaultWidth = width;
        m_height = m_defaultHeight = height;
        /* Buffers will be re-allocated when dequeued */
    } else {
        TRACE("size:%ix%i", width, height);
    }
    return NO_ERROR;
}

int WaylandNativeWindow::setUsage(int usage) {
    if ((usage | GRALLOC_USAGE_HW_TEXTURE) != m_usage)
    {
        TRACE("old-usage:x%x new-usage:x%x", m_usage, usage);
        m_usage = usage | GRALLOC_USAGE_HW_TEXTURE;
        /* Buffers will be re-allocated when dequeued */
    } else {
        TRACE("usage:x%x", usage);
    }
    return NO_ERROR;
}
// vim: noai:ts=4:sw=4:ss=4:expandtab
