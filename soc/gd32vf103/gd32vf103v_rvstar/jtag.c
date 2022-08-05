/**
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 *
 * \file  rv-link/link/arch/gd32vf103/longan-nano/jtag.c
 * \brief JTAG functions specifically for the Longan Nano board.
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

/* other library header file includes */
#include "nuclei_sdk_soc.h"
#include "config.h"

void rvl_jtag_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(RVL_LINK_TCK_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, RVL_LINK_TCK_PIN);
    gpio_init(RVL_LINK_TMS_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, RVL_LINK_TMS_PIN);
    gpio_init(RVL_LINK_TDI_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, RVL_LINK_TDI_PIN);
    gpio_init(RVL_LINK_TDO_PORT, GPIO_MODE_IPU, 0, RVL_LINK_TDO_PIN);
}


void rvl_jtag_fini(void)
{
    gpio_init(RVL_LINK_TCK_PORT, GPIO_MODE_AIN, 0, RVL_LINK_TCK_PIN);
    gpio_init(RVL_LINK_TMS_PORT, GPIO_MODE_AIN, 0, RVL_LINK_TMS_PIN);
    gpio_init(RVL_LINK_TDI_PORT, GPIO_MODE_AIN, 0, RVL_LINK_TDI_PIN);
    gpio_init(RVL_LINK_TDO_PORT, GPIO_MODE_AIN, 0, RVL_LINK_TDO_PIN);
}


void rvl_jtag_srst_put(int srst)
{
#ifdef RVL_LINK_SRST_PORT
    if (srst) {
        gpio_init(RVL_LINK_SRST_PORT, GPIO_MODE_AIN, 0, RVL_LINK_SRST_PIN);
    } else {
        gpio_bit_reset(RVL_LINK_SRST_PORT, RVL_LINK_SRST_PIN);
        gpio_init(RVL_LINK_SRST_PORT, GPIO_MODE_OUT_OD, GPIO_OSPEED_10MHZ, RVL_LINK_SRST_PIN);
    }
#endif
}

void rvl_jtag_tms_put(int tms)
{
    if (tms) {
        GPIO_BOP(RVL_LINK_TMS_PORT) = (uint32_t)RVL_LINK_TMS_PIN;
    } else {
        GPIO_BC(RVL_LINK_TMS_PORT) = (uint32_t)RVL_LINK_TMS_PIN;
    }
}


void rvl_jtag_tdi_put(int tdi)
{
    if (tdi) {
        GPIO_BOP(RVL_LINK_TDI_PORT) = (uint32_t)RVL_LINK_TDI_PIN;
    } else {
        GPIO_BC(RVL_LINK_TDI_PORT) = (uint32_t)RVL_LINK_TDI_PIN;
    }
}


void rvl_jtag_tck_put(int tck)
{
    if (tck) {
        GPIO_BOP(RVL_LINK_TCK_PORT) = (uint32_t)RVL_LINK_TCK_PIN;
    } else {
        GPIO_BC(RVL_LINK_TCK_PORT) = (uint32_t)RVL_LINK_TCK_PIN;
    }
}

void rvl_jtag_tck_high_delay()
{
    uint32_t start = __RV_CSR_READ(mcycle);

    while (__RV_CSR_READ(mcycle) - start < RVL_JTAG_TCK_HIGH_DELAY) ;
}

void rvl_jtag_tck_low_delay()
{
    uint32_t start = __RV_CSR_READ(mcycle);

    while (__RV_CSR_READ(mcycle) - start < RVL_JTAG_TCK_LOW_DELAY) ;
}

int rvl_jtag_tdo_get()
{
    if ((uint32_t) RESET != (GPIO_ISTAT(RVL_LINK_TDO_PORT) & (RVL_LINK_TDO_PIN))) {
        return 1;
    } else {
        return 0;
    }
}
