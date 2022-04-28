/*
 * Copyright (c) 2013-2022 Jolla Ltd.
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

#ifndef __WRAPPERS_H__
#define __WRAPPERS_H__

#ifdef __cplusplus
extern "C" {
#endif

enum wrapper_type_t {
    WRAPPER_UNHOOKED = 0,
    WRAPPER_DYNHOOK,
    WRAPPER_HOOKED
};

void *create_wrapper(const char *symbol, void *function, int wrapper_type);
void release_all_wrappers();

int wrappers_enabled();

// taken from <linux/elf.h>
#define ELF_ST_TYPE(x)       (((unsigned int) x) & 0xf)

#ifdef __cplusplus
}
#endif

#endif

