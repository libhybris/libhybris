/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <signal.h>

#include <limits.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include "bionic_macros.h"




#ifndef _KERNEL__NSIG
#define _KERNEL__NSIG 64
#endif


/* Userspace's NSIG is the kernel's _NSIG + 1. */
#define _NSIG (_KERNEL__NSIG + 1)
#define NSIG _NSIG

typedef int sig_atomic_t;

typedef __sighandler_t sig_t; /* BSD compatibility. */
typedef __sighandler_t sighandler_t; /* glibc compatibility. */

#if defined(__LP64__) || defined(__mips__)
typedef sigset_t sigset64_t;
#else
typedef struct { unsigned long __bits[_KERNEL__NSIG/LONG_BIT]; } sigset64_t;
#endif


#define BIONIC_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&) = delete;             \
  void operator=(const TypeName&) = delete

class ScopedSignalBlocker {
 public:
  // Block all signals.
  explicit ScopedSignalBlocker() {
    //sigfillset64(&set);
    //sigprocmask64(SIG_BLOCK, &set, &old_set_);
    sigset_t mask;
    sigfillset(&mask);
    sigprocmask(SIG_SETMASK, &mask, &old_mask);
  }

  // Block just the specified signal.
  explicit ScopedSignalBlocker(int signal) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, signal);
    sigprocmask(SIG_SETMASK, &mask, &old_mask);
    // sigaddset64(&mask, signal);
    //sigprocmask64(SIG_BLOCK, &set, &old_set_);
  }

  ~ScopedSignalBlocker() {
    reset();
  }

  void reset() {
   // sigprocmask64(SIG_SETMASK, &old_set_, nullptr);
   sigprocmask(SIG_SETMASK, &old_mask, NULL);
  }

 // sigset64_t old_set_;
  sigset_t old_mask;
  BIONIC_DISALLOW_COPY_AND_ASSIGN(ScopedSignalBlocker);
};
