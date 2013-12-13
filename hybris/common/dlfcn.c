/*
 * Copyright (c) 2013 Intel Corporation
 * Copyright (c) 2013 Nomovok Ltd
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

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

#include "logging.h"

#include <../include/hybris/dlfcn/dlfcn.h>
#include <../include/hybris/internal/binding.h>

#define HWOVERRIDE_PATH "/system/lib/hw/"
#define DEFAULTSO ".default.so"
#define SOSUFFIX ".so"
#define HWVENDOR_PATH "/system/vendor/lib/hw/"

void *hybris_dlopen(const char *filename, int flag)
{
    return android_dlopen(filename,flag);
}


void *hybris_dlsym(void *handle, const char *symbol)
{
    return android_dlsym(handle,symbol);
}


int   hybris_dlclose(void *handle)
{
    return android_dlclose(handle);
}


char *hybris_dlerror(void)
{
    return android_dlerror();
}

static char *find_override(const char *filename)
{
    /*
     * The HWOVERRIDE_PATH is a special directory. Libraries in that directory
     * may be overridden by vendor specific implementation. If filename begins
     * with HWOVERRIDE_PATH and ends with .default.so search for override
     */
    int filename_len = strlen(filename);
    int hwoverride_len = strlen(HWOVERRIDE_PATH);
    int defaultso_len = strlen(DEFAULTSO);
    int basename_len = filename_len - hwoverride_len - defaultso_len;
    if (filename_len > hwoverride_len + defaultso_len
            && !strncmp(filename, HWOVERRIDE_PATH, hwoverride_len)
            && !strncmp(filename + hwoverride_len + basename_len, DEFAULTSO,
                        defaultso_len)) {

        /* try to find overriding library vendor hw library directory */
        const char *basename = filename + hwoverride_len;
        DIR *vendorhwdir = NULL;
        struct dirent *entry;
        vendorhwdir = opendir(HWVENDOR_PATH);

        if (vendorhwdir == NULL)
            return NULL;

        while (entry = readdir(vendorhwdir)) {

            /*
             * file in vendor hw directory needs to have .so suffix and have
             * same base name.
             */
            int sosuffix_len = strlen(SOSUFFIX);
            int entry_len = strlen(entry->d_name);
            char *entry_suffix = entry->d_name + (entry_len - sosuffix_len);

            if (entry_len > sosuffix_len + basename_len
                    && !strncmp(entry->d_name, basename, basename_len)
                    && !strncmp(entry_suffix, SOSUFFIX, strlen(SOSUFFIX))) {

                /* create new path. Add vendor lib dir + filename */
                int hwvendor_path_len = strlen(HWVENDOR_PATH);
                int len = hwvendor_path_len + entry_len + 1;
                char *override = malloc(len);
                if (!override)
                    return NULL;
                strncpy(override, HWVENDOR_PATH, hwvendor_path_len);
                strncpy(override + hwvendor_path_len, entry->d_name, entry_len);
                override[len] = '\0';
                HYBRIS_INFO("Override HW library %s -> %s\n", filename,
                            override);
                return override;
            }
        }
    }
    return NULL;
}

void *hybris_dlopen_resolvefirst(const char* filename, int flag)
{
    char *override = NULL;
    void *ret;
    if (filename == NULL)
        return NULL;

    if (!getenv("HYBRIS_NO_HWOVERRIDE"))
        override = find_override(filename);
    ret = android_dlopen(override ? override : filename, flag);
    if (override)
            free((void *)override);
    return ret;
}

// vim: noai:ts=4:sw=4:ss=4:expandtab
