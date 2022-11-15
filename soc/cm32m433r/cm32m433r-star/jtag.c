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

GPIO_InitType GPIO_InitStructure;

void rv_jtag_init(void)
{
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOF, ENABLE);

    GPIO_InitStructure.Pin        = RV_LINK_TCK_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RV_LINK_TCK_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin        = RV_LINK_TMS_PIN;
    GPIO_Init(RV_LINK_TMS_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin        = RV_LINK_TDI_PIN;
    GPIO_Init(RV_LINK_TDI_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin        = RV_LINK_TDO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RV_LINK_TDO_PORT, &GPIO_InitStructure);
}

void rv_jtag_fini(void)
{
}

void rv_jtag_tck_put(int tck)
{
    GPIO_WriteBit(RV_LINK_TCK_PORT, RV_LINK_TCK_PIN, tck);
}

void rv_jtag_tms_put(int tms)
{
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.Pin        = RV_LINK_TMS_PIN;
    GPIO_Init(RV_LINK_TMS_PORT, &GPIO_InitStructure);
    GPIO_WriteBit(RV_LINK_TMS_PORT, RV_LINK_TMS_PIN, tms);
}

int rv_jtag_tms_get(int tms)
{
    if (tms) {
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPU;
    } else {
        GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IPD;
    }
    GPIO_InitStructure.Pin        = RV_LINK_TMS_PIN;
    GPIO_Init(RV_LINK_TMS_PORT, &GPIO_InitStructure);
    return GPIO_ReadInputDataBit(RV_LINK_TMS_PORT, RV_LINK_TMS_PIN);
}

inline void rv_jtag_tdi_put(int tdi)
{
    GPIO_WriteBit(RV_LINK_TDI_PORT, RV_LINK_TDI_PIN, tdi);
}

inline int rv_jtag_tdo_get()
{
    return GPIO_ReadInputDataBit(RV_LINK_TDO_PORT, RV_LINK_TDO_PIN);
}
