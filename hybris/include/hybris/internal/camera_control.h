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
#include <gui/SurfaceTexture.h>

#include <stdint.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

struct CameraControlListener;

struct CameraControl : public android::CameraListener,
    public android::SurfaceTexture::FrameAvailableListener
{
    android::Mutex guard;
    CameraControlListener* listener;
    android::sp<android::Camera> camera;
    android::CameraParameters camera_parameters;
    android::sp<android::SurfaceTexture> preview_texture;

    // From android::SurfaceTexture::FrameAvailableListener
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
