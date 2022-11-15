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

#include "gdb-packet.h"
/* other library header file includes */
#include "nuclei_sdk_soc.h"

#define USARTy                      UART4
#define USARTy_GPIO                 GPIOD
#define USARTy_CLK                  RCC_APB1_PERIPH_UART4
#define USARTy_GPIO_CLK             RCC_APB2_PERIPH_GPIOD
#define USARTy_TxPin                GPIO_PIN_0
#define USARTy_RxPin                GPIO_PIN_1
#define USARTy_APBxClkCmd           RCC_EnableAPB1PeriphClk
#define USARTy_REMAP			    GPIO_RMP3_UART4
#define USARTy_IRQn                 UART4_IRQn
#define USARTy_IRQHandler           UART4_IRQHandler

void RCC_Configuration(void);
void GPIO_Configuration(void);
void ECLIC_Configuration(void);

TaskHandle_t transport_tx_xHandle = NULL;
TaskHandle_t transport_rx_xHandle = NULL;

void transport_tx_xTask(void* pvParameters)
{
    char ch;

    for (;;) {
        xQueueReceive(gdb_txdata_xQueue, &ch, portMAX_DELAY);
        USART_SendData(USARTy, ch);
        /* Loop until the end of transmission */
		while (USART_GetFlagStatus(USARTy, USART_FLAG_TXDE) == RESET);
    }
}

void UART4_IRQHandler(void)
{
    char ch;

    if (USART_GetIntStatus(USARTy, USART_INT_RXDNE) != RESET)
    {
        /* Read one byte from the receive data register */
        ch = USART_ReceiveData(USARTy);
        xQueueSendFromISR(gdb_rxdata_xQueue, &ch, pdFALSE);
    }
}

void rv_transport_init(void)
{
    BaseType_t xReturned;

    // Enable interrupts in general.
    __enable_irq();

    /* System Clocks Configuration */
    RCC_Configuration();

    /* ECLIC configuration */
    ECLIC_Configuration();

    /* Configure the GPIO ports */
    GPIO_Configuration();

    /* USARTy and USARTz configuration ------------------------------------------------------*/
    USART_InitType USART_InitStructure;
    USART_InitStructure.BaudRate            = 115200; /* Band rate is too high may cause data lost in this example.
                                                    When system clock is 8M, the maximum band rate allowed is 38400 */
    USART_InitStructure.WordLength          = USART_WL_8B;
    USART_InitStructure.StopBits            = USART_STPB_1;
    USART_InitStructure.Parity              = USART_PE_NO;
    USART_InitStructure.HardwareFlowControl = USART_HFCTRL_NONE;
    USART_InitStructure.Mode                = USART_MODE_RX | USART_MODE_TX;

    /* Configure USARTy and USARTz */
    USART_Init(USARTy, &USART_InitStructure);

    /* Enable the USARTx */
    USART_Enable(USARTy, ENABLE);

    /* Enable USARTy Receive and Transmit interrupts */
    USART_ConfigInt(USARTy, USART_INT_RXDNE, ENABLE);

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

/**
 * @brief  Configures the different system clocks.
 */
void RCC_Configuration(void)
{
    /* Enable GPIO clock */
    RCC_EnableAPB2PeriphClk(USARTy_GPIO_CLK | RCC_APB2_PERIPH_AFIO, ENABLE);

    /* Enable USARTy and USARTz Clock */
    USARTy_APBxClkCmd(USARTy_CLK, ENABLE);
}

/**
 * @brief  Configures the different GPIO ports.
 */
void GPIO_Configuration(void)
{
    GPIO_ConfigPinRemap(USARTy_REMAP, ENABLE);

    GPIO_InitType GPIO_InitStructure;
    /* Configure USARTy Rx as input floating */
    GPIO_InitStructure.Pin       = USARTy_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USARTy_GPIO, &GPIO_InitStructure);
    /* Configure USARTy Tx as alternate function push-pull */
    GPIO_InitStructure.Pin        = USARTy_TxPin;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_PP;
    GPIO_Init(USARTy_GPIO, &GPIO_InitStructure);
}

/**
 * @brief  Configures the enhanced core local interrupt controller.
 */
void ECLIC_Configuration(void)
{
    /* Configure the ECLIC level and priority Bits */
    ECLIC_SetCfgNlbits(0); /* 0 bits for level, 4 bits for priority */

    /* Enable the USARTy Interrupt */
    ECLIC_SetPriorityIRQ(USARTy_IRQn, 1);
    ECLIC_SetTrigIRQ(USARTy_IRQn, ECLIC_LEVEL_TRIGGER);
    ECLIC_EnableIRQ(USARTy_IRQn);
}
