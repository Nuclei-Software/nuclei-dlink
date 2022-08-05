/**
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 *
 * \file  rv-link/gdb-server/gdb-server.c
 * \brief Implementation of the GDB server.
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

/* own header file include */
#include "gdb-server.h"

/* system library header file includes */
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* other library header file includes */
#include "riscv_encoding.h"

/* other project header file includes */
#include "target-config.h"
#include "target.h"

/* own component header file includes */
#include "gdb-packet.h"
#include "ringbuffer.h"

static gdb_packet_t cmd = {0};
static gdb_packet_t rsp = {0};

typedef int16_t gdb_server_tid_t;

typedef struct gdb_server_s
{
    bool target_running;
    bool gdb_connected;
    rvl_target_halt_info_t halt_info;
    rvl_target_error_t target_error;
    rvl_target_addr_t mem_addr;
    size_t mem_len;
    uint8_t mem_buffer[GDB_PACKET_BUFF_SIZE];
    int flash_err;
    int i;
    rvl_target_reg_t regs[RVL_TARGET_CONFIG_REG_NUM];
    rvl_target_reg_t reg_tmp;
    int reg_tmp_num;

    gdb_server_tid_t tid_g;
    gdb_server_tid_t tid_G;
    gdb_server_tid_t tid_m;
    gdb_server_tid_t tid_M;
    gdb_server_tid_t tid_c;

    rvl_target_breakpoint_type_t breakpoint_type;
    rvl_target_addr_t breakpoint_addr;
    int breakpoint_kind;
    int breakpoint_err;
} gdb_server_t;

static gdb_server_t gdb_server_i;
#define self gdb_server_i

void gdb_server_connected(void);
void gdb_server_disconnected(void);
void gdb_server_cmd_c(void);
void gdb_server_cmd_ctrl_c(void);
void gdb_server_cmd_g(void);
void gdb_server_cmd_G(void);
void gdb_server_cmd_H(void);
void gdb_server_cmd_k(void);
void gdb_server_cmd_m(void);
void gdb_server_cmd_M(void);
void gdb_server_cmd_p(void);
void gdb_server_cmd_P(void);
void gdb_server_cmd_q(void);
void gdb_server_cmd_Q(void);
void gdb_server_cmd_qRcmd(void);
void gdb_server_cmd_qSupported(void);
void gdb_server_cmd_question_mark(void);
void gdb_server_cmd_s(void);
void gdb_server_cmd_v(void);
void gdb_server_cmd_vFlashDone(void);
void gdb_server_cmd_vFlashErase(void);
void gdb_server_cmd_vFlashWrite(void);
void gdb_server_cmd_vMustReplyEmpty(void);
void gdb_server_cmd_X(void);
void gdb_server_cmd_z(void);
void gdb_server_cmd_Z(void);

static void gdb_server_target_run(bool run);
static void gdb_server_reply_ok(void);
static void gdb_server_reply_empty(void);
static void gdb_server_reply_err(int err);

static void gdb_server_cmd_qxfer_features_read_target_xml(void);
static void gdb_server_cmd_qxfer_memory_map_read(void);

static void bin_to_hex(const uint8_t *bin, char *hex, size_t nbyte);
static void word_to_hex_le(uint32_t word, char *hex);
static void hex_to_bin(const char *hex, uint8_t *bin, size_t nbyte);
static void hex_to_word_le(const char *hex, uint32_t *word);
static size_t bin_decode(const uint8_t* xbin, uint8_t* bin, size_t xbin_len);


void gdb_server_init(void)
{
    gdb_server_target_run(false);
    self.gdb_connected = false;
}


void gdb_server_poll(void)
{
    char c;
    size_t ret, len;

    for (;;) {
        rsp.len = 0;
        if (self.gdb_connected && self.target_running) {
            ret = gdb_resp_buf_getchar(&c);
            if (ret > 0) {
                rsp.data[0] = 'O';
                len = 0;
                do {
                    bin_to_hex((uint8_t*)&c, &rsp.data[1 + len * 2], 1);
                    len++;
                    ret = gdb_resp_buf_getchar(&c);
                } while (ret > 0);
                //TODO:send ringbuffer
            }

            xQueueReceive(gdb_cmd_packet_xQueue, &cmd, portMAX_DELAY);
            if (*cmd.data == '\x03' && cmd.len == 1) {
                gdb_server_cmd_ctrl_c();
                gdb_server_target_run(false);
                strncpy(rsp.data, "T02", GDB_PACKET_BUFF_SIZE);
                rsp.len = 3;
                // rsp.len = strlen(rsp.data);
                xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
            }

            if (self.target_running) {
                rvl_target_halt_check(&self.halt_info);
                if (self.halt_info.reason != rvl_target_halt_reason_running) {
                    gdb_server_target_run(false);

                    if (self.halt_info.reason == rvl_target_halt_reason_other) {
                        strncpy(rsp.data, "T05", GDB_PACKET_BUFF_SIZE);
                    } else if (self.halt_info.reason == rvl_target_halt_reason_write_watchpoint) {
                        snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "T05watch:%x;", (unsigned int)self.halt_info.addr);
                    } else if (self.halt_info.reason == rvl_target_halt_reason_read_watchpoint) {
                        snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "T05rwatch:%x;", (unsigned int)self.halt_info.addr);
                    } else if (self.halt_info.reason == rvl_target_halt_reason_access_watchpoint) {
                        snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "T05awatch:%x;", (unsigned int)self.halt_info.addr);
                    } else if (self.halt_info.reason == rvl_target_halt_reason_hardware_breakpoint) {
                        strncpy(rsp.data, "T05hwbreak:;", GDB_PACKET_BUFF_SIZE);
                    } else if (self.halt_info.reason == rvl_target_halt_reason_software_breakpoint) {
                        strncpy(rsp.data, "T05swbreak:;", GDB_PACKET_BUFF_SIZE);
                    } else {
                        //TODO:
                    }
                    rsp.len = strlen(rsp.data);
                    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
                }
            }
        } else {
            xQueueReceive(gdb_cmd_packet_xQueue, &cmd, portMAX_DELAY);
            c = *cmd.data;
            if (c == 'q') {
                gdb_server_cmd_q();
            } else if (c == 'Q') {
                gdb_server_cmd_Q();
            } else if (c == 'H') {
                gdb_server_cmd_H();
            } else if (c == '?') {
                gdb_server_cmd_question_mark();
            } else if (c == 'g') {
                gdb_server_cmd_g();
            } else if (c == 'G') {
                gdb_server_cmd_G();
            } else if (c == 'k') {
                gdb_server_cmd_k();
            } else if (c == 'c') {
                gdb_server_cmd_c();
            } else if (c == 'm') {
                gdb_server_cmd_m();
            } else if (c == 'M') {
                gdb_server_cmd_M();
            } else if (c == 'X') {
                gdb_server_cmd_X();
            } else if (c == 'p') {
                gdb_server_cmd_p();
            } else if (c == 'P') {
                gdb_server_cmd_P();
            } else if (c == 's') {
                gdb_server_cmd_s();
            } else if (c == 'z') {
                gdb_server_cmd_z();
            } else if (c == 'Z') {
                gdb_server_cmd_Z();
            } else if (c == 'v') {
                gdb_server_cmd_v();
            } else {
                gdb_server_reply_empty();
            }
        }
    }
}


/*
 * ‘q name params...’
 * General query (‘q’) and set (‘Q’).
 */
void gdb_server_cmd_q(void)
{
    if (strncmp(cmd.data, "qSupported:", 11) == 0) {
        gdb_server_cmd_qSupported();
    } else if (strncmp(cmd.data, "qXfer:features:read:target.xml:", 31) == 0) {
        gdb_server_cmd_qxfer_features_read_target_xml();
    } else if (strncmp(cmd.data, "qXfer:memory-map:read::", 23) == 0) {
        gdb_server_cmd_qxfer_memory_map_read();
    } else if (strncmp(cmd.data, "qRcmd,", 6) == 0) {
        gdb_server_cmd_qRcmd();
    } else {
        gdb_server_reply_empty();
    }
}


/*
 * ‘qSupported [:gdbfeature [;gdbfeature]... ]’
 * Tell the remote stub about features supported by gdb, and query the stub for
 * features it supports.
 */
void gdb_server_cmd_qSupported(void)
{
    const char qSupported_res[] =
            "PacketSize=405"
            ";QStartNoAckMode+"
            ";qXfer:features:read+"
            ";qXfer:memory-map:read+"
            ";swbreak+"
            ";hwbreak+"
            ;

    gdb_set_no_ack_mode(false);
    strncpy(rsp.data, qSupported_res, GDB_PACKET_BUFF_SIZE);
    rsp.len = strlen(qSupported_res);
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
    gdb_server_connected();
}


/*
 * qXfer:features:read:target.xml
 */
static void gdb_server_cmd_qxfer_features_read_target_xml(void)
{
    size_t target_xml_len;
    const char *target_xml_str;
    unsigned int read_addr;
    unsigned int read_len;

    sscanf(&cmd.data[31], "%x,%x", &read_addr, &read_len);

    if (read_len > GDB_PACKET_BUFF_SIZE) {
        read_len = GDB_PACKET_BUFF_SIZE;
    }

    target_xml_len = rvl_target_get_target_xml_len();

    if (read_len >= target_xml_len - read_addr) {
        read_len = target_xml_len - read_addr;
        rsp.data[0] = 'l';
    } else {
        rsp.data[0] = 'm';
    }

    target_xml_str = rvl_target_get_target_xml();
    strncpy(&rsp.data[1], &target_xml_str[read_addr], read_len);
    rsp.len = read_len + 1;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}

/*
 * qXfer:memory-map:read::
 */
static void gdb_server_cmd_qxfer_memory_map_read(void)
{
    size_t res_len;

    res_len = 0;
    res_len += snprintf(&rsp.data[0], GDB_PACKET_BUFF_SIZE, "l<memory-map>");
#if RVL_TARGET_CONFIG_ADDR_WIDTH == 32
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "<memory type=\"%s\" start=\"0x%x\" length=\"0x%x\"", "ram", 0x00, 0x00);
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "/>");
#else
#error FIXME
#endif
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "</memory-map>");

    rsp.len = res_len;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
#if 0
    size_t memory_map_len;
    const rvl_target_memory_t* memory_map;
    size_t res_len;

    /*
     * Assuming that a packet of data can be sent out!
     */

    memory_map_len = rvl_target_get_memory_map_len();
    memory_map = rvl_target_get_memory_map();

    res_len = 0;
    res_len += snprintf(&rsp.data[0], GDB_PACKET_BUFF_SIZE, "l<memory-map>");
    for(self.i = 0; self.i < memory_map_len; self.i++) {
#if RVL_TARGET_CONFIG_ADDR_WIDTH == 32
        res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
                "<memory type=\"%s\" start=\"0x%x\" length=\"0x%x\"",
                memory_map[self.i].type == rvl_target_memory_type_flash ? "flash" : "ram",
                (unsigned int)memory_map[self.i].start, (unsigned int)memory_map[self.i].length);

        if (memory_map[self.i].type == rvl_target_memory_type_flash) {
            res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
                    "><property name=\"blocksize\">0x%x</property></memory>",
                    (unsigned int)memory_map[self.i].blocksize);
        } else {
            res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
                    "/>");
        }
#else
#error FIXME
#endif
    }
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "</memory-map>");

    rsp.len = res_len;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
#endif
}
/*
 * ‘qRcmd,command’
 * command (hex encoded) is passed to the local interpreter for execution.
 */
void gdb_server_cmd_qRcmd(void)
{
    char c;
    size_t ret, len;
    const char * err_str;
    uint32_t err_pc;
    int err;
    const char unspported_monitor_command[] = ":( unsupported monitor command!\n";

    self.mem_len = (cmd.len - 6) / 2;
    hex_to_bin(&cmd.data[6], self.mem_buffer, self.mem_len);
    self.mem_buffer[self.mem_len] = 0;

    if (strncmp((char*)self.mem_buffer, "reset", 5) == 0) {
        rvl_target_reset();
        gdb_server_reply_ok();
    } else if (strncmp((char*)self.mem_buffer, "halt", 4) == 0) {
        gdb_server_reply_ok();
    } else if (strncmp((char*)self.mem_buffer, "show error", 10) == 0) {
        rvl_target_get_error(&err_str, &err_pc);
        snprintf((char*)self.mem_buffer, GDB_PACKET_BUFF_SIZE,
                "RV-LINK ERROR: %s, @0x%08x\r\n", err_str, (unsigned int)err_pc);
        len = strlen((char*)self.mem_buffer);
        rsp.data[0] = 'O';
        bin_to_hex(self.mem_buffer, &rsp.data[1], len);
        rsp.len = len * 2 + 1;
        // rsp.len = strlen(rsp.data);
        xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
        gdb_server_reply_ok();
    } else {
        bin_to_hex((uint8_t*)unspported_monitor_command, rsp.data, sizeof(unspported_monitor_command) - 1);
        rsp.len = (sizeof(unspported_monitor_command) - 1) * 2;
        // rsp.len = strlen(rsp.data);
        xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
    }
}


/*
 * ‘Q name params...’
 * General query (‘q’) and set (‘Q’).
 */
void gdb_server_cmd_Q(void)
{
    if (strncmp(cmd.data, "QStartNoAckMode", 15) == 0) {
        gdb_server_reply_ok();
        gdb_set_no_ack_mode(true);
    } else {
        gdb_server_reply_empty();
    }
}


/*
 * ‘H op thread-id’
 * Set thread for subsequent operations (‘m’, ‘M’, ‘g’, ‘G’, et.al.).
 */
void gdb_server_cmd_H(void)
{
    unsigned int n;
    char ch;
    gdb_server_tid_t tid;

    ch = cmd.data[1];
    if (ch == 'g' || ch == 'G' || ch == 'm' || ch == 'M' || ch == 'c') {
        sscanf(&cmd.data[2], "%x", &n);
        gdb_server_reply_ok();

        tid = (gdb_server_tid_t)n;
        if (ch == 'g') {
            self.tid_g = tid;
        } else if (ch == 'G') {
            self.tid_G = tid;
        } else if (ch == 'm') {
            self.tid_m = tid;
        } else if (ch == 'M') {
            self.tid_M = tid;
        } else {
            self.tid_c = tid;
        }
    } else {
        gdb_server_reply_empty();
    }
}


/*
 * ‘?’
 * Indicate the reason the target halted. The reply is the same as for step and continue.
 */
void gdb_server_cmd_question_mark(void)
{
    strncpy(rsp.data, "S02", GDB_PACKET_BUFF_SIZE);
    rsp.len = 3;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


/*
 * ‘g’
 * Read general registers.
 */
void gdb_server_cmd_g(void)
{
    int i;

    rvl_target_read_core_registers(&self.regs[0]);

    for(i = 0; i < RVL_TARGET_CONFIG_REG_NUM; i++) {
        word_to_hex_le(self.regs[i], &rsp.data[i * (RVL_TARGET_CONFIG_REG_WIDTH / 8 * 2)]);
    }
    rsp.len = RVL_TARGET_CONFIG_REG_WIDTH / 8 * 2 * RVL_TARGET_CONFIG_REG_NUM;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


/*
 * ‘G XX...’
 * Write general registers.
 */
void gdb_server_cmd_G(void)
{
    int i;

    for(i = 0; i < RVL_TARGET_CONFIG_REG_NUM; i++) {
        hex_to_word_le(&cmd.data[i * (RVL_TARGET_CONFIG_REG_WIDTH / 8 * 2) + 1], &self.regs[i]);
    }

    rvl_target_write_core_registers(&self.regs[0]);

    gdb_server_reply_ok();
}


/*
 * ‘p n’
 * Read the value of register n; n is in hex.
 */
void gdb_server_cmd_p(void)
{
    sscanf(&cmd.data[1], "%x", &self.reg_tmp_num);

    rvl_target_read_register(&self.reg_tmp, self.reg_tmp_num);

    word_to_hex_le(self.reg_tmp, &rsp.data[0]);

    rsp.len = RVL_TARGET_CONFIG_REG_WIDTH / 8 * 2;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


/*
 * ‘P n...=r...’
 * Write register n... with value r... The register number n is in hexadecimal,
 * and r... contains two hex digits for each byte in the register (target byte order).
 */
void gdb_server_cmd_P(void)
{
    const char *p;

    sscanf(&cmd.data[1], "%x", &self.reg_tmp_num);
    p = strchr(&cmd.data[1], '=');
    p++;

    hex_to_word_le(p, &self.reg_tmp);
    rvl_target_write_register(self.reg_tmp, self.reg_tmp_num);

    gdb_server_reply_ok();
}


/*
 * ‘m addr,length’
 * Read length addressable memory units starting at address addr.
 * Note that addr may not be aligned to any particular boundary.
 */
void gdb_server_cmd_m(void)
{
    char *p;

    p = strchr(&cmd.data[1], ',');
    p++;
#if RVL_TARGET_CONFIG_ADDR_WIDTH == 32
    sscanf(&cmd.data[1], "%x", (unsigned int*)(&self.mem_addr));
#else
#error
#endif
    sscanf(p, "%x", (unsigned int*)(&self.mem_len));
    if (self.mem_len > sizeof(self.mem_buffer)) {
        self.mem_len = sizeof(self.mem_buffer);
    }

    rvl_target_read_memory(self.mem_buffer, self.mem_addr, self.mem_len);

    bin_to_hex(self.mem_buffer, rsp.data, self.mem_len);
    rsp.len = self.mem_len * 2;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


/*
 * ‘M addr,length:XX...’
 * Write length addressable memory units starting at address addr.
 */
void gdb_server_cmd_M(void)
{
    const char *p;

    sscanf(&cmd.data[1], "%x,%x", (unsigned int*)(&self.mem_addr), (unsigned int*)(&self.mem_len));
    p = strchr(&cmd.data[1], ':');
    p++;

    if (self.mem_len > sizeof(self.mem_buffer)) {
        self.mem_len = sizeof(self.mem_buffer);
    }

    hex_to_bin(p, self.mem_buffer, self.mem_len);
    rvl_target_write_memory(self.mem_buffer, self.mem_addr, self.mem_len);

    gdb_server_reply_ok();
}


/*
 * ‘X addr,length:XX...’
 * Write data to memory, where the data is transmitted in binary.
 */
void gdb_server_cmd_X(void)
{
    const char *p;
    size_t length;

    sscanf(&cmd.data[1], "%x,%x", (unsigned int*)(&self.mem_addr), (unsigned int*)(&self.mem_len));
    if (self.mem_len == 0) {
        gdb_server_reply_ok();
        return;
    }

    p = strchr(&cmd.data[1], ':');
    p++;

    if (self.mem_len > sizeof(self.mem_buffer)) {
        self.mem_len = sizeof(self.mem_buffer);
    }

    length = cmd.len - ((size_t)p - (size_t)cmd.data);
    bin_decode((uint8_t*)p, self.mem_buffer, length);

    rvl_target_write_memory(self.mem_buffer, self.mem_addr, self.mem_len);

    gdb_server_reply_ok();
}


/*
 * ‘k’
 * Kill request.
 */
void gdb_server_cmd_k(void)
{
    gdb_server_reply_ok();

    gdb_server_disconnected();
}


/*
 * ‘c [addr]’
 * Continue at addr, which is the address to resume. If addr is omitted, resume
 * at current address.
 */
void gdb_server_cmd_c(void)
{
    rvl_target_resume();
    gdb_server_target_run(true);
}


/*
 * ‘s [addr]’
 * Single step, resuming at addr. If addr is omitted, resume at same address.
 */
void gdb_server_cmd_s(void)
{
    rvl_target_step();
    gdb_server_target_run(true);
}


/*
 * ‘z type,addr,kind’
 * Insert (‘Z’) or remove (‘z’) a type breakpoint or watchpoint starting at address
 * address of kind kind.
 */
void gdb_server_cmd_z(void)
{
    int type, addr, kind;

    sscanf(cmd.data, "z%x,%x,%x", &type, &addr, &kind);
    self.breakpoint_type = type;
    self.breakpoint_addr = addr;
    self.breakpoint_kind = kind;

    rvl_target_remove_breakpoint(
            self.breakpoint_type, self.breakpoint_addr, self.breakpoint_kind, &self.breakpoint_err);
    if (self.breakpoint_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(self.breakpoint_err);
    }
}


/*
 * ‘Z type,addr,kind’
 * Insert (‘Z’) or remove (‘z’) a type breakpoint or watchpoint starting at address
 * address of kind kind.
 */
void gdb_server_cmd_Z(void)
{
    int type, addr, kind;

    sscanf(cmd.data, "Z%x,%x,%x", &type, &addr, &kind);
    self.breakpoint_type = type;
    self.breakpoint_addr = addr;
    self.breakpoint_kind = kind;

    rvl_target_insert_breakpoint(
            self.breakpoint_type, self.breakpoint_addr, self.breakpoint_kind, &self.breakpoint_err);
    if (self.breakpoint_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(self.breakpoint_err);
    }
}


/*
 * Ctrl+C
 */
void gdb_server_cmd_ctrl_c(void)
{
    rvl_target_halt();
}


/*
 * ‘v’
 * Packets starting with ‘v’ are identified by a multi-letter name.
 */
void gdb_server_cmd_v(void)
{
    if (strncmp(cmd.data, "vFlashErase:", 12) == 0) {
        gdb_server_cmd_vFlashErase();
    } else if (strncmp(cmd.data, "vFlashWrite:", 12) == 0) {
        gdb_server_cmd_vFlashWrite();
    } else if (strncmp(cmd.data, "vFlashDone", 10) == 0) {
        gdb_server_cmd_vFlashDone();
    } else if (strncmp(cmd.data, "vMustReplyEmpty", 15) == 0) {
        gdb_server_cmd_vMustReplyEmpty();
    } else {
        gdb_server_reply_empty();
    }
}


/*
 * ‘vFlashErase:addr,length’
 * Direct the stub to erase length bytes of flash starting at addr.
 */
void gdb_server_cmd_vFlashErase(void)
{
    int addr, length;

    sscanf(&cmd.data[12], "%x,%x", &addr, &length);
    self.mem_addr = addr;
    self.mem_len = length;

    rvl_target_flash_erase(self.mem_addr, self.mem_len, &self.flash_err);
    if (self.flash_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(self.flash_err);
        gdb_server_disconnected();
    }
}


/*
 * ‘vFlashWrite:addr:XX...’
 * Direct the stub to write data to flash address addr.
 * The data is passed in binary form using the same encoding as for the ‘X’ packet.
 */
void gdb_server_cmd_vFlashWrite(void)
{
    int addr;
    const char *p;
    size_t length;

    sscanf(&cmd.data[12], "%x", &addr);
    self.mem_addr = addr;

    p = strchr(&cmd.data[12], ':');
    p++;

    length = cmd.len - ((size_t)p - (size_t)cmd.data);
    self.mem_len = bin_decode((uint8_t*)p, self.mem_buffer, length);

    rvl_target_flash_write(self.mem_addr, self.mem_len, self.mem_buffer, &self.flash_err);
    if (self.flash_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(self.flash_err);
        gdb_server_disconnected();
    }
}


/*
 * ‘vFlashDone’
 * Indicate to the stub that flash programming operation is finished.
 */
void gdb_server_cmd_vFlashDone(void)
{
    rvl_target_flash_done();
    gdb_server_reply_ok();
}


/*
 * ‘vMustReplyEmpty’
 * RV-LINK uses an unexpected vMustReplyEmpty response to inform the error
 */
void gdb_server_cmd_vMustReplyEmpty(void)
{
    size_t len;
    char c;

    if (self.target_error == rvl_target_error_none) {
        gdb_server_reply_empty();
    } else {
        len = 0;
        while (gdb_resp_buf_getchar(&c)) {
            rsp.data[len] = c;
            len++;
        }
        rsp.len = len;
        // rsp.len = strlen(rsp.data);
        xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);

        gdb_server_disconnected();
    }
}


static void gdb_server_reply_ok(void)
{
    strncpy(rsp.data, "OK", GDB_PACKET_BUFF_SIZE);
    rsp.len = 2;
    // rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


static void gdb_server_reply_empty(void)
{
    rsp.len = 0;
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


static void gdb_server_reply_err(int err)
{
    snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "E%02x", err);
    rsp.len = strlen(rsp.data);
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


void gdb_server_connected(void)
{
    char c;

    while (gdb_resp_buf_getchar(&c)) {};

    self.gdb_connected = true;
    gdb_server_target_run(false);

    riscv_target_init();
    riscv_target_init_post(&self.target_error);

    if (self.target_error == rvl_target_error_none) {
        rvl_target_halt();
        riscv_target_init_after_halted(&self.target_error);
    }

    if (self.target_error != rvl_target_error_none) {
        switch(self.target_error) {
        case rvl_target_error_line:
            gdb_resp_buf_puts("\nRV-LINK ERROR: the target is not connected!\n");
            break;
        case rvl_target_error_compat:
            gdb_resp_buf_puts("\nRV-LINK ERROR: the target is not supported, upgrade RV-LINK firmware!\n");
            break;
        case rvl_target_error_debug_module:
            gdb_resp_buf_puts("\nRV-LINK ERROR: something wrong with debug module!\n");
            break;
        case rvl_target_error_protected:
            gdb_resp_buf_puts("\nRV-LINK ERROR: the target under protected! disable protection then try again.\n");
            break;
        default:
            gdb_resp_buf_puts("\nRV-LINK ERROR: unknown error!\n");
            break;
        }
    }
}


void gdb_server_disconnected(void)
{
    if (self.target_running == false) {
        if (self.target_error != rvl_target_error_line) {
            rvl_target_resume();
            gdb_server_target_run(true);
        }
    }

    if (self.target_error != rvl_target_error_line) {
        riscv_target_fini_pre();
    }
    riscv_target_fini();

    self.gdb_connected = false;
}


static void gdb_server_target_run(bool run)
{
    self.target_running = run;
}


static void bin_to_hex(const uint8_t *bin, char *hex, size_t nbyte)
{
    size_t i;
    uint8_t hi;
    uint8_t lo;

    for(i = 0; i < nbyte; i++) {
        hi = (*bin >> 4) & 0xf;
        lo = *bin & 0xf;

        if (hi < 10) {
            *hex = '0' + hi;
        } else {
            *hex = 'a' + hi - 10;
        }

        hex++;

        if (lo < 10) {
            *hex = '0' + lo;
        } else {
            *hex = 'a' + lo - 10;
        }

        hex++;
        bin++;
    }
}


static void word_to_hex_le(uint32_t word, char *hex)
{
    uint8_t bytes[4];

    bytes[0] = word & 0xff;
    bytes[1] = (word >> 8) & 0xff;
    bytes[2] = (word >> 16) & 0xff;
    bytes[3] = (word >> 24) & 0xff;

    bin_to_hex(bytes, hex, 4);
}


static void hex_to_bin(const char *hex, uint8_t *bin, size_t nbyte)
{
    size_t i;
    uint8_t hi, lo;
    for(i = 0; i < nbyte; i++) {
        if (hex[i * 2] <= '9') {
            hi = hex[i * 2] - '0';
        } else if (hex[i * 2] <= 'F') {
            hi = hex[i * 2] - 'A' + 10;
        } else {
            hi = hex[i * 2] - 'a' + 10;
        }

        if (hex[i * 2 + 1] <= '9') {
            lo = hex[i * 2 + 1] - '0';
        } else if (hex[i * 2 + 1] <= 'F') {
            lo = hex[i * 2 + 1] - 'A' + 10;
        } else {
            lo = hex[i * 2 + 1] - 'a' + 10;
        }

        bin[i] = (hi << 4) | lo;
    }
}


static void hex_to_word_le(const char *hex, uint32_t *word)
{
    uint8_t bytes[4];

    hex_to_bin(hex, bytes, 4);

    *word = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}


static size_t bin_decode(const uint8_t* xbin, uint8_t* bin, size_t xbin_len)
{
    size_t bin_len = 0;
    size_t i;
    int escape_found = 0;

    for(i = 0; i < xbin_len; i++) {
        if (xbin[i] == 0x7d) {
            escape_found = 1;
        } else {
            if (escape_found) {
                bin[bin_len] = xbin[i] ^ 0x20;
                escape_found = 0;
            } else {
                bin[bin_len] = xbin[i];
            }
            bin_len++;
        }
    }

    return bin_len;
}
