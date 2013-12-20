/*
 * Copyright (C) 2013 Canonical Ltd
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
 *
 * Authored by: Jim Hodapp <jim.hodapp@canonical.com>
 */

// Uncomment to enable verbose debug output
#define LOG_NDEBUG 0

#undef LOG_TAG
#define LOG_TAG "SurfaceTextureClientHybris"

#include <hybris/media/surface_texture_client_hybris.h>
#include "surface_texture_client_hybris_priv.h"

#include <ui/GraphicBuffer.h>
#include <utils/Log.h>
#include <ui/Region.h>
#include <gui/Surface.h>
#include <gui/SurfaceTextureClient.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__);

using namespace android;

// ----- Begin _SurfaceTextureClientHybris API ----- //

static inline _SurfaceTextureClientHybris *get_internal_stch(SurfaceTextureClientHybris stc, const char * func)
{
    if (stc == NULL)
    {
        ALOGE("stc must not be NULL (%s)", func);
        return NULL;
    }

    _SurfaceTextureClientHybris *s = static_cast<_SurfaceTextureClientHybris*>(stc);
    assert(s->refcount >= 1);

    return s;
}

_SurfaceTextureClientHybris::_SurfaceTextureClientHybris()
    : refcount(1),
      ready(false)
{
    REPORT_FUNCTION()
}

_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const _SurfaceTextureClientHybris &stch)
    : SurfaceTextureClient::SurfaceTextureClient(),
      refcount(stch.refcount),
      ready(false)
{
    REPORT_FUNCTION()
}

_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const sp<ISurfaceTexture> &st)
    : SurfaceTextureClient::SurfaceTextureClient(st),
      refcount(1),
      ready(false)
{
    REPORT_FUNCTION()
}

_SurfaceTextureClientHybris::~_SurfaceTextureClientHybris()
{
    REPORT_FUNCTION()

    ready = false;
}

bool _SurfaceTextureClientHybris::isReady() const
{
    return ready;
}

int _SurfaceTextureClientHybris::dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd)
{
    return SurfaceTextureClient::dequeueBuffer(buffer, fenceFd);
}

int _SurfaceTextureClientHybris::queueBuffer(ANativeWindowBuffer* buffer, int fenceFd)
{
    return SurfaceTextureClient::queueBuffer(buffer, fenceFd);
}

void _SurfaceTextureClientHybris::setISurfaceTexture(const sp<ISurfaceTexture>& surface_texture)
{
    SurfaceTextureClient::setISurfaceTexture(surface_texture);

    // Ready for rendering
    ready = true;
}

void _SurfaceTextureClientHybris::setHardwareRendering(bool do_hardware_rendering)
{
    hardware_rendering = do_hardware_rendering;
}

bool _SurfaceTextureClientHybris::hardwareRendering()
{
  return hardware_rendering;
}

// ----- End _SurfaceTextureClientHybris API ----- //

static inline void set_surface(_SurfaceTextureClientHybris *stch, const sp<SurfaceTexture> &surface_texture)
{
    REPORT_FUNCTION()

    if (stch == NULL)
        return;

    stch->setISurfaceTexture(surface_texture->getBufferQueue());
}

SurfaceTextureClientHybris surface_texture_client_create_by_id(unsigned int texture_id)
{
    REPORT_FUNCTION()

    if (texture_id == 0)
    {
        ALOGE("Cannot create new SurfaceTextureClientHybris, texture id must be > 0.");
        return NULL;
    }

    _SurfaceTextureClientHybris *stch(new _SurfaceTextureClientHybris);

    ALOGD("stch: %p (%s)", stch, __PRETTY_FUNCTION__);

    // Use a new native buffer allocator vs the default one, which means it'll use the proper one
    // that will allow rendering to work with Mir
    sp<NativeBufferAlloc> native_alloc(new NativeBufferAlloc());
    sp<BufferQueue> buffer_queue(new BufferQueue(false, NULL, native_alloc));

    if (stch->surface_texture != NULL)
      stch->surface_texture.clear();

    const bool allow_synchronous_mode = true;
    stch->surface_texture = new SurfaceTexture(texture_id, allow_synchronous_mode,
            GL_TEXTURE_EXTERNAL_OES, true, buffer_queue);
    set_surface(stch, stch->surface_texture);

    return stch;
}

uint8_t surface_texture_client_is_ready_for_rendering(SurfaceTextureClientHybris stc)
{
    REPORT_FUNCTION()

    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
        return false;

    return static_cast<uint8_t>(s->isReady());
}

uint8_t surface_texture_client_hardware_rendering(SurfaceTextureClientHybris stc)
{
    REPORT_FUNCTION()

    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
        return false;

    return s->hardwareRendering();
}

void surface_texture_client_set_hardware_rendering(SurfaceTextureClientHybris stc, uint8_t hardware_rendering)
{
    REPORT_FUNCTION()

    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
        return;

    s->setHardwareRendering(static_cast<bool>(hardware_rendering));
}

void surface_texture_client_get_transformation_matrix(SurfaceTextureClientHybris stc, float *matrix)
{
    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
        return;

    s->surface_texture->getTransformMatrix(static_cast<GLfloat*>(matrix));
}

void surface_texture_client_update_texture(SurfaceTextureClientHybris stc)
{
    REPORT_FUNCTION()

    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
        return;

    s->surface_texture->updateTexImage();
}

void surface_texture_client_destroy(SurfaceTextureClientHybris stc)
{
    REPORT_FUNCTION()

    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
    {
        ALOGE("s == NULL, cannot destroy SurfaceTextureClientHybris instance");
        return;
    }

    s->refcount = 0;

    delete s;
}

void surface_texture_client_ref(SurfaceTextureClientHybris stc)
{
    REPORT_FUNCTION()

    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
        return;

    s->refcount++;
}

void surface_texture_client_unref(SurfaceTextureClientHybris stc)
{
    REPORT_FUNCTION()

    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
    {
        ALOGE("s == NULL, cannot unref SurfaceTextureClientHybris instance");
        return;
    }

    if (s->refcount > 1)
        s->refcount--;
    else
        surface_texture_client_destroy (stc);
}

void surface_texture_client_set_surface_texture(SurfaceTextureClientHybris stc, EGLNativeWindowType native_window)
{
    _SurfaceTextureClientHybris *s = get_internal_stch(stc, __PRETTY_FUNCTION__);
    if (s == NULL)
        return;

    if (native_window == NULL)
    {
        ALOGE("native_window must not be NULL");
        return;
    }

    sp<Surface> surface = static_cast<Surface*>(native_window);
    s->setISurfaceTexture(surface->getSurfaceTexture());
}
