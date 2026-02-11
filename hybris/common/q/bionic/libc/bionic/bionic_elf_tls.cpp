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

#include "private/bionic_elf_tls.h"

#include <async_safe/CHECK.h>
#include <async_safe/log.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include "macros.h"
#include "page.h"
#include "private/ScopedRWLock.h"
#include "private/ScopedSignalBlocker.h"
#include "private/bionic_globals.h"
#include "private/bionic_tls.h"
#include "pthread_internal.h"


#define __BIONIC_ALIGN(__value, __alignment) (((__value) + (__alignment)-1) & ~((__alignment)-1))
#define __predict_true(exp)     __builtin_expect((exp) != 0, 1)
#define __predict_false(exp)    __builtin_expect((exp) != 0, 0)
// Every call to __tls_get_addr needs to check the generation counter, so
// accesses to the counter need to be as fast as possible. Keep a copy of it in
// a hidden variable, which can be accessed without using the GOT. The linker
// will update this variable when it updates its counter.
//
// hybris: the definition lives in linker_tls.cpp - only the linker updates it,
// since bionic libc.so's constructor never runs here.

// Search for a TLS segment in the given phdr table. Returns true if it has a
// TLS segment and false otherwise.
bool __bionic_get_tls_segment(const ElfW(Phdr)* phdr_table, size_t phdr_count,
                              ElfW(Addr) load_bias, TlsSegment* out) {
  for (size_t i = 0; i < phdr_count; ++i) {
    const ElfW(Phdr)& phdr = phdr_table[i];
    if (phdr.p_type == PT_TLS) {
        out->size = phdr.p_memsz;
        out->alignment = phdr.p_align;
        out->init_ptr = reinterpret_cast<void*>(load_bias + phdr.p_vaddr);
        out->init_size = phdr.p_filesz;
      return true;
    }
  }
  return false;
}

// Return true if the alignment of a TLS segment is a valid power-of-two. Also
// cap the alignment if it's too high.
bool __bionic_check_tls_alignment(size_t* alignment) {
  // N.B. The size does not need to be a multiple of the alignment. With
  // ld.bfd (or after using binutils' strip), the TLS segment's size isn't
  // rounded up.
  if (*alignment == 0 || !powerof2(*alignment)) {
    return false;
  }
  // Bionic only respects TLS alignment up to one page.
  *alignment = MIN(*alignment, PAGE_SIZE);
  return true;
}

size_t StaticTlsLayout::offset_thread_pointer() const {
  return 0;
}

void StaticTlsLayout::finish_layout() {
  // Round the offset up to the alignment.
  offset_ = round_up_with_overflow_check(offset_, alignment_);

  if (overflowed_ || offset_ > MAX_SIZE) {
    async_safe_fatal("error: TLS segments in static TLS overflowed (size %zu, max %zu)",
                     offset_, MAX_SIZE);
  }
}

// The size is not required to be a multiple of the alignment. The alignment
// must be a positive power-of-two.
size_t StaticTlsLayout::reserve(size_t size, size_t alignment) {
  offset_ = round_up_with_overflow_check(offset_, alignment);
  const size_t result = offset_;
  if (__builtin_add_overflow(offset_, size, &offset_)) overflowed_ = true;
  alignment_ = MAX(alignment_, alignment);

  if (offset_ > MAX_SIZE) overflowed_ = true;

  return result;
}

size_t StaticTlsLayout::round_up_with_overflow_check(size_t value, size_t alignment) {
  const size_t old_value = value;
  value = __BIONIC_ALIGN(value, alignment);
  if (value < old_value) overflowed_ = true;
  return value;
}

// Copy each TLS module's initialization image into a newly-allocated block of
// static TLS memory. To reduce dirty pages, this function only writes to pages
// within the static TLS that need initialization. The memory should already be
// zero-initialized on entry.
extern "C" __thread void* hybris_tls_storage[];

// hybris: the range may have been left by an unloaded module, so zero it rather
// than assume it is untouched: the .tbss tail past init_size has to read as
// zero, and leftovers would show through.
static void init_static_tls_segment(char* static_tls, const TlsModule& module) {
  char* dest = static_tls + module.static_offset;
  memset(dest, 0, module.segment.size);
  if (module.segment.init_size > 0) {
    memcpy(dest, module.segment.init_ptr, module.segment.init_size);
  }
}

void __init_static_tls(void* static_tls) {
  if (static_tls == nullptr) {
    static_tls = hybris_tls_storage;
  }
  TlsModules& modules = __libc_shared_globals()->tls_modules;
  ScopedSignalBlocker ssb;
  ScopedReadLock locker(&modules.rwlock);

  for (size_t i = 0; i < modules.module_count; ++i) {
    TlsModule& module = modules.module_table[i];
    // hybris: unlike bionic, the static modules are not a prefix of the table -
    // an unloaded module leaves a slot a dynamic one can take, so keep walking
    // instead of stopping at the first dynamic module.
    if (module.static_offset == SIZE_MAX) {
      continue;
    }

    init_static_tls_segment(static_cast<char*>(static_tls), module);
  }
}

// Copy a single static TLS module's initialization image into the current
// thread's storage. Used when a module is registered so the loading thread
// sees its initialized data immediately.
void __init_static_tls_module(size_t module_idx) {
  char* static_tls = reinterpret_cast<char*>(hybris_tls_storage);
  TlsModules& modules = __libc_shared_globals()->tls_modules;
  ScopedSignalBlocker ssb;
  ScopedReadLock locker(&modules.rwlock);

  TlsModule& module = modules.module_table[module_idx];
  if (module.static_offset != SIZE_MAX) {
    init_static_tls_segment(static_tls, module);
  }
}

static inline size_t dtv_size_in_bytes(size_t module_count) {
  return sizeof(TlsDtv) + module_count * sizeof(void*);
}

// Calculates the number of module slots to allocate in a new DTV. For small
// objects (up to 1KiB), the TLS allocator allocates memory in power-of-2 sizes,
// so for better space usage, ensure that the DTV size (header + slots) is a
// power of 2.
//
// The lock on TlsModules must be held.
static size_t calculate_new_dtv_count() {
  size_t loaded_cnt = __libc_shared_globals()->tls_modules.module_count;
  size_t bytes = dtv_size_in_bytes(MAX(1, loaded_cnt));
  if (!powerof2(bytes)) {
    bytes = BIONIC_ROUND_UP_POWER_OF_2(bytes);
  }
  return (bytes - sizeof(TlsDtv)) / sizeof(void*);
}

// This function must be called with signals blocked and a write lock on
// TlsModules held.
static void update_tls_dtv(bionic_tcb* tcb) {
  const TlsModules& modules = __libc_shared_globals()->tls_modules;
  BionicAllocator& allocator = __libc_shared_globals()->tls_allocator;

  // If DTV hasn't been ever initialized, TLS_SLOT_DTV should be NULL
  const bool has_dtv = tcb->tls_slot(TLS_SLOT_DTV) != nullptr;

  // Use the generation counter from the shared globals instead of the local
  // copy, which won't be initialized yet if __tls_get_addr is called before
  // libc.so's constructor.
  if (has_dtv && __get_tcb_dtv(tcb)->generation == atomic_load(&modules.generation)) {
    return;
  }

  const size_t old_cnt = has_dtv ? __get_tcb_dtv(tcb)->count : 0;

  // If the DTV isn't large enough, allocate a larger one. Because a signal
  // handler could interrupt the fast path of __tls_get_addr, we don't free the
  // old DTV. Instead, we add the old DTV to a list, then free all of a thread's
  // DTVs at thread-exit. Each time the DTV is reallocated, its size at least
  // doubles.
  if (modules.module_count > old_cnt) {
    size_t new_cnt = calculate_new_dtv_count();
    TlsDtv* const old_dtv = has_dtv ? __get_tcb_dtv(tcb) : NULL;
    TlsDtv* const new_dtv = static_cast<TlsDtv*>(allocator.alloc(dtv_size_in_bytes(new_cnt)));
    // TlsDtv* const new_dtv = static_cast<TlsDtv*>(malloc(dtv_size_in_bytes(new_cnt)));
    if (has_dtv) {
      memcpy(new_dtv, old_dtv, dtv_size_in_bytes(old_cnt));
    }
    new_dtv->count = new_cnt;
    new_dtv->next = old_dtv;
    __set_tcb_dtv(tcb, new_dtv);
  }

  TlsDtv* const dtv = __get_tcb_dtv(tcb);

  char* static_tls = reinterpret_cast<char*>(hybris_tls_storage);

  // Initialize static TLS modules and free unloaded modules.
  for (size_t i = 0; i < dtv->count; ++i) {
    if (i < modules.module_count) {
      const TlsModule& mod = modules.module_table[i];
      if (mod.static_offset != SIZE_MAX) {
        dtv->modules[i] = static_tls + mod.static_offset;
        continue;
      }
      if (mod.first_generation != kTlsGenerationNone &&
          mod.first_generation <= dtv->generation) {
        continue;
      }
    }
    if (modules.on_destruction_cb != nullptr) {
      void* dtls_begin = dtv->modules[i];
      void* dtls_end =
          static_cast<void*>(static_cast<char*>(dtls_begin) + allocator.get_chunk_size(dtls_begin));
      modules.on_destruction_cb(dtls_begin, dtls_end);
    }
    //allocator.free(dtv->modules[i]);
    dtv->modules[i] = nullptr;
  }

  dtv->generation = atomic_load(&modules.generation);
}

__attribute__((noinline)) static void* tls_get_addr_slow_path(const TlsIndex* ti) {
  TlsModules& modules = __libc_shared_globals()->tls_modules;
  bionic_tcb* tcb = __get_bionic_tcb();

  // Block signals and lock TlsModules. We may need the allocator, so take
  // a write lock.
  ScopedSignalBlocker ssb;
  ScopedWriteLock locker(&modules.rwlock);

  update_tls_dtv(tcb);

  TlsDtv* dtv = __get_tcb_dtv(tcb);
  const size_t module_idx = __tls_module_id_to_idx(ti->module_id);
  void* mod_ptr = dtv->modules[module_idx];
  if (mod_ptr == nullptr) {
    const TlsSegment& segment = modules.module_table[module_idx].segment;
    mod_ptr = __libc_shared_globals()->tls_allocator.memalign(segment.alignment, segment.size);
    if (segment.init_size > 0) {
      memcpy(mod_ptr, segment.init_ptr, segment.init_size);
    }
    dtv->modules[module_idx] = mod_ptr;

    // Reports the allocation to the listener, if any.
    if (modules.on_creation_cb != nullptr) {
      modules.on_creation_cb(mod_ptr,
                             static_cast<void*>(static_cast<char*>(mod_ptr) + segment.size));
    }
  }

  return static_cast<char*>(mod_ptr) + ti->offset + TLS_DTV_OFFSET;
}

// Returns the address of a thread's TLS memory given a module ID and an offset
// into that module's TLS segment. This function is called on every access to a
// dynamic TLS variable on targets that don't use TLSDESC. arm64 uses TLSDESC,
// so it only calls this function on a thread's first access to a module's TLS
// segment.
//
// On most targets, this accessor function is __tls_get_addr and
// TLS_GET_ADDR_CCONV is unset. 32-bit x86 uses ___tls_get_addr instead and a
// regparm() calling convention.
extern "C" void* TLS_GET_ADDR(const TlsIndex* ti) TLS_GET_ADDR_CCONV {
  // hybris: TLS_SLOT_DTV of thread local storage might be uninitialized
  bionic_tcb* tcb = __get_bionic_tcb();
  if (tcb->tls_slot(TLS_SLOT_DTV) == nullptr)
    return tls_get_addr_slow_path(ti);

  TlsDtv* dtv = __get_tcb_dtv(tcb);

  // TODO: See if we can use a relaxed memory ordering here instead.
  size_t generation = atomic_load(&__libc_tls_generation_copy);
  if (__predict_true(generation == dtv->generation)) {
    void* mod_ptr = dtv->modules[__tls_module_id_to_idx(ti->module_id)];
    if (__predict_true(mod_ptr != nullptr)) {
      return static_cast<char*>(mod_ptr) + ti->offset + TLS_DTV_OFFSET;
    }
  }

  return tls_get_addr_slow_path(ti);
}

// This function frees:
//  - TLS modules referenced by the current DTV.
//  - The list of DTV objects associated with the current thread.
//
// The caller must have already blocked signals.
void __free_dynamic_tls(bionic_tcb* tcb) {
  TlsModules& modules = __libc_shared_globals()->tls_modules;
  BionicAllocator& allocator = __libc_shared_globals()->tls_allocator;

  // If we didn't allocate any dynamic memory, skip out early without taking
  // the lock.
  TlsDtv* dtv = __get_tcb_dtv(tcb);
  if (dtv->generation == kTlsGenerationNone) {
    return;
  }

  // We need the write lock to use the allocator.
  ScopedWriteLock locker(&modules.rwlock);

  // First free everything in the current DTV.
  for (size_t i = 0; i < dtv->count; ++i) {
    if (i < modules.module_count && modules.module_table[i].static_offset != SIZE_MAX) {
      // This module's TLS memory is allocated statically, so don't free it here.
      continue;
    }

    if (modules.on_destruction_cb != nullptr) {
      void* dtls_begin = dtv->modules[i];
      void* dtls_end =
          static_cast<void*>(static_cast<char*>(dtls_begin) + allocator.get_chunk_size(dtls_begin));
      modules.on_destruction_cb(dtls_begin, dtls_end);
    }

    //allocator.free(dtv->modules[i]);
  }

  // Now free the thread's list of DTVs.
  while (dtv->generation != kTlsGenerationNone) {
    TlsDtv* next = dtv->next;
    //allocator.free(dtv);
    dtv = next;
  }

  // Clear the DTV slot. The DTV must not be used again with this thread.
  tcb->tls_slot(TLS_SLOT_DTV) = nullptr;
}

// Invokes all the registered thread_exit callbacks, if any.
void __notify_thread_exit_callbacks() {
  TlsModules& modules = __libc_shared_globals()->tls_modules;
  if (modules.first_thread_exit_callback == nullptr) {
    // If there is no first_thread_exit_callback, there shouldn't be a tail.
    CHECK(modules.thread_exit_callback_tail_node == nullptr);
    return;
  }

  // Callbacks are supposed to be invoked in the reverse order
  // in which they were registered.
  CallbackHolder* node = modules.thread_exit_callback_tail_node;
  while (node != nullptr) {
    node->cb();
    node = node->prev;
  }
  modules.first_thread_exit_callback();
}
