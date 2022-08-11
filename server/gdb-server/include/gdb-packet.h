/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __GDB_PACKET_H__
#define __GDB_PACKET_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"

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
