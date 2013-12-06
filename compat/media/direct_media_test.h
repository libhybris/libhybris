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

#ifndef DIRECT_MEDIA_TEST_H_
#define DIRECT_MEDIA_TEST_H_

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <utils/threads.h>

namespace android {

class RenderInput;

class WindowRenderer
{
public:
    WindowRenderer(int width, int height);
    ~WindowRenderer();

private:
    // The GL thread functions
    static int threadStart(void* self);
    void glThread();

    // These variables are used to communicate between the GL thread and
    // other threads.
    Mutex mLock;
    Condition mCond;
    enum {
        CMD_IDLE,
        CMD_RENDER_INPUT,
        CMD_RESERVE_TEXTURE,
        CMD_DELETE_TEXTURE,
        CMD_QUIT,
    };
    int mThreadCmd;
    RenderInput* mThreadRenderInput;
    GLuint mThreadTextureId;
};
} // android

#endif
