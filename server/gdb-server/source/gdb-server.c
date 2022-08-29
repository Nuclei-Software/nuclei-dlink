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
#include "riscv-target.h"

static gdb_packet_t cmd = {0};
static gdb_packet_t rsp = {0};

typedef int16_t gdb_server_tid_t;

typedef struct gdb_server_s
{
    bool target_running;
    bool gdb_connected;
    rv_target_halt_info_t halt_info;
    rv_target_error_t target_error;
    
    uint32_t mem_addr;
    uint32_t mem_len;
    uint8_t mem_buffer[GDB_PACKET_BUFF_SIZE];
    uint32_t flash_err;
    uint32_t i;
    uint64_t regs[RVL_TARGET_CONFIG_REG_NUM];
    uint64_t reg_tmp;
    uint32_t reg_tmp_num;

    gdb_server_tid_t tid_g;
    gdb_server_tid_t tid_G;
    gdb_server_tid_t tid_m;
    gdb_server_tid_t tid_M;
    gdb_server_tid_t tid_c;

    rv_target_breakpoint_type_t breakpoint_type;
    uint32_t breakpoint_addr;
    uint32_t breakpoint_kind;
    uint32_t breakpoint_err;
} gdb_server_t;

static gdb_server_t gdb_server_i;

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

static void bin_to_hex(const uint8_t *bin, char *hex, uint32_t nbyte);
static void uint32_to_hex_le(uint32_t data, char *hex);
static void uint64_to_hex_le(uint64_t data, char *hex);
static void hex_to_bin(const char *hex, uint8_t *bin, uint32_t nbyte);
static void hex_to_uint32_le(const char *hex, uint32_t *data);
static void hex_to_uint64_le(const char *hex, uint64_t *data);
static uint32_t bin_decode(const uint8_t* xbin, uint8_t* bin, uint32_t xbin_len);


void gdb_server_init(void)
{
    gdb_server_target_run(false);
    gdb_server_i.gdb_connected = false;
}


void gdb_server_poll(void)
{
    char c;
    BaseType_t xReturned;
    uint32_t ret, len;

    for (;;) {
        rsp.len = 0;
        memset(rsp.data, 0x00, GDB_PACKET_BUFF_SIZE);
        if (gdb_server_i.gdb_connected && gdb_server_i.target_running) {
            xReturned = xQueueReceive(gdb_cmd_packet_xQueue, &cmd, (100 / portTICK_PERIOD_MS));
            if (xReturned == pdPASS) {
                if (*cmd.data == '\x03' && cmd.len == 1) {
                    gdb_server_cmd_ctrl_c();
                    gdb_server_target_run(false);
                    strncpy(rsp.data, "T02", GDB_PACKET_BUFF_SIZE);
                    rsp.len = 3;
                    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
                }
            }

            if (gdb_server_i.target_running) {
                rv_target_halt_check(&gdb_server_i.halt_info);
                if (gdb_server_i.halt_info.reason != rv_target_halt_reason_running) {
                    gdb_server_target_run(false);

                    if (gdb_server_i.halt_info.reason == rv_target_halt_reason_other) {
                        strncpy(rsp.data, "T05", GDB_PACKET_BUFF_SIZE);
                    } else if (gdb_server_i.halt_info.reason == rv_target_halt_reason_write_watchpoint) {
                        snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "T05watch:%x;", (unsigned int)gdb_server_i.halt_info.addr);
                    } else if (gdb_server_i.halt_info.reason == rv_target_halt_reason_read_watchpoint) {
                        snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "T05rwatch:%x;", (unsigned int)gdb_server_i.halt_info.addr);
                    } else if (gdb_server_i.halt_info.reason == rv_target_halt_reason_access_watchpoint) {
                        snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "T05awatch:%x;", (unsigned int)gdb_server_i.halt_info.addr);
                    } else if (gdb_server_i.halt_info.reason == rv_target_halt_reason_hardware_breakpoint) {
                        strncpy(rsp.data, "T05hwbreak:;", GDB_PACKET_BUFF_SIZE);
                    } else if (gdb_server_i.halt_info.reason == rv_target_halt_reason_software_breakpoint) {
                        strncpy(rsp.data, "T05swbreak:;", GDB_PACKET_BUFF_SIZE);
                    } else {
                        //TODO:
                    }
                    rsp.len = strlen(rsp.data);
                    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
                }
            }
        } else {
            xReturned = xQueueReceive(gdb_cmd_packet_xQueue, &cmd, portMAX_DELAY);
            if (xReturned == pdPASS) {
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
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
    gdb_server_connected();
}


/*
 * qXfer:features:read:target.xml
 */
static void gdb_server_cmd_qxfer_features_read_target_xml(void)
{
    uint32_t target_xml_len;
    const char *target_xml_str;
    unsigned int read_addr;
    unsigned int read_len;

    sscanf(&cmd.data[31], "%x,%x", &read_addr, &read_len);

    if (read_len > GDB_PACKET_BUFF_SIZE) {
        read_len = GDB_PACKET_BUFF_SIZE;
    }

    target_xml_len = rv_target_get_target_xml_len();

    if (read_len >= target_xml_len - read_addr) {
        read_len = target_xml_len - read_addr;
        rsp.data[0] = 'l';
    } else {
        rsp.data[0] = 'm';
    }

    target_xml_str = rv_target_get_target_xml();
    strncpy(&rsp.data[1], &target_xml_str[read_addr], read_len);
    rsp.len = read_len + 1;
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}

/*
 * qXfer:memory-map:read::
 */
static void gdb_server_cmd_qxfer_memory_map_read(void)
{
    uint32_t res_len;

    res_len = 0;
    res_len += snprintf(&rsp.data[0], GDB_PACKET_BUFF_SIZE, "l<memory-map>");
    // TODO:
    // ram
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "<memory type=\"%s\" start=\"0x%x\" length=\"0x%x\"", "ram", 0x00000000, 0x20000000);
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "/>");
    // flash
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "<memory type=\"%s\" start=\"0x%x\" length=\"0x%x\"", "flash", 0x20000000, 0x10000000);
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
                    "><property name=\"blocksize\">0x%x</property></memory>", 0x10000);
    // ram
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "<memory type=\"%s\" start=\"0x%x\" length=\"0x%x\"", "ram", 0x30000000, (~0) - 0x30000000 + 1);
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "/>");
    res_len += snprintf(&rsp.data[res_len], GDB_PACKET_BUFF_SIZE - res_len,
            "</memory-map>");

    rsp.len = res_len;
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}
/*
 * ‘qRcmd,command’
 * command (hex encoded) is passed to the local interpreter for execution.
 */
void gdb_server_cmd_qRcmd(void)
{
    char c;
    uint32_t ret, len;
    const char * err_str;
    uint32_t err_pc;
    int err;
    const char unspported_monitor_command[] = ":( unsupported monitor command!\n";

    gdb_server_i.mem_len = (cmd.len - 6) / 2;
    hex_to_bin(&cmd.data[6], gdb_server_i.mem_buffer, gdb_server_i.mem_len);
    gdb_server_i.mem_buffer[gdb_server_i.mem_len] = 0;

    if (strncmp((char*)gdb_server_i.mem_buffer, "reset", 5) == 0) {
        rv_target_reset();
        gdb_server_reply_ok();
    } else if (strncmp((char*)gdb_server_i.mem_buffer, "halt", 4) == 0) {
        gdb_server_reply_ok();
    } else if (strncmp((char*)gdb_server_i.mem_buffer, "show error", 10) == 0) {
        rv_target_get_error(&err_str, &err_pc);
        snprintf((char*)gdb_server_i.mem_buffer, GDB_PACKET_BUFF_SIZE,
                "RV-LINK ERROR: %s, @0x%08x\r\n", err_str, (unsigned int)err_pc);
        len = strlen((char*)gdb_server_i.mem_buffer);
        rsp.data[0] = 'O';
        bin_to_hex(gdb_server_i.mem_buffer, &rsp.data[1], len);
        rsp.len = len * 2 + 1;
        xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
        gdb_server_reply_ok();
    } else {
        bin_to_hex((uint8_t*)unspported_monitor_command, rsp.data, sizeof(unspported_monitor_command) - 1);
        rsp.len = (sizeof(unspported_monitor_command) - 1) * 2;
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
            gdb_server_i.tid_g = tid;
        } else if (ch == 'G') {
            gdb_server_i.tid_G = tid;
        } else if (ch == 'm') {
            gdb_server_i.tid_m = tid;
        } else if (ch == 'M') {
            gdb_server_i.tid_M = tid;
        } else {
            gdb_server_i.tid_c = tid;
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
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


/*
 * ‘g’
 * Read general registers.
 */
void gdb_server_cmd_g(void)
{
    int i;

    rv_target_read_core_registers(&gdb_server_i.regs[0]);

    for(i = 0; i < RVL_TARGET_CONFIG_REG_NUM; i++) {
        if (XLEN_RV32 == rv_target_xlen()) {
            uint32_to_hex_le(gdb_server_i.regs[i], &rsp.data[i * (XLEN_RV32 * 2)]);
        } else if (XLEN_RV64 == rv_target_xlen()) {
            uint64_to_hex_le(gdb_server_i.regs[i], &rsp.data[i * (XLEN_RV64 * 2)]);
        } else {
            //TODO:
            return;
        }
    }
    if (XLEN_RV32 == rv_target_xlen()) {
        rsp.len = XLEN_RV32 * 2 * RVL_TARGET_CONFIG_REG_NUM;
    } else if (XLEN_RV64 == rv_target_xlen()) {
        rsp.len = XLEN_RV64 * 2 * RVL_TARGET_CONFIG_REG_NUM;
    } else {
        //TODO:
        return;
    }
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
        if (XLEN_RV32 == rv_target_xlen()) {
            hex_to_uint32_le(&cmd.data[i * (XLEN_RV32 * 2) + 1], (uint32_t*)&gdb_server_i.regs[i]);
        } else if (XLEN_RV64 == rv_target_xlen()) {
            hex_to_uint64_le(&cmd.data[i * (XLEN_RV64 * 2) + 1], &gdb_server_i.regs[i]);
        }  else {
            //TODO:
            return;
        }
    }

    rv_target_write_core_registers(&gdb_server_i.regs[0]);

    gdb_server_reply_ok();
}


/*
 * ‘p n’
 * Read the value of register n; n is in hex.
 */
void gdb_server_cmd_p(void)
{
    sscanf(&cmd.data[1], "%x", &gdb_server_i.reg_tmp_num);

    rv_target_read_register(&gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);
    if (XLEN_RV32 == rv_target_xlen()) {
        uint32_to_hex_le(gdb_server_i.reg_tmp, &rsp.data[0]);
        rsp.len = XLEN_RV32 * 2;
    } else if (XLEN_RV64 == rv_target_xlen()) {
        uint64_to_hex_le(gdb_server_i.reg_tmp, &rsp.data[0]);
        rsp.len = XLEN_RV64 * 2;
    } else {
        //TODO:
        return;
    }
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

    sscanf(&cmd.data[1], "%x", &gdb_server_i.reg_tmp_num);
    p = strchr(&cmd.data[1], '=');
    p++;
    if (XLEN_RV32 == rv_target_xlen()) {
        hex_to_uint32_le(p, (uint32_t*)&gdb_server_i.reg_tmp);
    } else if (XLEN_RV64 == rv_target_xlen()) {
        hex_to_uint64_le(p, &gdb_server_i.reg_tmp);
    } else {
        //TODO:
        return;
    }
    rv_target_write_register(&gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);

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
    sscanf(&cmd.data[1], "%x", (unsigned int*)(&gdb_server_i.mem_addr));
    sscanf(p, "%x", (unsigned int*)(&gdb_server_i.mem_len));
    if (gdb_server_i.mem_len > sizeof(gdb_server_i.mem_buffer)) {
        gdb_server_i.mem_len = sizeof(gdb_server_i.mem_buffer);
    }

    rv_target_read_memory(gdb_server_i.mem_buffer, gdb_server_i.mem_addr, gdb_server_i.mem_len);

    bin_to_hex(gdb_server_i.mem_buffer, rsp.data, gdb_server_i.mem_len);
    rsp.len = gdb_server_i.mem_len * 2;
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
}


/*
 * ‘M addr,length:XX...’
 * Write length addressable memory units starting at address addr.
 */
void gdb_server_cmd_M(void)
{
    const char *p;

    sscanf(&cmd.data[1], "%x,%x", (unsigned int*)(&gdb_server_i.mem_addr), (unsigned int*)(&gdb_server_i.mem_len));
    p = strchr(&cmd.data[1], ':');
    p++;

    if (gdb_server_i.mem_len > sizeof(gdb_server_i.mem_buffer)) {
        gdb_server_i.mem_len = sizeof(gdb_server_i.mem_buffer);
    }

    hex_to_bin(p, gdb_server_i.mem_buffer, gdb_server_i.mem_len);
    rv_target_write_memory(gdb_server_i.mem_buffer, gdb_server_i.mem_addr, gdb_server_i.mem_len);

    gdb_server_reply_ok();
}


/*
 * ‘X addr,length:XX...’
 * Write data to memory, where the data is transmitted in binary.
 */
void gdb_server_cmd_X(void)
{
    const char *p;
    uint32_t length;

    sscanf(&cmd.data[1], "%x,%x", (unsigned int*)(&gdb_server_i.mem_addr), (unsigned int*)(&gdb_server_i.mem_len));
    if (gdb_server_i.mem_len == 0) {
        gdb_server_reply_ok();
        return;
    }

    p = strchr(&cmd.data[1], ':');
    p++;

    if (gdb_server_i.mem_len > sizeof(gdb_server_i.mem_buffer)) {
        gdb_server_i.mem_len = sizeof(gdb_server_i.mem_buffer);
    }

    length = cmd.len - ((uint32_t)p - (uint32_t)cmd.data);
    bin_decode((uint8_t*)p, gdb_server_i.mem_buffer, length);

    rv_target_write_memory(gdb_server_i.mem_buffer, gdb_server_i.mem_addr, gdb_server_i.mem_len);

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
    rv_target_resume();
    gdb_server_target_run(true);
}


/*
 * ‘s [addr]’
 * Single step, resuming at addr. If addr is omitted, resume at same address.
 */
void gdb_server_cmd_s(void)
{
    rv_target_step();
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
    gdb_server_i.breakpoint_type = type;
    gdb_server_i.breakpoint_addr = addr;
    gdb_server_i.breakpoint_kind = kind;

    rv_target_remove_breakpoint(
            gdb_server_i.breakpoint_type, gdb_server_i.breakpoint_addr, gdb_server_i.breakpoint_kind, &gdb_server_i.breakpoint_err);
    if (gdb_server_i.breakpoint_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(gdb_server_i.breakpoint_err);
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
    gdb_server_i.breakpoint_type = type;
    gdb_server_i.breakpoint_addr = addr;
    gdb_server_i.breakpoint_kind = kind;

    rv_target_insert_breakpoint(
            gdb_server_i.breakpoint_type, gdb_server_i.breakpoint_addr, gdb_server_i.breakpoint_kind, &gdb_server_i.breakpoint_err);
    if (gdb_server_i.breakpoint_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(gdb_server_i.breakpoint_err);
    }
}


/*
 * Ctrl+C
 */
void gdb_server_cmd_ctrl_c(void)
{
    rv_target_halt();
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
    gdb_server_i.mem_addr = addr;
    gdb_server_i.mem_len = length;

    rv_target_flash_erase(gdb_server_i.mem_addr, gdb_server_i.mem_len, &gdb_server_i.flash_err);
    if (gdb_server_i.flash_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(gdb_server_i.flash_err);
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
    uint32_t length;

    sscanf(&cmd.data[12], "%x", &addr);
    gdb_server_i.mem_addr = addr;

    p = strchr(&cmd.data[12], ':');
    p++;

    length = cmd.len - ((uint32_t)p - (uint32_t)cmd.data);
    gdb_server_i.mem_len = bin_decode((uint8_t*)p, gdb_server_i.mem_buffer, length);

    rv_target_flash_write(gdb_server_i.mem_addr, gdb_server_i.mem_len, gdb_server_i.mem_buffer, &gdb_server_i.flash_err);
    if (gdb_server_i.flash_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(gdb_server_i.flash_err);
        gdb_server_disconnected();
    }
}


/*
 * ‘vFlashDone’
 * Indicate to the stub that flash programming operation is finished.
 */
void gdb_server_cmd_vFlashDone(void)
{
    rv_target_flash_done();
    gdb_server_reply_ok();
}


/*
 * ‘vMustReplyEmpty’
 * RV-LINK uses an unexpected vMustReplyEmpty response to inform the error
 */
void gdb_server_cmd_vMustReplyEmpty(void)
{
    uint32_t len;
    char c;

    if (gdb_server_i.target_error == rv_target_error_none) {
        gdb_server_reply_empty();
    } else {
        gdb_server_disconnected();
    }
}


static void gdb_server_reply_ok(void)
{
    strncpy(rsp.data, "OK", GDB_PACKET_BUFF_SIZE);
    rsp.len = 2;
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
    gdb_server_i.target_error = rv_target_error_none;
    gdb_server_i.gdb_connected = true;
    gdb_server_target_run(false);

    rv_target_init();
    rv_target_init_post(&gdb_server_i.target_error);

    if (gdb_server_i.target_error == rv_target_error_none) {
        rv_target_halt();
        rv_target_init_after_halted(&gdb_server_i.target_error);
    }

    if (gdb_server_i.target_error != rv_target_error_none) {
        switch(gdb_server_i.target_error) {
        case rv_target_error_line:
            //gdb_resp_buf_puts("\nRV-LINK ERROR: the target is not connected!\n");
            break;
        case rv_target_error_compat:
            //gdb_resp_buf_puts("\nRV-LINK ERROR: the target is not supported, upgrade RV-LINK firmware!\n");
            break;
        case rv_target_error_debug_module:
            //gdb_resp_buf_puts("\nRV-LINK ERROR: something wrong with debug module!\n");
            break;
        case rv_target_error_protected:
            //gdb_resp_buf_puts("\nRV-LINK ERROR: the target under protected! disable protection then try again.\n");
            break;
        default:
            //gdb_resp_buf_puts("\nRV-LINK ERROR: unknown error!\n");
            break;
        }
    }
}


void gdb_server_disconnected(void)
{
    if (gdb_server_i.target_running == false) {
        if (gdb_server_i.target_error != rv_target_error_line) {
            rv_target_resume();
            gdb_server_target_run(true);
        }
    }

    if (gdb_server_i.target_error != rv_target_error_line) {
        rv_target_fini_pre();
    }
    rv_target_deinit();

    gdb_server_i.gdb_connected = false;
}


static void gdb_server_target_run(bool run)
{
    gdb_server_i.target_running = run;
}


static void bin_to_hex(const uint8_t *bin, char *hex, uint32_t nbyte)
{
    uint32_t i;
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


static void uint32_to_hex_le(uint32_t data, char *hex)
{
    uint8_t bytes[4];

    bytes[0] = data & 0xff;
    bytes[1] = (data >> 8) & 0xff;
    bytes[2] = (data >> 16) & 0xff;
    bytes[3] = (data >> 24) & 0xff;

    bin_to_hex(bytes, hex, 4);
}

static void uint64_to_hex_le(uint64_t data, char *hex)
{
    uint8_t bytes[8];

    bytes[0] = data & 0xff;
    bytes[1] = (data >> 8) & 0xff;
    bytes[2] = (data >> 16) & 0xff;
    bytes[3] = (data >> 24) & 0xff;
    bytes[4] = (data >> 32) & 0xff;
    bytes[5] = (data >> 40) & 0xff;
    bytes[6] = (data >> 48) & 0xff;
    bytes[7] = (data >> 56) & 0xff;

    bin_to_hex(bytes, hex, 8);
}

static void hex_to_bin(const char *hex, uint8_t *bin, uint32_t nbyte)
{
    uint32_t i;
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


static void hex_to_uint32_le(const char *hex, uint32_t *data)
{
    uint8_t bytes[4];

    hex_to_bin(hex, bytes, 4);

    *data = (uint32_t)bytes[0] | 
            ((uint32_t)bytes[1] << 8) | 
            ((uint32_t)bytes[2] << 16) | 
            ((uint32_t)bytes[3] << 24);
}

static void hex_to_uint64_le(const char *hex, uint64_t *data)
{
    uint8_t bytes[8];

    hex_to_bin(hex, bytes, 8);

    *data = (uint64_t)bytes[0] | 
            ((uint64_t)bytes[1] << 8) | 
            ((uint64_t)bytes[2] << 16) | 
            ((uint64_t)bytes[3] << 24) | 
            ((uint64_t)bytes[4] << 32) | 
            ((uint64_t)bytes[5] << 40) | 
            ((uint64_t)bytes[6] << 48) | 
            ((uint64_t)bytes[7] << 56);
}


static uint32_t bin_decode(const uint8_t* xbin, uint8_t* bin, uint32_t xbin_len)
{
    uint32_t bin_len = 0;
    uint32_t i;
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
