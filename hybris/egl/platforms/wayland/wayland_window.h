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

#ifndef Wayland_WINDOW_H
#define Wayland_WINDOW_H
#include "wayland_window_common.h"
#include "eglnativewindowbase.h"
#include <linux/fb.h>

#include <hybris/gralloc/gralloc.h>

extern "C" {

#include <wayland-client.h>
#include <wayland-egl.h>
#include "wayland-android-client-protocol.h"
#include <pthread.h>
}

#include <list>
#include <deque>

class WaylandNativeWindow : public EGLBaseNativeWindow {
public:
    WaylandNativeWindow(struct wl_egl_window *win, struct wl_display *display, android_wlegl *wlegl);
    ~WaylandNativeWindow();

    void lock();
    void unlock();
    void frame();
    void resize(unsigned int width, unsigned int height);
    void releaseBuffer(struct wl_buffer *buffer);
    int postBuffer(ANativeWindowBuffer *buffer);

    virtual int setSwapInterval(int interval);
    void prepareSwap(EGLint *damage_rects, EGLint damage_n_rects);
    void finishSwap();

    static void sync_callback(void *data, struct wl_callback *callback, uint32_t serial);
    static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                       const char *interface, uint32_t version);
    static void resize_callback(struct wl_egl_window *egl_window, void *);
    static void destroy_window_callback(void *data);
    struct wl_event_queue *wl_queue;

protected:
    // overloads from BaseNativeWindow
    virtual int dequeueBuffer(BaseNativeWindowBuffer **buffer, int *fenceFd);
    virtual int lockBuffer(BaseNativeWindowBuffer* buffer);
    virtual int queueBuffer(BaseNativeWindowBuffer* buffer, int fenceFd);
    virtual int cancelBuffer(BaseNativeWindowBuffer* buffer, int fenceFd);
    virtual unsigned int type() const;
    virtual unsigned int width() const;
    virtual unsigned int height() const;
    virtual unsigned int format() const;
    virtual unsigned int defaultWidth() const;
    virtual unsigned int defaultHeight() const;
    virtual unsigned int queueLength() const;
    virtual unsigned int transformHint() const;
    virtual unsigned int getUsage() const;
    // perform calls
    virtual int setUsage(uint64_t usage);
    virtual int setBuffersFormat(int format);
    virtual int setBuffersDimensions(int width, int height);
    virtual int setBufferCount(int cnt);
    virtual int setSurfaceDamage(android_native_rect_t *rects, size_t n_rects);

private:
    WaylandNativeWindowBuffer *addBuffer();
    void destroyBuffer(WaylandNativeWindowBuffer *);
    void destroyBuffers();
    int readQueue(bool block);

    std::list<WaylandNativeWindowBuffer *> m_bufList;
    std::list<WaylandNativeWindowBuffer *> fronted;
    std::list<WaylandNativeWindowBuffer *> posted;
    std::list<WaylandNativeWindowBuffer *> post_registered;
    std::deque<WaylandNativeWindowBuffer *> queue;
    struct wl_egl_window *m_window;
    struct wl_display *m_display;
    WaylandNativeWindowBuffer *m_lastBuffer;
    int m_width;
    int m_height;
    int m_format;
    unsigned int m_defaultWidth;
    unsigned int m_defaultHeight;
    uint64_t m_usage;
    struct android_wlegl *m_android_wlegl;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    int m_queueReads;
    int m_freeBufs;
    EGLint *m_damage_rects, m_damage_n_rects;
    struct wl_callback *frame_callback;
    int m_swap_interval;
};

#endif
// vim: noai:ts=4:sw=4:ss=4:expandtab
