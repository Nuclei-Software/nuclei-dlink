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

#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
 extern "C" {
#endif

/* other library header file includes */
#include "nuclei_sdk_soc.h"

/* JTAG TCK pin definition */
#define RV_LINK_TCK_PORT                   GPIOF
#define RV_LINK_TCK_PIN                    GPIO_PIN_12 /* JTCK */

/* JTAG TMS pin definition */
#define RV_LINK_TMS_PORT                   GPIOF
#define RV_LINK_TMS_PIN                    GPIO_PIN_13 /* JTMS */

/* JTAG TDI pin definition */
#define RV_LINK_TDI_PORT                   GPIOF
#define RV_LINK_TDI_PIN                    GPIO_PIN_14 /* JTDI */

/* JTAG TDO pin definition */
#define RV_LINK_TDO_PORT                   GPIOF
#define RV_LINK_TDO_PIN                    GPIO_PIN_15 /* JTDO */

void rv_jtag_init(void);

void rv_jtag_fini(void);

void rv_jtag_tck_put(int tck);

void rv_jtag_tms_put(int tms);

int rv_jtag_tms_get();

void rv_jtag_tdi_put(int tdi);

int rv_jtag_tdo_get();

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */
