/*
 * Copyright (C) 2018 The Android Open Source Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#pragma once

#include <stdbool.h>

// Structures for android_mallopt.

typedef struct {
  // Pointer to the buffer allocated by a call to M_GET_MALLOC_LEAK_INFO.
  uint8_t* buffer;
  // The size of the "info" buffer.
  size_t overall_size;
  // The size of a single entry.
  size_t info_size;
  // The sum of all allocations that have been tracked. Does not include
  // any heap overhead.
  size_t total_memory;
  // The maximum number of backtrace entries.
  size_t backtrace_size;
} android_mallopt_leak_info_t;

// Opcodes for android_mallopt.

enum {
  // Marks the calling process as a profileable zygote child, possibly
  // initializing profiling infrastructure.
  M_INIT_ZYGOTE_CHILD_PROFILING = 1,
#define M_INIT_ZYGOTE_CHILD_PROFILING M_INIT_ZYGOTE_CHILD_PROFILING
  M_RESET_HOOKS = 2,
#define M_RESET_HOOKS M_RESET_HOOKS
  // Set an upper bound on the total size in bytes of all allocations made
  // using the memory allocation APIs.
  //   arg = size_t*
  //   arg_size = sizeof(size_t)
  M_SET_ALLOCATION_LIMIT_BYTES = 3,
#define M_SET_ALLOCATION_LIMIT_BYTES M_SET_ALLOCATION_LIMIT_BYTES
  // Called after the zygote forks to indicate this is a child.
  M_SET_ZYGOTE_CHILD = 4,
#define M_SET_ZYGOTE_CHILD M_SET_ZYGOTE_CHILD

  // Options to dump backtraces of allocations. These options only
  // work when malloc debug has been enabled.

  // Writes the backtrace information of all current allocations to a file.
  // NOTE: arg_size has to be sizeof(FILE*) because FILE is an opaque type.
  //   arg = FILE*
  //   arg_size = sizeof(FILE*)
  M_WRITE_MALLOC_LEAK_INFO_TO_FILE = 5,
#define M_WRITE_MALLOC_LEAK_INFO_TO_FILE M_WRITE_MALLOC_LEAK_INFO_TO_FILE
  // Get information about the backtraces of all
  //   arg = android_mallopt_leak_info_t*
  //   arg_size = sizeof(android_mallopt_leak_info_t)
  M_GET_MALLOC_LEAK_INFO = 6,
#define M_GET_MALLOC_LEAK_INFO M_GET_MALLOC_LEAK_INFO
  // Free the memory allocated and returned by M_GET_MALLOC_LEAK_INFO.
  //   arg = android_mallopt_leak_info_t*
  //   arg_size = sizeof(android_mallopt_leak_info_t)
  M_FREE_MALLOC_LEAK_INFO = 7,
#define M_FREE_MALLOC_LEAK_INFO M_FREE_MALLOC_LEAK_INFO
};

// Manipulates bionic-specific handling of memory allocation APIs such as
// malloc. Only for use by the Android platform itself.
//
// On success, returns true. On failure, returns false and sets errno.
extern "C" bool android_mallopt(int opcode, void* arg, size_t arg_size);
