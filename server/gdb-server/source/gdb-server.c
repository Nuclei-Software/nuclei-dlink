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
#include "riscv-target.h"
#include "encoding.h"
#include "flash.h"

static gdb_packet_t cmd;
static gdb_packet_t rsp;

typedef int16_t gdb_server_tid_t;

typedef struct gdb_server_s
{
    bool target_running;
    bool gdb_connected;
    bool restore_reg_flag;
    rv_target_halt_info_t halt_info;
    rv_target_error_t target_error;

    uint64_t mem_addr;
    uint32_t mem_len;
    uint8_t mem_buffer[GDB_PACKET_BUFF_SIZE];
    uint32_t flash_err;
    uint32_t i;
    uint64_t regs[RV_TARGET_CONFIG_REG_NUM];
    uint64_t reg_tmp[4];
    uint32_t reg_tmp_num;

    rv_target_breakpoint_type_t breakpoint_type;
    uint64_t breakpoint_addr;
    uint32_t breakpoint_kind;
    uint32_t breakpoint_err;
} gdb_server_t;

static gdb_server_t gdb_server_i;

void gdb_server_cmd_ctrl_c(void);
void gdb_server_cmd_q(void);
void gdb_server_cmd_qRcmd(void);
void gdb_server_cmd_Q(void);
void gdb_server_cmd_g(void);
void gdb_server_cmd_G(void);
void gdb_server_cmd_k(void);
void gdb_server_cmd_c(void);
void gdb_server_cmd_m(void);
void gdb_server_cmd_M(void);
void gdb_server_cmd_x(void);
void gdb_server_cmd_X(void);
void gdb_server_cmd_p(void);
void gdb_server_cmd_P(void);
void gdb_server_cmd_s(void);
void gdb_server_cmd_z(void);
void gdb_server_cmd_Z(void);
void gdb_server_cmd_v(void);
void gdb_server_cmd_custom(void);
void gdb_server_cmd_custom_set(const char* data);
void gdb_server_cmd_custom_read(const char* data);
void gdb_server_cmd_custom_algorithm(const char* data);

void gdb_server_connected(void);
void gdb_server_disconnected(void);

static void gdb_server_target_run(bool run);
static void gdb_server_reply_ok(void);
static void gdb_server_reply_err(int err);
static void gdb_server_send_response(void);

static void bin_to_hex(const uint8_t *bin, char *hex, uint32_t nbyte);
static void hex_to_bin(const char *hex, uint8_t *bin, uint32_t nbyte);
static void uint32_to_hex_le(uint32_t data, char *hex);
static void uint64_to_hex_le(uint64_t data, char *hex);
static void hex_to_uint32_le(const char *hex, uint32_t *data);
static void hex_to_uint64_le(const char *hex, uint64_t *data);
static uint32_t bin_encode(uint8_t* xbin, uint8_t* bin, uint32_t bin_len);
static uint32_t bin_decode(const uint8_t* xbin, uint8_t* bin, uint32_t xbin_len);

void gdb_server_init(void)
{
    gdb_server_target_run(false);
    gdb_server_i.gdb_connected = false;
    gdb_set_no_ack_mode(false);
    rsp.data = &rsp_buffer[2];
}

void gdb_server_poll(void)
{
    char c;
    BaseType_t xReturned;
    uint32_t ret, len;

    for (;;) {
        if (gdb_server_i.gdb_connected && gdb_server_i.target_running) {
            xReturned = xQueueReceive(gdb_cmd_packet_xQueue, &cmd, (100 / portTICK_PERIOD_MS));
            if (xReturned == pdPASS) {
                if (*cmd.data == '\x03' && cmd.len == 1) {
                    gdb_server_cmd_ctrl_c();
                    gdb_server_target_run(false);
                    strncpy(rsp.data, "T02", GDB_PACKET_BUFF_SIZE);
                    rsp.len = 3;
                    gdb_server_send_response();
                }
            }

            if (gdb_server_i.target_running) {
                rv_target_halt_check(&gdb_server_i.halt_info);
                if (gdb_server_i.halt_info.reason != rv_target_halt_reason_running) {
                    gdb_server_target_run(false);
                    if (gdb_server_i.restore_reg_flag) {
                        /* Restore registers */
                        rv_target_write_core_registers(gdb_server_i.regs);
                        gdb_server_i.restore_reg_flag = false;
                    }
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
                    }
                    rsp.len = strlen(rsp.data);
                    gdb_server_send_response();
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
                } else if (c == 'x') {
                    gdb_server_cmd_x();
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
                } else if (c == '+') {
                    gdb_server_cmd_custom();
                }
            }
        }
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
 * ‘q name params...’
 * General query (‘q’) and set (‘Q’).
 */
void gdb_server_cmd_q(void)
{
    if (strncmp(cmd.data, "qRcmd,", 6) == 0) {
        gdb_server_cmd_qRcmd();
    }
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
        rv_target_halt();
        rv_target_init_after_halted(&gdb_server_i.target_error);
        gdb_server_reply_ok();
    } else {
        bin_to_hex((uint8_t*)unspported_monitor_command, rsp.data, sizeof(unspported_monitor_command) - 1);
        rsp.len = (sizeof(unspported_monitor_command) - 1) * 2;
        gdb_server_send_response();
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
    }
}

/*
 * ‘g’
 * Read general registers.
 */
void gdb_server_cmd_g(void)
{
    int i;

    rv_target_read_core_registers(gdb_server_i.regs);

    for(i = 0; i < RV_TARGET_CONFIG_REG_NUM; i++) {
        if (MXL_RV32 == rv_target_mxl()) {
            uint32_to_hex_le(*(((uint32_t*)gdb_server_i.regs) + i), &rsp.data[i * (MXL_RV32 * 8)]);
        } else if (MXL_RV64 == rv_target_mxl()) {
            uint64_to_hex_le(gdb_server_i.regs[i], &rsp.data[i * (MXL_RV64 * 8)]);
        }
    }
    if (MXL_RV32 == rv_target_mxl()) {
        rsp.len = MXL_RV32 * 8 * RV_TARGET_CONFIG_REG_NUM;
    } else if (MXL_RV64 == rv_target_mxl()) {
        rsp.len = MXL_RV64 * 8 * RV_TARGET_CONFIG_REG_NUM;
    }
    gdb_server_send_response();
}

/*
 * ‘G XX...’
 * Write general registers.
 */
void gdb_server_cmd_G(void)
{
    int i;

    for(i = 0; i < RV_TARGET_CONFIG_REG_NUM; i++) {
        if (MXL_RV32 == rv_target_mxl()) {
            hex_to_uint32_le(&cmd.data[i * (MXL_RV32 * 8) + 1], ((uint32_t*)gdb_server_i.regs) + i);
        } else if (MXL_RV64 == rv_target_mxl()) {
            hex_to_uint64_le(&cmd.data[i * (MXL_RV64 * 8) + 1], &gdb_server_i.regs[i]);
        }
    }

    rv_target_write_core_registers(gdb_server_i.regs);

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
    gdb_server_send_response();
}

/*
 * ‘M addr,length:XX...’
 * Write length addressable memory units starting at address addr.
 */
void gdb_server_cmd_M(void)
{
    const char *p;

    sscanf(&cmd.data[1], "%x,%x", &gdb_server_i.mem_addr, &gdb_server_i.mem_len);
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
 * ‘x addr,length’
 * Read length addressable memory units starting at address addr.
 * Note that addr may not be aligned to any particular boundary.
 */
void gdb_server_cmd_x(void)
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

    rsp.len = bin_encode(rsp.data, gdb_server_i.mem_buffer, gdb_server_i.mem_len);;
    gdb_server_send_response();
}

/*
 * ‘X addr,length:XX...’
 * Write data to memory, where the data is transmitted in binary.
 */
void gdb_server_cmd_X(void)
{
    const char *p;
    uint32_t length;

    sscanf(&cmd.data[1], "%x,%x", &gdb_server_i.mem_addr, &gdb_server_i.mem_len);
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
 * ‘p n’
 * Read the value of register n; n is in hex.
 */
void gdb_server_cmd_p(void)
{
    sscanf(&cmd.data[1], "%x", &gdb_server_i.reg_tmp_num);

    rv_target_read_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);
    if ((gdb_server_i.reg_tmp_num >= RV_REG_V0) && (gdb_server_i.reg_tmp_num <= RV_REG_V31)) {
        uint32_t data_bits = rv_target_vlenb() * 8;
        uint32_t xlen = rv_target_mxl() * 32;
        uint32_t debug_vl = (data_bits + xlen - 1) / xlen;
        rsp.len = 0;
        for (int i = 0; i < debug_vl; i++) {
            if (MXL_RV32 == rv_target_mxl()) {
                uint32_to_hex_le(*((uint32_t*)gdb_server_i.reg_tmp + i), rsp.data + rsp.len);
                rsp.len += MXL_RV32 * 8;
            } else if (MXL_RV64 == rv_target_mxl()) {
                uint64_to_hex_le(*((uint64_t*)gdb_server_i.reg_tmp + i), rsp.data + rsp.len);
                rsp.len += MXL_RV64 * 8;
            }
        }
    } else {
        if (MXL_RV32 == rv_target_mxl()) {
            uint32_to_hex_le(*((uint32_t*)gdb_server_i.reg_tmp), rsp.data);
            rsp.len = MXL_RV32 * 8;
        } else if (MXL_RV64 == rv_target_mxl()) {
            uint64_to_hex_le(*((uint64_t*)gdb_server_i.reg_tmp), rsp.data);
            rsp.len = MXL_RV64 * 8;
        }
    }
    gdb_server_send_response();
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
    if ((gdb_server_i.reg_tmp_num >= RV_REG_V0) && (gdb_server_i.reg_tmp_num <= RV_REG_V31)) {
        uint32_t data_bits = rv_target_vlenb() * 8;
        uint32_t xlen = rv_target_mxl() * 32;
        uint32_t debug_vl = (data_bits + xlen - 1) / xlen;
        for (int i = 0; i < debug_vl; i++) {
            if (MXL_RV32 == rv_target_mxl()) {
                hex_to_uint32_le(p, (uint32_t*)gdb_server_i.reg_tmp + i);
                p += MXL_RV32 * 8;
            } else if (MXL_RV64 == rv_target_mxl()) {
                hex_to_uint64_le(p, (uint64_t*)gdb_server_i.reg_tmp + i);
                p += MXL_RV64 * 8;
            }
        }
    } else {
        if (MXL_RV32 == rv_target_mxl()) {
            hex_to_uint32_le(p, (uint32_t*)gdb_server_i.reg_tmp);
        } else if (MXL_RV64 == rv_target_mxl()) {
            hex_to_uint64_le(p, (uint64_t*)gdb_server_i.reg_tmp);
        }
    }
    rv_target_write_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);

    gdb_server_reply_ok();
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
    sscanf(cmd.data, "z%x,%x,%x", &gdb_server_i.breakpoint_type, &gdb_server_i.breakpoint_addr, &gdb_server_i.breakpoint_kind);

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
    sscanf(cmd.data, "Z%x,%x,%x", &gdb_server_i.breakpoint_type, &gdb_server_i.breakpoint_addr, &gdb_server_i.breakpoint_kind);

    rv_target_insert_breakpoint(
            gdb_server_i.breakpoint_type, gdb_server_i.breakpoint_addr, gdb_server_i.breakpoint_kind, &gdb_server_i.breakpoint_err);
    if (gdb_server_i.breakpoint_err == 0) {
        gdb_server_reply_ok();
    } else {
        gdb_server_reply_err(gdb_server_i.breakpoint_err);
    }
}

/*
 * ‘v’
 * Packets starting with ‘v’ are identified by a multi-letter name.
 */
void gdb_server_cmd_v(void)
{
    const char *p;
    static uint32_t spi_base_addr;
    static uint32_t flash_block_size;
    uint32_t parameter[2];
    if (strncmp(cmd.data, "vFlashInit:", 11) == 0) {
        sscanf(cmd.data, "vFlashInit:%x,%x;", &spi_base_addr, &flash_block_size);
        flash_init(spi_base_addr);
    } else if (strncmp(cmd.data, "vFlashErase:", 12) == 0) {
        sscanf(cmd.data, "vFlashErase:%x,%x;", &parameter[0], &parameter[1]);
        flash_erase(spi_base_addr, parameter[0], parameter[0] + parameter[1]);
    } else if (strncmp(cmd.data, "vFlashWrite:", 12) == 0) {
        sscanf(cmd.data, "vFlashWrite:%x:", &parameter[0]);
        p = strchr(&cmd.data[12], ':');
        p++;
        parameter[1] = cmd.len - ((uint32_t)p - (uint32_t)cmd.data);
        gdb_server_i.mem_len = bin_decode((uint8_t*)p, gdb_server_i.mem_buffer, parameter[1]);

        flash_write(spi_base_addr, gdb_server_i.mem_buffer, parameter[0], gdb_server_i.mem_len);
    } else if (strncmp(cmd.data, "vFlashDone", 10) == 0) {
    }
    gdb_server_reply_ok();
}

/*
 * ‘+’
 * Packets starting with ‘+’ custom command.
 * Rsponse starting with '-' custom command
 */
void gdb_server_cmd_custom(void)
{
    const char *p;

    p = strchr(&cmd.data[1], ':') + 1;
    if (strncmp(p, "set", strlen("set")) == 0) {
        gdb_server_cmd_custom_set(p);
    } else if (strncmp(p, "read", strlen("read")) == 0) {
        gdb_server_cmd_custom_read(p);
    } else if (strncmp(p, "algorithm", strlen("algorithm")) == 0) {
        gdb_server_cmd_custom_algorithm(p);
    }
}

void gdb_server_cmd_custom_set(const char* data)
{
    const char *p;

    p = strchr(data, ':') + 1;
    if (strncmp(p, "protocol", strlen("protocol")) == 0) {
        p = strchr(p, ':') + 1;
        if (strncmp(p, "jtag", strlen("jtag")) == 0) {
            rv_target_set_protocol(TARGET_PROTOCOL_JTAG);
            strncpy(rsp.data, "-:set:protocol:jtag:OK;", 23);
            rsp.len = 23;
        } else if (strncmp(p, "cjtag", strlen("cjtag")) == 0) {
            rv_target_set_protocol(TARGET_PROTOCOL_CJTAG);
            strncpy(rsp.data, "-:set:protocol:cjtag:OK;", 24);
            rsp.len = 24;
        }
        gdb_server_send_response();
        gdb_server_connected();
    }
}

void gdb_server_cmd_custom_read(const char* data)
{
    const char *p;

    p = strchr(data, ':') + 1;
    if (strncmp(p, "misa", strlen("misa")) == 0) {
        strncpy(rsp.data, "-:read:misa:", 12);
        rsp.len = 12;
        uint32_t misa = rv_target_misa();
        sprintf(&rsp.data[rsp.len], "%08x;", misa);
        rsp.len += 9;
        gdb_server_send_response();
    } else if (strncmp(p, "vlenb", strlen("vlenb")) == 0) {
        strncpy(rsp.data, "-:read:vlenb:", 13);
        rsp.len = 13;
        uint64_t vlenb = rv_target_vlenb();
        sprintf(&rsp.data[rsp.len], "%016x;", vlenb);
        rsp.len += 17;
        gdb_server_send_response();
    }
}

void gdb_server_cmd_custom_algorithm(const char* data)
{
    const char *p;
    uint32_t loader_addr;
    uint32_t cs;
    uint32_t spi_base;
    uint32_t params1, params2, params3;

    p = strchr(data, ':') + 1;
    sscanf(p, "%x,%x,%x,%x,%x,%x;", &loader_addr, &cs, &spi_base, &params1, &params2, &params3);

    /* Save registers */
    rv_target_read_core_registers(gdb_server_i.regs);
    gdb_server_i.restore_reg_flag = true;
    /* Run algorithm */
    gdb_server_i.reg_tmp_num = RV_REG_PC;
    gdb_server_i.reg_tmp[0] = loader_addr;
    rv_target_write_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);
    gdb_server_i.reg_tmp_num = RV_REG_A0;
    gdb_server_i.reg_tmp[0] = cs;
    rv_target_write_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);
    gdb_server_i.reg_tmp_num = RV_REG_A1;
    gdb_server_i.reg_tmp[0] = spi_base;
    rv_target_write_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);
    gdb_server_i.reg_tmp_num = RV_REG_A2;
    gdb_server_i.reg_tmp[0] = params1;
    rv_target_write_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);
    gdb_server_i.reg_tmp_num = RV_REG_A3;
    gdb_server_i.reg_tmp[0] = params2;
    rv_target_write_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);
    gdb_server_i.reg_tmp_num = RV_REG_A4;
    gdb_server_i.reg_tmp[0] = params3;
    rv_target_write_register(gdb_server_i.reg_tmp, gdb_server_i.reg_tmp_num);

    strncpy(rsp.data, "-:algorithm:OK;", 15);
    rsp.len = 15;
    gdb_server_send_response();
}

void gdb_server_connected(void)
{
    gdb_server_i.target_error = rv_target_error_none;
    gdb_server_i.gdb_connected = true;
    gdb_server_target_run(false);
    gdb_set_no_ack_mode(false);
    gdb_server_i.restore_reg_flag = false;

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

static void gdb_server_reply_ok(void)
{
    strncpy(rsp.data, "OK", GDB_PACKET_BUFF_SIZE);
    rsp.len = 2;
    gdb_server_send_response();
}

static void gdb_server_reply_err(int err)
{
    snprintf(rsp.data, GDB_PACKET_BUFF_SIZE, "E%02x", err);
    rsp.len = strlen(rsp.data);
    gdb_server_send_response();
}

static void gdb_server_send_response(void)
{
    xQueueSend(gdb_rsp_packet_xQueue, &rsp, portMAX_DELAY);
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

static uint32_t bin_encode(uint8_t* xbin, uint8_t* bin, uint32_t bin_len)
{
    uint32_t xbin_len = 0;
    uint32_t i;

    for(i = 0; i < bin_len; i++) {
        if ((bin[i] == '#') || (bin[i] == '$') || (bin[i] == '}') || (bin[i] == '*')) {
            xbin[xbin_len++] = 0x7d;
            xbin[xbin_len++] = bin[i] ^ 0x20;
        } else {
            xbin[xbin_len++] = bin[i];
        }
        if (xbin_len >= bin_len) {
            break;
        }
    }

    return xbin_len;
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
