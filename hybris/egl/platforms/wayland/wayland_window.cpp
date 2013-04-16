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
	int i;

	this->m_window = window;
	this->m_display = display;
	this->m_width = window->width;
	this->m_height = window->height;
	this->m_defaultWidth = window->width;
	this->m_defaultHeight = window->height;
	this->m_format = 1;
        this->wl_queue = wl_display_create_queue(display);
	this->frame_callback = NULL;
	this->registry = wl_display_get_registry(display);
   	wl_proxy_set_queue((struct wl_proxy *) this->registry,
                      this->wl_queue);
        wl_registry_add_listener(this->registry, &registry_listener, this);

	assert(wayland_roundtrip(this) >= 0);
        assert(this->m_android_wlegl != NULL);
        
        this->m_gralloc = gralloc;
        this->m_alloc = alloc_device;
        
	m_usage=GRALLOC_USAGE_HW_RENDER | GRALLOC_USAGE_HW_TEXTURE;
	pthread_mutex_init(&mutex, NULL);

        printf("WaylandNativeWindow created in %p\n", pthread_self());
}

WaylandNativeWindow::~WaylandNativeWindow()
{
}

buffer_handle_t WaylandNativeWindowBuffer::getHandle()
{
  return handle;
}

void WaylandNativeWindow::frame() {
	this->frame_callback = NULL;
}


// overloads from BaseNativeWindow
int WaylandNativeWindow::setSwapInterval(int interval) {
    TRACE("%s\n",__PRETTY_FUNCTION__);
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
	std::list<WaylandNativeWindowBuffer *>::iterator it = fronted.begin();
 
	for (; it != fronted.end(); it++)
	{
		if ((*it)->wlbuffer == buffer)
			break;
	}
	assert(it != fronted.end());
	WaylandNativeWindowBuffer *buf = *it;
	fronted.erase(it);
	wl_buffer_destroy(buf->wlbuffer);
	buf->wlbuffer = NULL;
	assert(this->m_alloc->free(this->m_alloc, buf->getHandle()) == 0);
	delete buf;
}


int WaylandNativeWindow::dequeueBuffer(BaseNativeWindowBuffer **buffer){
    WaylandNativeWindowBuffer *backbuf;
    lock();

    wl_display_dispatch_queue_pending(m_display, this->wl_queue);
    if (queued.empty())
    { 
	printf("!!! queued empty, but dequeuing\n");
    }   
 
    if (!queued.empty())
    { 
        WaylandNativeWindowBuffer *front = queued.front();
	int ret = 0;
	queued.pop_front();

	fronted.push_back(front);
	printf("Fronted %p with %p inside\n", front, front->wlbuffer);
	while (this->frame_callback && ret != -1)
        	ret = wl_display_dispatch_queue(m_display, this->wl_queue);

	wl_surface_attach(m_window->surface, front->wlbuffer, 0, 0); 
        wl_surface_damage(m_window->surface, 0, 0, front->width, front->height);
        wl_surface_commit(m_window->surface);


        this->frame_callback = wl_surface_frame(m_window->surface);
        wl_callback_add_listener(this->frame_callback, &frame_listener, this);
        wl_proxy_set_queue((struct wl_proxy *) this->frame_callback, this->wl_queue);
    }
#if 0 
    if (dequeued.empty())
    { 
#endif
        backbuf = new WaylandNativeWindowBuffer(m_width, m_height, m_format, m_usage);
        int err = m_alloc->alloc(m_alloc,
                        backbuf->width ? backbuf->width : 1, backbuf->height ? backbuf->height : 1, backbuf->format,
                        backbuf->usage,
                        &backbuf->handle,
                        &backbuf->stride);
	assert(err == 0);

	/* Register back buffer with compositor */
	struct wl_array ints;
	int *ints_data;
	struct android_wlegl_handle *wlegl_handle;
	buffer_handle_t handle;
	
	handle = backbuf->handle;

	wl_array_init(&ints);
	ints_data = (int*) wl_array_add(&ints, handle->numInts*sizeof(int));
	memcpy(ints_data, handle->data + handle->numFds, handle->numInts*sizeof(int));
	wlegl_handle = android_wlegl_create_handle(m_android_wlegl, handle->numFds, &ints);
	wl_array_release(&ints);
	for (int i = 0; i < handle->numFds; i++) {
		android_wlegl_handle_add_fd(wlegl_handle, handle->data[i]);
	}

	backbuf->wlbuffer = android_wlegl_create_buffer(m_android_wlegl,
			backbuf->width, backbuf->height, backbuf->stride,
			backbuf->format, backbuf->usage, wlegl_handle);

	android_wlegl_handle_destroy(wlegl_handle);
	backbuf->common.incRef(&backbuf->common);


	printf("Add listener for %p with %p inside\n", backbuf, backbuf->wlbuffer);
	wl_buffer_add_listener(backbuf->wlbuffer, &wl_buffer_listener, this);
	wl_proxy_set_queue((struct wl_proxy *) backbuf->wlbuffer,
			this->wl_queue);

	*buffer = backbuf;
#if 0
    }
    else
    {
	backbuf = dequeued.front();
	dequeued.pop_front();

	assert(backbuf->wlbuffer == NULL);

	/* Register back buffer with compositor */
	struct wl_array ints;
	int *ints_data;
	struct android_wlegl_handle *wlegl_handle;
	buffer_handle_t handle;
	
	handle = backbuf->handle;

	wl_array_init(&ints);
	ints_data = (int*) wl_array_add(&ints, handle->numInts*sizeof(int));
	memcpy(ints_data, handle->data + handle->numFds, handle->numInts*sizeof(int));
	wlegl_handle = android_wlegl_create_handle(m_android_wlegl, handle->numFds, &ints);
	wl_array_release(&ints);
	for (int i = 0; i < handle->numFds; i++) {
		android_wlegl_handle_add_fd(wlegl_handle, handle->data[i]);
	}

	backbuf->wlbuffer = android_wlegl_create_buffer(m_android_wlegl,
			backbuf->width, backbuf->height, backbuf->stride,
			backbuf->format, backbuf->usage, wlegl_handle);

	android_wlegl_handle_destroy(wlegl_handle);

	wl_buffer_add_listener(backbuf->wlbuffer, &wl_buffer_listener, this);
	wl_proxy_set_queue((struct wl_proxy *) backbuf->wlbuffer,
			this->wl_queue);
	backbuf->common.incRef(&backbuf->common);
	*buffer = backbuf;
 
    } 
#endif
    unlock();
    return NO_ERROR;
}

int WaylandNativeWindow::lockBuffer(BaseNativeWindowBuffer* buffer){
    TRACE("%s ===================\n",__PRETTY_FUNCTION__);
    return NO_ERROR;
}

int WaylandNativeWindow::queueBuffer(BaseNativeWindowBuffer* buffer){
   lock();
   queued.push_back(static_cast<WaylandNativeWindowBuffer *>(buffer));
   unlock();
   return NO_ERROR;
}

int WaylandNativeWindow::cancelBuffer(BaseNativeWindowBuffer* buffer){
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

unsigned int WaylandNativeWindow::width() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_width);
    return m_width;
}

unsigned int WaylandNativeWindow::height() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_height);
    return m_height;
}

unsigned int WaylandNativeWindow::format() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_format);
    return m_format;
}

unsigned int WaylandNativeWindow::defaultWidth() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_defaultWidth);
    return m_defaultWidth;
}

unsigned int WaylandNativeWindow::defaultHeight() const {
    TRACE("%s value: %i\n",__PRETTY_FUNCTION__, m_defaultHeight);
    return m_defaultHeight;
}

unsigned int WaylandNativeWindow::queueLength() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 1;
}

unsigned int WaylandNativeWindow::type() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return NATIVE_WINDOW_SURFACE_TEXTURE_CLIENT;
}

unsigned int WaylandNativeWindow::transformHint() const {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    return 0;
}

int WaylandNativeWindow::setBuffersFormat(int format) {
    TRACE("%s format %i\n",__PRETTY_FUNCTION__, format);
    m_format = format;
    return NO_ERROR;
}

int WaylandNativeWindow::setBuffersDimensions(int width, int height) {
    TRACE("%s size %ix%i\n",__PRETTY_FUNCTION__, width, height);
    return NO_ERROR;
}

int WaylandNativeWindow::setUsage(int usage) {
    TRACE("%s\n",__PRETTY_FUNCTION__);
    m_usage = usage | GRALLOC_USAGE_HW_TEXTURE;
    return NO_ERROR;
}
