#ifndef __RV_LINK_GDB_SERVER_GDB_PACKET_H__
#define __RV_LINK_GDB_SERVER_GDB_PACKET_H__
/**
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 *
 * \file  rv-link/gdb-server/gdb-packet.h
 * \brief Interface of the GDB server RSP packet handler.
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

/* system library header file includes */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Kernel includes. */
#include "FreeRTOS.h" /* Must come first. */
#include "queue.h"    /* RTOS queue related API prototypes. */
#include "semphr.h"   /* Semaphore related API prototypes. */
#include "task.h"     /* RTOS task related API prototypes. */
#include "timers.h"   /* Software timer related API prototypes. */

#define GDB_DATA_CACHE_SIZE           (1024)
#define GDB_PACKET_BUFF_SIZE          (1024 + 8)
#define GDB_PACKET_NUM                (3)

typedef struct {
    uint32_t len;
    char data[GDB_PACKET_BUFF_SIZE];
} gdb_packet_t;

extern QueueHandle_t gdb_cmd_packet_xQueue;
extern QueueHandle_t gdb_rsp_packet_xQueue;
extern QueueHandle_t gdb_rxdata_xQueue;
extern QueueHandle_t gdb_txdata_xQueue;

void gdb_packet_init(void);

/*
 * Enter NoAckMode.
 */
void gdb_set_no_ack_mode(bool no_ack_mode);

#endif /* __RV_LINK_GDB_SERVER_GDB_PACKET_H__ */
