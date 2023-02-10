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

#include "jtag.h"

void rv_jtag_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(RV_LINK_TCK_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, RV_LINK_TCK_PIN);
    gpio_init(RV_LINK_TMS_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, RV_LINK_TMS_PIN);
    gpio_init(RV_LINK_TDI_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, RV_LINK_TDI_PIN);
    gpio_init(RV_LINK_TDO_PORT, GPIO_MODE_IPD, 0, RV_LINK_TDO_PIN);
}

void rv_jtag_fini(void)
{
    gpio_init(RV_LINK_TCK_PORT, GPIO_MODE_AIN, 0, RV_LINK_TCK_PIN);
    gpio_init(RV_LINK_TMS_PORT, GPIO_MODE_AIN, 0, RV_LINK_TMS_PIN);
    gpio_init(RV_LINK_TDI_PORT, GPIO_MODE_AIN, 0, RV_LINK_TDI_PIN);
    gpio_init(RV_LINK_TDO_PORT, GPIO_MODE_AIN, 0, RV_LINK_TDO_PIN);
}
