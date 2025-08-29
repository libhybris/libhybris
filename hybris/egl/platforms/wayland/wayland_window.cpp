/****************************************************************************************
 **
 ** Copyright (C) 2013-2022 Jolla Ltd.
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


#include <android-config.h>
#include <hardware/gralloc.h>
#include "wayland_window.h"
#include <wayland-egl-backend.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "logging.h"
#include <eglhybris.h>

#if ANDROID_VERSION_MAJOR>=4 && ANDROID_VERSION_MINOR>=2 || ANDROID_VERSION_MAJOR>=5
extern "C" {
#include <sync/sync.h>
}
#endif

static void
wl_buffer_release(void *data, struct wl_buffer *buffer)
{
    WaylandNativeWindow *win = static_cast<WaylandNativeWindow *>(data);
    win->releaseBuffer(buffer);
}

static struct wl_buffer_listener wl_buffer_listener = {
    wl_buffer_release
};

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

int WaylandNativeWindow::postBuffer(ANativeWindowBuffer* buffer)
{
    TRACE("");
    WaylandNativeWindowBuffer *wnb = NULL;

    lock();
    std::list<WaylandNativeWindowBuffer *>::iterator it = post_registered.begin();
    for (; it != post_registered.end(); ++it)
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
    ret = readQueue(false);

    if (ret < 0) {
        unlock();
        return ret;
    }

    if (wnb->wlbuffer == NULL)
    {
        wnb->wlbuffer_from_native_handle(m_android_wlegl, m_display, wl_queue);
        TRACE("%p add listener with %p inside", wnb, wnb->wlbuffer);
        wl_buffer_add_listener(wnb->wlbuffer, &wl_buffer_listener, this);
        wl_proxy_set_queue((struct wl_proxy *) wnb->wlbuffer, this->wl_queue);
        post_registered.push_back(wnb);
    }
    TRACE("%p DAMAGE AREA: %dx%d", wnb, wnb->width, wnb->height);
    wl_surface_attach(m_window->surface, wnb->wlbuffer, 0, 0);
    wl_surface_damage(m_window->surface, 0, 0, wnb->width, wnb->height);
    wl_surface_commit(m_window->surface);
    wl_display_flush(m_display);

    posted.push_back(wnb);
    unlock();

    return NO_ERROR;
}

int WaylandNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer, int *fenceFd){
    HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer", "");

    WaylandNativeWindowBuffer *wnb=NULL;
    TRACE("%p", buffer);

    lock();
    readQueue(false);

    HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer_wait_for_buffer", "");

    HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);

    while (m_freeBufs==0) {
        HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);
        readQueue(true);
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
    if (wnb->width != m_width || wnb->height != m_height
        || wnb->format != m_format || wnb->usage != m_usage)
    {
        TRACE("wnb:%p,win:%p %i,%i %i,%i x%x,x%x x%x,x%x",
            wnb,m_window,
            wnb->width,m_width, wnb->height,m_height,
            wnb->format,m_format, wnb->usage,m_usage);
        destroyBuffer(wnb);
        m_bufList.erase(it);
        wnb = addBuffer();
    }

    wnb->busy = 1;
    *buffer = wnb;
    queue.push_back(wnb);
    --m_freeBufs;

    HYBRIS_TRACE_COUNTER("wayland-platform", "m_freeBufs", "%i", m_freeBufs);
    HYBRIS_TRACE_BEGIN("wayland-platform", "dequeueBuffer_gotBuffer", "-%p", wnb);
    HYBRIS_TRACE_END("wayland-platform", "dequeueBuffer_gotBuffer", "-%p", wnb);
    HYBRIS_TRACE_END("wayland-platform", "dequeueBuffer_wait_for_buffer", "");

    unlock();
    return NO_ERROR;
}

void WaylandNativeWindow::prepareSwap(EGLint *damage_rects, EGLint damage_n_rects)
{
    setSurfaceDamage(reinterpret_cast<android_native_rect_t*>(damage_rects), damage_n_rects);
}

int WaylandNativeWindow::setSurfaceDamage(android_native_rect_t *rects, size_t n_rects)
{
    lock();
    if (m_damage_rects) {
        free(m_damage_rects);
    }
    size_t size = sizeof(EGLint) * 4 * n_rects;
    m_damage_rects = (EGLint*)malloc(size);
    memcpy(m_damage_rects, rects, size);
    m_damage_n_rects = n_rects;
    unlock();

    return NO_ERROR;
}

void WaylandNativeWindow::finishSwap()
{
    int ret = 0;
    lock();
    if (!m_window) {
        unlock();
        return;
    }

    WaylandNativeWindowBuffer *wnb = queue.front();
    if (!wnb) {
        wnb = m_lastBuffer;
    } else {
        queue.pop_front();
    }
    assert(wnb);
    m_lastBuffer = wnb;
    wnb->busy = 1;

    ret = readQueue(false);
    if (this->frame_callback) {
        do {
            ret = readQueue(true);
        } while (this->frame_callback && ret != -1);
    }
    if (ret < 0) {
        HYBRIS_TRACE_END("wayland-platform", "queueBuffer_wait_for_frame_callback", "-%p", wnb);
        unlock();
        return;
    }

    if (wnb->wlbuffer == NULL)
    {
        wnb->init(m_android_wlegl, m_display, wl_queue);
        TRACE("%p add listener with %p inside", wnb, wnb->wlbuffer);
        wl_buffer_add_listener(wnb->wlbuffer, &wl_buffer_listener, this);
        wl_proxy_set_queue((struct wl_proxy *) wnb->wlbuffer, this->wl_queue);
    }

    if (m_swap_interval > 0) {
        this->frame_callback = wl_surface_frame(m_window->surface);
        wl_callback_add_listener(this->frame_callback, &frame_listener, this);
        wl_proxy_set_queue((struct wl_proxy *) this->frame_callback, this->wl_queue);
    }

    wl_surface_attach(m_window->surface, wnb->wlbuffer, 0, 0);
    /* FIXME:
    if (m_damage_rects) {
        for (int nr = 0; nr < m_damage_n_rects; nr++) {
            EGLint *rect = &m_damage_rects[nr * 4];
            wl_surface_damage(m_window->surface, rect[0], rect[1], rect[2], rect[3]);
        }
    } else {...}
    */
    wl_surface_damage(m_window->surface, 0, 0, wnb->width, wnb->height);
    wl_surface_commit(m_window->surface);
    // Some compositors, namely Weston, queue buffer release events instead
    // of sending them immediately.  If a frame event is used, this should
    // not be a problem.  Without a frame event, we need to send a sync
    // request to ensure that they get flushed.
    wl_callback_destroy(wl_display_sync(m_display));
    wl_display_flush(m_display);
    fronted.push_back(wnb);

    m_window->attached_width = wnb->width;
    m_window->attached_height = wnb->height;

    if (m_damage_rects) {
        free(m_damage_rects);
    }
    m_damage_rects = NULL;
    m_damage_n_rects = 0;
    unlock();
}

static int debugenvchecked = 0;

int WaylandNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer, int fenceFd)
{
    WaylandNativeWindowBuffer *wnb = (WaylandNativeWindowBuffer*) buffer;

    HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer", "-%p", wnb);
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

#if ANDROID_VERSION_MAJOR>=4 && ANDROID_VERSION_MINOR>=2 || ANDROID_VERSION_MAJOR>=5
    HYBRIS_TRACE_BEGIN("wayland-platform", "queueBuffer_waiting_for_fence", "-%p", wnb);
    if (fenceFd >= 0)
    {
        sync_wait(fenceFd, -1);
        close(fenceFd);
    }
    HYBRIS_TRACE_END("wayland-platform", "queueBuffer_waiting_for_fence", "-%p", wnb);
#endif

    HYBRIS_TRACE_COUNTER("wayland-platform", "fronted.size", "%i", fronted.size());
    HYBRIS_TRACE_END("wayland-platform", "queueBuffer", "-%p", wnb);
    unlock();

    return NO_ERROR;
}

// vim: noai:ts=4:sw=4:ss=4:expandtab
