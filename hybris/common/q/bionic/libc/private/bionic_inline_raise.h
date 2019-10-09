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

#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

// An inline version of pthread_sigqueue(pthread_self(), ...), to reduce the number of
// uninteresting stack frames at the top of a crash.
static inline __always_inline void inline_raise(int sig, void* value = nullptr) {
  // Protect ourselves against stale cached PID/TID values by fetching them via syscall.
  // http://b/37769298
  pid_t pid = syscall(__NR_getpid);
  pid_t tid = syscall(__NR_gettid);
  siginfo_t info = {};
  info.si_code = SI_QUEUE;
  info.si_pid = pid;
  info.si_uid = getuid();
  info.si_value.sival_ptr = value;

#if defined(__arm__)
  register long r0 __asm__("r0") = pid;
  register long r1 __asm__("r1") = tid;
  register long r2 __asm__("r2") = sig;
  register long r3 __asm__("r3") = reinterpret_cast<long>(&info);
  register long r7 __asm__("r7") = __NR_rt_tgsigqueueinfo;
  __asm__("swi #0" : "=r"(r0) : "r"(r0), "r"(r1), "r"(r2), "r"(r3), "r"(r7) : "memory");
#elif defined(__aarch64__)
  register long x0 __asm__("x0") = pid;
  register long x1 __asm__("x1") = tid;
  register long x2 __asm__("x2") = sig;
  register long x3 __asm__("x3") = reinterpret_cast<long>(&info);
  register long x8 __asm__("x8") = __NR_rt_tgsigqueueinfo;
  __asm__("svc #0" : "=r"(x0) : "r"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x8) : "memory");
#else
  syscall(__NR_rt_tgsigqueueinfo, pid, tid, sig, &info);
#endif
}

