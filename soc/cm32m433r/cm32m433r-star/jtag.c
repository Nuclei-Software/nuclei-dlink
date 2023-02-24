/*
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 * Copyright (c) 2022 Nuclei Limited. All rights reserved.
 *
 * Dlink is licensed under the Mulan PSL v1.
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
    RCC_EnableAPB2PeriphClk(RCC_APB2_PERIPH_GPIOF, ENABLE);

    GPIO_InitType GPIO_InitStructure;
    GPIO_InitStructure.Pin        = RV_JTAG_TCK_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(RV_JTAG_TCK_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin        = RV_JTAG_TMS_PIN;
    GPIO_Init(RV_JTAG_TMS_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin        = RV_JTAG_TDI_PIN;
    GPIO_Init(RV_JTAG_TDI_PORT, &GPIO_InitStructure);
    GPIO_InitStructure.Pin        = RV_JTAG_TDO_PIN;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING;
    GPIO_Init(RV_JTAG_TDO_PORT, &GPIO_InitStructure);
}

void rv_jtag_fini(void)
{
}
