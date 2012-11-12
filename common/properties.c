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
	if (ret) {
		printf("found %s for %s\n", key, ret);
	}
	if (ret == NULL) {
		if (default_value != NULL) {
			strcpy(value, default_value);
			return strlen(value);
		}
		else {
			return 0;
		}
	}
	if (ret) {
		strcpy(value, ret);
		free(ret);
		return strlen(value);
	}
	else {
		return 0;
	}
}

int property_set(const char *key, const char *value)
{
	printf("property_set: %s %s\n", key, value);
}

// vim:ts=4:sw=4:noexpandtab
