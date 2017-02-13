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

#include <map>
#include <pthread.h>
#include "pthread_helpers.h"

static std::map<pthread_t, pid_t> pthread_pid_map;
static pthread_mutex_t map_lock = PTHREAD_MUTEX_INITIALIZER;

extern "C" void init_pthread_helpers()
{
    pthread_mutex_init(&map_lock, NULL);
}

extern "C" void remove_pthread_pid_mapping(pid_t pid)
{
    pthread_mutex_lock(&map_lock);

    for(std::map<pthread_t, pid_t>::iterator it = pthread_pid_map.begin(); it != pthread_pid_map.end(); it++)
    {
         if(it->second == pid)
         {
              pthread_pid_map.erase(it);
              break;
         }
    }

    pthread_mutex_unlock(&map_lock);
}

extern "C" void add_pthread_pid_mapping(pthread_t thread, pid_t pid)
{
    pthread_mutex_lock(&map_lock);

    pthread_pid_map[thread] = pid;

    pthread_mutex_unlock(&map_lock);
}

extern "C" pid_t get_pid_for_pthread(pthread_t thread)
{
    pthread_mutex_lock(&map_lock);

    pid_t p = pthread_pid_map[thread];

    pthread_mutex_unlock(&map_lock);

    return p;
}

