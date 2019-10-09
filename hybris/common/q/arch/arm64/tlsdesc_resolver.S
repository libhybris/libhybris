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

#include <private/bionic_asm.h>
#include <private/bionic_asm_tls.h>

.globl __tls_get_addr

// These resolver functions must preserve every register except x0. They set x0
// to the offset of the TLS symbol relative to the thread pointer.

ENTRY_PRIVATE(tlsdesc_resolver_static)
  ldr x0, [x0, #8]
  ret
END(tlsdesc_resolver_static)

ENTRY_PRIVATE(tlsdesc_resolver_dynamic)
  stp x19, x20, [sp, #-32]!
  .cfi_def_cfa_offset 32
  .cfi_rel_offset x19, 0
  .cfi_rel_offset x20, 8
  stp x21, x22, [sp, #16]
  .cfi_rel_offset x21, 16
  .cfi_rel_offset x22, 24

  mrs x19, tpidr_el0            // __get_tls()
  ldr x20, [x19, #(TLS_SLOT_DTV * 8)]
  ldr x21, [x20]                // TlsDtv::generation

  ldr x0, [x0, #8]              // TlsDynamicResolverArg*
  ldr x22, [x0]                 // TlsDynamicResolverArg::generation

  cmp x21, x22
  b.lo .fallback

  ldr x21, [x0, #8]             // TlsIndex::module
  ldr x22, [x0, #16]            // TlsIndex::offset
  ldr x21, [x20, x21, lsl #3]   // TlsDtv::modules[module]
  cbz x21, .fallback
  add x0, x21, x22
  sub x0, x0, x19

  ldp x21, x22, [sp, #16]
  .cfi_remember_state
  .cfi_restore x21
  .cfi_restore x22
  ldp x19, x20, [sp], #32
  .cfi_adjust_cfa_offset -32
  .cfi_restore x19
  .cfi_restore x20
  ret

.fallback:
  .cfi_restore_state
  ldp x21, x22, [sp, #16]
  .cfi_restore x21
  .cfi_restore x22
  ldp x19, x20, [sp], #32
  .cfi_adjust_cfa_offset -32
  .cfi_restore x19
  .cfi_restore x20
  b tlsdesc_resolver_dynamic_slow_path
END(tlsdesc_resolver_dynamic)

#define SAVE_REG(x, slot)                 \
    str x, [sp, #((slot) * 8)];           \
    .cfi_rel_offset x, (slot) * 8;        \

#define SAVE_GPR_PAIR(x, y, slot)         \
    stp x, y, [sp, #((slot) * 8)];        \
    .cfi_rel_offset x, (slot) * 8;        \
    .cfi_rel_offset y, ((slot) + 1) * 8;  \

#define SAVE_VEC_PAIR(x, y, slot)         \
    stp x, y, [sp, #((slot) * 8)];        \
    .cfi_rel_offset x, (slot) * 8;        \
    .cfi_rel_offset y, ((slot) + 2) * 8;  \

#define RESTORE_REG(x, slot)              \
    ldr x, [sp, #((slot) * 8)];           \
    .cfi_restore x;                       \

#define RESTORE_REG_PAIR(x, y, slot)      \
    ldp x, y, [sp, #((slot) * 8)];        \
    .cfi_restore x;                       \
    .cfi_restore y;                       \

// On entry, x0 is the address of a TlsDynamicResolverArg object rather than
// the TlsDescriptor address passed to the original resolver function.
ENTRY_PRIVATE(tlsdesc_resolver_dynamic_slow_path)
  sub sp, sp, #(8 * 84)
  .cfi_def_cfa_offset (8 * 84)
  SAVE_GPR_PAIR(x29, x30, 0)
  mov x29, sp

  // Avoid leaking the contents of the shadow call stack register (x18) into
  // memory. x19 through x29 are callee-save registers, so we do not need to
  // save them.
  SAVE_GPR_PAIR(x1,  x2,  2)
  SAVE_GPR_PAIR(x3,  x4,  4)
  SAVE_GPR_PAIR(x5,  x6,  6)
  SAVE_GPR_PAIR(x7,  x8,  8)
  SAVE_GPR_PAIR(x9,  x10, 10)
  SAVE_GPR_PAIR(x11, x12, 12)
  SAVE_GPR_PAIR(x13, x14, 14)
  SAVE_GPR_PAIR(x15, x16, 16)
  SAVE_REG(x17, 18)

  SAVE_VEC_PAIR(q0,  q1,  20)
  SAVE_VEC_PAIR(q2,  q3,  24)
  SAVE_VEC_PAIR(q4,  q5,  28)
  SAVE_VEC_PAIR(q6,  q7,  32)
  SAVE_VEC_PAIR(q8,  q9,  36)
  SAVE_VEC_PAIR(q10, q11, 40)
  SAVE_VEC_PAIR(q12, q13, 44)
  SAVE_VEC_PAIR(q14, q15, 48)
  SAVE_VEC_PAIR(q16, q17, 52)
  SAVE_VEC_PAIR(q18, q19, 56)
  SAVE_VEC_PAIR(q20, q21, 60)
  SAVE_VEC_PAIR(q22, q23, 64)
  SAVE_VEC_PAIR(q24, q25, 68)
  SAVE_VEC_PAIR(q26, q27, 72)
  SAVE_VEC_PAIR(q28, q29, 76)
  SAVE_VEC_PAIR(q30, q31, 80)

  add x0, x0, #8
  bl __tls_get_addr
  mrs x1, tpidr_el0 // __get_tls()
  sub x0, x0, x1

  RESTORE_REG_PAIR(q30, q31, 80)
  RESTORE_REG_PAIR(q28, q29, 76)
  RESTORE_REG_PAIR(q26, q27, 72)
  RESTORE_REG_PAIR(q24, q25, 68)
  RESTORE_REG_PAIR(q22, q23, 64)
  RESTORE_REG_PAIR(q20, q21, 60)
  RESTORE_REG_PAIR(q18, q19, 56)
  RESTORE_REG_PAIR(q16, q17, 52)
  RESTORE_REG_PAIR(q14, q15, 48)
  RESTORE_REG_PAIR(q12, q13, 44)
  RESTORE_REG_PAIR(q10, q11, 40)
  RESTORE_REG_PAIR(q8,  q9,  36)
  RESTORE_REG_PAIR(q6,  q7,  32)
  RESTORE_REG_PAIR(q4,  q5,  28)
  RESTORE_REG_PAIR(q2,  q3,  24)
  RESTORE_REG_PAIR(q0,  q1,  20)

  RESTORE_REG(x17, 18)
  RESTORE_REG_PAIR(x15, x16, 16)
  RESTORE_REG_PAIR(x13, x14, 14)
  RESTORE_REG_PAIR(x11, x12, 12)
  RESTORE_REG_PAIR(x9,  x10, 10)
  RESTORE_REG_PAIR(x7,  x8,  8)
  RESTORE_REG_PAIR(x5,  x6,  6)
  RESTORE_REG_PAIR(x3,  x4,  4)
  RESTORE_REG_PAIR(x1,  x2,  2)

  RESTORE_REG_PAIR(x29, x30, 0)
  add sp, sp, #(8 * 84)
  .cfi_def_cfa_offset 0
  ret
END(tlsdesc_resolver_dynamic_slow_path)

// The address of an unresolved weak TLS symbol evaluates to NULL with TLSDESC.
// The value returned by this function is added to the thread pointer, so return
// a negated thread pointer to cancel it out.
ENTRY_PRIVATE(tlsdesc_resolver_unresolved_weak)
  str x19, [sp, #-16]!
  .cfi_def_cfa_offset 16
  .cfi_rel_offset x19, 0
  ldr x19, [x0, #8]
  mrs x0, tpidr_el0             // __get_tls()
  sub x0, x19, x0
  ldr x19, [sp], #16
  .cfi_def_cfa_offset 0
  .cfi_restore x19
  ret
END(tlsdesc_resolver_unresolved_weak)
