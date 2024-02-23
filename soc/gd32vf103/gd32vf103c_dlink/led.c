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

#include "led.h"

void rv_led_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);

    gpio_init(RV_LED_R_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, RV_LED_R_PIN);
    gpio_init(RV_LED_G_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, RV_LED_G_PIN);
    gpio_init(RV_LED_B_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, RV_LED_B_PIN);

    RV_LED_R(1);
    RV_LED_G(0);
    RV_LED_B(0);
}
