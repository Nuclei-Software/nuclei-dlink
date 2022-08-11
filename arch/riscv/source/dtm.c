/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "config.h"
#include "dtm.h"
#include "tap.h"

typedef struct rvl_dtm_s {
    uint8_t abits;
    uint32_t in[2];
    uint32_t out[2];
    uint8_t last_jtag_reg;
    uint8_t idle;
} rvl_dtm_t;

static rvl_dtm_t rvl_dtm_i;
#define self rvl_dtm_i

void rvl_dtm_init(void)
{
    self.last_jtag_reg = 0;

    rvl_tap_init();
    rvl_tap_go_idle();
}

void rvl_dtm_fini(void)
{
    rvl_tap_fini();
}

void rvl_dtm_idcode(rvl_dtm_idcode_t* idcode)
{
    if (self.last_jtag_reg != RISCV_DTM_JTAG_REG_IDCODE) {
        self.in[0] = RISCV_DTM_JTAG_REG_IDCODE;
        self.last_jtag_reg = RISCV_DTM_JTAG_REG_IDCODE;
        rvl_tap_shift_ir(self.out, self.in, 5);
    }

    self.in[0] = 0;
    rvl_tap_shift_dr(self.out, self.in, 32);

    idcode->word = self.out[0];
}

void rvl_dtm_dtmcs_dmireset(void)
{
    if (self.last_jtag_reg != RISCV_DTM_JTAG_REG_DTMCS) {
        self.in[0] = RISCV_DTM_JTAG_REG_DTMCS;
        self.last_jtag_reg = RISCV_DTM_JTAG_REG_DTMCS;
        rvl_tap_shift_ir(self.out, self.in, 5);
    }

    self.in[0] = RISCV_DTMCS_DMI_RESET;
    rvl_tap_shift_dr(self.out, self.in, 32);

    if (self.idle) {
        rvl_tap_run(self.idle);
    }
}

void rvl_dtm_run(uint32_t ticks)
{
    rvl_tap_run(ticks);
}

void rvl_dtm_dtmcs(rvl_dtm_dtmcs_t* dtmcs)
{
    if (self.last_jtag_reg != RISCV_DTM_JTAG_REG_DTMCS) {
        self.in[0] = RISCV_DTM_JTAG_REG_DTMCS;
        self.last_jtag_reg = RISCV_DTM_JTAG_REG_DTMCS;
        rvl_tap_shift_ir(self.out, self.in, 5);
    }

    self.in[0] = 0;
    rvl_tap_shift_dr(self.out, self.in, 32);

    dtmcs->word = self.out[0];

#ifdef RISCV_DEBUG_SPEC_VERSION_V0P13
    self.abits = dtmcs->abits;
#else
    self.abits = dtmcs->loabits | (dtmcs->hiabits << 4);
#endif

    self.idle = dtmcs->idle;

    if (self.idle > 0) {
        self.idle -= 1;
    }
}

#ifdef RISCV_DEBUG_SPEC_VERSION_V0P13
void rvl_dtm_dtmcs_dmihardreset(void)
{
    if (self.last_jtag_reg != RISCV_DTM_JTAG_REG_DTMCS) {
        self.in[0] = RISCV_DTM_JTAG_REG_DTMCS;
        self.last_jtag_reg = RISCV_DTM_JTAG_REG_DTMCS;
        rvl_tap_shift_ir(self.out, self.in, 5);
    }

    self.in[0] = RISCV_DTMCS_DMI_HARD_RESET;
    rvl_tap_shift_dr(self.out, self.in, 32);

    if (self.idle) {
        rvl_tap_run(self.idle);
    }
}
#endif

void rvl_dtm_dmi(uint32_t addr, rvl_dmi_reg_t* data, uint32_t* op)
{
    rvl_assert(addr <= ((1 << self.abits) - 1));
    rvl_assert(data != NULL);
    rvl_assert(op != NULL && *op <= 3);

    if (self.last_jtag_reg != RISCV_DTM_JTAG_REG_DMI) {
        self.in[0] = RISCV_DTM_JTAG_REG_DMI;
        self.last_jtag_reg = RISCV_DTM_JTAG_REG_DMI;
        rvl_tap_shift_ir(self.out, self.in, 5);
    }

#ifdef RISCV_DEBUG_SPEC_VERSION_V0P13
    self.in[0] = (*data << 2) | (*op & 0x3);
    self.in[1] = (*data >> 30) | (addr << 2);
    rvl_tap_shift_dr(self.out, self.in, 32 + 2 + self.abits);
#else
    self.in[0] = (*data << 2) | (*op & 0x3);
    self.in[1] = ((*data >> 30) & 0xf) | (addr << 4);
    rvl_tap_shift_dr(self.out, self.in, 34 + 2 + self.abits);
#endif

    if (self.idle) {
        rvl_tap_run(self.idle);
    }

#ifdef RISCV_DEBUG_SPEC_VERSION_V0P13
    *op = self.out[0] & 0x3;
    *data = (self.out[0] >> 2) | (self.out[1] << 30);
#else
    *op = self.out[0] & 0x3;
    *data = (self.out[0] >> 2) | ((self.out[1] & 0xf) << 30);
#endif
}

uint32_t rvl_dtm_get_dtmcs_abits(void)
{
    return (uint32_t)self.abits;
}

uint32_t rvl_dtm_get_dtmcs_idle(void)
{
    return (uint32_t)self.idle;
}
