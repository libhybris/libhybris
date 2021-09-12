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

#ifndef PLATFORM_COMMON_H_
#define PLATFORM_COMMON_H_

/* Needed for ICS window.h */
#include <string.h>
#include <system/window.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void hybris_dump_buffer_to_file(struct ANativeWindowBuffer *buf);

#ifdef __cplusplus
}
#endif

#endif
