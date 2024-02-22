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

#ifndef __PORT_H__
#define __PORT_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "semphr.h"   /* RTOS semaphore related API prototypes. */
#include "task.h"     /* RTOS task related API prototypes. */

#define DLINK_FIRMWARE_VERSION ("v0.9.0")

#ifndef RV_TARGET_CONFIG_DMI_RETRIES
#define RV_TARGET_CONFIG_DMI_RETRIES                    (6)
#endif

#ifndef RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM
#define RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM        (8)
#endif

#ifndef RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM
#define RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM        (32)
#endif

#define RV_TARGET_CONFIG_REG_NUM                        (33)

#define GDB_PACKET_BUFF_SIZE                            (0x400)

void rv_board_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_H__ */
