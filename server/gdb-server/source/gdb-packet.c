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

#include "gdb-packet.h"
#include "drv_usb_hw.h"
#include "cdc_acm_core.h"
#include "led.h"

extern __IO uint8_t packet_sent;
extern __IO uint8_t packet_receive;
extern __IO uint32_t receive_length;
extern usb_core_driver USB_OTG_dev;

TaskHandle_t gdb_cmd_packet_xHandle = NULL;
TaskHandle_t gdb_rsp_packet_xHandle = NULL;
QueueHandle_t gdb_cmd_packet_xQueue;
QueueHandle_t gdb_rsp_packet_xQueue;

uint8_t cmd_buffer[GDB_PACKET_BUFF_SIZE + 64];
uint8_t rsp_buffer[GDB_PACKET_BUFF_SIZE + 64];

static gdb_packet_t cmd;
static gdb_packet_t rsp;

bool no_ack_mode;

static uint32_t gdb_packet_checksum(const uint8_t* p, uint32_t len);

void gdb_cmd_packet_vTask(void* pvParameters)
{
    BaseType_t xReturned;
    int dollar, sharp, total;
    uint8_t temp[CDC_ACM_DATA_PACKET_SIZE];

    for (;;) {
        dollar = -1;
        sharp = -1;
        total = 0;
        while (1) {
            if (USBD_CONFIGURED == USB_OTG_dev.dev.cur_status) {
                RV_LED_G(1);
                packet_receive = 0;
                usbd_ep_recev(&USB_OTG_dev, CDC_ACM_DATA_OUT_EP, temp, CDC_ACM_DATA_PACKET_SIZE);
                while (!packet_receive);
                RV_LED_G(0);
                for (int i = 0;i < receive_length;i++) {
                    if (-1 == dollar && '$' == temp[i]) {
                        dollar = i + total;
                    }
                    if (-1 == sharp && '#' == temp[i]) {
                        sharp = i + total;
                    }
                }
                if (dollar >= 0) {
                    memcpy(&cmd_buffer[total], temp, receive_length);
                }
                total += receive_length;
                if (dollar >= 0 && sharp > 0) {
                    break;
                }
            }
        }
        cmd.len = sharp - dollar - 1;
        cmd.data = &cmd_buffer[dollar + 1];
        cmd.data[cmd.len] = '\0';
        xQueueSend(gdb_cmd_packet_xQueue, &cmd, portMAX_DELAY);
    }
}

void gdb_rsp_packet_vTask(void* pvParameters)
{
    BaseType_t xReturned;
    uint32_t checksum;

    for (;;) {
        xQueueReceive(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
        checksum = gdb_packet_checksum((const uint8_t*)rsp.data, rsp.len);
        snprintf(&rsp.data[rsp.len], 5, "#%02x|", checksum);
        if (no_ack_mode) {
            rsp.data -= 1;
            rsp.data[0] = '$';
            rsp.len += 5;
        } else {
            rsp.data -= 2;
            rsp.data[0] = '+';
            rsp.data[1] = '$';
            rsp.len += 6;
        }
        do {
            if (USBD_CONFIGURED == USB_OTG_dev.dev.cur_status) {
                RV_LED_G(0);
                packet_sent = 0;
                if (rsp.len >= CDC_ACM_DATA_PACKET_SIZE) {
                    usbd_ep_send(&USB_OTG_dev, CDC_ACM_DATA_IN_EP, rsp.data, CDC_ACM_DATA_PACKET_SIZE);
                    rsp.data += CDC_ACM_DATA_PACKET_SIZE;
                    rsp.len -= CDC_ACM_DATA_PACKET_SIZE;
                } else {
                    usbd_ep_send(&USB_OTG_dev, CDC_ACM_DATA_IN_EP, rsp.data, rsp.len);
                    rsp.data += rsp.len;
                    rsp.len -= rsp.len;
                }
                while (!packet_sent);
                RV_LED_G(1);
            }
        } while (rsp.len);
    }
}

void gdb_packet_init(void)
{
    BaseType_t xReturned;

    gdb_cmd_packet_xQueue = xQueueCreate(1, sizeof(gdb_packet_t));
    if (gdb_cmd_packet_xQueue == NULL) {
        /* Queue was not created and must not be used. */
    }

    gdb_rsp_packet_xQueue= xQueueCreate(1, sizeof(gdb_packet_t));
    if (gdb_rsp_packet_xQueue == NULL) {
        /* Queue was not created and must not be used. */
    }

    xReturned = xTaskCreate(gdb_cmd_packet_vTask,     /* Function that implements the task. */
                            "gdb_cmd_packet",         /* Text name for the task. */
                            256,                      /* Stack size in words, not bytes. */
                            NULL,                     /* Parameter passed into the task. */
                            2,                        /* Priority at which the task is created. */
                            &gdb_cmd_packet_xHandle); /* Used to pass out the created task's handle. */
    if(xReturned != pdPASS) {
        /* error msg */
        vTaskDelete(gdb_cmd_packet_xHandle);
    }

    xReturned = xTaskCreate(gdb_rsp_packet_vTask,     /* Function that implements the task. */
                            "gdb_rsp_packet",         /* Text name for the task. */
                            256,                      /* Stack size in words, not bytes. */
                            NULL,                     /* Parameter passed into the task. */
                            3,                        /* Priority at which the task is created. */
                            &gdb_rsp_packet_xHandle); /* Used to pass out the created task's handle. */
    if(xReturned != pdPASS) {
        /* error msg */
        vTaskDelete(gdb_rsp_packet_xHandle);
    }
}

/*---------------------------------------------------------------------------*/
void gdb_set_no_ack_mode(bool mode)
{
    no_ack_mode = mode;
}
/*---------------------------------------------------------------------------*/
static uint32_t gdb_packet_checksum(const uint8_t* p, uint32_t len)
{
    uint32_t checksum = 0;
    uint32_t i;

    for (i = 0; i < len; i++) {
        checksum += p[i];
    }
    checksum &= 0xff;

    return checksum;
}
