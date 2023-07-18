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
#include "riscv-tap.h"
#include "port.h"
#include "jtag.h"

static bool oscan1_mode = false;

void rv_tap_init(void)
{
    oscan1_mode = false;
    rv_jtag_init();
}

void rv_tap_deinit(void)
{
    rv_jtag_fini();
}

static inline uint64_t rv_tap_tick(uint32_t tms, uint32_t tdi)
{
    uint64_t tdo;

    if (oscan1_mode) {
        /*
        *     ___     ___     ___
        * ___|   |___|   |___|   |
        */
        RV_JTAG_TMS_PUT(tdi ^ 1);
        RV_JTAG_TCK_PUT(1);
        RV_JTAG_TCK_PUT(0);
        RV_JTAG_TMS_PUT(tms);
        RV_JTAG_TCK_PUT(1);
        RV_JTAG_TCK_PUT(0);
        RV_JTAG_TMS_MODE(0, tms);
        tdo = RV_JTAG_TMS_GET;
        RV_JTAG_TCK_PUT(1);
        RV_JTAG_TCK_PUT(0);
        RV_JTAG_TMS_MODE(1, tms);
    } else {
        /*
        *     ___
        * ___|   |
        */
        RV_JTAG_TDI_PUT(tdi);
        RV_JTAG_TMS_PUT(tms);
        RV_JTAG_TCK_PUT(1);
        tdo = RV_JTAG_TDO_GET;
        RV_JTAG_TCK_PUT(0);
    }

    return tdo;
}

static inline uint64_t rv_tap_shift(uint64_t in, uint32_t len, uint32_t post, uint32_t pre)
{
    int i;
    uint64_t out = 0;

    rv_tap_tick(0, 1);/* Select-DR(IR)-Scan -> Capture-DR(IR) */
    if (len) {
        rv_tap_tick(0, 1);/* Capture-DR(IR) -> Shift-DR(IR) */
        /* Select Tap Post*/
        for(i = 0; i < post; i++) {
            rv_tap_tick(0, 1);/* Shift-DR(IR) -> Shift-DR(IR) */
        }
        if (pre) {
            for(i = 0; i < len; i++) {
                out |= rv_tap_tick(0, (in >> i) & 1) << i;/* Shift-DR(IR) -> Shift-DR(IR) */
            }
            /* Select Tap Pre*/
            for(i = 0; i < (pre - 1); i++) {
                rv_tap_tick(0, 1);/* Shift-DR(IR) -> Shift-DR(IR) */
            }
            rv_tap_tick(1, 1);/* Shift-DR(IR) -> Exit1-DR(IR) */
        } else {
            for(i = 0; i < (len - 1); i++) {
                out |= rv_tap_tick(0, (in >> i) & 1) << i;/* Shift-DR(IR) -> Shift-DR(IR) */
            }
            out |= rv_tap_tick(1, (in >> i) & 1) << i;/* Shift-DR(IR) -> Shift-DR(IR) */
        }
    } else {
        rv_tap_tick(1, 1);/* Capture-DR(IR) -> Exit1-DR(IR) */
    }
    rv_tap_tick(1, 1);/* Exit1-DR(IR) -> Update-DR(IR) */
    rv_tap_tick(0, 1);/* Update-DR(IR) -> Run-Test-Idle */
    return out;
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

uint64_t rv_tap_shift_scan(uint64_t in, uint32_t ir, uint32_t len, uint32_t post, uint32_t pre)
{
    rv_tap_tick(1, 1);/* Run-Test-Idle -> Select-DR-Scan */
    if (ir) {
        rv_tap_tick(1, 1);/* Select-DR-Scan -> Select-IR-Scan */
    }
    return rv_tap_shift(in, len, post, pre);
}

void rv_tap_oscan1_mode(void)
{
    uint64_t tdi = 0xFFFFFFFF;
    uint64_t tdo;

    oscan1_mode = false;

    // reset TAP to IDLE
    rv_tap_reset(50);

    // 2 DR ZBS
    rv_tap_shift_scan(tdi, 0, 0, 0, 0);
    rv_tap_shift_scan(tdi, 0, 0, 0, 0);

    // enter command level 2
    rv_tap_shift_scan(tdi, 0, 1, 0, 0);

    // set scan format to OScan1
    rv_tap_shift_scan(tdi, 0, 3, 0, 0);//cp0 = 3
    rv_tap_shift_scan(tdi, 0, 9, 0, 0);//cp1 = 9

    // check packet
    rv_tap_idle(4);

    // oscan1 mode enable
    oscan1_mode = true;

    // read back0 register
    rv_tap_shift_scan(tdi, 0, 9, 0, 0);//cp0 = 9
    rv_tap_shift_scan(tdi, 0, 0, 0, 0);//cp1 = 0
    tdo = rv_tap_shift_scan(tdi, 0, 32, 0, 0);//crscan = 32
    if ((tdo & 0xFF) != 9) {
        oscan1_mode = false;
        rv_tap_reset(50);
        return;
    }

    // check packet
    rv_tap_idle(4);

    // 1 IR ZBS
    rv_tap_shift_scan(tdi, 1, 0, 0, 0);
}

static inline void rvl_tap_escape(uint32_t n)
{
	RV_JTAG_TCK_PUT(0);
	RV_JTAG_TCK_PUT(1);
	int tms = 0;
	for(int i = 0; i < n ; i++)
	{
		RV_JTAG_TMS_PUT(tms);
		tms = !tms;
	}
	RV_JTAG_TCK_PUT(0);
}

static inline void rvl_tap_oac(void)
{
	rv_tap_tick(0 , 0);
	rv_tap_tick(0 , 0);
	rv_tap_tick(1 , 0);
	rv_tap_tick(1 , 0);
}

static inline void rvl_tap_ec(void)
{
	rv_tap_tick(0 , 0);
	rv_tap_tick(0 , 0);
	rv_tap_tick(0 , 0);
	rv_tap_tick(1 , 0);
}

static inline void rvl_tap_cp(void)
{
	rv_tap_tick(0 , 0);
	rv_tap_tick(0 , 0);
	rv_tap_tick(0 , 0);
	rv_tap_tick(0 , 0);
}

void rv_tap_oscan1_mode_short(void)
{
    oscan1_mode = false;
    rvl_tap_escape(10);
    rv_tap_reset(50);
    rvl_tap_escape(7);
    rvl_tap_oac();
    rvl_tap_ec();
    rvl_tap_cp();
    oscan1_mode = true;
    rv_tap_tick(0, 0);// TLR
}

void rv_tap_oscan1_mode_short_exit(void)
{
    rvl_tap_escape(10);
    rv_tap_reset(50);
}