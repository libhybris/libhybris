/*
 * Copyright (C) 2018 TheKit <nekit1000@gmail.com>
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <mutex>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <cutils/log.h>
#include <sync/sync.h>

#include "hybris-gralloc.h"
#include <hwcomposer_window.h>

#include "hwc2_compatibility_layer.h"

const char vertex_src [] =
"                                        \
   attribute vec4        position;       \
   varying mediump vec2  pos;            \
   uniform vec4          offset;         \
                                         \
   void main()                           \
   {                                     \
      gl_Position = position + offset;   \
      pos = position.xy;                 \
   }                                     \
";


const char fragment_src [] =
"                                                      \
   varying mediump vec2    pos;                        \
   uniform mediump float   phase;                      \
                                                       \
   void  main()                                        \
   {                                                   \
      gl_FragColor  =  vec4( 1., 0.9, 0.7, 1.0 ) *     \
        cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y)   \
             + atan(pos.y,pos.x) - phase );            \
   }                                                   \
";

GLuint load_shader(const char *shader_source, GLenum type)
{
    GLuint  shader = glCreateShader(type);

    glShaderSource(shader, 1, &shader_source, NULL);
    glCompileShader(shader);

    return shader;
}


GLfloat norm_x    =  0.0;
GLfloat norm_y    =  0.0;
GLfloat offset_x  =  0.0;
GLfloat offset_y  =  0.0;
GLfloat p1_pos_x  =  0.0;
GLfloat p1_pos_y  =  0.0;

GLint phase_loc;
GLint offset_loc;
GLint position_loc;

const float vertexArray[] = {
    0.0,  1.0,  0.0,
    -1.,  0.0,  0.0,
    0.0, -1.0,  0.0,
    1.,  0.0,  0.0,
    0.0,  1.,  0.0
};

std::mutex hotplugMutex;
std::condition_variable hotplugCv;
hwc2_compat_device_t* hwcDevice;

class HWComposer : public HWComposerNativeWindow
{
    private:
        hwc2_compat_layer_t *layer;
        hwc2_compat_display_t *hwcDisplay;
        int lastPresentFence = -1;
    protected:
        void present(HWComposerNativeWindowBuffer *buffer);

    public:

        HWComposer(unsigned int width, unsigned int height, unsigned int format,
                hwc2_compat_display_t *display, hwc2_compat_layer_t *layer);
        void set();
};

HWComposer::HWComposer(unsigned int width, unsigned int height,
                    unsigned int format, hwc2_compat_display_t* display,
                    hwc2_compat_layer_t *layer) :
                    HWComposerNativeWindow(width, height, format)
{
    this->layer = layer;
    this->hwcDisplay = display;
}

void HWComposer::present(HWComposerNativeWindowBuffer *buffer)
{
    uint32_t numTypes = 0;
    uint32_t numRequests = 0;
    hwc2_display_t displayId = 0;

    hwc2_error_t error = hwc2_compat_display_validate(hwcDisplay, &numTypes,
                                                      &numRequests);

    if (error != HWC2_ERROR_NONE && error != HWC2_ERROR_HAS_CHANGES) {
        ALOGE("prepare: validate failed for display %d: %s (%d)",
              (uint32_t) displayId,
              to_string(static_cast<HWC2::Error>(error)).c_str(), error);
        return;
    }

    if (numTypes || numRequests) {
        ALOGE("prepare: validate required changes for display %d: %s (%d)",
              (uint32_t) displayId,
              to_string(static_cast<HWC2::Error>(error)).c_str(), error);
        return;
    }

    error = hwc2_compat_display_accept_changes(hwcDisplay);
    if (error != HWC2_ERROR_NONE) {
        ALOGE("prepare: acceptChanges failed: %s",
              to_string(static_cast<HWC2::Error>(error)).c_str());
        return;
    }

    hwc2_compat_display_set_client_target(hwcDisplay, /* slot */0, buffer,
                                          getFenceBufferFd(buffer),
                                          HAL_DATASPACE_UNKNOWN);

    int presentFence;
    hwc2_compat_display_present(hwcDisplay, &presentFence);

    if (error != HWC2_ERROR_NONE) {
        ALOGE("presentAndGetReleaseFences: failed for display %d: %s (%d)",
              (uint32_t) displayId,
              to_string(static_cast<HWC2::Error>(error)).c_str(), error);
        return;
    }

    hwc2_compat_out_fences_t* fences;
    error = hwc2_compat_display_get_release_fences(
        hwcDisplay, &fences);

    if (error != HWC2_ERROR_NONE) {
        ALOGE("presentAndGetReleaseFences: Failed to get release fences "
              "for display %d: %s (%d)",
              (uint32_t) displayId, to_string(static_cast<HWC2::Error>(error)).c_str(),
              error);
        return;
    }

    int fenceFd = hwc2_compat_out_fences_get_fence(fences, layer);
    if (fenceFd != -1)
        setFenceBufferFd(buffer, fenceFd);

    hwc2_compat_out_fences_destroy(fences);

    if (lastPresentFence != -1) {
        sync_wait(lastPresentFence, -1);
        close(lastPresentFence);
    }
    lastPresentFence = presentFence;
}

void onVsyncReceived(HWC2EventListener* listener, int32_t sequenceId,
                     hwc2_display_t display, int64_t timestamp)
{
}

void onHotplugReceived(HWC2EventListener* listener, int32_t sequenceId,
                       hwc2_display_t display, bool connected,
                       bool primaryDisplay)
{
    ALOGI("onHotplugReceived(%d, %" PRIu64 ", %s, %s)",
        sequenceId, display,
        connected ?
                "connected" : "disconnected",
        primaryDisplay ? "primary" : "external");

    {
        std::lock_guard<std::mutex> lock(hotplugMutex);
        hwc2_compat_device_on_hotplug(hwcDevice, display, connected);
    }

    hotplugCv.notify_all();
}

void onRefreshReceived(HWC2EventListener* listener,
                       int32_t sequenceId, hwc2_display_t display)
{
}

HWC2EventListener eventListener = {
    &onVsyncReceived,
    &onHotplugReceived,
    &onRefreshReceived
};

int main()
{
    EGLDisplay display;
    EGLConfig ecfg;
    EGLint num_config;
    EGLint attr[] = {       // some attributes to set up our egl-interface
        EGL_BUFFER_SIZE, 32,
        EGL_RENDERABLE_TYPE,
        EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLSurface surface;
    EGLint ctxattr[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    EGLContext context;

    EGLBoolean rv;

    int composerSequenceId = 0;

    hwcDevice = hwc2_compat_device_new(false);
    assert(hwcDevice);

    hwc2_compat_device_register_callback(hwcDevice, &eventListener,
                                         composerSequenceId);

    std::unique_lock<std::mutex> lock(hotplugMutex);
    hwc2_compat_display_t* hwcDisplay;
    while (!(hwcDisplay = hwc2_compat_device_get_display_by_id(hwcDevice, 0))) {
        /* Wait at most 5s for hotplug events */
        hotplugCv.wait_for(lock, std::chrono::seconds(5));
    }
    hotplugMutex.unlock();
    assert(hwcDisplay);

    hwc2_compat_display_set_power_mode(hwcDisplay, HWC2_POWER_MODE_ON);

    std::shared_ptr<HWC2DisplayConfig> config = {
        hwc2_compat_display_get_active_config(hwcDisplay), free };

    printf("width: %i height: %i\n", config->width, config->height);

    hwc2_compat_layer_t* layer = hwc2_compat_display_create_layer(hwcDisplay);

    hwc2_compat_layer_set_composition_type(layer, HWC2_COMPOSITION_CLIENT);
    hwc2_compat_layer_set_blend_mode(layer, HWC2_BLEND_MODE_NONE);
    hwc2_compat_layer_set_source_crop(layer, 0.0f, 0.0f, config->width,
                                      config->height);
    hwc2_compat_layer_set_display_frame(layer, 0, 0, config->width,
                                        config->height);
    hwc2_compat_layer_set_visible_region(layer, 0, 0, config->width,
                                         config->height);

    HWComposer *win = new HWComposer(config->width, config->height,
                                     HAL_PIXEL_FORMAT_RGBA_8888, hwcDisplay,
                                     layer);
    printf("created native window\n");
    hybris_gralloc_initialize(0);

    display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    assert(eglGetError() == EGL_SUCCESS);
    assert(display != EGL_NO_DISPLAY);

    rv = eglInitialize(display, 0, 0);
    assert(eglGetError() == EGL_SUCCESS);
    assert(rv == EGL_TRUE);

    rv = eglChooseConfig((EGLDisplay) display, attr, &ecfg, 1, &num_config);
    assert(eglGetError() == EGL_SUCCESS);
    assert(rv == EGL_TRUE);

    surface = eglCreateWindowSurface((EGLDisplay) display, ecfg,
        (EGLNativeWindowType) static_cast<ANativeWindow *> (win), NULL);
    assert(eglGetError() == EGL_SUCCESS);
    assert(surface != EGL_NO_SURFACE);

    context = eglCreateContext((EGLDisplay) display, ecfg, EGL_NO_CONTEXT,
                               ctxattr);
    assert(eglGetError() == EGL_SUCCESS);
    assert(context != EGL_NO_CONTEXT);

    assert(eglMakeCurrent((EGLDisplay) display, surface, surface,
                          context) == EGL_TRUE);

    const char *version = (const char *)glGetString(GL_VERSION);
    assert(version);
    printf("%s\n",version);

    GLuint vertexShader = load_shader (vertex_src, GL_VERTEX_SHADER);
    GLuint fragmentShader = load_shader(fragment_src, GL_FRAGMENT_SHADER);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);

    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    position_loc = glGetAttribLocation(shaderProgram, "position");
    phase_loc = glGetUniformLocation(shaderProgram, "phase");
    offset_loc = glGetUniformLocation(shaderProgram, "offset");

    if (position_loc < 0 ||  phase_loc < 0 || offset_loc < 0) {
        return 1;
    }

    glClearColor (1., 1., 1., 1.); // background color
    float phase = 0;
    int i, oldretire = -1, oldrelease = -1, oldrelease2 = -1;
    for (i=0; i<60*60; ++i) {
        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1f (phase_loc, phase);
        phase = fmodf (phase + 0.5f, 2.f * 3.141f);

        glUniform4f(offset_loc, offset_x, offset_y, 0.0, 0.0);

        glVertexAttribPointer(position_loc, 3, GL_FLOAT,
                              GL_FALSE, 0, vertexArray);
        glEnableVertexAttribArray (position_loc);
        glDrawArrays (GL_TRIANGLE_STRIP, 0, 5);

        eglSwapBuffers ((EGLDisplay) display, surface);
    }

    printf("stop\n");

    return 0;
}
