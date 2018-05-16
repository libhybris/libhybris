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

#if !defined(__LP64__) && __mips_isa_rev >= 5
#include <sys/prctl.h>
#endif

#include "linker.h"
#include "linker_debug.h"
#include "linker_globals.h"
#include "linker_phdr.h"
#include "linker_relocs.h"
#include "linker_reloc_iterators.h"
#include "linker_sleb128.h"
#include "linker_soinfo.h"

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

    DEBUG("Processing \"%s\" relocation at index %zd", get_realpath(), idx);
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
        DL_ERR("cannot locate symbol \"%s\" referenced by \"%s\"...",
               sym_name, get_realpath());
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
        DL_ERR("%s: invalid symbol \"%s\" (PROTECTED/UNDEFINED) ",
               get_realpath(), sym_name);
        return false;
      }
      s = local_sym;
      lsi = this;
    } else {
      DL_ERR("%s: invalid symbol \"%s\" visibility: 0x%x",
             get_realpath(), sym_name, st_visibility);
      return false;
    }

    if (s == nullptr) {
      // We only allow an undefined symbol if this is a weak reference.
      if (ELF_ST_BIND(local_sym->st_info) != STB_WEAK) {
        DL_ERR("%s: cannot locate \"%s\"...", get_realpath(), sym_name);
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

#if !defined(__LP64__)

// Checks for mips32's various floating point abis.
// (Mips64 Android has a single floating point abi and doesn't need any checks)

// Linux kernel has declarations similar to the following
//   in <linux>/arch/mips/include/asm/elf.h,
// but that non-uapi internal header file will never be imported
// into bionic's kernel headers.

#define PT_MIPS_ABIFLAGS  0x70000003	// is .MIPS.abiflags segment

struct mips_elf_abiflags_v0 {
  uint16_t version;  // version of this structure
  uint8_t  isa_level, isa_rev, gpr_size, cpr1_size, cpr2_size;
  uint8_t  fp_abi;  // mips32 ABI variants for floating point
  uint32_t isa_ext, ases, flags1, flags2;
};

// Bits of flags1:
#define MIPS_AFL_FLAGS1_ODDSPREG 1  // Uses odd-numbered single-prec fp regs

// Some values of fp_abi:        via compiler flag:
#define MIPS_ABI_FP_DOUBLE 1  // -mdouble-float
#define MIPS_ABI_FP_XX     5  // -mfpxx
#define MIPS_ABI_FP_64A    7  // -mips32r* -mfp64 -mno-odd-spreg

#if __mips_isa_rev >= 5
static bool mips_fre_mode_on = false;  // have set FRE=1 mode for process
#endif

bool soinfo::mips_check_and_adjust_fp_modes() {
  mips_elf_abiflags_v0* abiflags = nullptr;
  int mips_fpabi;

  // Find soinfo's optional .MIPS.abiflags segment
  for (size_t i = 0; i<phnum; ++i) {
    const ElfW(Phdr)& ph = phdr[i];
    if (ph.p_type == PT_MIPS_ABIFLAGS) {
      if (ph.p_filesz < sizeof (mips_elf_abiflags_v0)) {
        DL_ERR("Corrupt PT_MIPS_ABIFLAGS header found \"%s\"", get_realpath());
        return false;
      }
      abiflags = reinterpret_cast<mips_elf_abiflags_v0*>(ph.p_vaddr + load_bias);
      break;
    }
  }

  // FP ABI-variant compatibility checks for MIPS o32 ABI
  if (abiflags == nullptr) {
    // Old compilers lack the new abiflags section.
    // These compilers used -mfp32 -mdouble-float -modd-spreg defaults,
    //   ie FP32 aka DOUBLE, using odd-numbered single-prec regs
    mips_fpabi = MIPS_ABI_FP_DOUBLE;
  } else {
    mips_fpabi = abiflags->fp_abi;
    if ( (abiflags->flags1 & MIPS_AFL_FLAGS1_ODDSPREG)
         && (mips_fpabi == MIPS_ABI_FP_XX ||
             mips_fpabi == MIPS_ABI_FP_64A   ) ) {
      // Android supports fewer cases than Linux
      DL_ERR("Unsupported odd-single-prec FloatPt reg uses in \"%s\"",
             get_realpath());
      return false;
    }
  }
  if (!(mips_fpabi == MIPS_ABI_FP_DOUBLE ||
#if __mips_isa_rev >= 5
        mips_fpabi == MIPS_ABI_FP_64A    ||
#endif
        mips_fpabi == MIPS_ABI_FP_XX       )) {
    DL_ERR("Unsupported MIPS32 FloatPt ABI %d found in \"%s\"",
           mips_fpabi, get_realpath());
    return false;
  }

#if __mips_isa_rev >= 5
  // Adjust process's FR Emulation mode, if needed
  //
  // On Mips R5 & R6, Android runs continuously in FR=1 64bit-fpreg mode.
  // NDK mips32 apps compiled with old compilers generate FP32 code
  //   which expects FR=0 32-bit fp registers.
  // NDK mips32 apps compiled with newer compilers generate modeless
  //   FPXX code which runs on both FR=0 and FR=1 modes.
  // Android itself is compiled in FP64A which requires FR=1 mode.
  // FP32, FPXX, and FP64A all interlink okay, without dynamic FR mode
  //   changes during calls.  For details, see
  //   http://dmz-portal.mips.com/wiki/MIPS_O32_ABI_-_FR0_and_FR1_Interlinking
  // Processes containing FR32 FR=0 code are run via kernel software assist,
  //   which maps all odd-numbered single-precision reg refs onto the
  //   upper half of the paired even-numbered double-precision reg.
  // FRE=1 triggers traps to the kernel's emulator on every single-precision
  //   fp op (for both odd and even-numbered registers).
  // Turning on FRE=1 traps is done at most once per process, simultanously
  //   for all threads of that process, when dlopen discovers FP32 code.
  // The kernel repacks threads' registers when FRE mode is turn on or off.
  //   These asynchronous adjustments are wrong if any thread was executing
  //   FPXX code using odd-numbered single-precision regs.
  // Current Android compilers default to the -mno-oddspreg option,
  //   and this requirement is checked by Android's dlopen.
  //   So FRE can always be safely turned on for FP32, anytime.
  // Deferred enhancement: Allow loading of odd-spreg FPXX modules.

  if (mips_fpabi == MIPS_ABI_FP_DOUBLE && !mips_fre_mode_on) {
    // Turn on FRE mode, which emulates mode-sensitive FR=0 code on FR=1
    //   register files, by trapping to kernel on refs to single-precision regs
    if (prctl(PR_SET_FP_MODE, PR_FP_MODE_FR|PR_FP_MODE_FRE)) {
      DL_ERR("Kernel or cpu failed to set FRE mode required for running \"%s\"",
             get_realpath());
      return false;
    }
    DL_WARN("Using FRE=1 mode to run \"%s\"", get_realpath());
    mips_fre_mode_on = true;  // Avoid future redundant mode-switch calls
    // FRE mode is never turned back off.
    // Deferred enhancement:
    //   Reset FRE mode when dlclose() removes all FP32 modules
  }
#else
  // Android runs continuously in FR=0 32bit-fpreg mode.
#endif  // __mips_isa_rev
  return true;
}

#endif  // __LP64___
