#ifndef __RV_LINK_TARGET_ARCH_RISCV_DTM_H__
#define __RV_LINK_TARGET_ARCH_RISCV_DTM_H__
/**
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2021, Micha Hoiting <micha.hoiting@gmail.com>
 *
 * \file  rv-link/target/arch/riscv/dtm.h
 * \brief Handling of the JTAG Debug Transport Module (DTM) interface.
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

/* system library header file includes */
#include <stdint.h>

/* other library header file includes */

/* own component header file includes */
#include "target-config.h"

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

#if RVL_TARGET_CONFIG_RISCV_DEBUG_SPEC == RISCV_DEBUG_SPEC_VERSION_V0P13
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
#elif RVL_TARGET_CONFIG_RISCV_DEBUG_SPEC == RISCV_DEBUG_SPEC_VERSION_V0P11
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
#else
#error No RVL_TARGET_CONFIG_RISCV_DEBUG_SPEC defined
#endif

void rvl_dtm_init(void);
void rvl_dtm_fini(void);
void rvl_dtm_idcode(rvl_dtm_idcode_t* idcode);
void rvl_dtm_dtmcs(rvl_dtm_dtmcs_t* dtmcs);
void rvl_dtm_dtmcs_dmireset(void);

#if RVL_TARGET_CONFIG_RISCV_DEBUG_SPEC == RISCV_DEBUG_SPEC_VERSION_V0P13
void rvl_dtm_dtmcs_dmihardreset(void);
#endif

void rvl_dtm_dmi(uint32_t addr, rvl_dmi_reg_t* data, uint32_t* op);
void rvl_dtm_run(uint32_t ticks);
uint32_t rvl_dtm_get_dtmcs_abits(void);
uint32_t rvl_dtm_get_dtmcs_idle(void);

#endif /* __RV_LINK_TARGET_ARCH_RISCV_DTM_H__ */
