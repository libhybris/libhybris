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

#include <android-config.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <string.h>
#include <stdio.h>
#include "config.h"
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "logging.h"

#include "windowbuffer.h"

#include <hybris/gralloc/gralloc.h>

extern "C" void hybris_dump_buffer_to_file(ANativeWindowBuffer *buf)
{
	static int cnt = 0;
	void *vaddr;
	int ret = hybris_gralloc_lock(buf->handle, (uint32_t)buf->usage, 0, 0, buf->width, buf->height, &vaddr);
	(void)ret;
	TRACE("buf:%p gralloc lock returns %i", buf, ret);
	TRACE("buf:%p lock to vaddr %p", buf, vaddr);
	char b[1024];
	int bytes_pp = 0;

	if (buf->format == HAL_PIXEL_FORMAT_RGBA_8888 || buf->format == HAL_PIXEL_FORMAT_BGRA_8888)
		bytes_pp = 4;
	else if (buf->format == HAL_PIXEL_FORMAT_RGB_565)
		bytes_pp = 2;

	snprintf(b, 1020, "vaddr.%p.%p.%i.%is%ix%ix%i", buf, vaddr, cnt, buf->width, buf->stride, buf->height, bytes_pp);
	cnt++;
	int fd = ::open(b, O_WRONLY|O_CREAT, S_IRWXU);
	if(fd < 0)
		return;
	if (::write(fd, vaddr, buf->stride * buf->height * bytes_pp) < 0)
		TRACE("dump buffer to file failed with error %i", errno);
	::close(fd);
	hybris_gralloc_unlock(buf->handle);
}
