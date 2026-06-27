/*
 * Copyright (c) 2025 Nikita Ukhrenkov <thekit@disroot.org>
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

#include "tls_patcher.h"
#include "logging.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>

extern void* _hybris_hook___get_tls_hooks(void);

/* Architecture-specific patch function */
extern void hybris_patch_tls_arch(void* segment_addr, size_t segment_size, int tls_offset);

/* Calculate offset from thread pointer to our TLS area */
static int hybris_calculate_tls_offset(void) {
    void* tp = __builtin_thread_pointer();
    void* tls_area = _hybris_hook___get_tls_hooks();
    return (int)((uintptr_t)tls_area - (uintptr_t)tp) / sizeof(uintptr_t);
}

/* Extract filename from path without modifying input */
static const char* get_basename(const char* path) {
    const char* base = strrchr(path, '/');
    return base ? base + 1 : path;
}

/* Check if library should be patched based on HYBRIS_PATCH_TLS value */
static int should_patch_library(const char* library_name) {
    static const char* patch_tls = NULL;
    static int init_done = 0;

    /* Initialize on first call */
    if (!init_done) {
        patch_tls = getenv("HYBRIS_PATCH_TLS");
        init_done = 1;
    }

    /* No environment variable set - do nothing */
    if (!patch_tls) {
        return 0;
    }

    /* Simple enable/disable */
    if (patch_tls[0] == '0' || patch_tls[0] == '1') {
        return patch_tls[0] == '1';
    }

    /* Check if library basename is in colon-separated list */
    const char *start = patch_tls;
    const char *name = get_basename(library_name);
    size_t name_len = strlen(name);

    while (start && *start) {
        const char *end = strchr(start, ':');
        size_t len = end ? (size_t)(end - start) : strlen(start);

        if (len == name_len && strncmp(start, name, len) == 0) {
            return 1;
        }

        start = end ? end + 1 : NULL;
    }

    return 0;
}

void hybris_patch_tls(void* segment_addr, size_t segment_size, const char* library_name) {
    if (!should_patch_library(library_name)) {
        return;
    }

    HYBRIS_DEBUG_LOG(HOOKS, "Patching TLS accesses in %s", library_name);

    int tls_offset = hybris_calculate_tls_offset();
    HYBRIS_DEBUG_LOG(HOOKS, "Offset from thread pointer to hybris tls_space (in words): %d",
        tls_offset);

    hybris_patch_tls_arch(segment_addr, segment_size, tls_offset);
}

#ifdef __aarch64__
#include "tls_patcher_aarch64.c"
#endif