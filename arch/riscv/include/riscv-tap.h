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
#include <stdint.h>

#define RV_TAP_DR_PRE   (0)
#define RV_TAP_DR_POST  (0)
#define RV_TAP_IR_PRE   (0)
#define RV_TAP_IR_POST  (0)

void rv_tap_init(void);
void rv_tap_deinit(void);
void rv_tap_reset(uint32_t len);
void rv_tap_idle(uint32_t len);
void rv_tap_shift_dr(uint32_t* out, uint32_t* in, uint32_t len);
void rv_tap_shift_ir(uint32_t* out, uint32_t* in, uint32_t len);
