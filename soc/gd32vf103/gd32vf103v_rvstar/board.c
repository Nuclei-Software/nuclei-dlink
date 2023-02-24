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
#include "port.h"
#include "gdb-packet.h"
#include "led.h"
#include "drv_usb_hw.h"
#include "cdc_acm_core.h"

usb_core_driver USB_OTG_dev =
{
    .dev = {
        .desc = {
            .dev_desc       = (uint8_t *)&device_descriptor,
            .config_desc    = (uint8_t *)&configuration_descriptor,
            .strings        = usbd_strings,
        }
    }
};

void rv_board_init(void)
{
    rv_led_init();

    RV_LED_R(1);
    usb_rcu_config();
    usb_timer_init();
    usb_intr_config();
    usbd_init (&USB_OTG_dev, USB_CORE_ENUM_FS, &usbd_cdc_cb);
    /* check if USB device is enumerated successfully */
    while (USBD_CONFIGURED != USB_OTG_dev.dev.cur_status) {}
    RV_LED_R(0);
    RV_LED_G(1);
}
