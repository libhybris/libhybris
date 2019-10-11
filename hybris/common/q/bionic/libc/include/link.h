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
#ifndef _LINK_H_
#define _LINK_H_

#include <android/versioning.h> // for __INTRODUCED_IN

#include <stdint.h>
#include <sys/cdefs.h>
#include <sys/types.h>

#include <elf.h>

__BEGIN_DECLS

typedef Elf32_Word Elf32_Relr;
typedef Elf64_Xword Elf64_Relr;

#if defined(__LP64__)
#define ElfW(type) Elf64_ ## type
#else
#define ElfW(type) Elf32_ ## type
#endif

struct dl_phdr_info {
  ElfW(Addr) dlpi_addr;
  const char* dlpi_name;
  const ElfW(Phdr)* dlpi_phdr;
  ElfW(Half) dlpi_phnum;
};

#if defined(__arm__)
int dl_iterate_phdr(int (*__callback)(struct dl_phdr_info*, size_t, void*), void* __data) __INTRODUCED_IN(21);
#else
int dl_iterate_phdr(int (*__callback)(struct dl_phdr_info*, size_t, void*), void* __data);
#endif

#ifdef __arm__
typedef uintptr_t _Unwind_Ptr;
_Unwind_Ptr dl_unwind_find_exidx(_Unwind_Ptr, int*);
#endif

/* Used by the dynamic linker to communicate with the debugger. */
struct link_map {
  ElfW(Addr) l_addr;
  char* l_name;
  ElfW(Dyn)* l_ld;
  struct link_map* l_next;
  struct link_map* l_prev;
};

/* Used by the dynamic linker to communicate with the debugger. */
struct r_debug {
  int32_t r_version;
  struct link_map* r_map;
  ElfW(Addr) r_brk;
  enum {
    RT_CONSISTENT,
    RT_ADD,
    RT_DELETE
  } r_state;
  ElfW(Addr) r_ldbase;
};

__END_DECLS

#endif
