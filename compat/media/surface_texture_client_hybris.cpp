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
#include "decoding_service_priv.h"

#include <ui/GraphicBuffer.h>
#include <utils/Log.h>
#include <ui/Region.h>
#include <gui/Surface.h>
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
#include <gui/SurfaceTextureClient.h>
#endif
#include <gui/IGraphicBufferProducer.h>
#include <gui/GLConsumer.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

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

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
_SurfaceTextureClientHybris::_SurfaceTextureClientHybris()
    : refcount(1),
      ready(false)
{
    REPORT_FUNCTION()
}
#endif

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR>=4
_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const sp<BufferQueue> &bq)
    : Surface::Surface(bq, true),
      refcount(1),
      ready(false)
{
    REPORT_FUNCTION()
}

_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const sp<IGraphicBufferProducer> &st)
    : Surface::Surface(st, true),
      refcount(1),
      ready(false)
{
    REPORT_FUNCTION()
}
#endif

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const _SurfaceTextureClientHybris &stch)
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    : SurfaceTextureClient::SurfaceTextureClient(),
#else
    : Surface::Surface(new BufferQueue(), true),
#endif
      refcount(stch.refcount),
      ready(false)
{
    REPORT_FUNCTION()
}

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const sp<ISurfaceTexture> &st)
    : SurfaceTextureClient::SurfaceTextureClient(st),
#else
_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const sp<IGraphicBufferProducer> &st)
    : Surface::Surface(st, false),
#endif
      refcount(1),
      ready(false)
{
    REPORT_FUNCTION()
}
#endif

_SurfaceTextureClientHybris::_SurfaceTextureClientHybris(const android::sp<android::IGraphicBufferProducer> &st,
        bool producerIsControlledByApp)
    : Surface::Surface(st, producerIsControlledByApp),
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

void _SurfaceTextureClientHybris::setReady(bool ready)
{
    this->ready = ready;
}

int _SurfaceTextureClientHybris::dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd)
{
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    return SurfaceTextureClient::dequeueBuffer(buffer, fenceFd);
#else
    return Surface::dequeueBuffer(buffer, fenceFd);
#endif
}

int _SurfaceTextureClientHybris::queueBuffer(ANativeWindowBuffer* buffer, int fenceFd)
{
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    return SurfaceTextureClient::queueBuffer(buffer, fenceFd);
#else
    return Surface::queueBuffer(buffer, fenceFd);
#endif
}

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
void _SurfaceTextureClientHybris::setISurfaceTexture(const sp<ISurfaceTexture>& surface_texture)
{
    SurfaceTextureClient::setISurfaceTexture(surface_texture);
#else
void _SurfaceTextureClientHybris::setISurfaceTexture(const sp<IGraphicBufferProducer>& surface_texture)
{
    // We don't need to set up the IGraphicBufferProducer as stc needs it when created
#endif

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

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
static inline void set_surface(_SurfaceTextureClientHybris *stch, const sp<SurfaceTexture> &surface_texture)
#else
static inline void set_surface(_SurfaceTextureClientHybris *stch, const sp<GLConsumer> &surface_texture)
#endif
{
    REPORT_FUNCTION()

    if (stch == NULL)
        return;

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    stch->setISurfaceTexture(surface_texture->getBufferQueue());
#else
    stch->setISurfaceTexture(stch->getIGraphicBufferProducer());
#endif
}

SurfaceTextureClientHybris surface_texture_client_create_by_id(unsigned int texture_id)
{
    REPORT_FUNCTION()

    if (texture_id == 0)
    {
        ALOGE("Cannot create new SurfaceTextureClientHybris, texture id must be > 0.");
        return NULL;
    }

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=3
    // Use a new native buffer allocator vs the default one, which means it'll use the proper one
    // that will allow rendering to work with Mir
    sp<NativeBufferAlloc> native_alloc(new NativeBufferAlloc());

    sp<BufferQueue> buffer_queue(new BufferQueue(false, NULL, native_alloc));
    _SurfaceTextureClientHybris *stch(new _SurfaceTextureClientHybris);
#else
    sp<BufferQueue> buffer_queue(new BufferQueue(NULL));
    _SurfaceTextureClientHybris *stch(new _SurfaceTextureClientHybris(buffer_queue));
#endif

    ALOGD("stch: %p (%s)", stch, __PRETTY_FUNCTION__);

    if (stch->surface_texture != NULL)
      stch->surface_texture.clear();

    const bool allow_synchronous_mode = true;
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    stch->surface_texture = new SurfaceTexture(texture_id, allow_synchronous_mode, GL_TEXTURE_EXTERNAL_OES, true, buffer_queue);
    set_surface(stch, stch->surface_texture);
#else
    stch->surface_texture = new GLConsumer(buffer_queue, texture_id, GL_TEXTURE_EXTERNAL_OES, true, true);
#endif
    set_surface(stch, stch->surface_texture);

    return stch;
}

SurfaceTextureClientHybris surface_texture_client_create_by_igbp(IGBPWrapperHybris wrapper)
{
    if (wrapper == NULL)
    {
        ALOGE("Cannot create new SurfaceTextureClientHybris, wrapper must not be NULL.");
        return NULL;
    }

    IGBPWrapper *igbp = static_cast<IGBPWrapper*>(wrapper);
    // The producer should be the same BufferQueue as what the client is using but over Binder
    // Allow the app to control the producer side BufferQueue
    _SurfaceTextureClientHybris *stch(new _SurfaceTextureClientHybris(igbp->producer, true));
    // Ready for rendering
    stch->setReady();
    return stch;
}

GLConsumerWrapperHybris gl_consumer_create_by_id_with_igbc(unsigned int texture_id, IGBCWrapperHybris wrapper)
{
    REPORT_FUNCTION()

    if (texture_id == 0)
    {
        ALOGE("Cannot create new SurfaceTextureClientHybris, texture id must be > 0.");
        return NULL;
    }

    if (wrapper == NULL)
    {
        ALOGE("Cannot create new GLConsumerHybris, wrapper must not be NULL.");
        return NULL;
    }

    IGBCWrapper *igbc = static_cast<IGBCWrapper*>(wrapper);
    // Use a fence guard and consumer is controlled by app:
    sp<_GLConsumerHybris> gl_consumer = new _GLConsumerHybris(igbc->consumer, texture_id, GL_TEXTURE_EXTERNAL_OES, true, true);
    GLConsumerWrapper *glc_wrapper = new GLConsumerWrapper(gl_consumer);

    return glc_wrapper;
}

int gl_consumer_set_frame_available_cb(GLConsumerWrapperHybris wrapper, FrameAvailableCbHybris cb, void *context)
{
    REPORT_FUNCTION()

    if (wrapper == NULL)
    {
        ALOGE("Cannot set GLConsumerWrapperHybris, wrapper must not be NULL");
        return BAD_VALUE;
    }
    if (cb == NULL)
    {
        ALOGE("Cannot set FrameAvailableCbHybris, cb must not be NULL");
        return BAD_VALUE;
    }

    GLConsumerWrapper *glc_wrapper = static_cast<GLConsumerWrapper*>(wrapper);
    sp<_GLConsumerHybris> glc_hybris = static_cast<_GLConsumerHybris*>(glc_wrapper->consumer.get());
    glc_hybris->createFrameAvailableListener(cb, wrapper, context);

    return OK;
}

void gl_consumer_get_transformation_matrix(GLConsumerWrapperHybris wrapper, float *matrix)
{
    REPORT_FUNCTION()

    if (wrapper == NULL)
    {
        ALOGE("Cannot set GLConsumerWrapperHybris, wrapper must not be NULL");
        return;
    }

    GLConsumerWrapper *glc_wrapper = static_cast<GLConsumerWrapper*>(wrapper);
    sp<_GLConsumerHybris> glc_hybris = static_cast<_GLConsumerHybris*>(glc_wrapper->consumer.get());
    glc_hybris->getTransformMatrix(static_cast<GLfloat*>(matrix));
}

void gl_consumer_update_texture(GLConsumerWrapperHybris wrapper)
{
    REPORT_FUNCTION()

    if (wrapper == NULL)
    {
        ALOGE("Cannot set GLConsumerWrapperHybris, wrapper must not be NULL");
        return;
    }

    GLConsumerWrapper *glc_wrapper = static_cast<GLConsumerWrapper*>(wrapper);
    sp<_GLConsumerHybris> glc_hybris = static_cast<_GLConsumerHybris*>(glc_wrapper->consumer.get());
    glc_hybris->updateTexImage();
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
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    s->setISurfaceTexture(surface->getSurfaceTexture());
#else
    s->setISurfaceTexture(surface->getIGraphicBufferProducer());
#endif
}
