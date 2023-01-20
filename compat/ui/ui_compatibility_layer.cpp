/*
 * Copyright (C) 2013 Simon Busch <morphis@gravedo.de>
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
 */

#include <fcntl.h>
#include <sys/stat.h>

#if ANDROID_VERSION_MAJOR>=10
#include <ui/Gralloc.h>
#endif
#include <ui/GraphicBuffer.h>
#include <ui/GraphicBufferMapper.h>
#include <ui/GraphicBufferAllocator.h>

#include <hybris/ui/ui_compatibility_layer.h>

struct graphic_buffer
{
    android::GraphicBuffer *self;
};

struct graphic_buffer* graphic_buffer_new(void)
{
    struct graphic_buffer *buffer = NULL;

    buffer = (struct graphic_buffer*) malloc(sizeof(struct graphic_buffer));
    if (!buffer)
        return NULL;

    buffer->self = new android::GraphicBuffer();

    return buffer;
}

struct graphic_buffer* graphic_buffer_new_sized(uint32_t w, uint32_t h,
                                                int32_t format, uint32_t usage)
{
    struct graphic_buffer *buffer = NULL;

    buffer = (struct graphic_buffer*) malloc(sizeof(struct graphic_buffer));
    if (!buffer)
        return NULL;

    buffer->self = new android::GraphicBuffer(w, h, format, usage);

    return buffer;
}

struct graphic_buffer* graphic_buffer_new_existing(uint32_t w, uint32_t h,
                                              int32_t format, uint32_t usage,
                                              uint32_t stride, void *handle,
                                              bool keepOwnership)
{
    struct graphic_buffer *buffer = NULL;

    buffer = (struct graphic_buffer*) malloc(sizeof(struct graphic_buffer));
    if (!buffer)
        return NULL;

#if ANDROID_VERSION_MAJOR>=8
    buffer->self = new android::GraphicBuffer(w, h, format, 1/* layerCount */, usage, stride,
                                              (native_handle_t*) handle, keepOwnership);
#else
    buffer->self = new android::GraphicBuffer(w, h, format, usage, stride,
                                              (native_handle_t*) handle, keepOwnership);
#endif

    return buffer;

}

void graphic_buffer_free(struct graphic_buffer *buffer)
{
    if (!buffer)
        return;

    free(buffer);
}

uint32_t graphic_buffer_get_width(struct graphic_buffer *buffer)
{
    return buffer->self->getWidth();
}

uint32_t graphic_buffer_get_height(struct graphic_buffer *buffer)
{
    return buffer->self->getHeight();
}

uint32_t graphic_buffer_get_stride(struct graphic_buffer *buffer)
{
    return buffer->self->getStride();
}

uint32_t graphic_buffer_get_usage(struct graphic_buffer *buffer)
{
    return buffer->self->getUsage();
}

int32_t graphic_buffer_get_pixel_format(struct graphic_buffer *buffer)
{
    return buffer->self->getPixelFormat();
}

uint32_t graphic_buffer_reallocate(struct graphic_buffer *buffer, uint32_t w,
                                   uint32_t h, int32_t f, uint32_t usage)
{
#if ANDROID_VERSION_MAJOR>=8
    return buffer->self->reallocate(w, h, f, 1/* layerCount */, usage);
#else
    return buffer->self->reallocate(w, h, f, usage);
#endif
}

uint32_t graphic_buffer_lock(struct graphic_buffer *buffer, uint32_t usage, void **vaddr)
{
    return buffer->self->lock(usage, vaddr);
}

uint32_t graphic_buffer_unlock(struct graphic_buffer *buffer)
{
    return buffer->self->unlock();
}

void* graphic_buffer_get_native_buffer(struct graphic_buffer *buffer)
{
    return buffer->self->getNativeBuffer();
}

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=3
void graphic_buffer_set_index(struct graphic_buffer *buffer, int index)
{
    return buffer->self->setIndex(index);
}

int graphic_buffer_get_index(struct graphic_buffer *buffer)
{
    return buffer->self->getIndex();
}
#endif

int graphic_buffer_init_check(struct graphic_buffer *buffer)
{
    return buffer->self->initCheck();
}

#if ANDROID_VERSION_MAJOR>=10
using android::GraphicBufferAllocator;
using android::GraphicBufferMapper;
using android::PixelFormat;
using android::status_t;

status_t graphic_buffer_allocator_allocate(uint32_t width, uint32_t height,
                                           PixelFormat format, uint32_t layerCount, uint64_t usage,
                                           buffer_handle_t* handle, uint32_t* stride,
                                           uint64_t graphicBufferId, const char* requestorName)
{
    return GraphicBufferAllocator::getInstance().allocate(width, height, format, layerCount, usage,
                                                          handle, stride, graphicBufferId, requestorName);
}

status_t graphic_buffer_allocator_free(buffer_handle_t handle) {
    return GraphicBufferAllocator::getInstance().free(handle);
}

status_t graphic_buffer_mapper_import_buffer(buffer_handle_t rawHandle,
        uint32_t width, uint32_t height, uint32_t layerCount,
        PixelFormat format, uint64_t usage, uint32_t stride,
        buffer_handle_t* outHandle)
{
    return GraphicBufferMapper::getInstance().importBuffer(rawHandle, width, height, layerCount,
                                                           format, usage, stride, outHandle);
}

status_t graphic_buffer_mapper_import_buffer_no_size(buffer_handle_t rawHandle,
        buffer_handle_t* outHandle)
{
    // adapted from GraphicBufferMapper::importBuffer() but skips validation part
    // needed to implement hybris_gralloc_retain() which doesn't have buffer information passed

    auto &mapper = GraphicBufferMapper::getInstance().getGrallocMapper();
    buffer_handle_t bufferHandle;
    status_t error = mapper.importBuffer(android::hardware::hidl_handle(rawHandle), &bufferHandle);
    if (error != android::NO_ERROR) {
        ALOGW("importBuffer(%p) failed: %d", rawHandle, error);
        return error;
    }

    *outHandle = bufferHandle;

    return android::NO_ERROR;
}

status_t graphic_buffer_mapper_free_buffer(buffer_handle_t handle) {
    return GraphicBufferMapper::getInstance().freeBuffer(handle);
}


status_t graphic_buffer_mapper_lock(buffer_handle_t handle, uint32_t usage, const ARect* bounds,
                                   void** vaddr, int32_t* outBytesPerPixel,
                                   int32_t* outBytesPerStride) {
    auto rect = android::Rect(bounds->left, bounds->top, bounds->right, bounds->bottom);
    return GraphicBufferMapper::getInstance().lock(handle, usage, rect,
                                                   vaddr, outBytesPerPixel, outBytesPerStride);
}

status_t graphic_buffer_mapper_unlock(buffer_handle_t handle)
{
    return GraphicBufferMapper::getInstance().unlock(handle);
}
#endif // ANDROID_VERSION_MAJOR>=10
