/*
 * Copyright (c) 2017 Franz-Josef Haider <f_haider@gmx.at>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include "wrapper_code.h"

void
wrapper_code_generic()
{
#ifdef __arm__
    // we can never use r0-r11, neither the stack
    asm volatile(
        // preserve the registers
    ".arm\n\
        push {r0-r11, lr}\n"

    ".arm\n\
        ldr r0, fun\n" // load the function pointer to r0
    ".arm\n\
        ldr r1, name\n" // load the address of the functions name to r1
    ".arm\n\
        ldr r2, str\n" // load the string to print
    ".arm\n\
        ldr r4, tc\n" // load the address of trace_callback to r4
    ".arm\n\
        blx r4\n" // call trace_callback

        // restore the registers
    ".arm\n\
        pop {r0-r11, lr}\n"
    ".arm\n\
        ldr pc, fun\n"     // jump to function

        // dummy instructions, this is where we locate our pointers
        "name: .word 0xFFFFFFFF\n" // name of function to call
        "fun: .word 0xFFFFFFFF\n" // function to call
        "tc: .word 0xFFFFFFFF\n" // address of trace_callback
        "str: .word 0xFFFFFFFF\n" // the string being printed in trace_callback
    );
#elif defined(__aarch64__)
    // we can never use r0-r11, neither the stack
    asm volatile(
        // preserve the registers
        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x0, x1, [sp]\n"    // save 2 pairs of registers
        "stp  x2, x3, [sp,#16]\n"

        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x4, x5, [sp]\n"    // save 2 pairs of registers
        "stp  x6, x7, [sp,#16]\n"

        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x8, x9, [sp]\n"    // save 2 pairs of registers
        "stp  x10, x11, [sp,#16]\n"

        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x12, x13, [sp]\n"  // save 2 pairs of registers
        "stp  x14, x15, [sp,#16]\n"

        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x16, x17, [sp]\n"  // save 2 pairs of registers
        "stp  x18, x19, [sp,#16]\n"

        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x20, x21, [sp]\n"  // save 2 pairs of registers
        "stp  x22, x23, [sp,#16]\n"

        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x24, x25, [sp]\n"  // save 2 pairs of registers
        "stp  x26, x27, [sp,#16]\n"

        "sub  sp, sp, #32\n"     // open up some temp stack space
        "stp  x28, x29, [sp]\n"  // save a pair of registers
        "str  x30, [sp,#16]\n"   // save x30

        "ldr x0, fun\n" // load the function pointer to r0
        "ldr x1, name\n" // load the address of the functions name to r1
        "ldr x2, str\n" // load the string to print
        "ldr x4, tc\n" // load the address of trace_callback to r4
        "blr x4\n" // call trace_callback

        // restore the registers
        "ldp  x28, x29, [sp]\n"  // restore a pair of registers
        "ldr  x30, [sp,#16]\n"   // restore x30
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldp  x24, x25, [sp]\n"  // restore 2 pairs of registers
        "ldp  x26, x27, [sp,#16]\n"
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldp  x20, x21, [sp]\n"  // restore 2 pairs of registers
        "ldp  x22, x23, [sp,#16]\n"
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldp  x16, x17, [sp]\n"  // restore 2 pairs of registers
        "ldp  x18, x19, [sp,#16]\n"
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldp  x12, x13, [sp]\n"  // restore 2 pairs of registers
        "ldp  x14, x15, [sp,#16]\n"
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldp  x8, x9, [sp]\n"    // restore 2 pairs of registers
        "ldp  x10, x11, [sp,#16]\n"
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldp  x4, x5, [sp]\n"    // restore 2 pairs of registers
        "ldp  x6, x7, [sp,#16]\n"
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldp  x0, x1, [sp]\n"    // restore 2 pairs of registers
        "ldp  x2, x3, [sp,#16]\n"
        "add  sp, sp, #32\n"     // "free" the temp stack space

        "ldr  x15, fun\n" // load the function pointer to "temporary" r15
        "br   x15\n"  // branch unconditionally to r15

        // dummy instructions, this is where we locate our pointers
        ".align 3\n" // advance location counter to be multiple of 8
        "name: .dword 0xFFFFFFFFFFFFFFFF\n" // name of function to call
        "fun: .dword 0xFFFFFFFFFFFFFFFF\n" // function to call
        "tc: .dword 0xFFFFFFFFFFFFFFFF\n" // address of trace_callback
        "str: .dword 0xFFFFFFFFFFFFFFFF\n" // the string being printed in trace_callback
    );
#endif
}
