/*
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 * Copyright (c) 2022 Nuclei Limited. All rights reserved.
 *
 * \file  rv-link/main.c
 * \brief Main RV-Link application.
 *
 * RV-LINK is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v1 for more details.
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
