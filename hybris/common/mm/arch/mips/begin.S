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

    .text
    .align    4
    .type    __start,@function

    .ent    __start
    .globl    __start
    .globl    _start
__start:
_start:
    .set    noreorder
    bal    1f
    nop
1:  .cpload    $31
    .set    reorder

    /* Discover the load address */
    la    $t0, 1f
    bal    1f
1:  subu    $t0, $ra, $t0

#define DT_PLTGOT 3
#define DT_MIPS_LOCAL_GOTNO 0x7000000a

    /* Search dynamic table for DT_MIPS_LOCAL_GOTNO and DT_PLTGOT values */
    la    $t1, _DYNAMIC
    addu    $t1, $t0
    li    $t3, DT_PLTGOT
    li    $t4, DT_MIPS_LOCAL_GOTNO
0:
    lw    $t2, 0($t1)
    beqz    $t2, .Lrelocate_local_got

    bne    $t2, $t3, 1f    /* DT_PLTGOT? */
    lw    $s0, 4($t1)
    addu    $s0, $t0
    b    2f

1:  bne    $t2, $t4, 1f    /* DT_MIPS_LOCAL_GOTNO? */
    lw    $s1, 4($t1)
    b    2f

1:
2:  addu    $t1, 8
    b    0b

.Lrelocate_local_got:
    /*
     * Relocate the local GOT entries
     * got[0] is address of lazy resolver function
     * got[1] may be used for a GNU extension
     */

    addu    $s0, 4
    subu    $s1, 1
    lw    $t1, ($s0)
    bgez    $t1, 9f
    addu    $s0, 4
    subu    $s1, 1
    b    9f

1:  lw    $t1, ($s0)
    addu    $t1, $t0
    sw    $t1, ($s0)
    addu    $s0, 4
9:  subu    $s1, 1
    bgez    $s1, 1b

    /* call linker_init */
    move    $a0, $sp
    addiu    $sp, -4*4        /* space for arg saves in linker_init */
    la    $t9, __linker_init
    jalr    $t9
    move    $t9, $v0
    addu    $sp, 4*4        /* restore sp */
    j    $t9
    .end    __start
