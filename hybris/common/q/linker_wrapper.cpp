/*
 * Copyright (C) 2017 The Android Open Source Project
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

#include "private/KernelArgumentBlock.h"

extern const char linker_offset;

// This will be replaced by host_bionic_inject, but must be non-zero
// here so that it's placed in the data section.
uintptr_t original_start = 42;

/* Find the load bias and base address of an executable or shared object loaded
 * by the kernel. The ELF file's PHDR table must have a PT_PHDR entry.
 *
 * A VDSO doesn't have a PT_PHDR entry in its PHDR table.
 */
static void get_elf_base_from_phdr(const ElfW(Phdr)* phdr_table, size_t phdr_count,
                                   ElfW(Addr)* base, ElfW(Addr)* load_bias) {
  for (size_t i = 0; i < phdr_count; ++i) {
    if (phdr_table[i].p_type == PT_PHDR) {
      *load_bias = reinterpret_cast<ElfW(Addr)>(phdr_table) - phdr_table[i].p_vaddr;
      *base = reinterpret_cast<ElfW(Addr)>(phdr_table) - phdr_table[i].p_offset;
      return;
    }
  }
}

/*
 * This is the entry point for the linker wrapper, which finds
 * the real linker, then bootstraps into it.
 */
extern "C" ElfW(Addr) __linker_init(void* raw_args) {
  KernelArgumentBlock args(raw_args);

  ElfW(Addr) base_addr = 0;
  ElfW(Addr) load_bias = 0;
  get_elf_base_from_phdr(
    reinterpret_cast<ElfW(Phdr)*>(args.getauxval(AT_PHDR)), args.getauxval(AT_PHNUM),
    &base_addr, &load_bias);

  ElfW(Addr) linker_addr = base_addr + reinterpret_cast<uintptr_t>(&linker_offset);
  ElfW(Addr) linker_entry_offset = reinterpret_cast<ElfW(Ehdr)*>(linker_addr)->e_entry;

  for (ElfW(auxv_t)* v = args.auxv; v->a_type != AT_NULL; ++v) {
    if (v->a_type == AT_BASE) {
      // Set AT_BASE to the embedded linker
      v->a_un.a_val = linker_addr;
    }
    if (v->a_type == AT_ENTRY) {
      // Set AT_ENTRY to the proper entry point
      v->a_un.a_val = base_addr + original_start;
    }
  }

  // Return address of linker entry point
  return linker_addr + linker_entry_offset;
}
