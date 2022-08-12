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

TaskHandle_t gdb_cmd_packet_xHandle = NULL;
TaskHandle_t gdb_rsp_packet_xHandle = NULL;
QueueHandle_t gdb_cmd_packet_xQueue;
QueueHandle_t gdb_rsp_packet_xQueue;
QueueHandle_t gdb_rxdata_xQueue;
QueueHandle_t gdb_txdata_xQueue;

static gdb_packet_t cmd = {0};
static gdb_packet_t rsp = {0};

bool no_ack_mode;

static uint8_t gdb_packet_checksum(const uint8_t* p, size_t len);

void gdb_cmd_packet_vTask(void* pvParameters)
{
    char ch;
    BaseType_t xReturned;

    for (;;) {
        cmd.len = 0;
        memset(cmd.data, 0x00, GDB_PACKET_BUFF_SIZE);
        xQueueReceive(gdb_rxdata_xQueue, &ch, portMAX_DELAY);
        switch (ch) {
            /* Start Packet */
            case '$':
                while (1) {
                    xReturned = xQueueReceive(gdb_rxdata_xQueue, &ch, (100 / portTICK_PERIOD_MS));
                    if (xReturned != pdPASS) {
                        /* timeout warning msg */
                        break;
                    }
                    /* End packet */
                    if (ch == '#') {
                        if (cmd.len > 0) {
                            xQueueSend(gdb_cmd_packet_xQueue, &cmd, portMAX_DELAY);
                        }
                        break;
                    }
                    cmd.data[cmd.len++] = ch;
                }
                break;
            /* Ctrl + C */
            case '\x03':
                cmd.data[cmd.len++] = ch;
                xQueueSend(gdb_cmd_packet_xQueue, &cmd, portMAX_DELAY);
                break;
            default:
                break;
        }
    }
}

void gdb_rsp_packet_vTask(void* pvParameters)
{
    char ch;
    uint8_t checksum;
    BaseType_t xReturned;

    for (;;) {
        xQueueReceive(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
        if (no_ack_mode) {
            ch = '$';
            xQueueSend(gdb_txdata_xQueue, &ch, portMAX_DELAY);
        } else {
            ch = '+';
            xQueueSend(gdb_txdata_xQueue, &ch, portMAX_DELAY);
            ch = '$';
            xQueueSend(gdb_txdata_xQueue, &ch, portMAX_DELAY);
        }
        for (int i = 0;i < rsp.len;i++) {
            xQueueSend(gdb_txdata_xQueue, &rsp.data[i], portMAX_DELAY);
        }
        ch = '#';
        xQueueSend(gdb_txdata_xQueue, &ch, portMAX_DELAY);
        checksum = gdb_packet_checksum((const uint8_t*)&rsp.data, rsp.len);
        /* 'checksum[0]' 'checksum[1]' '\0' */
        snprintf(rsp.data, 3, "%02x", checksum);
        xQueueSend(gdb_txdata_xQueue, &rsp.data[0], portMAX_DELAY);
        xQueueSend(gdb_txdata_xQueue, &rsp.data[1], portMAX_DELAY);
    }
}

void gdb_packet_init(void)
{
    BaseType_t xReturned;

    no_ack_mode = false;

    gdb_rxdata_xQueue = xQueueCreate(GDB_DATA_CACHE_SIZE, sizeof(char));
    if (gdb_rxdata_xQueue == NULL) {
        /* Queue was not created and must not be used. */
    }

    gdb_txdata_xQueue= xQueueCreate(GDB_DATA_CACHE_SIZE, sizeof(char));
    if (gdb_txdata_xQueue == NULL) {
        /* Queue was not created and must not be used. */
    }

    gdb_cmd_packet_xQueue = xQueueCreate(GDB_PACKET_BUFF_NUM, sizeof(gdb_packet_t));
    if (gdb_cmd_packet_xQueue == NULL) {
        /* Queue was not created and must not be used. */
    }

    gdb_rsp_packet_xQueue= xQueueCreate(GDB_PACKET_BUFF_NUM, sizeof(gdb_packet_t));
    if (gdb_rsp_packet_xQueue == NULL) {
        /* Queue was not created and must not be used. */
    }

    xReturned = xTaskCreate(gdb_cmd_packet_vTask,     /* Function that implements the task. */
                            "gdb_cmd_packet",         /* Text name for the task. */
                            256,                      /* Stack size in words, not bytes. */
                            NULL,                     /* Parameter passed into the task. */
                            3,                        /* Priority at which the task is created. */
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
static uint8_t gdb_packet_checksum(const uint8_t* p, size_t len)
{
    uint32_t checksum = 0;
    size_t i;

    for (i = 0; i < len; i++) {
        checksum += p[i];
    }
    checksum &= 0xff;

    return (uint8_t)checksum;
}
