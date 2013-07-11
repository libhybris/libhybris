/*
 * Copyright (c) 2012 Carsten Munk <carsten.munk@gmail.com>
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

#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "logging.h"

#define PROP_NAME_MAX 32

static char *find_key(const char *key)
{
	FILE *f = fopen("/system/build.prop", "r");
	char buf[1024];
	char *mkey, *value;

	if (!f)
		return NULL;

	while (fgets(buf, 1024, f) != NULL) {
		if (strchr(buf, '\r'))
			*(strchr(buf, '\r')) = '\0';
		if (strchr(buf, '\n'))
			*(strchr(buf, '\n')) = '\0';

		mkey = strtok(buf, "=");

		if (!mkey)
			continue;

		value = strtok(NULL, "=");
		if (!value)
			continue;

		if (strcmp(key, mkey) == 0) {
			fclose(f);
			return strdup(value);
		}
	}

	fclose(f);
	return NULL;
}

static char *find_key_kernel_cmdline(const char *key)
{
	char cmdline[1024];
	char *ptr;
	int fd;

	fd = open("/proc/cmdline", O_RDONLY);
	if (fd >= 0) {
		int n = read(fd, cmdline, 1023);
		if (n < 0) n = 0;

		/* get rid of trailing newline, it happens */
		if (n > 0 && cmdline[n-1] == '\n') n--;

		cmdline[n] = 0;
		close(fd);
	} else {
		cmdline[0] = 0;
	}

	ptr = cmdline;

	while (ptr && *ptr) {
		char *x = strchr(ptr, ' ');
		if (x != 0) *x++ = 0;

		char *name = ptr;
		ptr = x;

		char *value = strchr(name, '=');
		int name_len = strlen(name);

		if (value == 0) continue;
		*value++ = 0;
		if (name_len == 0) continue;

		if (!strncmp(name, "androidboot.", 12) && name_len > 12) {
			const char *boot_prop_name = name + 12;
			char prop[PROP_NAME_MAX];
			snprintf(prop, sizeof(prop), "ro.%s", boot_prop_name);
			if (strcmp(prop, key) == 0)
				return strdup(value);
		}
	}

	return NULL;
}

int property_get(const char *key, char *value, const char *default_value)
{
	char *ret = NULL; 

	//printf("property_get: %s\n", key);

	/* default */
	ret = find_key(key);

#if 0
 if (strcmp(key, "ro.kernel.qemu") == 0)
 {
    ret = "0";
 }  
 else if (strcmp(key, "ro.hardware") == 0)
 { 
    ret = "tenderloin";
 } 
 else if (strcmp(key, "ro.product.board") == 0)
 {
    ret = "tenderloin";
 }
 else if (strcmp(key, "ro.board.platform") == 0)
 { 
    ret = "msm8660";
 }
 else if (strcmp(key, "ro.arch") == 0)
 {
    ret = "armeabi";
 }
 else if (strcmp(key, "debug.composition.type") == 0)
 {
    ret = "c2d"; 
 }
 else if (strcmp(key, "debug.sf.hw") == 0)
 {
   ret = "1";
 }
 else if (strcmp(key, "debug.gr.numframebuffers") == 0)
 { 
   ret = "1"; 
 }  
#endif
	if (ret == NULL) {
		/* Property might be available via /proc/cmdline */
		ret = find_key_kernel_cmdline(key);
	}

	if (ret) {
		TRACE("found %s for %s\n", key, ret);
		strcpy(value, ret);
		free(ret);
		return strlen(value);
	} else if (default_value != NULL) {
		strcpy(value, default_value);
		return strlen(value);
	}

	return 0;
}

int property_set(const char *key, const char *value)
{
	printf("property_set: %s %s\n", key, value);
	TRACE("property_set: %s %s\n", key, value);
	return 0;
}

// vim:ts=4:sw=4:noexpandtab
