/*
 * Copyright (c) 2022 Jolla Ltd.
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
 */

#ifndef EGLNATIVEWINDOWBASE_H
#define EGLNATIVEWINDOWBASE_H

/* for ICS window.h */
#include <string.h>
#include <system/window.h>
#include <EGL/egl.h>

#include "nativewindowbase.h"

/**
 * @brief A Class to do common ANativeWindow initialization and thunk c-style
 *        callbacks into C++ method calls.
 **/
class EGLBaseNativeWindow : public BaseNativeWindow
{
public:
	operator EGLNativeWindowType()
	{
		EGLNativeWindowType ret = reinterpret_cast<EGLNativeWindowType>(static_cast<ANativeWindow *>(this));
		return ret;
	}
};

#endif
