/*
 * Copyright (c) 2025 Nikita Ukhrenkov <thekit@disroot.org>
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

#include <stdint.h>
#include <stddef.h>
#include "logging.h"

#define MRS_OPCODE 0xD53
#define TPIDR_EL0 0x5E82

#define LDR_OPCODE 0xE5
#define STR_OPCODE 0xE4

#define TLS_SLOT_APP 2
#define TLS_SLOT_STACK_GUARD 5

/* Instruction format for MRS X<rt>, TPIDR_EL0 */
struct __attribute__((__packed__)) mrs_inst {
    uint8_t rt:5;        /* 0:4   - Destination register */
    uint16_t sys_reg:15; /* 5:19  - System register encoding */
    uint16_t opcode:12;  /* 20:31 - Must be 0xD53 (MRS instruction) */
};
_Static_assert(sizeof(struct mrs_inst) == 4, "MRS instruction size must be 4");

/* Instruction format for LDR X<rt>, [X<rn>, #imm]
                      and STR X<rt>, [X<rn>, #imm] */
struct __attribute__((__packed__)) ldr_inst {
    uint32_t rt:5;       /* 0:4   - Destination register */
    uint32_t rn:5;       /* 5:9   - Base register */
    uint32_t imm12:12;   /* 10:21 - 12-bit immediate scaled by 8 */
    uint32_t opcode:8;   /* 22:29 - Must be 0xe5 (LDR with unsigned offset) */
    uint32_t size:2;     /* 30:31 - Must be 0b11 for 64-bit load */
};
_Static_assert(sizeof(struct ldr_inst) == 4, "LDR instruction size must be 4");

void hybris_patch_tls_arch(void* segment_addr, size_t segment_size, int tls_offset) {
    static int error_reported = 0;

    /* LDR immediate must fit in 12 bits */
    if (tls_offset > 0xFFF) {
        if (!error_reported) {
            fprintf(stderr,
                "HYBRIS_TLS_PATCH: TLS area offset %d is too large for LDR instruction",
                tls_offset);
            error_reported = 1;
        }
        return;
    }

    uint32_t* text = (uint32_t*)segment_addr;
    size_t count = segment_size / sizeof(uint32_t);

    for (size_t i = 0; i < count; i++) {
        const struct mrs_inst* mrs = (const struct mrs_inst*)&text[i];

        /* Look for MRS instruction that reads TPIDR_EL0 */
        if (mrs->opcode != MRS_OPCODE || mrs->sys_reg != TPIDR_EL0) {
            continue;
        }

        /* Found MRS X<rt>, TPIDR_EL0, scan next few instructions for matching LDR */
        for (int j = 1; j < 5 && (i + j) < count; j++) {
            struct ldr_inst* ldr = (struct ldr_inst*)&text[i + j];

            /* Check for LDR or STR instruction using the same register as MRS */
            if ((ldr->opcode == LDR_OPCODE || ldr->opcode == STR_OPCODE)
                 && ldr->size == 0b11 && ldr->rn == mrs->rt) {
                if (ldr->imm12 > TLS_SLOT_APP && ldr->imm12 < TLS_SLOT_STACK_GUARD) {
                    ldr->imm12 += tls_offset;
                }
                break;
            }
        }
    }
}
