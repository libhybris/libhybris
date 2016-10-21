/*
 * Copyright (C) 2015 The Android Open Source Project
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

#include "linker.h"
#include "linker_debug.h"
#include "linker_relocs.h"
#include "linker_reloc_iterators.h"
#include "linker_sleb128.h"

template bool soinfo::relocate<plain_reloc_iterator>(const VersionTracker& version_tracker,
                                                     plain_reloc_iterator&& rel_iterator,
                                                     const soinfo_list_t& global_group,
                                                     const soinfo_list_t& local_group);

template bool soinfo::relocate<packed_reloc_iterator<sleb128_decoder>>(
    const VersionTracker& version_tracker,
    packed_reloc_iterator<sleb128_decoder>&& rel_iterator,
    const soinfo_list_t& global_group,
    const soinfo_list_t& local_group);

template <typename ElfRelIteratorT>
bool soinfo::relocate(const VersionTracker& version_tracker,
                      ElfRelIteratorT&& rel_iterator,
                      const soinfo_list_t& global_group,
                      const soinfo_list_t& local_group) {
  for (size_t idx = 0; rel_iterator.has_next(); ++idx) {
    const auto rel = rel_iterator.next();

    if (rel == nullptr) {
      return false;
    }

    ElfW(Word) type = ELFW(R_TYPE)(rel->r_info);
    ElfW(Word) sym = ELFW(R_SYM)(rel->r_info);

    ElfW(Addr) reloc = static_cast<ElfW(Addr)>(rel->r_offset + load_bias);
    ElfW(Addr) sym_addr = 0;
    const char* sym_name = nullptr;

    DEBUG("Processing '%s' relocation at index %zd", get_soname(), idx);
    if (type == R_GENERIC_NONE) {
      continue;
    }

    const ElfW(Sym)* s = nullptr;
    soinfo* lsi = nullptr;

    if (sym != 0) {
      sym_name = get_string(symtab_[sym].st_name);
      const version_info* vi = nullptr;

      if (!lookup_version_info(version_tracker, sym, sym_name, &vi)) {
        return false;
      }

      if (!soinfo_do_lookup(this, sym_name, vi, &lsi, global_group, local_group, &s)) {
        return false;
      }

      if (s == nullptr) {
        // mips does not support relocation with weak-undefined symbols
        DL_ERR("cannot locate symbol \"%s\" referenced by \"%s\"...", sym_name, get_soname());
        return false;
      } else {
        // We got a definition.
        sym_addr = lsi->resolve_symbol_address(s);
      }
      count_relocation(kRelocSymbol);
    }

    switch (type) {
      case R_MIPS_REL32:
#if defined(__LP64__)
        // MIPS Elf64_Rel entries contain compound relocations
        // We only handle the R_MIPS_NONE|R_MIPS_64|R_MIPS_REL32 case
        if (ELF64_R_TYPE2(rel->r_info) != R_MIPS_64 ||
            ELF64_R_TYPE3(rel->r_info) != R_MIPS_NONE) {
          DL_ERR("Unexpected compound relocation type:%d type2:%d type3:%d @ %p (%zu)",
                 type, static_cast<unsigned>(ELF64_R_TYPE2(rel->r_info)),
                 static_cast<unsigned>(ELF64_R_TYPE3(rel->r_info)), rel, idx);
          return false;
        }
#endif
        count_relocation(s == nullptr ? kRelocAbsolute : kRelocRelative);
        MARK(rel->r_offset);
        TRACE_TYPE(RELO, "RELO REL32 %08zx <- %08zx %s", static_cast<size_t>(reloc),
                   static_cast<size_t>(sym_addr), sym_name ? sym_name : "*SECTIONHDR*");
        if (s != nullptr) {
          *reinterpret_cast<ElfW(Addr)*>(reloc) += sym_addr;
        } else {
          *reinterpret_cast<ElfW(Addr)*>(reloc) += load_bias;
        }
        break;
      default:
        DL_ERR("unknown reloc type %d @ %p (%zu)", type, rel, idx);
        return false;
    }
  }
  return true;
}

bool soinfo::mips_relocate_got(const VersionTracker& version_tracker,
                               const soinfo_list_t& global_group,
                               const soinfo_list_t& local_group) {
  ElfW(Addr)** got = plt_got_;
  if (got == nullptr) {
    return true;
  }

  // got[0] is the address of the lazy resolver function.
  // got[1] may be used for a GNU extension.
  // Set it to a recognizable address in case someone calls it (should be _rtld_bind_start).
  // FIXME: maybe this should be in a separate routine?
  if ((flags_ & FLAG_LINKER) == 0) {
    size_t g = 0;
    got[g++] = reinterpret_cast<ElfW(Addr)*>(0xdeadbeef);
    if (reinterpret_cast<intptr_t>(got[g]) < 0) {
      got[g++] = reinterpret_cast<ElfW(Addr)*>(0xdeadfeed);
    }
    // Relocate the local GOT entries.
    for (; g < mips_local_gotno_; g++) {
      got[g] = reinterpret_cast<ElfW(Addr)*>(reinterpret_cast<uintptr_t>(got[g]) + load_bias);
    }
  }

  // Now for the global GOT entries...
  got = plt_got_ + mips_local_gotno_;
  for (ElfW(Word) sym = mips_gotsym_; sym < mips_symtabno_; sym++, got++) {
    // This is an undefined reference... try to locate it.
    const ElfW(Sym)* local_sym = symtab_ + sym;
    const char* sym_name = get_string(local_sym->st_name);
    soinfo* lsi = nullptr;
    const ElfW(Sym)* s = nullptr;

    ElfW(Word) st_visibility = (local_sym->st_other & 0x3);

    if (st_visibility == STV_DEFAULT) {
      const version_info* vi = nullptr;

      if (!lookup_version_info(version_tracker, sym, sym_name, &vi)) {
        return false;
      }

      if (!soinfo_do_lookup(this, sym_name, vi, &lsi, global_group, local_group, &s)) {
        return false;
      }
    } else if (st_visibility == STV_PROTECTED) {
      if (local_sym->st_value == 0) {
        DL_ERR("%s: invalid symbol \"%s\" (PROTECTED/UNDEFINED) ", get_soname(), sym_name);
        return false;
      }
      s = local_sym;
      lsi = this;
    } else {
      DL_ERR("%s: invalid symbol \"%s\" visibility: 0x%x", get_soname(), sym_name, st_visibility);
      return false;
    }

    if (s == nullptr) {
      // We only allow an undefined symbol if this is a weak reference.
      if (ELF_ST_BIND(local_sym->st_info) != STB_WEAK) {
        DL_ERR("%s: cannot locate \"%s\"...", get_soname(), sym_name);
        return false;
      }
      *got = 0;
    } else {
      // FIXME: is this sufficient?
      // For reference see NetBSD link loader
      // http://cvsweb.netbsd.org/bsdweb.cgi/src/libexec/ld.elf_so/arch/mips/mips_reloc.c?rev=1.53&content-type=text/x-cvsweb-markup
      *got = reinterpret_cast<ElfW(Addr)*>(lsi->resolve_symbol_address(s));
    }
  }
  return true;
}

