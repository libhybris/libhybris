#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/matroxfb.h> // for FBIO_WAITFORVSYNC
#include <sys/mman.h> //mmap, munmap
#include <errno.h>
#include <fdpass.h>

#include "nativewindowbase.h"
#include "offscreen_window.h"
#include <assert.h>

OffscreenNativeWindowBuffer::OffscreenNativeWindowBuffer()
{
}

OffscreenNativeWindowBuffer::OffscreenNativeWindowBuffer(unsigned int width, unsigned int height,
														unsigned int format, unsigned int usage)
{
	ANativeWindowBuffer::width = width;
	ANativeWindowBuffer::height = height;
	ANativeWindowBuffer::format = format;
	ANativeWindowBuffer::usage = usage;
};

void OffscreenNativeWindowBuffer::writeToFd(int fd)
{
    assert(handle);
    write(fd, &width, sizeof(width));
    write(fd, &height, sizeof(height));
    write(fd, &stride, sizeof(stride));
    write(fd, &format, sizeof(format));
    write(fd, &usage, sizeof(usage));

    write(fd, &handle->numFds, sizeof(handle->numFds));
    write(fd, &handle->numInts, sizeof(handle->numInts));

    for (int i = 0; i < handle->numFds; i++) {
        sock_fd_write(fd, (void *)"1", 1, handle->data[i]);
    }
    for (int i = handle->numFds; i < handle->numFds + handle->numInts; i++) {
        write(fd, &handle->data[i], sizeof(handle->data[0]));
    }
}

void OffscreenNativeWindowBuffer::readFromFd(int fd)
{
    int ret=0;
    char buf[10];
    ret = read(fd, &width, sizeof(width));
    assert(ret == sizeof(width)); 

    ret = read(fd, &height, sizeof(height));
    assert(ret == sizeof(height));

    ret = read(fd, &stride, sizeof(stride));
    assert(ret == sizeof(stride));

    ret = read(fd, &format, sizeof(format));
    assert(ret == sizeof(format));

    ret = read(fd, &usage, sizeof(usage));
    assert(ret == sizeof(usage));

    int numFds = 0;
    int numInts =0;
    ret = read(fd, &numFds, sizeof(numFds));
    assert(ret == sizeof(numFds));
    ret = read(fd, &numInts, sizeof(numInts));
    assert(ret == sizeof(numInts));

    if(handle) {
        native_handle_close(handle);
        native_handle_delete(const_cast<native_handle_t*>(handle));
    }

    handle = native_handle_create(numFds, numInts);
    for (int i = 0; i < numFds; i++) {
        sock_fd_read(fd, buf, sizeof(buf), (int *) &handle->data[i]);
    }
    for (int i = numFds; i < numFds + numInts; i++) {
        read(fd, (void *) &handle->data[i], sizeof(handle->data[i]));
    }

}

buffer_handle_t OffscreenNativeWindowBuffer::getHandle()
{
  return handle;
}

