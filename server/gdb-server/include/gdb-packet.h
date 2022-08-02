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

#define GDB_PACKET_COMMAND_BUFFER_SIZE          1024
#define GDB_PACKET_RESPONSE_BUFFER_SIZE         1024

typedef enum send_flags_e {
    GDB_PACKET_SEND_FLAG_DOLLAR                 = (1 << 0),
    GDB_PACKET_SEND_FLAG_SHARP                  = (1 << 1),
    GDB_PACKET_SEND_FLAG_ALL                    = (GDB_PACKET_SEND_FLAG_DOLLAR | GDB_PACKET_SEND_FLAG_SHARP),
} gdb_packet_send_flags_t;

void gdb_packet_init(void);

/*
 * GDB server calls gdb_packet_command_buffer to get the address of the first
 * character of the command, not including '$'. If NULL is returned, it means
 * that the USB Serial has actually received the command and cannot be used by
 * GDB server temporarily.
 */
const char* gdb_packet_command_buffer(void);

/*
 * GDB server calls gdb_packet_command_length to get the length of the command,
 * not including the '$', '#' or checksum. This should only be called when
 * gdb_packet_command_buffer returns a non-NULL value.
 */
size_t gdb_packet_command_length(void);

/*
 * GDB server calls gdb_packet_command_done after processing the command to
 * inform GDB serial that it can receive the next command.
 */
void gdb_packet_command_done(void);

/*
 * GDB server calls gdb_packet_response_buffer to get the first address of the
 * response data buffer, GDB server does not need to write '$''#' checksum.
 * If NULL is returned, it means that the response buffer is actually used by
 * USB Serial, and GDB server cannot be used.
 */
char* gdb_packet_response_buffer(void);

/*
 * GDB server calls gdb_packet_response_done to notify GDB serial to send
 * a response, before sending, GDB serial fills in the '$', '#' and checksum
 * according to the send_flags.
 */
void gdb_packet_response_done(size_t len, gdb_packet_send_flags_t send_flags);

/*
 * Enter NoAckMode.
 */
void gdb_packet_no_ack_mode(bool no_ack_mode);

/* TODO correct comments
 * After USB serial receives the data, it calls gdb_packet_process_command to
 * transfer data to the component using the virtual serial port.
 * If the return value is false, then the buffer is full and try again later;
 * if the return value is true, the transmission is successful and the next
 * packet of data can be received.
 */
bool gdb_packet_process_command(const uint8_t* buffer, size_t len);

/* TODO correct comments
 * USB serial calls gdb_packet_get_response to check if there is data to
 * be sent. If the return value is valid, then there is data to be sent, and the
 * length of the data to be sent is returned by the len parameter; if it returns
 * NULL, it means that there is no data to send.
 */
const uint8_t* gdb_packet_get_response(size_t* len);

/* TODO correct comments
 * After USB serial sends data, call gdb_packet_release_response to notify the
 * components using the virtual serial port. After receiving the notification,
 * the consumer can start using the sending buffer again.
 */
void gdb_packet_release_response(void);

#endif /* __RV_LINK_GDB_SERVER_GDB_PACKET_H__ */
