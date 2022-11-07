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

#include "config.h"
#include "gdb-packet.h"

TaskHandle_t transport_tx_xHandle = NULL;
TaskHandle_t transport_rx_xHandle = NULL;

void transport_tx_xTask(void* pvParameters)
{
    char ch;

    for (;;) {
        xQueueReceive(gdb_txdata_xQueue, &ch, portMAX_DELAY);
        usart_write(UART4, ch);
    }
}

void UART4_IRQHandler(void)
{
    char ch;

    if (SET == usart_interrupt_flag_get(UART4, USART_INT_FLAG_RBNE)) {
        if (SET == usart_flag_get(UART4, USART_FLAG_RBNE)) {
            ch = usart_data_receive(UART4);
            xQueueSendFromISR(gdb_rxdata_xQueue, &ch, pdFALSE);
        }
        usart_interrupt_flag_clear(UART4, USART_INT_FLAG_RBNE);
    }
}

void rv_transport_init(void)
{
    BaseType_t xReturned;

    // Enable interrupts in general.
    __enable_irq();
    ECLIC_Register_IRQ(UART4_IRQn,
                        ECLIC_NON_VECTOR_INTERRUPT,
                        ECLIC_LEVEL_TRIGGER,
                        1,
                        0,
                        UART4_IRQHandler);

    /* enable USART clock */
    rcu_periph_clock_enable(RCU_UART4);
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_GPIOC);
    rcu_periph_clock_enable(RCU_GPIOD);
    gpio_init(GPIOC, GPIO_MODE_AF_PP, GPIO_OSPEED_10MHZ, GPIO_PIN_12);
    gpio_init(GPIOD, GPIO_MODE_IPU, GPIO_OSPEED_10MHZ, GPIO_PIN_2);
    /* USART configure */
    usart_deinit(UART4);
    usart_baudrate_set(UART4, 921600);
    usart_word_length_set(UART4, USART_WL_8BIT);
    usart_stop_bit_set(UART4, USART_STB_1BIT);
    usart_parity_config(UART4, USART_PM_NONE);
    usart_hardware_flow_rts_config(UART4, USART_RTS_DISABLE);
    usart_hardware_flow_cts_config(UART4, USART_CTS_DISABLE);
    usart_receive_config(UART4, USART_RECEIVE_ENABLE);
    usart_transmit_config(UART4, USART_TRANSMIT_ENABLE);
    usart_interrupt_enable(UART4, USART_INT_RBNE);
    usart_enable(UART4);

    xReturned = xTaskCreate(transport_tx_xTask,       /* Function that implements the task. */
                            "transport_tx",           /* Text name for the task. */
                            256,                      /* Stack size in words, not bytes. */
                            NULL,                     /* Parameter passed into the task. */
                            3,                        /* Priority at which the task is created. */
                            &transport_tx_xHandle);   /* Used to pass out the created task's handle. */
    if(xReturned != pdPASS) {
        /* error msg */
        vTaskDelete(transport_tx_xHandle);
    }
}
