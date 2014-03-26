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
#define LOG_TAG "MediaCodecLayer"

#include <hybris/media/media_codec_layer.h>
#include <hybris/media/media_compatibility_layer.h>
#include <hybris/media/media_format_layer.h>

#include "media_format_layer_priv.h"
#include "surface_texture_client_hybris_priv.h"
#include "decoding_service_priv.h"

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>

#include <fcntl.h>
#include <sys/stat.h>

#include <binder/ProcessState.h>

#include <media/stagefright/foundation/AHandler.h>
#include <media/stagefright/foundation/AString.h>
#include <media/ICrypto.h>
#include <media/stagefright/foundation/ABuffer.h>
#include <media/stagefright/foundation/ADebug.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaErrors.h>
#include <media/stagefright/NativeWindowWrapper.h>

#include <gui/IGraphicBufferProducer.h>
#include <binder/IServiceManager.h>

#include <utils/Vector.h>
#include <utils/Log.h>
#include <utils/RefBase.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__);

using namespace android;

struct _MediaCodecDelegate : public AHandler
{
public:
    typedef sp<_MediaCodecDelegate> Ptr;

    explicit _MediaCodecDelegate(void *context);
    virtual ~_MediaCodecDelegate();

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg) { }

public:
    sp<MediaCodec> media_codec;
    sp<ALooper> looper;

    Vector<sp<ABuffer> > input_buffers;
    Vector<sp<ABuffer> > output_buffers;
    List<MediaCodecBufferInfo> available_output_buffer_infos;
    List<size_t> available_input_buffer_indices;
    bool output_format_changed;
    bool hardware_rendering;

    void *context;
    unsigned int refcount;
};

_MediaCodecDelegate::_MediaCodecDelegate(void *context)
    : output_format_changed(false),
      hardware_rendering(false),
      context(context),
      refcount(1)
{
    REPORT_FUNCTION()
}

_MediaCodecDelegate::~_MediaCodecDelegate()
{
    REPORT_FUNCTION()
}

static inline _MediaCodecDelegate *get_internal_delegate(MediaCodecDelegate delegate)
{
    if (delegate == NULL)
    {
        ALOGE("delegate must not be NULL");
        return NULL;
    }

    _MediaCodecDelegate *d = static_cast<_MediaCodecDelegate*>(delegate);
    // Some simple sanity checks that must be true for a valid MediaCodecDelegate instance
    if (d->media_codec == NULL || d->refcount < 1)
        return NULL;

    return d;
}

// ----- DecodingService C API ----- //
//
// FIXME: These functions really need to be moved into decoding_service.cpp, but for some
// reason were segfaulting when I tried them in there last

namespace {

    DecodingClient& decoding_client_instance()
    {
        static DecodingClient instance;
        return instance;
    }
}

void decoding_service_init()
{
    REPORT_FUNCTION();

    // Register the service with Binder ServiceManager
    DecodingService::instantiate();

    ProcessState::self()->startThreadPool();
}

IGBCWrapperHybris decoding_service_get_igraphicbufferconsumer()
{
    REPORT_FUNCTION();

    sp<IGraphicBufferConsumer> consumer;
    decoding_client_instance().getIGraphicBufferConsumer(&consumer);
    IGBCWrapper *wrapper = new IGBCWrapper(consumer);
    return wrapper;
}

IGBPWrapperHybris decoding_service_get_igraphicbufferproducer()
{
    REPORT_FUNCTION();

    sp<IServiceManager> service_manager = defaultServiceManager();
    sp<IBinder> service = service_manager->getService(
            String16(IDecodingService::exported_service_name()));

    sp<IGraphicBufferProducer> producer;
    DecodingClient::service_instance()->getIGraphicBufferProducer(&producer);
    IGBPWrapper *wrapper = new IGBPWrapper(producer);

    return wrapper;
}

DSSessionWrapperHybris decoding_service_create_session()
{
    REPORT_FUNCTION();

    sp<IServiceManager> service_manager = defaultServiceManager();
    sp<IBinder> service = service_manager->getService(
            String16(IDecodingService::exported_service_name()));

    // Create a new decoding service session
    sp<BnDecodingServiceSession> session(new BnDecodingServiceSession());
    // This new session will destroy and replace any existing session
    DecodingClient::service_instance()->registerSession(session);
    DSSessionWrapper *wrapper(new DSSessionWrapper(session));

    return wrapper;
}

void decoding_service_set_client_death_cb(DecodingClientDeathCbHybris cb, void *context)
{
    REPORT_FUNCTION();

    if (cb == NULL)
    {
        ALOGE("cb must not be NULL");
        return;
    }

    DecodingService::service_instance()->setDecodingClientDeathCb(cb, context);
}

// ----- End of DecodingService C API ----- //

MediaCodecDelegate media_codec_create_by_codec_name(const char *name)
{
    REPORT_FUNCTION()

    if (name == NULL)
    {
        ALOGE("name must not be NULL");
        return NULL;
    }

    ALOGD("Creating codec '%s'", name);

    ProcessState::self()->startThreadPool();

    _MediaCodecDelegate *d(new _MediaCodecDelegate(NULL));
    d->looper = new ALooper;
    d->looper->start();

    d->media_codec = android::MediaCodec::CreateByComponentName(d->looper, name);

    return d;
}

#ifdef SIMPLE_PLAYER
MediaCodec* media_codec_get(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return NULL;

    return d->media_codec.get();
}
#endif

MediaCodecDelegate media_codec_create_by_codec_type(const char *type)
{
    REPORT_FUNCTION()

    if (type == NULL)
    {
        ALOGE("type must not be NULL");
        return NULL;
    }

    ALOGD("Creating codec by type '%s'", type);

    ProcessState::self()->startThreadPool();

    _MediaCodecDelegate *d(new _MediaCodecDelegate(NULL));
    d->looper = new ALooper;
    d->looper->start();

    d->media_codec = android::MediaCodec::CreateByType(d->looper, type, false);

    return d;
}

void media_codec_delegate_destroy(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
    {
        ALOGE("d == NULL, cannot destroy MediaCodecDelegate instance");
        return;
    }

    ALOGI("Releasing media_codec");
    d->media_codec->release();
    ALOGI("Stopping looper");
    d->looper->stop();

    ALOGI("Setting refcount = 0");
    d->refcount = 0;

    ALOGI("Deleting the MediaCodecDelegate instance");
    delete d;
}

void media_codec_delegate_ref(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return;

    d->refcount++;
}

void media_codec_delegate_unref(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
    {
        ALOGE("d == NULL, cannot unref MediaCodecDelegate instance");
        return;
    }

    if (d->refcount > 1)
        d->refcount--;
    else
        media_codec_delegate_destroy (delegate);
}

#ifdef SIMPLE_PLAYER
int media_codec_configure(MediaCodecDelegate delegate, MediaFormat format, void *nativeWindow, uint32_t flags)
#else
int media_codec_configure(MediaCodecDelegate delegate, MediaFormat format, SurfaceTextureClientHybris stc, uint32_t flags)
#endif
{
    REPORT_FUNCTION()

    if (format == NULL)
    {
        ALOGE("format must not be NULL");
        return BAD_VALUE;
    }

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    _MediaFormat *format_priv = static_cast<_MediaFormat*>(format);
#ifndef SIMPLE_PLAYER
    _SurfaceTextureClientHybris *stch = static_cast<_SurfaceTextureClientHybris*>(stc);
#endif

    sp<AMessage> aformat = new AMessage;
    aformat->setString("mime", format_priv->mime.c_str());
    if (format_priv->duration_us > 0)
        aformat->setInt64("durationUs", format_priv->duration_us);
    aformat->setInt32("width", format_priv->width);
    aformat->setInt32("height", format_priv->height);
    if (format_priv->max_input_size > 0)
        aformat->setInt32("max-input-size", format_priv->max_input_size);

    ALOGD("Format: %s", aformat->debugString().c_str());

#ifdef SIMPLE_PLAYER
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    sp<SurfaceTextureClient> surfaceTextureClient = static_cast<SurfaceTextureClient*>(nativeWindow);
#else
    sp<Surface> surfaceTextureClient = static_cast<Surface*>(nativeWindow);
#endif
    // TODO: Don't just pass NULL for the security when DRM is needed
    d->media_codec->configure(aformat, surfaceTextureClient, NULL, flags);
#else
    ALOGD("SurfaceTextureClientHybris: %p", stch);

    // Make sure we're ready to configure the codec and the Surface/SurfaceTextureClient together
    if (stch != NULL && stch->hardwareRendering() && stch->isReady())
    {
        ALOGD("Doing hardware decoding with hardware rendering");
        // TODO: Don't just pass NULL for the security when DRM is needed
        d->media_codec->configure(aformat, stch, NULL, flags);
    }
    else
    {
        ALOGD("Doing hardware decoding path with software rendering");
        // This scenario is for hardware video decoding, but software rendering, therefore there's
        // no need to pass a valid Surface/SurfaceTextureClient instance to configure()
        d->media_codec->configure(aformat, NULL, NULL, flags);
    }
#endif

    return OK;
}

int media_codec_set_surface_texture_client(MediaCodecDelegate delegate, SurfaceTextureClientHybris stc)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;
    if (stc == NULL)
    {
        ALOGE("stc must not be NULL");
        return BAD_VALUE;
    }

    _SurfaceTextureClientHybris *stcu = static_cast<_SurfaceTextureClientHybris*>(stc);
    status_t err = native_window_api_connect(stcu, NATIVE_WINDOW_API_MEDIA);
    if (err != OK)
    {
        ALOGE("native_window_api_connect returned an error: %s (%d)", strerror(-err), err);
        return err;
    }

    return OK;
}

int media_codec_queue_csd(MediaCodecDelegate delegate, MediaFormat format)
{
    REPORT_FUNCTION()

    if (format == NULL)
    {
        ALOGE("format must not be NULL");
        return BAD_VALUE;
    }

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    _MediaFormat *format_priv = static_cast<_MediaFormat*>(format);
    assert(format_priv->csd != NULL);

    status_t err = OK;

    Vector<sp<ABuffer> > input_bufs[1];
    err = d->media_codec->getInputBuffers(&input_bufs[0]);
    CHECK_EQ(err, static_cast<status_t>(OK));

    for (size_t i=0; i<2; ++i)
    {
        const sp<ABuffer> &srcBuffer = format_priv->csd;

        size_t index = 0;
        err = d->media_codec->dequeueInputBuffer(&index, -1ll);
        CHECK_EQ(err, static_cast<status_t>(OK));

        const sp<ABuffer> &dstBuffer = input_bufs[0].itemAt(index);

        CHECK_LE(srcBuffer->size(), dstBuffer->capacity());
        dstBuffer->setRange(0, srcBuffer->size());
        memcpy(dstBuffer->data(), srcBuffer->data(), srcBuffer->size());

        AString err_msg;
        err = d->media_codec->queueInputBuffer(
                index,
                0,
                dstBuffer->size(),
                0ll,
                MediaCodec::BUFFER_FLAG_CODECCONFIG);
        CHECK_EQ(err, static_cast<status_t>(OK));
    }

    return err;
}

int media_codec_start(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    return d->media_codec->start();
}

int media_codec_stop(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    return d->media_codec->stop();
}

int media_codec_release(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    return d->media_codec->release();
}

int media_codec_flush(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    d->available_output_buffer_infos.clear();

    return d->media_codec->flush();
}

size_t media_codec_get_input_buffers_size(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    status_t ret = d->media_codec->getInputBuffers(&d->input_buffers);
    if (ret != OK)
    {
        ALOGE("Failed to get input buffers size");
        return 0;
    }
    ALOGD("Got %d input buffers", d->input_buffers.size());

    return d->input_buffers.size();
}

uint8_t *media_codec_get_nth_input_buffer(MediaCodecDelegate delegate, size_t n)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return NULL;

    if (d->input_buffers.size() == 0)
    {
        status_t ret = d->media_codec->getInputBuffers(&d->input_buffers);
        if (ret != OK)
        {
            ALOGE("Failed to get input buffers");
            return NULL;
        }
    }

    if (n > d->input_buffers.size())
    {
      ALOGE("Failed to get %uth input buffer, n > total buffer size", n);
      return NULL;
    }

    return d->input_buffers.itemAt(n).get()->data();
}

size_t media_codec_get_nth_input_buffer_capacity(MediaCodecDelegate delegate, size_t n)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    Vector<sp<ABuffer> > input_buffers;
    status_t ret = d->media_codec->getInputBuffers(&input_buffers);
    if (ret != OK)
    {
        ALOGE("Failed to get input buffers");
        return 0;
    }

    if (n > input_buffers.size())
    {
      ALOGE("Failed to get %uth input buffer capacity, n > total buffer size", n);
      return 0;
    }

    return input_buffers[n].get()->capacity();
}

size_t media_codec_get_output_buffers_size(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    status_t ret = d->media_codec->getOutputBuffers(&d->output_buffers);
    if (ret != OK)
    {
        ALOGE("Failed to get output buffers size");
        return 0;
    }
    ALOGD("Got %d output buffers", d->output_buffers.size());

    return d->output_buffers.size();
}

uint8_t *media_codec_get_nth_output_buffer(MediaCodecDelegate delegate, size_t n)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return NULL;

    status_t ret = d->media_codec->getOutputBuffers(&d->output_buffers);
    if (ret != OK)
    {
        ALOGE("Failed to get output buffers");
        return NULL;
    }

    if (n > d->output_buffers.size())
    {
      ALOGE("Failed to get %uth output buffer, n > total buffer size", n);
      return NULL;
    }

    return d->output_buffers.itemAt(n).get()->data();
}

size_t media_codec_get_nth_output_buffer_capacity(MediaCodecDelegate delegate, size_t n)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    status_t ret = d->media_codec->getOutputBuffers(&d->output_buffers);
    if (ret != OK)
    {
        ALOGE("Failed to get output buffers");
        return 0;
    }

    if (n > d->output_buffers.size())
    {
      ALOGE("Failed to get %uth output buffer capacity, n > total buffer size", n);
      return 0;
    }

    return d->output_buffers[n].get()->capacity();
}

#define INFO_TRY_AGAIN_LATER        -1
#define INFO_OUTPUT_FORMAT_CHANGED  -2
#define INFO_OUTPUT_BUFFERS_CHANGED -4

int media_codec_dequeue_output_buffer(MediaCodecDelegate delegate, MediaCodecBufferInfo *info, int64_t timeout_us)
{
    REPORT_FUNCTION()

    if (info == NULL)
    {
        ALOGE("info must not be NULL");
        return BAD_VALUE;
    }

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    int ret = d->media_codec->dequeueOutputBuffer(&info->index, &info->offset, &info->size, &info->presentation_time_us, &info->flags, timeout_us);
    ALOGD("dequeueOutputBuffer() ret: %d", ret);
    info->render_retries = 0;

    if (ret == -EAGAIN)
    {
        ALOGD("dequeueOutputBuffer returned %d", ret);
        return INFO_TRY_AGAIN_LATER;
    }
    else if (ret & ~INFO_OUTPUT_BUFFERS_CHANGED)
    {
        ALOGD("Output buffers changed (ret: %d)", ret);
        return INFO_OUTPUT_BUFFERS_CHANGED + 1;
    }
    // FIXME: Get rid of the hardcoded -10 and replace with more elegant solution
    else if (ret & ~(INFO_FORMAT_CHANGED - 10))
    {
        ALOGD("Output buffer format changed (ret: %d)", ret);
        d->output_format_changed = true;
        return -2;
    }

    ALOGD("Dequeued output buffer:\n-----------------------");
    ALOGD("index: %u", info->index);
    ALOGD("offset: %d", info->offset);
    ALOGD("size: %d", info->size);
    ALOGD("presentation_time_us: %lld", info->presentation_time_us);
    ALOGD("flags: %d", info->flags);

    // Keep track of the used output buffer info
    d->available_output_buffer_infos.push_back(*info);

    return OK;
}

int media_codec_queue_input_buffer(MediaCodecDelegate delegate, const MediaCodecBufferInfo *info)
{
    REPORT_FUNCTION()

    if (info == NULL)
    {
        ALOGE("info must not be NULL");
        return BAD_VALUE;
    }

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    // Make sure that there is at least one dequeued input buffer available
    if (d->available_input_buffer_indices.empty())
    {
        ALOGE("Input buffer index %d has not been dequeued, cannot queue input buffer", info->index);
        return BAD_VALUE;
    }

    const size_t index = *d->available_input_buffer_indices.begin();
    d->available_input_buffer_indices.erase(d->available_input_buffer_indices.begin());

    ALOGD("info->index: %d", index);
    ALOGD("info->offset: %d", info->offset);
    ALOGD("info->size: %d", info->size);
    ALOGD("info->presentation_time_us: %lld", info->presentation_time_us);
    ALOGD("info->flags: %d", info->flags);

    AString err_msg;
    status_t ret = d->media_codec->queueInputBuffer(index, info->offset, info->size,
            info->presentation_time_us, info->flags, &err_msg);
    if (ret != OK)
    {
        ALOGE("Failed to queue input buffer (err: %d, index: %d)", ret, index);
        ALOGE("Detailed error message: %s", err_msg.c_str());
    }

    return ret;
}

int media_codec_dequeue_input_buffer(MediaCodecDelegate delegate, size_t *index, int64_t timeout_us)
{
    REPORT_FUNCTION()

    if (index == NULL)
    {
        ALOGE("index must not be NULL");
        return BAD_VALUE;
    }

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    status_t ret = d->media_codec->dequeueInputBuffer(index, timeout_us);
    if (ret == -EAGAIN)
    {
        ALOGD("dequeueInputBuffer returned %d, tried timeout: %lld", ret, timeout_us);
        return INFO_TRY_AGAIN_LATER;
    }
    else if (ret == OK)
    {
        ALOGD("Dequeued input buffer (index: %d)", *index);
        d->available_input_buffer_indices.push_back(*index);
    }
    else
        ALOGE("Failed to dequeue input buffer (err: %d, index: %d)", ret, *index);

    return ret;
}

int media_codec_release_output_buffer(MediaCodecDelegate delegate, size_t index, uint8_t render)
{
    REPORT_FUNCTION()
    ALOGV("Requesting to release output buffer index: %d, render: %d", index, render);

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return BAD_VALUE;

    status_t ret = OK;

    auto it = d->available_output_buffer_infos.begin();
    while (it != d->available_output_buffer_infos.end())
    {
        MediaCodecBufferInfo *info = &*it;
        ALOGD("info index: %d", info->index);
        ALOGD("info render_retries: %u", info->render_retries);
        if (info->render_retries == 1)
        {
            ALOGV("Rendering and releasing output buffer %d from the available indices list", info->index);
            ret = d->media_codec->renderOutputBufferAndRelease(info->index);
            if (ret != OK)
            {
                ALOGE("Failed to release output buffer (ret: %d, index: %d)", ret, info->index);
                ++info->render_retries;
            }
            else
            {
                ALOGV("Successfully rendered output buffer %d on a second try.", info->index);
                d->available_output_buffer_infos.erase(it);
            }
        }
        else if (info->render_retries > 1)
        {
            ALOGV("Tried to render output buffer %d twice, dropping.", info->index);
            ret = d->media_codec->releaseOutputBuffer(info->index);
            d->available_output_buffer_infos.erase(d->available_output_buffer_infos.begin());
        }

        ++it;
    }

    MediaCodecBufferInfo *info = &*d->available_output_buffer_infos.begin();
    // Either render and release the output buffer, or just release.
    if (render)
    {
        ALOGV("Rendering and releasing output buffer %d from the available indices list", info->index);
        ret = d->media_codec->renderOutputBufferAndRelease(info->index);
    }
    else
    {
        ALOGV("Releasing output buffer %d from the available indices list", info->index);
        ret = d->media_codec->releaseOutputBuffer(info->index);
    }
    if (ret != OK)
    {
        ALOGE("Failed to release output buffer (ret: %d, index: %d)", ret, info->index);
        ++info->render_retries;
    } else {
        ALOGV("Released output buffer %d from the available buffer infos list", info->index);
        d->available_output_buffer_infos.erase(d->available_output_buffer_infos.begin());
    }

    return ret;
}

MediaFormat media_codec_get_output_format(MediaCodecDelegate delegate)
{
    REPORT_FUNCTION()

    _MediaCodecDelegate *d = get_internal_delegate(delegate);
    if (d == NULL)
        return NULL;

    _MediaFormat *f = new _MediaFormat();

    sp<AMessage> msg_format;
    status_t ret = d->media_codec->getOutputFormat(&msg_format);
    if (ret != OK)
    {
        ALOGE("Failed to get the output format");
        return NULL;
    }

    ALOGD("Output format: %s", msg_format->debugString().c_str());

    CHECK(msg_format->findString("mime", &f->mime));
    CHECK(msg_format->findInt32("width", &f->width));
    CHECK(msg_format->findInt32("height", &f->height));
    CHECK(msg_format->findInt32("stride", &f->stride));
    CHECK(msg_format->findInt32("slice-height", &f->slice_height));
    CHECK(msg_format->findInt32("color-format", &f->color_format));
    Rect crop;
    CHECK(msg_format->findRect("crop", &crop.left, &crop.top, &crop.right, &crop.bottom));

    return f;
}
