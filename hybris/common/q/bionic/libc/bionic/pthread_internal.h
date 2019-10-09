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

#include <pthread.h>
#include <stdatomic.h>
#define __has_feature(x) 0 
#if __has_feature(hwaddress_sanitizer)
#include <sanitizer/hwasan_interface.h>
#else
#define __hwasan_thread_enter()
#define __hwasan_thread_exit()
#endif

#include "private/bionic_elf_tls.h"
#include "private/bionic_lock.h"
#include "private/bionic_tls.h"

// Has the thread been detached by a pthread_join or pthread_detach call?
#define PTHREAD_ATTR_FLAG_DETACHED 0x00000001

// Has the thread been joined by another thread?
#define PTHREAD_ATTR_FLAG_JOINED 0x00000002

// Used for pthread_attr_setinheritsched. We need two flags for this apparent
// boolean because our historical behavior matches neither of the POSIX choices.
#define PTHREAD_ATTR_FLAG_INHERIT 0x00000004
#define PTHREAD_ATTR_FLAG_EXPLICIT 0x00000008

enum ThreadJoinState {
  THREAD_NOT_JOINED,
  THREAD_EXITED_NOT_JOINED,
  THREAD_JOINED,
  THREAD_DETACHED
};

class thread_local_dtor;

class pthread_internal_t {
 public:
  class pthread_internal_t* next;
  class pthread_internal_t* prev;

  pid_t tid;

 private:
  pid_t cached_pid_;

 public:
  pid_t invalidate_cached_pid() {
    pid_t old_value;
    get_cached_pid(&old_value);
    set_cached_pid(0);
    return old_value;
  }

  void set_cached_pid(pid_t value) {
    cached_pid_ = value;
  }

  bool get_cached_pid(pid_t* cached_pid) {
    *cached_pid = cached_pid_;
    return (*cached_pid != 0);
  }

  pthread_attr_t attr;

  _Atomic(ThreadJoinState) join_state;

  __pthread_cleanup_t* cleanup_stack;

  void* (*start_routine)(void*);
  void* start_routine_arg;
  void* return_value;

  void* alternate_signal_stack;

  // The start address of the shadow call stack's guard region (arm64 only).
  // This address is only used to deallocate the shadow call stack on thread
  // exit; the address of the stack itself is stored only in the x18 register.
  // Because the protection offered by SCS relies on the secrecy of the stack
  // address, storing the address here weakens the protection, but only
  // slightly, because it is relatively easy for an attacker to discover the
  // address of the guard region anyway (e.g. it can be discovered by reference
  // to other allocations), but not the stack itself, which is <0.1% of the size
  // of the guard region.
  //
  // There are at least two other options for discovering the start address of
  // the guard region on thread exit, but they are not as simple as storing in
  // TLS.
  // 1) Derive it from the value of the x18 register. This is only possible in
  //    processes that do not contain legacy code that might clobber x18,
  //    therefore each process must declare early during process startup whether
  //    it might load legacy code.
  // 2) Mark the guard region as such using prctl(PR_SET_VMA_ANON_NAME) and
  //    discover its address by reading /proc/self/maps. One issue with this is
  //    that reading /proc/self/maps can race with allocations, so we may need
  //    code to handle retries.
  void* shadow_call_stack_guard_region;

  Lock startup_handshake_lock;

  void* mmap_base;
  size_t mmap_size;

  thread_local_dtor* thread_local_dtors;

  /*
   * The dynamic linker implements dlerror(3), which makes it hard for us to implement this
   * per-thread buffer by simply using malloc(3) and free(3).
   */
  char* current_dlerror;
#define __BIONIC_DLERROR_BUFFER_SIZE 512
  char dlerror_buffer[__BIONIC_DLERROR_BUFFER_SIZE];

  bionic_tls* bionic_tls;

  int errno_value;
};

struct ThreadMapping {
  char* mmap_base;
  size_t mmap_size;

  char* static_tls;
  char* stack_base;
  char* stack_top;
};

__LIBC_HIDDEN__ void __init_tcb(bionic_tcb* tcb, pthread_internal_t* thread);
__LIBC_HIDDEN__ void __init_tcb_stack_guard(bionic_tcb* tcb);
__LIBC_HIDDEN__ void __init_tcb_dtv(bionic_tcb* tcb);
__LIBC_HIDDEN__ void __init_bionic_tls_ptrs(bionic_tcb* tcb, bionic_tls* tls);
__LIBC_HIDDEN__ bionic_tls* __allocate_temp_bionic_tls();
__LIBC_HIDDEN__ void __free_temp_bionic_tls(bionic_tls* tls);
__LIBC_HIDDEN__ void __init_additional_stacks(pthread_internal_t*);
__LIBC_HIDDEN__ int __init_thread(pthread_internal_t* thread);
__LIBC_HIDDEN__ ThreadMapping __allocate_thread_mapping(size_t stack_size, size_t stack_guard_size);

__LIBC_HIDDEN__ pthread_t __pthread_internal_add(pthread_internal_t* thread);
__LIBC_HIDDEN__ pthread_internal_t* __pthread_internal_find(pthread_t pthread_id, const char* caller);
__LIBC_HIDDEN__ pid_t __pthread_internal_gettid(pthread_t pthread_id, const char* caller);
__LIBC_HIDDEN__ void __pthread_internal_remove(pthread_internal_t* thread);
__LIBC_HIDDEN__ void __pthread_internal_remove_and_free(pthread_internal_t* thread);

static inline __always_inline bionic_tcb* __get_bionic_tcb() {
  return reinterpret_cast<bionic_tcb*>(&__get_tls()[MIN_TLS_SLOT]);
}

// Make __get_thread() inlined for performance reason. See http://b/19825434.
static inline __always_inline pthread_internal_t* __get_thread() {
  return static_cast<pthread_internal_t*>(__get_tls()[TLS_SLOT_THREAD_ID]);
}

static inline __always_inline bionic_tls& __get_bionic_tls() {
  return *static_cast<bionic_tls*>(__get_tls()[TLS_SLOT_BIONIC_TLS]);
}

static inline __always_inline TlsDtv* __get_tcb_dtv(bionic_tcb* tcb) {
  uintptr_t dtv_slot = reinterpret_cast<uintptr_t>(tcb->tls_slot(TLS_SLOT_DTV));
  return reinterpret_cast<TlsDtv*>(dtv_slot - offsetof(TlsDtv, generation));
}

static inline void __set_tcb_dtv(bionic_tcb* tcb, TlsDtv* val) {
  tcb->tls_slot(TLS_SLOT_DTV) = &val->generation;
}

extern "C" __LIBC_HIDDEN__ int __set_tls(void* ptr);

__LIBC_HIDDEN__ void pthread_key_clean_all(void);

// Address space is precious on LP32, so use the minimum unit: one page.
// On LP64, we could use more but there's no obvious advantage to doing
// so, and the various media processes use RLIMIT_AS as a way to limit
// the amount of allocation they'll do.
#define PTHREAD_GUARD_SIZE PAGE_SIZE

// SIGSTKSZ (8KiB) is not big enough.
// An snprintf to a stack buffer of size PATH_MAX consumes ~7KiB of stack.
// On 64-bit, logging uses more than 8KiB by itself, ucontext is comically
// large on aarch64, and we have effectively infinite address space, so double
// the signal stack size.
#if defined(__LP64__)
#define SIGNAL_STACK_SIZE_WITHOUT_GUARD (32 * 1024)
#else
#define SIGNAL_STACK_SIZE_WITHOUT_GUARD (16 * 1024)
#endif

// Traditionally we gave threads a 1MiB stack. When we started
// allocating per-thread alternate signal stacks to ease debugging of
// stack overflows, we subtracted the same amount we were using there
// from the default thread stack size. This should keep memory usage
// roughly constant.
#define PTHREAD_STACK_SIZE_DEFAULT ((1 * 1024 * 1024) - SIGNAL_STACK_SIZE_WITHOUT_GUARD)

// Leave room for a guard page in the internally created signal stacks.
#define SIGNAL_STACK_SIZE (SIGNAL_STACK_SIZE_WITHOUT_GUARD + PTHREAD_GUARD_SIZE)

// Needed by fork.
__LIBC_HIDDEN__ extern void __bionic_atfork_run_prepare();
__LIBC_HIDDEN__ extern void __bionic_atfork_run_child();
__LIBC_HIDDEN__ extern void __bionic_atfork_run_parent();
