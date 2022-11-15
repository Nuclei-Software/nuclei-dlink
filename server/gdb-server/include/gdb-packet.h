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

#ifndef __GDB_PACKET_H__
#define __GDB_PACKET_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "port.h"

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

#ifdef __cplusplus
}
#endif

#endif /* __GDB_PACKET_H__ */
