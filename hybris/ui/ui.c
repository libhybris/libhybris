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

#include <dlfcn.h>
#include <stddef.h>

#include <android-version.h>
#if ANDROID_VERSION_MAJOR>=10
#include <android/rect.h>
#include <cutils/native_handle.h>
#endif

#include <hybris/common/binding.h>
#include <hybris/ui/ui.h>

#define COMPAT_LIBRARY_PATH		"libui_compat_layer.so"

HYBRIS_LIBRARY_INITIALIZE(ui, COMPAT_LIBRARY_PATH);
HYBRIS_LIRBARY_CHECK_SYMBOL(ui);

HYBRIS_IMPLEMENT_FUNCTION0(ui, struct graphic_buffer*, graphic_buffer_new);
HYBRIS_IMPLEMENT_FUNCTION4(ui, struct graphic_buffer*,
	graphic_buffer_new_sized, uint32_t, uint32_t, int32_t, uint32_t);
HYBRIS_IMPLEMENT_FUNCTION7(ui, struct graphic_buffer*, graphic_buffer_new_existing,
	uint32_t, uint32_t, int32_t, uint32_t, uint32_t, void*, bool);
HYBRIS_IMPLEMENT_VOID_FUNCTION1(ui, graphic_buffer_free, struct graphic_buffer*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, uint32_t, graphic_buffer_get_width,
	struct graphic_buffer*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, uint32_t, graphic_buffer_get_height,
	struct graphic_buffer*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, uint32_t, graphic_buffer_get_stride,
	struct graphic_buffer*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, uint32_t, graphic_buffer_get_usage,
	struct graphic_buffer*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, int32_t, graphic_buffer_get_pixel_format,
	struct graphic_buffer*);
HYBRIS_IMPLEMENT_FUNCTION5(ui, uint32_t, graphic_buffer_reallocate,
	struct graphic_buffer*, uint32_t, uint32_t, int32_t, uint32_t);
HYBRIS_IMPLEMENT_FUNCTION3(ui, uint32_t, graphic_buffer_lock,
	struct graphic_buffer*, uint32_t, void**);
HYBRIS_IMPLEMENT_FUNCTION1(ui, uint32_t, graphic_buffer_unlock,
	struct graphic_buffer*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, void*, graphic_buffer_get_native_buffer,
	struct graphic_buffer*);
#if ANDROID_VERSION_MAJOR==4 && ANDROID_VERSION_MINOR<=3
HYBRIS_IMPLEMENT_VOID_FUNCTION2(ui, graphic_buffer_set_index,
	struct graphic_buffer*, int);
HYBRIS_IMPLEMENT_FUNCTION1(ui, int, graphic_buffer_get_index,
	struct graphic_buffer*);
#endif
HYBRIS_IMPLEMENT_FUNCTION1(ui, int, graphic_buffer_init_check,
	struct graphic_buffer*);

#if ANDROID_VERSION_MAJOR>=10
HYBRIS_IMPLEMENT_FUNCTION9(ui, status_t, graphic_buffer_allocator_allocate,
                           uint32_t, uint32_t,
                           PixelFormat, uint32_t, uint64_t,
                           buffer_handle_t*, uint32_t*,
                           uint64_t, const char*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, status_t, graphic_buffer_allocator_free,
                           buffer_handle_t);

HYBRIS_IMPLEMENT_FUNCTION8(ui, status_t, graphic_buffer_mapper_import_buffer,
                           buffer_handle_t, uint32_t, uint32_t, uint32_t,
                           PixelFormat, uint64_t, uint32_t,
                           buffer_handle_t*);

HYBRIS_IMPLEMENT_FUNCTION2(ui, status_t, graphic_buffer_mapper_import_buffer_no_size,
                           buffer_handle_t, buffer_handle_t*);

HYBRIS_IMPLEMENT_FUNCTION1(ui, status_t, graphic_buffer_mapper_free_buffer,
                           buffer_handle_t);

HYBRIS_IMPLEMENT_FUNCTION6(ui, status_t, graphic_buffer_mapper_lock,
                           buffer_handle_t, uint32_t, const ARect*,
                           void**, int32_t*, int32_t*);
HYBRIS_IMPLEMENT_FUNCTION1(ui, status_t, graphic_buffer_mapper_unlock,
                           buffer_handle_t);
#endif
