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

#ifndef __DTM_H__
#define __DTM_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"
#include "dmi.h"

/*
 * Debug Transport Module (DTM)
 */

#define RISCV_DTM_JTAG_REG_IDCODE           0x01
#define RISCV_DTM_JTAG_REG_DTMCS            0x10
#define RISCV_DTM_JTAG_REG_DMI              0x11
#define RISCV_DTM_JTAG_REG_BYPASS           0x1f


#define RISCV_DTMCS_DMI_RESET           (1 << 16)
#define RISCV_DTMCS_DMI_HARD_RESET      (1 << 17)


typedef union rvl_dtm_idcode_u {
    uint32_t word;
    struct {
        unsigned int fixed_one:     1;
        unsigned int manuf_id:      11;
        unsigned int part_number:   16;
        unsigned int version:       4;
    };
} rvl_dtm_idcode_t;

#ifdef RISCV_DEBUG_SPEC_VERSION_V0P13
typedef union rvl_dtm_dtmcs_u
{
    uint32_t word;
    struct {
        unsigned int version:       4;
        unsigned int abits:         6;
        unsigned int dmistat:       2;
        unsigned int idle:          3;
        unsigned int reserved15:    1;
        unsigned int dmireset:      1;
        unsigned int dmihardreset:  1;
        unsigned int reserved18:    14;
    };
} rvl_dtm_dtmcs_t;
#else
typedef union rvl_dtm_dtmcs_s
{
    uint32_t word;
    struct {
        unsigned int version:       4;
        unsigned int loabits:       4;
        unsigned int dmistat:       2;
        unsigned int idle:          3;
        unsigned int hiabits:       2;
        unsigned int reserved15:    1;
        unsigned int dmireset:      1;
        unsigned int reserved17:    15;
    };
} rvl_dtm_dtmcs_t;
#endif

void rvl_dtm_init(void);
void rvl_dtm_fini(void);
void rvl_dtm_idcode(rvl_dtm_idcode_t* idcode);
void rvl_dtm_dtmcs(rvl_dtm_dtmcs_t* dtmcs);
void rvl_dtm_dtmcs_dmireset(void);

#ifdef RISCV_DEBUG_SPEC_VERSION_V0P13
void rvl_dtm_dtmcs_dmihardreset(void);
#endif

void rvl_dtm_dmi(uint32_t addr, rvl_dmi_reg_t* data, uint32_t* op);
void rvl_dtm_run(uint32_t ticks);
uint32_t rvl_dtm_get_dtmcs_abits(void);
uint32_t rvl_dtm_get_dtmcs_idle(void);

#ifdef __cplusplus
}
#endif

#endif /* __DTM_H__ */
