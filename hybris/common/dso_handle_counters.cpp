/*
 * Copyright (C) 2020 UBports Foundation
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

#include <unordered_map>
#include <mutex>

#include <dlfcn.h>
#include <hybris/common/dlfcn.h>

#include "logging.h"
#define LOGD(message, ...) HYBRIS_DEBUG_LOG(HOOKS, message, ##__VA_ARGS__)

#include "dso_handle_counters.h"

/*
 * This file implements a reference-counting system for dso_handle symbols
 * having outstanding thread-local variables. This is inspired by an
 * implementation in Android's linker, but instead of using internal Linker
 * mechanism, this file uses public linker functions to find the library and
 * dlload() it to increment reference count.
 *
 * This file is implemented in C++ so that I can use C++'s unordered_map.
 *
 * See https://github.com/aosp-mirror/platform_bionic/commit/55547db4345ee692b9cfe727c97dd860ed8263f8
 */

struct DsoHandleInfo {
    size_t counter;
    void *dl_handle;
};

static std::unordered_map<void*, struct DsoHandleInfo *> g_dso_handle_counters;
static std::mutex g_dso_handle_counters_mutex;

void __hybris_add_thread_local_dtor(void* dso_handle)
{
    if (dso_handle == nullptr) {
        return;
    }

    std::lock_guard<std::mutex> l(g_dso_handle_counters_mutex);

    auto it = g_dso_handle_counters.find(dso_handle);
    if (it != g_dso_handle_counters.end()) {
        ++it->second->counter;
    } else {
        Dl_info dso_symbol_info;

        if (hybris_dladdr(dso_handle, &dso_symbol_info)) {
            struct DsoHandleInfo *info = new struct DsoHandleInfo;
            info->counter = 1U;
            info->dl_handle = hybris_dlopen(dso_symbol_info.dli_fname, /* flags */ 0);

            g_dso_handle_counters[dso_handle] = info;
        } else {
            LOGD(
                "__hybris_add_thread_local_dtor: Couldn't find a library by dso_handle=%p",
                dso_handle);
            // Nothing we can do.
        }
    }
}

void __hybris_remove_thread_local_dtor(void* dso_handle)
{
    if (dso_handle == nullptr) {
        return;
    }

    // Do dlclose() outside the lock to avoid deadlock.
    struct DsoHandleInfo *info = nullptr;

    {
        std::lock_guard<std::mutex> l(g_dso_handle_counters_mutex);

        auto it = g_dso_handle_counters.find(dso_handle);

        if (it == g_dso_handle_counters.end()) {
            LOGD(
                "__hybris_remove_thread_local_dtor: Couldn't find a library by dso_handle=%p",
                dso_handle);
            return;
        }

        if (--it->second->counter == 0) {
            info = it->second;
            g_dso_handle_counters.erase(it);
        }
    }

    if (info) {
        hybris_dlclose(info->dl_handle);
        delete info;
    }
}
