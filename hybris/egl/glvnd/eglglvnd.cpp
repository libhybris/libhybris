/*
 * Copyright (c) 2020 UBports Foundation.
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

#include "config.h"

#include <string>
#include <cstring>

#include <EGL/egl.h>
#include <glvnd/libeglabi.h>

#include "eglhybris.h"
#include "ws.h"
#include "egldispatchstubs.h"

// Implemented in egl.c
extern "C" EGLDisplay __eglHybrisGetPlatformDisplayCommon(EGLenum platform,
                            void *display_id, const EGLAttrib *attrib_list);

static const __EGLapiExports *__eglGLVNDApiExports = NULL;

/*
 * According to Mesa, the normal eglQueryString shouldn't include platform
 * extensions, so we use this to our advantage. We'll strip any platform
 * presents in normal string. Then, when we're queried supported platfrom
 * in glvnd's GetVendorString(), we check what a window system is loaded and
 * reply only the coresponding extensions.
 */

static std::string clientExtensionNoPlatform()
{
    std::string clientExts;
    const char * clientExtsOrigCStr = eglQueryString(EGL_NO_DISPLAY, EGL_EXTENSIONS);

    if (!clientExtsOrigCStr || clientExtsOrigCStr[0] == '\0')
        return clientExts;

    const std::string clientExtsOrig(clientExtsOrigCStr);

    std::size_t start = 0, end;
    do {
        end = clientExtsOrig.find(' ', start);
        // end might be string::npos, but removing start from ::npos shouldn't
        // cause any issue.
        auto ext = clientExtsOrig.substr(start, end - start);
        if (ext.find("_platform_") == std::string::npos) {
            if (clientExts.empty()) {
                clientExts = ext;
            } else {
                clientExts += ' ';
                clientExts += ext;
            }
        }

        start = end + 1; // Skip the space.
                         // If end is npos then this value won't be used anyway.
    } while (end != std::string::npos);

    return clientExts;
}

static const char * EGLAPIENTRY
__eglGLVNDQueryString(EGLDisplay dpy, EGLenum name)
{
    if (dpy == EGL_NO_DISPLAY && name == EGL_EXTENSIONS) {
        // Rely on C++11's static initialization guarantee.
        static const std::string clientExts = clientExtensionNoPlatform();

        // We can do this because if Android's EGL support client
        // extensions, it will at least have EGL_EXT_client_extensions.
        if (clientExts.empty())
            return NULL;
        else
            return clientExts.c_str();
    }

    // Use libhybris's normal eglQueryString() otherwise.
    return eglQueryString(dpy, name);
}

static const char *
__eglGLVNDGetVendorString(int name)
{
    if (name == __EGL_VENDOR_STRING_PLATFORM_EXTENSIONS) {
        return
            "EGL_KHR_platform_android"
#ifdef WANT_WAYLAND
            " EGL_EXT_platform_wayland EGL_KHR_platform_wayland"
#endif
            ;
    }

    return NULL;
}

static EGLBoolean __eglGLVNDgetSupportsAPI (EGLenum api)
{
    return api == EGL_OPENGL_ES_API;
}

static void *
__eglGLVNDGetProcAddress(const char *procName)
{
    if (strcmp(procName, "eglQueryString") == 0)
        return (void *) __eglGLVNDQueryString;
    else
        return (void *) eglGetProcAddress(procName);
}

/* For glvnd-supported libEGL, every symbols except for the entry point,
 * __egl_Main(), must be hidden. In our case, we pass -fvisibility=hidden
 * flag, then explicitly set this function visible.
 */

__attribute__ ((visibility ("default")))
EGLBoolean
__egl_Main(uint32_t version, const __EGLapiExports *exports,
     __EGLvendorInfo *vendor, __EGLapiImports *imports)
{
    if (EGL_VENDOR_ABI_GET_MAJOR_VERSION(version) !=
            EGL_VENDOR_ABI_MAJOR_VERSION)
        return EGL_FALSE;

    __eglGLVNDApiExports = exports;
    __eglInitDispatchStubs(exports);

    imports->getPlatformDisplay = __eglHybrisGetPlatformDisplayCommon;
    imports->getSupportsAPI = __eglGLVNDgetSupportsAPI;
    imports->getVendorString = __eglGLVNDGetVendorString;
    imports->getProcAddress = __eglGLVNDGetProcAddress;
    imports->getDispatchAddress = __eglDispatchFindDispatchFunction;
    imports->setDispatchIndex = __eglSetDispatchIndex;

    return EGL_TRUE;
}
