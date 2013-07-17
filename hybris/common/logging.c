
/*
 * Copyright (c) 2013 Thomas Perl <m@thp.io>
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


#include "logging.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

FILE *hybris_logging_target = NULL;

static enum hybris_log_level
hybris_minimum_log_level = HYBRIS_LOG_WARN;

static int
hybris_logging_initialized = 0;

static void
hybris_logging_initialize()
{
    const char *env = getenv("HYBRIS_LOGGING_LEVEL");

    if (env == NULL) {
        /* Nothing to do - use default level */
    } else if (strcmp(env, "debug") == 0) {
        hybris_minimum_log_level = HYBRIS_LOG_DEBUG;
    } else if (strcmp(env, "info") == 0) {
        hybris_minimum_log_level = HYBRIS_LOG_INFO;
    } else if (strcmp(env, "warn") == 0) {
        hybris_minimum_log_level = HYBRIS_LOG_WARN;
    } else if (strcmp(env, "error") == 0) {
        hybris_minimum_log_level = HYBRIS_LOG_ERROR;
    } else if (strcmp(env, "disabled") == 0) {
        hybris_minimum_log_level = HYBRIS_LOG_DISABLED;
    }

    env = getenv("HYBRIS_LOGGING_TARGET");
    if (env != NULL)
    {
        hybris_logging_target = fopen(env, "a");
    }
    if (hybris_logging_target == NULL)
        hybris_logging_target = stderr;
}

int
hybris_should_log(enum hybris_log_level level)
{
    /* Initialize logging level from environment */
    if (!hybris_logging_initialized) {
        hybris_logging_initialized = 1;
        hybris_logging_initialize();
    }

    return (level >= hybris_minimum_log_level);
}

void
hybris_set_log_level(enum hybris_log_level level)
{
    hybris_minimum_log_level = level;
}

