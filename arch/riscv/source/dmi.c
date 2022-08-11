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
#include "dmi.h"
#include "dtm.h"

typedef struct rvl_dmi_s
{
    uint32_t i;
    rvl_dmi_reg_t data;
    uint32_t op;
}rvl_dmi_t;

static rvl_dmi_t rvl_dmi_i;
#define self rvl_dmi_i


#ifndef RVL_TARGET_CONFIG_DMI_RETRIES
#define RVL_TARGET_CONFIG_DMI_RETRIES   6
#endif

void rvl_dmi_init(void)
{
    rvl_dtm_init();
}

void rvl_dmi_fini(void)
{
    rvl_dtm_fini();
}

void rvl_dmi_nop(void)
{
    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    rvl_dtm_dmi(0, &self.data, &self.op);
}

void rvl_dmi_read(uint32_t addr, rvl_dmi_reg_t* data, uint32_t *result)
{
    self.data = 0;
    self.op = RISCV_DMI_OP_READ;

    rvl_dtm_dmi(addr, &self.data, &self.op);

    for(self.i = 0; self.i < RVL_TARGET_CONFIG_DMI_RETRIES; self.i++) {
        self.data = 0;
        self.op = RISCV_DMI_OP_NOP;

        rvl_dtm_dmi(0, &self.data, &self.op);

        if(self.op == RISCV_DMI_RESULT_BUSY) {
            rvl_dtm_run(32);
        } else {
            break;
        }
    }

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        rvl_dtm_dtmcs_dmireset();
    } else {
        *data = self.data;
    }

    if(result) {
        *result = self.op;
    }
}

void rvl_dmi_write(uint32_t addr, rvl_dmi_reg_t data, uint32_t *result)
{
    self.data = data;
    self.op = RISCV_DMI_OP_WRITE;

    rvl_dtm_dmi(addr, &self.data, &self.op);

    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    rvl_dtm_dmi(0, &self.data, &self.op);

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        rvl_dtm_dtmcs_dmireset();
    }

    if(result) {
        *result = self.op;
    }
}

void rvl_dmi_read_vector(uint32_t start_addr, rvl_dmi_reg_t* buffer, uint32_t len, uint32_t *result)
{
    for(self.i = 0; self.i < len; self.i++) {
        self.data = 0;
        self.op = RISCV_DMI_OP_READ;
        rvl_dtm_dmi(start_addr + self.i, &self.data, &self.op);
        if(self.i > 0) {
            buffer[self.i - 1] = self.data;
        }
    }

    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    rvl_dtm_dmi(0, &self.data, &self.op);
    buffer[self.i - 1] = self.data;

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        rvl_dtm_dtmcs_dmireset();
    }

    *result = self.op;
}

void rvl_dmi_write_vector(uint32_t start_addr, const rvl_dmi_reg_t* buffer, uint32_t len, uint32_t *result)
{
    for(self.i = 0; self.i < len; self.i++) {
        self.data = buffer[self.i];
        self.op = RISCV_DMI_OP_WRITE;
        rvl_dtm_dmi(start_addr + self.i, &self.data, &self.op);
    }

    self.data = 0;
    self.op = RISCV_DMI_OP_NOP;

    rvl_dtm_dmi(0, &self.data, &self.op);

    if(self.op != RISCV_DMI_RESULT_DONE)
    {
        rvl_dtm_dtmcs_dmireset();
    }

    *result = self.op;
}
