#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/matroxfb.h> // for FBIO_WAITFORVSYNC
#include <sys/mman.h> //mmap, munmap
#include <errno.h>
#include <fdpass.h>

#include "offscreen_window.h"

OffscreenNativeWindowBuffer::OffscreenNativeWindowBuffer()
{
}

void OffscreenNativeWindowBuffer::writeToFd(int fd)
{
	write(fd, &width, sizeof(width));
	write(fd, &height, sizeof(height));
	write(fd, &stride, sizeof(stride));
	write(fd, &format, sizeof(format));
	write(fd, &usage, sizeof(format));
	write(fd, &handle->numFds, sizeof(handle->numFds));
	write(fd, &handle->numInts, sizeof(handle->numInts));
	int j;
	
	for (j = 0; j < handle->numFds; j++)
	{
		sock_fd_write(fd, (void *)"1", 1, handle->data[j]);
	}
	for (j = handle->numFds; j < handle->numFds + handle->numInts; j++)
	{
		write(fd, &handle->data[j], sizeof(handle->data[0]));
		printf("Send handle data %i\n", handle->data[j]);
	}
}

void OffscreenNativeWindowBuffer::readFromFd(int fd)
{
	char buf[10];
	read(fd, &width, sizeof(width));
	read(fd, &height, sizeof(height));
	read(fd, &stride, sizeof(stride));
	read(fd, &format, sizeof(format));
	read(fd, &usage, sizeof(format));
	int version, numFds, numInts; 
	read(fd, &numFds, sizeof(numFds));
	read(fd, &numInts, sizeof(numInts));
	handle = native_handle_create(numFds, numInts);
	int i;
	for (i = 0; i < numFds; i++)
	{
	sock_fd_read(fd, buf, sizeof(buf), (int *) &handle->data[i]);
	}
	for (i = numFds; i < numFds + numInts; i++)
	{
	read(fd, (void *) &handle->data[i], sizeof(handle->data[i]));
		printf("child: Got handle data %i\n", handle->data[i]);
	}

}

buffer_handle_t OffscreenNativeWindowBuffer::getHandle()
{
  return handle;
}

