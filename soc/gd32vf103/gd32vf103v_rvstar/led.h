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

#ifndef __LED_H__
#define __LED_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* other library header file includes */
#include "nuclei_sdk_soc.h"
#include <stdbool.h>

/* JTAG TCK pin definition */
#define RV_LED_R_PORT                   GPIOA
#define RV_LED_R_PIN                    GPIO_PIN_1 /* PA1, R */
#define RV_LED_R_PIN_SOURCE             GPIO_PIN_SOURCE_1

/* JTAG TMS pin definition */
#define RV_LED_G_PORT                   GPIOA
#define RV_LED_G_PIN                    GPIO_PIN_2 /* PA2, G */
#define RV_LED_G_PIN_SOURCE             GPIO_PIN_SOURCE_2

/* JTAG TDI pin definition */
#define RV_LED_B_PORT                   GPIOA
#define RV_LED_B_PIN                    GPIO_PIN_3 /* PA3, B */
#define RV_LED_B_PIN_SOURCE             GPIO_PIN_SOURCE_3

#define RV_LED_R(en) \
    if (en) { \
        GPIO_BC(RV_LED_R_PORT) = RV_LED_R_PIN; \
    } else { \
        GPIO_BOP(RV_LED_R_PORT) = RV_LED_R_PIN; \
    }

#define RV_LED_G(en) \
    if (en) { \
        GPIO_BC(RV_LED_G_PORT) = RV_LED_G_PIN; \
    } else { \
        GPIO_BOP(RV_LED_G_PORT) = RV_LED_G_PIN; \
    }

#define RV_LED_B(en) \
    if (en) { \
        GPIO_BC(RV_LED_B_PORT) = RV_LED_B_PIN; \
    } else { \
        GPIO_BOP(RV_LED_B_PORT) = RV_LED_B_PIN; \
    }

void rv_led_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H__ */
