/*
 * Copyright (C) 2014 Canonical Ltd
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

#define LOG_NDEBUG 0
#undef LOG_TAG
#define LOG_TAG "CameraServiceCompatLayer"

#include "media_recorder_factory.h"
#include "media_recorder.h"

#include <media/camera_record_service.h>
#include <CameraService.h>

#include <signal.h>

#define REPORT_FUNCTION() ALOGV("%s \n", __PRETTY_FUNCTION__)

using namespace android;

/*!
 * \brief main() instantiates the MediaRecorderFactory Binder server and the CameraService
 */
int main(int argc, char** argv)
{
    signal(SIGPIPE, SIG_IGN);

    ALOGV("Starting camera services (MediaRecorderFactory, CameraRecordService & CameraService)");

    // Instantiate the in-process MediaRecorderFactory which is responsible
    // for creating a new IMediaRecorder (MediaRecorder) instance over Binder
    MediaRecorderFactory::instantiate();
    // Enable audio recording for camera recording
    CameraRecordService::instantiate();
    CameraService::instantiate();
    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();
}
