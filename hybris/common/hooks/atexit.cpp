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

/*
 * The idea of hooking __cxa_atexit is to make sure that atexit hooks from
 * Android libraries are not called after we're unloaded.
 *
 * Imagine the following:
 *   - A Glibc library is dlopen()'ed, which loads libhybris-common.so
 *   - That library dlopen() an Android library which registers an atexit hook.
 *     That atexit hook uses a hooked symbol from libhybris-common.so
 *   - The Glibc library is dlclose()'ed, however, it doesn't register an
 *     atexit() hook to dlclose() the Android library, leaving atexit hooks
 *     inside Glibc's list.
 *   - As that Glibc library is the only user of libhybris-common.so, it gets
 *     unloaded too.
 *   - When the program exits, Glibc will call the Android library's atexit
 *     hook. However, as libhybris-common.so is already unloaded, the hook will
 *     segfault when trying to call the hooked function.
 *
 * By calling the hooks functions by the time we (libhybris-common.so) are
 * exiting, we can avoid the segfault, although we still have the lib lying
 * inside the address space.
 *
 * Preferbly, I would like to dlclose() all loaded libraries when we're
 * unloaded, although I haven't seen a way to do so.
 */

#include <mutex>
#include <set>

#include "atexit.h"

extern "C" int __cxa_atexit(void (*)(void*), void*, void*);
extern "C" void __cxa_finalize(void * d);

class SeenDSOCleanup {
public:
    int hook_atexit(void (*func)(void*), void * arg, void * dso_handle)
    {
        if (dso_handle == NULL)
            // Shouldn't happen, but just in case.
            return __cxa_atexit(func, arg, dso_handle);

        {
            std::lock_guard<std::mutex> l(seen_dso_handles_mutex);
            // std::set should do nothing if the dso is seen.
            seen_dso_handles.insert(dso_handle);
        }

        return __cxa_atexit(func, arg, dso_handle);
    }

    void hook_finalize(void * dso_handle)
    {
        if (dso_handle == NULL)
            return;

        {
            std::lock_guard<std::mutex> l(seen_dso_handles_mutex);

            // erase() returns number of elements removed.
            if (seen_dso_handles.erase(dso_handle) == 0)
                // Probably been cleaned by other invocation or didn't register
                // any hook.
                return;
        }

        __cxa_finalize(dso_handle);
    }

    ~SeenDSOCleanup()
    {
        for (auto dso_handle: seen_dso_handles)
            // Glibc releases a lock before calling this, so it's safe to call
            // this here.
            __cxa_finalize(dso_handle);
    }
private:
    std::set<void *> seen_dso_handles;
    std::mutex seen_dso_handles_mutex;
};

static class SeenDSOCleanup seen_dso_cleanup;

int _hybris_hook___cxa_atexit(void (*func)(void*), void * arg, void * dso_handle)
{
    return seen_dso_cleanup.hook_atexit(func, arg, dso_handle);
}

void _hybris_hook___cxa_finalize(void * d)
{
    return seen_dso_cleanup.hook_finalize(d);
}
