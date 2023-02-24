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

#ifndef __GDB_PACKET_H__
#define __GDB_PACKET_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "port.h"
#include "usbd_conf.h"

typedef struct {
    uint32_t len;
    uint8_t* data;
} gdb_packet_t;

extern QueueHandle_t gdb_cmd_packet_xQueue;
extern QueueHandle_t gdb_rsp_packet_xQueue;
extern uint8_t cmd_buffer[GDB_PACKET_BUFF_SIZE + 64];
extern uint8_t rsp_buffer[GDB_PACKET_BUFF_SIZE + 64];

void gdb_packet_init(void);

/*
 * Enter NoAckMode.
 */
void gdb_set_no_ack_mode(bool no_ack_mode);

#ifdef __cplusplus
}
#endif

#endif /* __GDB_PACKET_H__ */
