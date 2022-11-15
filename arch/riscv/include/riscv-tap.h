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

#ifndef __RISCV_TAP_H__
#define __RISCV_TAP_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

void rv_tap_init(void);
void rv_tap_deinit(void);
void rv_tap_reset(uint32_t len);
void rv_tap_idle(uint32_t len);
void rv_tap_shift_dr(uint32_t* out, uint32_t* in, uint32_t len, uint32_t post, uint32_t pre);
void rv_tap_shift_ir(uint32_t* out, uint32_t* in, uint32_t len, uint32_t post, uint32_t pre);
void rv_tap_oscan1_mode(uint32_t dr_post, uint32_t dr_pre, uint32_t ir_post, uint32_t ir_pre);

#ifdef __cplusplus
}
#endif

#endif /* __RISCV_TAP_H__ */
