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
    lock();
    m_damage_rects = damage_rects;
    m_damage_n_rects = damage_n_rects;
    unlock();
}

void WaylandNativeWindow::finishSwap()
{
    int ret = 0;
    lock();
    if (!m_window) {
        unlock();
        return;
    }

    ret = readQueue(false);
    if (this->frame_callback) {
        do {
            ret = readQueue(true);
        } while (this->frame_callback && ret != -1);
    }
    if (ret < 0) {
        HYBRIS_TRACE_END("wayland-platform", "queueBuffer_wait_for_frame_callback", "");
        unlock();
        return;
    }

    if (m_swap_interval > 0) {
        this->frame_callback = wl_surface_frame(wl_surface_wrapper);
        wl_callback_add_listener(this->frame_callback, &frame_listener, this);
    }

    WaylandNativeWindowBuffer *wnb = NULL;
    if (!queue.empty()) {
        wnb = queue.front();
        queue.pop_front();
    }

    if (wnb) {
        assert(wnb->busy == 1);

        if (!wnb->wlbuffer) {
            wnb->init(m_android_wlegl, m_display, wl_queue);
            TRACE("%p add listener with %p inside", wnb, wnb->wlbuffer);
            wl_buffer_add_listener(wnb->wlbuffer, &wl_buffer_listener, this);
        }

        wl_surface_attach(wl_surface_wrapper, wnb->wlbuffer, 0, 0);

        m_window->attached_width = wnb->width;
        m_window->attached_height = wnb->height;

        fronted.push_back(wnb);
    }

    // If the compositor doesn't support damage_buffer, we deliberately
    // ignore the damage region and post maximum damage, due to
    // https://bugs.freedesktop.org/78190
    if (wl_proxy_get_version((struct wl_proxy *) wl_surface_wrapper) >=
        WL_SURFACE_DAMAGE_BUFFER_SINCE_VERSION) {
        if (m_damage_n_rects > 0 && m_window->attached_height > 0) {
            for (int i = 0; i < m_damage_n_rects; i++) {
                const int *rect = &m_damage_rects[i * 4];
                wl_surface_damage_buffer(wl_surface_wrapper,
                                         rect[0], m_window->attached_height - rect[1] - rect[3],
                                         rect[2], rect[3]);
            }
        } else if (wnb) {
            wl_surface_damage_buffer(wl_surface_wrapper, 0, 0, INT32_MAX, INT32_MAX);
        }
    } else if (wnb) {
        wl_surface_damage(wl_surface_wrapper, 0, 0, INT32_MAX, INT32_MAX);
    }

    wl_surface_commit(wl_surface_wrapper);

    // If we're not waiting for a frame callback then we'll at least throttle
    // to a sync callback so that we always give a chance for the compositor to
    // handle the commit and send a release event before checking for a free buffer.
    if (this->frame_callback == NULL) {
        this->frame_callback = wl_display_sync(wl_dpy_wrapper);
        wl_callback_add_listener(this->frame_callback, &frame_listener, this);
    }

    wl_display_flush(m_display);

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
