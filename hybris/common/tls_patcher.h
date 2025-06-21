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

#ifndef TLS_PATCHER_H
#define TLS_PATCHER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*hybris_tls_patcher_t)(void* segment_addr, size_t segment_size, const char* library_name);

/* Patches TLS accesses in the given segment at runtime */
void hybris_patch_tls(void* segment_addr, size_t segment_size, const char* library_name);

#ifdef __cplusplus
}
#endif

#endif /* TLS_PATCHER_H */
