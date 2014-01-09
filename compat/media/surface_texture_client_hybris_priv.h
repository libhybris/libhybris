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

#include <gui/Surface.h>
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
#include <gui/SurfaceTextureClient.h>
#else
#include <gui/GLConsumer.h>
#endif

#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
struct _SurfaceTextureClientHybris : public android::SurfaceTextureClient
#else
struct _SurfaceTextureClientHybris : public android::Surface
#endif
{
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    _SurfaceTextureClientHybris();
#endif
    _SurfaceTextureClientHybris(const _SurfaceTextureClientHybris &stch);
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    _SurfaceTextureClientHybris(const android::sp<android::ISurfaceTexture> &st);
#else
    _SurfaceTextureClientHybris(const android::sp<android::IGraphicBufferProducer> &st);
#endif
    ~_SurfaceTextureClientHybris();

    /** Has a texture id or EGLNativeWindowType been passed in, meaning rendering will function? **/
    bool isReady() const;

public:
    int dequeueBuffer(ANativeWindowBuffer** buffer, int* fenceFd);
    int queueBuffer(ANativeWindowBuffer* buffer, int fenceFd);
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    void setISurfaceTexture(const android::sp<android::ISurfaceTexture>& surface_texture);
#else
    void setISurfaceTexture(const android::sp<android::IGraphicBufferProducer>& surface_texture);
#endif
    void setHardwareRendering(bool do_hardware_rendering);
    bool hardwareRendering();

    unsigned int refcount;
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    android::sp<android::SurfaceTexture> surface_texture;
#else
    android::sp<android::GLConsumer> surface_texture;
#endif

private:
    bool ready;
    bool hardware_rendering;
};

