/*
 * Copyright (c) 2017 Franz-Josef Haider <f_haider@gmx.at>
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

#ifndef __PTHREAD_HELPERS_H__
#define __PTHREAD_HELPERS_H__

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

void init_pthread_helpers();
void remove_pthread_pid_mapping(pid_t pid);
void add_pthread_pid_mapping(pthread_t thread, pid_t pid);
pid_t get_pid_for_pthread(pthread_t thread);

#ifdef __cplusplus
};
#endif

#endif

