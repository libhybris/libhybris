/*
 * Copyright (c) 2012 Simon Busch <morphis@gravedo.de>
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

#include <dlfcn.h>
#include <stddef.h>
#include <errno.h>
#include <android/hardware/hardware.h>
#include <hybris/internal/binding.h>

static void *_libhardware = NULL;

static int (*_hw_get_module)(const char *id, const struct hw_module_t **module) = NULL;

static int (*_hw_get_module_by_class)(const char *class_id, const char *inst, const struct hw_module_t **module) = NULL;

#define HARDWARE_DLSYM(fptr, sym) do { if (*(fptr) == NULL) { *(fptr) = (void *) android_dlsym(_libhardware, sym); } } while (0)

static void *_init_lib_hardware()
{
	if (!_libhardware)
		_libhardware = (void *) android_dlopen("/system/lib/libhardware.so", RTLD_LAZY);
	return _libhardware;
}

int hw_get_module(const char *id, const struct hw_module_t **module)
{
	if (!_init_lib_hardware())
		return -EINVAL;

	HARDWARE_DLSYM(&_hw_get_module, "hw_get_module");
	return (*_hw_get_module)(id, module);
}

int hw_get_module_by_class(const char *class_id, const char *inst,
                           const struct hw_module_t **module)
{
	if (!_init_lib_hardware())
		return -EINVAL;

	HARDWARE_DLSYM(&_hw_get_module_by_class, "hw_get_module_by_class");
	return (*_hw_get_module_by_class)(class_id, inst, module);
}

// vim:ts=4:sw=4:noexpandtab
