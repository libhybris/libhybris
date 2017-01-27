/*
 * Copyright (C) 2012 The Android Open Source Project
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

#include <stdint.h>
#include <sys/cdefs.h>

extern unsigned __linker_init(void* raw_args);

__LIBC_HIDDEN__ void _start() {
  void (*start)(void);

  void* raw_args = (void*) ((uintptr_t) __builtin_frame_address(0) + sizeof(void*));
  start = (void(*)(void))__linker_init(raw_args);

  /* linker init returns (%eax) the _entry address in the main image */
  /* entry point expects sp to point to raw_args */

  __asm__ (
     "mov %0, %%esp\n\t"
     "jmp *%1\n\t"
     : : "r"(raw_args), "r"(start) :
  );

  /* Unreachable */
}

/* Since linker has its own version of crtbegin (this file) it should have */
/* own version of __stack_chk_fail_local for the case when it's built with */
/* stack protector feature */

#include "arch-x86/bionic/__stack_chk_fail_local.h"
