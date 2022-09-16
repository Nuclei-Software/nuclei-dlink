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
#include "port.h"
#include "config.h"

static bool oscan1_mode = false;

static void rv_jtag_tms_put(int tms)
{
    gpio_init(RV_LINK_TMS_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, RV_LINK_TMS_PIN);
    if (tms) {
        GPIO_BOP(RV_LINK_TMS_PORT) = (uint32_t)RV_LINK_TMS_PIN;
    } else {
        GPIO_BC(RV_LINK_TMS_PORT) = (uint32_t)RV_LINK_TMS_PIN;
    }
}

static int rv_jtag_tms_get()
{
    gpio_init(RV_LINK_TMS_PORT, GPIO_MODE_IPU, 0, RV_LINK_TMS_PIN);
    if ((uint32_t) RESET != (GPIO_ISTAT(RV_LINK_TMS_PORT) & (RV_LINK_TMS_PIN))) {
        return 1;
    } else {
        return 0;
    }
}

static void rv_jtag_tdi_put(int tdi)
{
    gpio_init(RV_LINK_TDI_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, RV_LINK_TDI_PIN);
    if (tdi) {
        GPIO_BOP(RV_LINK_TDI_PORT) = (uint32_t)RV_LINK_TDI_PIN;
    } else {
        GPIO_BC(RV_LINK_TDI_PORT) = (uint32_t)RV_LINK_TDI_PIN;
    }
}

static void rv_jtag_tck_put(int tck)
{
    gpio_init(RV_LINK_TCK_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_10MHZ, RV_LINK_TCK_PIN);
    if (tck) {
        GPIO_BOP(RV_LINK_TCK_PORT) = (uint32_t)RV_LINK_TCK_PIN;
    } else {
        GPIO_BC(RV_LINK_TCK_PORT) = (uint32_t)RV_LINK_TCK_PIN;
    }
}

static int rv_jtag_tdo_get()
{
    gpio_init(RV_LINK_TDO_PORT, GPIO_MODE_IPU, 0, RV_LINK_TDO_PIN);
    if ((uint32_t) RESET != (GPIO_ISTAT(RV_LINK_TDO_PORT) & (RV_LINK_TDO_PIN))) {
        return 1;
    } else {
        return 0;
    }
}

void rv_tap_init(void)
{
    rv_jtag_init();
}

void rv_tap_deinit(void)
{
    rv_jtag_fini();
}

static uint32_t rv_tap_tick(uint32_t tms, uint32_t tdi)
{
    int tdo;

    if (oscan1_mode) {
            /*
        *     ___     ___     ___
        * ___|   |___|   |___|   |
        */
        rv_jtag_tms_put(tdi);
        rv_jtag_tck_put(1);
        rv_jtag_tck_put(0);
        rv_jtag_tms_put(tms);
        rv_jtag_tck_put(1);
        rv_jtag_tck_put(0);
        tdo = rv_jtag_tms_get();
        rv_jtag_tck_put(1);
        rv_jtag_tck_put(0);
    } else {
        /*
        *     ___
        * ___|   |
        */
        rv_jtag_tdi_put(tdi);
        rv_jtag_tms_put(tms);
        rv_jtag_tck_put(1);
        tdo = rv_jtag_tdo_get();
        rv_jtag_tck_put(0);
    }

    return tdo;
}

static void rv_tap_shift(uint32_t* out, uint32_t *in, uint32_t len, uint32_t post, uint32_t pre)
{
    int i, tdo;

    rv_tap_tick(0, 1);/* Select-DR(IR)-Scan -> Capture-DR(IR) */
    if (len) {
        rv_tap_tick(0, 1);/* Capture-DR(IR) -> Shift-DR(IR) */
        /* Select Tap Post*/
        for(i = 0; i < post; i++) {
            rv_tap_tick(0, 1);/* Shift-DR(IR) -> Shift-DR(IR) */
        }
        for(i = 0; i < len; i++) {
            if ((i == len - 1) && (post == 0)) {
                tdo = rv_tap_tick(1, (in[i / 32] >> (i % 32)) & 1);/* Shift-DR(IR) -> Exit1-DR(IR) */
            } else {
                tdo = rv_tap_tick(0, (in[i / 32] >> (i % 32)) & 1);/* Shift-DR(IR) -> Shift-DR(IR) */
            }
            if (tdo) {
                out[i / 32] |= 1 << (i % 32);
            } else {
                out[i / 32] &= ~(1 << (i % 32));
            }
        }
        /* Select Tap Pre*/
        for(i = 0; i < pre; i++) {
            if (i == pre -1) {
                rv_tap_tick(1, 1);/* Shift-DR(IR) -> Exit1-DR(IR) */
            } else {
                rv_tap_tick(0, 1);/* Shift-DR(IR) -> Shift-DR(IR) */
            }
        }
    } else {
        rv_tap_tick(1, 1);/* Capture-DR(IR) -> Exit1-DR(IR) */
    }
    rv_tap_tick(1, 1);/* Exit1-DR(IR) -> Update-DR(IR) */
    rv_tap_tick(0, 1);/* Update-DR(IR) -> Run-Test-Idle */
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
    rv_tap_tick(1, 1);/* Run-Test-Idle -> Select-DR-Scan */
    rv_tap_shift(out, in, len, RV_TAP_DR_POST, RV_TAP_DR_PRE);
}

void rv_tap_shift_ir(uint32_t* out, uint32_t* in, uint32_t len)
{
    rv_tap_tick(1, 1);/* Run-Test-Idle -> Select-DR-Scan */
    rv_tap_tick(1, 1);/* Select-DR-Scan -> Select-IR-Scan */
    rv_tap_shift(out, in, len, RV_TAP_IR_POST, RV_TAP_IR_PRE);
}

void rv_tap_oscan1_mode(void)
{
    uint32_t temp;

    oscan1_mode = false;

    // reset TAP to IDLE
    rv_tap_reset(50);

    // 2 DR ZBS
    rv_tap_shift_dr(&temp, &temp, 0);
    rv_tap_shift_dr(&temp, &temp, 0);

    // enter command level 2
    rv_tap_shift_dr(&temp, &temp, 1);

    // set scan format to OScan1
    rv_tap_shift_dr(&temp, &temp, 3);//cp0 = 3
    rv_tap_shift_dr(&temp, &temp, 9);//cp1 = 9

    // check packet
    rv_tap_idle(4);

    // oscan1 mode enable
    oscan1_mode = true;

    // read back0 register
    rv_tap_shift_dr(&temp, &temp, 9);//cp0 = 9
    rv_tap_shift_dr(&temp, &temp, 0);//cp1 = 0
    rv_tap_shift_dr(&temp, &temp, 32);//crscan = 32
    if ((temp & 0x3F) != 9) {
        rv_tap_reset(50);
        oscan1_mode = false;
        return;
    }

    // check packet
    rv_tap_idle(4);

    // 1 IR ZBS
    rv_tap_shift_ir(&temp, &temp, 0);
}
