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


#ifndef Wayland_WINDOW_H
#define Wayland_WINDOW_H
#include "nativewindowbase.h"
#include <linux/fb.h>
#include <android/hardware/gralloc.h>
extern "C" {

#include <wayland-client.h>
#include <wayland-egl.h>
#include "wayland-android-client-protocol.h"
#include <pthread.h>
}
#include <list>

class WaylandNativeWindowBuffer : public BaseNativeWindowBuffer
{
    friend class WaylandNativeWindow;
    protected:
    WaylandNativeWindowBuffer(unsigned int width,
                            unsigned int height,
                            unsigned int format,
                            unsigned int usage) {
        // Base members
        ANativeWindowBuffer::width = width;
        ANativeWindowBuffer::height = height;
        ANativeWindowBuffer::format = format;
        ANativeWindowBuffer::usage = usage;
        this->wlbuffer = NULL;
	this->busy = 0;
    };
    void* vaddr;
    public:
    buffer_handle_t getHandle();
    struct wl_buffer *wlbuffer;
    int busy;
};

class WaylandNativeWindow : public BaseNativeWindow {
public:
    WaylandNativeWindow(struct wl_egl_window *win, struct wl_display *display, const gralloc_module_t* gralloc, alloc_device_t* alloc_device);
    ~WaylandNativeWindow();

    void lock();
    void unlock();
    void frame();
    void releaseBuffer(struct wl_buffer *buffer);

    static void sync_callback(void *data, struct wl_callback *callback, uint32_t serial);
    static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name,
                       const char *interface, uint32_t version);
    struct wl_event_queue *wl_queue;

protected:
    // overloads from BaseNativeWindow
    virtual int setSwapInterval(int interval);
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
    // perform calls
    virtual int setUsage(int usage);
    virtual int setBuffersFormat(int format);
    virtual int setBuffersDimensions(int width, int height);
    virtual int setBufferCount(int cnt);
private:
    std::list<WaylandNativeWindowBuffer *> buffers; 
    std::list<WaylandNativeWindowBuffer *> fronted;
    struct wl_egl_window *m_window;
    struct wl_display *m_display;
    unsigned int m_width;
    unsigned int m_height;
    unsigned int m_format;
    unsigned int m_defaultWidth;
    unsigned int m_defaultHeight;
    unsigned int m_usage;
    struct android_wlegl *m_android_wlegl;
    alloc_device_t* m_alloc;
    struct wl_registry *registry;
    const gralloc_module_t* m_gralloc;
    pthread_mutex_t mutex;
    struct wl_callback *frame_callback;
    static int wayland_roundtrip(WaylandNativeWindow *display);
};

#endif
