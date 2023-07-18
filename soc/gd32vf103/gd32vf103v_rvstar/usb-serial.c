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

#include "usb-serial.h"
#include "drv_usb_hw.h"
#include "cdc_acm_core.h"
#include "led.h"

extern __IO uint32_t cdc1_packet_sent;

extern usb_core_driver USB_OTG_dev;

void USART0_IRQHandler(void)
{
    static uint8_t index = 0;
    static uint8_t cache[CDC_ACM_DATA_PACKET_SIZE];
    if (usart_interrupt_flag_get(UART_ITF, USART_INT_FLAG_RBNE) != RESET) {
        /* read one byte from the receive data register */
        cache[index] = (uint8_t)usart_data_receive(UART_ITF);
        index++;
        if ((index >= CDC_ACM_DATA_PACKET_SIZE) || ('\n' == cache[index - 1])) {
            cdc1_packet_sent = 0;
            usbd_ep_send(&USB_OTG_dev, CDC1_ACM_DATA_IN_EP, cache, index);
            index = 0;
        }
    }
}

void usb_serial_init(void)
{
    BaseType_t xReturned;

    /* enable GPIO clock */
    rcu_periph_clock_enable(UART_GPIO_CLK);
    /* enable USART clock */
    rcu_periph_clock_enable(UART_CLK);
    rcu_periph_clock_enable(RCU_AF);
    /* connect port to USARTx_Tx */
    gpio_init(UART_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_10MHZ, UART_TX_PIN);
    /* connect port to USARTx_Rx */
    gpio_init(UART_GPIO_PORT, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, UART_RX_PIN);
    /* USART configure */
    usart_deinit(UART_ITF);
    usart_baudrate_set(UART_ITF, 115200U);
    usart_word_length_set(UART_ITF, USART_WL_8BIT);
    usart_stop_bit_set(UART_ITF, USART_STB_1BIT);
    usart_parity_config(UART_ITF, USART_PM_NONE);
    usart_hardware_flow_rts_config(UART_ITF, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(UART_ITF, USART_CTS_DISABLE);
    usart_receive_config(UART_ITF, USART_RECEIVE_ENABLE);
    usart_transmit_config(UART_ITF, USART_TRANSMIT_ENABLE);
    usart_enable(UART_ITF);
    eclic_irq_enable(UART_IRQ, 1, ECLIC_PRIGROUP_LEVEL2_PRIO2);
    /* enable USART receive interrupt */
    usart_interrupt_enable(UART_ITF, USART_INT_RBNE);
}
