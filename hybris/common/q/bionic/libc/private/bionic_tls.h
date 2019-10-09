/*
 * Copyright (C) 2008 The Android Open Source Project
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

#include <locale.h>
#include <mntent.h>
#include <stdio.h>
#include <sys/cdefs.h>
#include <sys/param.h>

#include "bionic_asm_tls.h"
#include "bionic_macros.h"
#include "__get_tls.h"
#include "grp_pwd.h"

/** WARNING WARNING WARNING
 **
 ** This header file is *NOT* part of the public Bionic ABI/API and should not
 ** be used/included by user-serviceable parts of the system (e.g.
 ** applications).
 **/

class pthread_internal_t;

// This struct is small, so the linker can allocate a temporary copy on its
// stack. It can't be combined with pthread_internal_t because:
//  - native bridge requires pthread_internal_t to have the same layout across
//    architectures, and
//  - On x86, this struct would have to be placed at the front of
//    pthread_internal_t, moving fields like `tid`.
//  - We'd like to avoid having a temporary pthread_internal_t object that
//    needs to be transferred once the final size of static TLS is known.
struct bionic_tcb {
  void* raw_slots_storage[BIONIC_TLS_SLOTS];

  // Return a reference to a slot given its TP-relative TLS_SLOT_xxx index.
  // The thread pointer (i.e. __get_tls()) points at &tls_slot(0).
  void*& tls_slot(size_t tpindex) {
    return raw_slots_storage[tpindex - MIN_TLS_SLOT];
  }

  // Initialize the main thread's final object using its bootstrap object.
  void copy_from_bootstrap(const bionic_tcb* boot) {
    // Copy everything. Problematic slots will be reinitialized.
    *this = *boot;
  }

  pthread_internal_t* thread() {
    return static_cast<pthread_internal_t*>(tls_slot(TLS_SLOT_THREAD_ID));
  }
};

/*
 * Bionic uses some pthread keys internally. All pthread keys used internally
 * should be created in constructors, except for keys that may be used in or
 * before constructors.
 *
 * We need to manually maintain the count of pthread keys used internally, but
 * pthread_test should fail if we forget.
 *
 * These are the pthread keys currently used internally by libc:
 *  _res_key               libc (constructor in BSD code)
 */

#define LIBC_PTHREAD_KEY_RESERVED_COUNT 1

/* Internally, jemalloc uses a single key for per thread data. */
#define JEMALLOC_PTHREAD_KEY_RESERVED_COUNT 1
#define BIONIC_PTHREAD_KEY_RESERVED_COUNT (LIBC_PTHREAD_KEY_RESERVED_COUNT + JEMALLOC_PTHREAD_KEY_RESERVED_COUNT)

/*
 * Maximum number of pthread keys allocated.
 * This includes pthread keys used internally and externally.
 */
#define BIONIC_PTHREAD_KEY_COUNT (BIONIC_PTHREAD_KEY_RESERVED_COUNT + PTHREAD_KEYS_MAX)

class pthread_key_data_t {
 public:
  uintptr_t seq; // Use uintptr_t just for alignment, as we use pointer below.
  void* data;
};

// ~3 pages. This struct is allocated as static TLS memory (i.e. at a fixed
// offset from the thread pointer).
struct bionic_tls {
  pthread_key_data_t key_data[BIONIC_PTHREAD_KEY_COUNT];

  locale_t locale;

  char basename_buf[MAXPATHLEN];
  char dirname_buf[MAXPATHLEN];

  mntent mntent_buf;
  char mntent_strings[BUFSIZ];

  char ptsname_buf[32];
  char ttyname_buf[64];

  char strerror_buf[NL_TEXTMAX];
  char strsignal_buf[NL_TEXTMAX];

  group_state_t group;
  passwd_state_t passwd;

  // Initialize the main thread's final object using its bootstrap object.
  void copy_from_bootstrap(const bionic_tls* boot __attribute__((unused))) {
    // Nothing in bionic_tls needs to be preserved in the transition to the
    // final TLS objects, so don't copy anything.
  }
};

class KernelArgumentBlock;
extern "C" void __libc_init_main_thread_early(const KernelArgumentBlock& args, bionic_tcb* temp_tcb);
extern "C" void __libc_init_main_thread_late();
extern "C" void __libc_init_main_thread_final();
