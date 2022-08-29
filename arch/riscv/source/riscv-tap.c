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
#include "riscv-tap.h"
#include "riscv-jtag.h"
#include "config.h"

static uint32_t rv_tap_tick(uint32_t tms, uint32_t tdi)
{
    return rv_jtag_tick(tms, tdi);
}

static void rv_tap_shift(uint32_t* out, uint32_t *in, uint32_t len, uint32_t pre, uint32_t post)
{
    int i, tdo;

    rv_tap_tick(0, 1);
    rv_tap_tick(0, 1);

    for(i = 0; i < pre; i++) {
        rv_tap_tick(0, 1);
    }

    for(i = 0; i < len; i++) {
        if ((i == len - 1) && (post == 0)) {
            tdo = rv_tap_tick(1, (in[i / 32] >> (i % 32)) & 1);
        } else {
            tdo = rv_tap_tick(0, (in[i / 32] >> (i % 32)) & 1);
        }
        if (tdo) {
            out[i / 32] |= 1 << (i % 32);
        } else {
            out[i / 32] &= ~(1 << (i % 32));
        }
    }

    for(i = 0; i < post; i++) {
        if (i == post -1) {
            rv_tap_tick(1, 1);
        } else {
            rv_tap_tick(0, 1);
        }
    }

    rv_tap_tick(1, 1);
    rv_tap_tick(0, 1);
}

void rv_tap_init(void)
{
    rv_jtag_init();
}

void rv_tap_deinit(void)
{
    rv_jtag_deinit();
}

void rv_tap_reset(uint32_t len)
{
    int i;
    for (i = 0; i < len; i++) {
        rv_tap_tick(1, 1);
    }
    rv_tap_tick(0, 1);
}

void rv_tap_idle(uint32_t len)
{
    int i;
    for (i = 0; i < len; i++) {
        rv_tap_tick(0, 1);
    }
}

void rv_tap_shift_dr(uint32_t* out, uint32_t* in, uint32_t len)
{
    taskENTER_CRITICAL(); 
    rv_tap_tick(1, 1);
    rv_tap_shift(out, in, len, RV_TAP_DR_PRE, RV_TAP_DR_POST);
    taskEXIT_CRITICAL(); 
}

void rv_tap_shift_ir(uint32_t* out, uint32_t* in, uint32_t len)
{
    taskENTER_CRITICAL();
    rv_tap_tick(1, 1);
    rv_tap_tick(1, 1);
    rv_tap_shift(out, in, len, RV_TAP_IR_PRE, RV_TAP_IR_POST);
    taskEXIT_CRITICAL(); 
}
