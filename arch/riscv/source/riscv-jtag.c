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
#include "riscv-jtag.h"
#include "port.h"
#include "config.h"

static inline void rvl_jtag_tms_put(int tms)
{
    if (tms) {
        GPIO_BOP(RVL_LINK_TMS_PORT) = (uint32_t)RVL_LINK_TMS_PIN;
    } else {
        GPIO_BC(RVL_LINK_TMS_PORT) = (uint32_t)RVL_LINK_TMS_PIN;
    }
}


static inline void rvl_jtag_tdi_put(int tdi)
{
    if (tdi) {
        GPIO_BOP(RVL_LINK_TDI_PORT) = (uint32_t)RVL_LINK_TDI_PIN;
    } else {
        GPIO_BC(RVL_LINK_TDI_PORT) = (uint32_t)RVL_LINK_TDI_PIN;
    }
}


static inline void rvl_jtag_tck_put(int tck)
{
    if (tck) {
        GPIO_BOP(RVL_LINK_TCK_PORT) = (uint32_t)RVL_LINK_TCK_PIN;
    } else {
        GPIO_BC(RVL_LINK_TCK_PORT) = (uint32_t)RVL_LINK_TCK_PIN;
    }
}

static inline int rvl_jtag_tdo_get()
{
    if ((uint32_t) RESET != (GPIO_ISTAT(RVL_LINK_TDO_PORT) & (RVL_LINK_TDO_PIN))) {
        return 1;
    } else {
        return 0;
    }
}

static inline void rvl_jtag_tck_high_delay()
{
    uint32_t start = __RV_CSR_READ(CSR_MCYCLE);

    while (__RV_CSR_READ(CSR_MCYCLE) - start < RVL_JTAG_TCK_HIGH_DELAY) ;
}

static inline void rvl_jtag_tck_low_delay()
{
    uint32_t start = __RV_CSR_READ(CSR_MCYCLE);

    while (__RV_CSR_READ(CSR_MCYCLE) - start < RVL_JTAG_TCK_LOW_DELAY) ;
}

void rv_jtag_init(void)
{
    rvl_jtag_init();
    rvl_jtag_tms_put(1);
    rvl_jtag_tdi_put(1);
    rvl_jtag_tck_put(0);
}

void rv_jtag_deinit(void)
{
    rvl_jtag_fini();
}

uint32_t rv_jtag_tick(uint32_t tms, uint32_t tdi)
{
    int tdo;

    /*
    *     ___
    * ___|   |
    */

    rvl_jtag_tms_put(tms);
    rvl_jtag_tdi_put(tdi);
    asm volatile("": : :"memory");
    rvl_jtag_tck_low_delay();
    asm volatile("": : :"memory");
    rvl_jtag_tck_put(1);
    asm volatile("": : :"memory");
    tdo = rvl_jtag_tdo_get();
    asm volatile("": : :"memory");
    rvl_jtag_tck_high_delay();
    asm volatile("": : :"memory");
    rvl_jtag_tck_put(0);
    asm volatile("": : :"memory");

    return tdo;
}