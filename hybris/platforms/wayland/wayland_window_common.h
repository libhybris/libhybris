/****************************************************************************************
 **
 ** Copyright (C) 2013-2022 Jolla Ltd.
 ** Copyright (C) 2024 Jollyboys Ltd.
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

#ifndef WAYLAND_WINDOW_COMMON_H
#define WAYLAND_WINDOW_COMMON_H
#include "nativewindowbase.h"

#include <hybris/gralloc/gralloc.h>

extern "C" {

#include <wayland-client.h>
#include <wayland-egl.h>
#include "wayland-android-client-protocol.h"
}

class WaylandNativeWindowBuffer : public BaseNativeWindowBuffer
{
public:
    WaylandNativeWindowBuffer()
        : wlbuffer(0)
        , busy(0)
        , youngest(0)
        , other(0)
        , creation_callback(0)
    {}
    WaylandNativeWindowBuffer(ANativeWindowBuffer *other)
    {
        ANativeWindowBuffer::width = other->width;
        ANativeWindowBuffer::height = other->height;
        ANativeWindowBuffer::format = other->format;
        ANativeWindowBuffer::usage = other->usage;
        ANativeWindowBuffer::handle = other->handle;
        ANativeWindowBuffer::stride = other->stride;
        this->wlbuffer = NULL;
        this->creation_callback = NULL;
        this->busy = 0;
        this->other = other;
        this->youngest = 0;
    }

    struct wl_buffer *wlbuffer;
    int busy;
    int youngest;
    ANativeWindowBuffer *other;
    struct wl_callback *creation_callback;

    void wlbuffer_from_native_handle(struct android_wlegl *android_wlegl,
                                     struct wl_display *display,
                                     struct wl_event_queue *queue);

    virtual void init(struct android_wlegl *android_wlegl,
                      struct wl_display *display,
                      struct wl_event_queue *queue) {}
};

#ifdef HYBRIS_NO_SERVER_SIDE_BUFFERS

class ClientWaylandBuffer : public WaylandNativeWindowBuffer
{
friend class WaylandNativeWindow;
protected:
    ClientWaylandBuffer()
    {}

    ClientWaylandBuffer(unsigned int width,
                        unsigned int height,
                        unsigned int format,
                        uint64_t usage)
    {
        // Base members
        ANativeWindowBuffer::width = width;
        ANativeWindowBuffer::height = height;
        ANativeWindowBuffer::format = format;
        ANativeWindowBuffer::usage = usage;
        this->wlbuffer = NULL;
        this->creation_callback = NULL;
        this->busy = 0;
        this->other = NULL;
        int alloc_ok = hybris_gralloc_allocate(this->width ? this->width : 1,
                this->height ? this->height : 1,
                this->format, (uint32_t)this->usage,
                &this->handle, (uint32_t*)&this->stride);
        assert(alloc_ok == 0);
        this->youngest = 0;
        this->common.incRef(&this->common);
    }

    ~ClientWaylandBuffer()
    {
        hybris_gralloc_release(this->handle, 1);
    }

    void init(struct android_wlegl *android_wlegl,
              struct wl_display *display,
              struct wl_event_queue *queue) override;

protected:
    void* vaddr;
};

#else

class ServerWaylandBuffer : public WaylandNativeWindowBuffer
{
public:
    ServerWaylandBuffer(unsigned int w,
                        unsigned int h,
                        int _format,
                        uint64_t _usage,
                        android_wlegl *android_wlegl,
                        struct wl_event_queue *queue);
    ~ServerWaylandBuffer();
    void init(struct android_wlegl *android_wlegl,
              struct wl_display *display,
              struct wl_event_queue *queue) override;

    struct wl_array ints;
    struct wl_array fds;
    wl_buffer *m_buf;
    android_wlegl_server_buffer_handle *ssb;
};

#endif // HYBRIS_NO_SERVER_SIDE_BUFFERS

#endif
// vim: noai:ts=4:sw=4:ss=4:expandtab
