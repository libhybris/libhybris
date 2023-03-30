/*
 * Copyright (C) 2019 The Android Open Source Project
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

#include "private/bionic_call_ifunc_resolver.h"
#include <sys/auxv.h>
#include <sys/ifunc.h>

ElfW(Addr) __bionic_call_ifunc_resolver(ElfW(Addr) resolver_addr) {
#if defined(__aarch64__)
  typedef ElfW(Addr) (*ifunc_resolver_t)(uint64_t, __ifunc_arg_t*);
  static __ifunc_arg_t arg = { sizeof(__ifunc_arg_t), getauxval(AT_HWCAP), getauxval(AT_HWCAP2) };
  return reinterpret_cast<ifunc_resolver_t>(resolver_addr)(arg._hwcap | _IFUNC_ARG_HWCAP, &arg);
#elif defined(__arm__)
  typedef ElfW(Addr) (*ifunc_resolver_t)(unsigned long);
  static unsigned long hwcap = getauxval(AT_HWCAP);
  return reinterpret_cast<ifunc_resolver_t>(resolver_addr)(hwcap);
#else
  typedef ElfW(Addr) (*ifunc_resolver_t)(void);
  return reinterpret_cast<ifunc_resolver_t>(resolver_addr)();
#endif
}
