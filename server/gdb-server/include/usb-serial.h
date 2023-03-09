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

#ifndef __USB_SERIAL_H__
#define __USB_SERIAL_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "port.h"
#include "usbd_conf.h"

#define UART_ITF                       USART0
#define UART_TX_PIN                    GPIO_PIN_9
#define UART_RX_PIN                    GPIO_PIN_10
#define UART_GPIO_PORT                 GPIOA
#define UART_GPIO_CLK                  RCU_GPIOA
#define UART_CLK                       RCU_USART0
#define UART_IRQ                       USART0_IRQn
#define UART_IRQ_ISR                   USART0_IRQHandler

void usb_serial_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __USB_SERIAL_H__ */
