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
