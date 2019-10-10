#pragma once
#include <sys/cdefs.h>
#define __LIBC_HIDDEN__ __attribute__((visibility("hidden")))
#define __unused __attribute__((__unused__))
#define __BIONIC_ALIGN(__value, __alignment) (((__value) + (__alignment)-1) & ~((__alignment)-1))