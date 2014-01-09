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
 */

#ifndef CAMERA_CONTROL_H_
#define CAMERA_CONTROL_H_

#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
#include <gui/SurfaceTexture.h>
#else
#include <gui/GLConsumer.h>
#endif

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CameraControlListener;

struct CameraControl : public android::CameraListener,
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    public android::SurfaceTexture::FrameAvailableListener
#else
    public android::GLConsumer::FrameAvailableListener
#endif
{
    android::Mutex guard;
    CameraControlListener* listener;
    android::sp<android::Camera> camera;
    android::CameraParameters camera_parameters;
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=2
    android::sp<android::SurfaceTexture> preview_texture;
#else
    android::sp<android::GLConsumer> preview_texture;
#endif
    // From android::SurfaceTexture/GLConsumer::FrameAvailableListener
    void onFrameAvailable();

    // From android::CameraListener
    void notify(int32_t msg_type, int32_t ext1, int32_t ext2);

    void postData(
        int32_t msg_type,
        const android::sp<android::IMemory>& data,
        camera_frame_metadata_t* metadata);

    void postDataTimestamp(
        nsecs_t timestamp,
        int32_t msg_type,
        const android::sp<android::IMemory>& data);
};


#ifdef __cplusplus
}
#endif

#endif // CAMERA_CONTROL_H_
