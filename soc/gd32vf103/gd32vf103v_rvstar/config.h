#ifndef __RV_LINK_LINK_ARCH_GD32VF103V_RVSTAR_LINK_CONFIG_H__
#define __RV_LINK_LINK_ARCH_GD32VF103V_RVSTAR_LINK_CONFIG_H__
/**
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 *
 * \file  rv-link/link/arch/gd32vf103/longan-nano/link-config.h
 * \brief Link configuration header file for the Longan Nano board.
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

/* JTAG TCK pin definition */
#define RVL_LINK_TCK_PORT                   GPIOA
#define RVL_LINK_TCK_PIN                    GPIO_PIN_4 /* PA4, JTCK */

/* JTAG TMS pin definition */
#define RVL_LINK_TMS_PORT                   GPIOB
#define RVL_LINK_TMS_PIN                    GPIO_PIN_15 /* PB15, JTMS */

/* JTAG TDI pin definition */
#define RVL_LINK_TDI_PORT                   GPIOB
#define RVL_LINK_TDI_PIN                    GPIO_PIN_14 /* PB14, JTDI */

/* JTAG TDO pin definition */
#define RVL_LINK_TDO_PORT                   GPIOB
#define RVL_LINK_TDO_PIN                    GPIO_PIN_13 /* PB13, JTDO */

/* JTAG SRST pin definition */
#define RVL_LINK_SRST_PORT                  GPIOB
#define RVL_LINK_SRST_PIN                   GPIO_PIN_12 /* PB12, SRST */

#ifndef RVL_JTAG_TCK_FREQ_KHZ
#define RVL_JTAG_TCK_FREQ_KHZ       1000
#endif

#if RVL_JTAG_TCK_FREQ_KHZ >= 1000
#define RVL_JTAG_TCK_HIGH_DELAY     30
#define RVL_JTAG_TCK_LOW_DELAY      1
#elif RVL_JTAG_TCK_FREQ_KHZ >= 500
#define RVL_JTAG_TCK_HIGH_DELAY     (30 + 50)
#define RVL_JTAG_TCK_LOW_DELAY      (1 + 50)
#elif RVL_JTAG_TCK_FREQ_KHZ >= 200
#define RVL_JTAG_TCK_HIGH_DELAY     (30 + 200)
#define RVL_JTAG_TCK_LOW_DELAY      (1 + 200)
#else // 100KHz
#define RVL_JTAG_TCK_HIGH_DELAY     (30 + 450)
#define RVL_JTAG_TCK_LOW_DELAY      (1 + 450)
#endif

#endif /* __RV_LINK_LINK_ARCH_GD32VF103V_RVSTAR_LINK_CONFIG_H__ */
