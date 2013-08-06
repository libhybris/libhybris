/*
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
 *               2013 Simon Busch <morphis@gravedo.de>
 *               2008 The Android Open Source Project
 *               2013 Canonical Ltd
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

#ifndef PROPERTIES_H_
#define PROPERTIES_H_

#include <stdint.h>
#include <unistd.h>
#include <stdint.h>

/* Based on Android */
#define PROP_SERVICE_NAME "property_service"

#define PROP_NAME_MAX 32
#define PROP_VALUE_MAX 92

/* Only SETPROP is defined by Android, for GETPROP and LISTPROP to work
 * an extended Android init service needs to be in place */
#define PROP_MSG_SETPROP 1
#define PROP_MSG_GETPROP 2
#define PROP_MSG_LISTPROP 3

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct prop_msg_s {
		unsigned cmd;
		char name[PROP_NAME_MAX];
		char value[PROP_VALUE_MAX];
	} prop_msg_t;

	int property_set(const char *key, const char *value);
	int property_get(const char *key, char *value, const char *default_value);
	int property_list(void (*propfn)(const char *key, const char *value, void *cookie), void *cookie);

#ifdef __cplusplus
}
#endif

#endif // PROPERTIES_H_
