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

#ifndef __JTAG_H__
#define __JTAG_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* other library header file includes */
#include "nuclei_sdk_soc.h"

/* JTAG TCK pin definition */
#define RV_JTAG_TCK_PORT                   GPIOB
#define RV_JTAG_TCK_PIN                    GPIO_PIN_15 /* PB15, JTCK */
#define RV_JTAG_TCK_PIN_SOURCE             GPIO_PIN_SOURCE_15

/* JTAG TMS pin definition */
#define RV_JTAG_TMS_PORT                   GPIOB
#define RV_JTAG_TMS_PIN                    GPIO_PIN_13 /* PB13, JTMS */
#define RV_JTAG_TMS_PIN_SOURCE             GPIO_PIN_SOURCE_13

/* JTAG TDI pin definition */
#define RV_JTAG_TDI_PORT                   GPIOB
#define RV_JTAG_TDI_PIN                    GPIO_PIN_12 /* PB12, JTDI */
#define RV_JTAG_TDI_PIN_SOURCE             GPIO_PIN_SOURCE_12

/* JTAG TDO pin definition */
#define RV_JTAG_TDO_PORT                   GPIOB
#define RV_JTAG_TDO_PIN                    GPIO_PIN_14 /* PB14, JTDO */
#define RV_JTAG_TDO_PIN_SOURCE             GPIO_PIN_SOURCE_14

#define RV_JTAG_TCK_PUT(tck) \
    if (tck) { \
        GPIO_BOP(RV_JTAG_TCK_PORT) = RV_JTAG_TCK_PIN; \
    } else { \
        GPIO_BC(RV_JTAG_TCK_PORT) = RV_JTAG_TCK_PIN; \
    }

#define RV_JTAG_TMS_MODE(out, tms) \
    if (out) { \
        GPIO_CTL1(RV_JTAG_TMS_PORT) = 0x33380000; \
    } else { \
        GPIO_CTL1(RV_JTAG_TMS_PORT) = 0x33880000; \
        if (tms) { \
            GPIO_BOP(RV_JTAG_TMS_PORT) = RV_JTAG_TMS_PIN; \
        } else { \
            GPIO_BC(RV_JTAG_TMS_PORT) = RV_JTAG_TMS_PIN; \
        } \
    }

#define RV_JTAG_TMS_PUT(tms) \
    if (tms) { \
        GPIO_BOP(RV_JTAG_TMS_PORT) = RV_JTAG_TMS_PIN; \
    } else { \
        GPIO_BC(RV_JTAG_TMS_PORT) = RV_JTAG_TMS_PIN; \
    }

#define RV_JTAG_TMS_GET \
    (GPIO_ISTAT(RV_JTAG_TMS_PORT) & RV_JTAG_TMS_PIN) >> RV_JTAG_TMS_PIN_SOURCE

#define RV_JTAG_TDI_PUT(tdi) \
    if (tdi) { \
        GPIO_BOP(RV_JTAG_TDI_PORT) = RV_JTAG_TDI_PIN; \
    } else { \
        GPIO_BC(RV_JTAG_TDI_PORT) = RV_JTAG_TDI_PIN; \
    }

#define RV_JTAG_TDO_GET \
    (GPIO_ISTAT(RV_JTAG_TDO_PORT) & RV_JTAG_TDO_PIN) >> RV_JTAG_TDO_PIN_SOURCE

void rv_jtag_init(void);

void rv_jtag_fini(void);

#ifdef __cplusplus
}
#endif

#endif /* __JTAG_H__ */
