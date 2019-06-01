/*
 * Copyright (c) 2018 Jolla Ltd. <franz.haider@jolla.com>
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

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

#include <hybris/common/binding.h>

static void *libcutils = NULL;

// These may point to the libhybris implementation or to the bionic implementation, depending on the linker being used.
static int (*bionic_property_list)(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie) = NULL;
static int (*bionic_property_get)(const char *key, char *value, const char *default_value) = NULL;
static int (*bionic_property_set)(const char *key, const char *value) = NULL;

static void unload_libcutils(void)
{
    if (libcutils) {
        android_dlclose(libcutils);
    }
}

#define PROPERTY_DLSYM(func) {*(void **)(&bionic_##func) = (void*)android_dlsym(libcutils, #func); \
                              if (!bionic_##func) { \
                                  fprintf(stderr, "failed to load " #func " from bionic libcutils\n"); \
                                  abort(); \
                              }}

static void ensure_bionic_properties_initialized(void)
{
    if (!libcutils) {
        libcutils = android_dlopen("libcutils.so", RTLD_LAZY);
        if (libcutils) {
            PROPERTY_DLSYM(property_get);
            PROPERTY_DLSYM(property_set);
            PROPERTY_DLSYM(property_list);
            atexit(unload_libcutils);
        } else {
            fprintf(stderr, "failed to load bionic libc.so\n");
            abort();
        }
    }
}

int property_list(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie)
{
    ensure_bionic_properties_initialized();

    return bionic_property_list(propfn, cookie);
}

int property_get(const char *key, char *value, const char *default_value)
{
    ensure_bionic_properties_initialized();

    return bionic_property_get(key, value, default_value);
}

int property_set(const char *key, const char *value)
{
    ensure_bionic_properties_initialized();

    return bionic_property_set(key, value);
}

