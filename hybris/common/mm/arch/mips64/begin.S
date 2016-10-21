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

#include <private/bionic_asm.h>

#if (_MIPS_SIM == _ABIO32) || (_MIPS_SIM == _ABI32)
#define ELF_DYNSZ       8
#define ELF_DYN_TAG     0
#define ELF_DYN_VAL     4
#define GOTENT_SZ       4
#else
#define ELF_DYNSZ       16
#define ELF_DYN_TAG     0
#define ELF_DYN_VAL     8
#define GOTENT_SZ       8
#endif

    .text
    .align	4
    .type	__start,@function

    .ent	__start
    .globl	__start
    .globl	_start
__start:
_start:
    .set	noreorder
    bal		1f
     nop
1:
#if (_MIPS_SIM == _ABIO32) || (_MIPS_SIM == _ABI32)
    .cpload	ra
#else
    .cpsetup	ra, $0, 1b
#endif
    .set	reorder

    /* Discover the load address */
    LA		t0, 1b
    PTR_SUBU	t0, ra, t0

#define DT_PLTGOT 3
#define DT_MIPS_LOCAL_GOTNO 0x7000000a

    /* Search dynamic table for DT_MIPS_LOCAL_GOTNO and DT_PLTGOT values */
    LA		t1, _DYNAMIC
    PTR_ADDU	t1, t0
    LI		t3, DT_PLTGOT
    LI		ta0, DT_MIPS_LOCAL_GOTNO
0:
    REG_L	t2, ELF_DYN_TAG(t1)
    beqz	t2, .Lrelocate_local_got

    bne		t2, t3, 1f	/* DT_PLTGOT? */
    REG_L	s0, ELF_DYN_VAL(t1)
    PTR_ADDU	s0, t0
    b		2f

1:
    bne		t2, ta0, 2f    /* DT_MIPS_LOCAL_GOTNO? */
    REG_L	s1, ELF_DYN_VAL(t1)

2:  PTR_ADDU    t1, ELF_DYNSZ
    b		0b

.Lrelocate_local_got:
    /*
     * Relocate the local GOT entries
     * got[0] is address of lazy resolver function
     * got[1] may be used for a GNU extension
     */

    PTR_ADDU	s0, GOTENT_SZ
    SUBU	s1, 1
    PTR_L	t1, (s0)
    bgez	t1, 9f
    PTR_ADDU	s0, GOTENT_SZ
    SUBU	s1, 1
    b		9f

1:  PTR_L	t1, (s0)
    PTR_ADDU	t1, t0
    PTR_S	t1, (s0)
    PTR_ADDU	s0, GOTENT_SZ
9:  SUBU	s1, 1
    bgez	s1, 1b

    /* call linker_init */
    move	a0, sp
#if (_MIPS_SIM == _ABIO32) || (_MIPS_SIM == _ABI32)
    PTR_SUBU	sp, 4*REGSZ       /* space for arg saves in linker_init */
#endif
    jal		__linker_init
#if (_MIPS_SIM == _ABIO32) || (_MIPS_SIM == _ABI32)
    PTR_ADDU	sp, 4*REGSZ        /* restore sp */
#endif
    move	t9, v0
    j		t9
    .end    __start
