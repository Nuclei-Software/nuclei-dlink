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
#include "riscv-target.h"
#include "riscv-tap.h"
#include "config.h"

typedef struct {
    rv_dm_t dm;
    rv_tr32_t tr32;
    rv_tr64_t tr64;
    rv_dtm_t dtm;
    rv_dmi_t dmi;
    uint32_t xlen;
    uint32_t flen;
    rv_target_interface_t interface;
} rv_target_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t prv: 2;
        uint32_t step: 1;
        uint32_t nmip: 1;
        uint32_t mprven: 1;
        uint32_t reserved5: 1;
        uint32_t cause: 3;
        uint32_t stoptime: 1;
        uint32_t stopcount: 1;
        uint32_t stepie: 1;
        uint32_t ebreaku: 1;
        uint32_t ebreaks: 1;
        uint32_t reserved14: 1;
        uint32_t ebreakm: 1;
        uint32_t reserved16: 12;
        uint32_t xdebugver: 4;
    };
} rv_dcsr_t;

typedef union {
    uint32_t value;
    struct {
        uint32_t a: 1;                           /*!< bit:     0  Atomic extension */
        uint32_t b: 1;                           /*!< bit:     1  Tentatively reserved for Bit-Manipulation extension */
        uint32_t c: 1;                           /*!< bit:     2  Compressed extension */
        uint32_t d: 1;                           /*!< bit:     3  Double-precision floating-point extension */
        uint32_t e: 1;                           /*!< bit:     4  RV32E base ISA */
        uint32_t f: 1;                           /*!< bit:     5  Single-precision floating-point extension */
        uint32_t g: 1;                           /*!< bit:     6  Additional standard extensions present */
        uint32_t h: 1;                           /*!< bit:     7  Hypervisor extension */
        uint32_t i: 1;                           /*!< bit:     8  RV32I/64I/128I base ISA */
        uint32_t j: 1;                           /*!< bit:     9  Tentatively reserved for Dynamically Translated Languages extension */
        uint32_t reserved10: 1;                  /*!< bit:     10 Reserved  */
        uint32_t l: 1;                           /*!< bit:     11 Tentatively reserved for Decimal Floating-Point extension  */
        uint32_t m: 1;                           /*!< bit:     12 Integer Multiply/Divide extension */
        uint32_t n: 1;                           /*!< bit:     13 User-level interrupts supported  */
        uint32_t reserved14: 1;                  /*!< bit:     14 Reserved  */
        uint32_t p: 1;                           /*!< bit:     15 Tentatively reserved for Packed-SIMD extension  */
        uint32_t q: 1;                           /*!< bit:     16 Quad-precision floating-point extension  */
        uint32_t resreved17: 1;                  /*!< bit:     17 Reserved  */
        uint32_t s: 1;                           /*!< bit:     18 Supervisor mode implemented  */
        uint32_t t: 1;                           /*!< bit:     19 Tentatively reserved for Transactional Memory extension  */
        uint32_t u: 1;                           /*!< bit:     20 User mode implemented  */
        uint32_t v: 1;                           /*!< bit:     21 Tentatively reserved for Vector extension  */
        uint32_t reserved22: 1;                  /*!< bit:     22 Reserved  */
        uint32_t x: 1;                           /*!< bit:     23 Non-standard extensions present  */
        uint32_t reserved24: 6;                  /*!< bit:     24..29 Reserved  */
        uint32_t mxl: 2;                         /*!< bit:     30..31 Machine XLEN  */
    };
} rv_misa_rv32_t;

typedef union {
    uint64_t value;
    struct {
        uint64_t a: 1;                           /*!< bit:     0  Atomic extension */
        uint64_t b: 1;                           /*!< bit:     1  Tentatively reserved for Bit-Manipulation extension */
        uint64_t c: 1;                           /*!< bit:     2  Compressed extension */
        uint64_t d: 1;                           /*!< bit:     3  Double-precision floating-point extension */
        uint64_t e: 1;                           /*!< bit:     4  RV32E base ISA */
        uint64_t f: 1;                           /*!< bit:     5  Single-precision floating-point extension */
        uint64_t g: 1;                           /*!< bit:     6  Additional standard extensions present */
        uint64_t h: 1;                           /*!< bit:     7  Hypervisor extension */
        uint64_t i: 1;                           /*!< bit:     8  RV32I/64I/128I base ISA */
        uint64_t j: 1;                           /*!< bit:     9  Tentatively reserved for Dynamically Translated Languages extension */
        uint64_t reserved10: 1;                  /*!< bit:     10 Reserved  */
        uint64_t l: 1;                           /*!< bit:     11 Tentatively reserved for Decimal Floating-Point extension  */
        uint64_t m: 1;                           /*!< bit:     12 Integer Multiply/Divide extension */
        uint64_t n: 1;                           /*!< bit:     13 User-level interrupts supported  */
        uint64_t reserved14: 1;                  /*!< bit:     14 Reserved  */
        uint64_t p: 1;                           /*!< bit:     15 Tentatively reserved for Packed-SIMD extension  */
        uint64_t q: 1;                           /*!< bit:     16 Quad-precision floating-point extension  */
        uint64_t resreved17: 1;                  /*!< bit:     17 Reserved  */
        uint64_t s: 1;                           /*!< bit:     18 Supervisor mode implemented  */
        uint64_t t: 1;                           /*!< bit:     19 Tentatively reserved for Transactional Memory extension  */
        uint64_t u: 1;                           /*!< bit:     20 User mode implemented  */
        uint64_t v: 1;                           /*!< bit:     21 Tentatively reserved for Vector extension  */
        uint64_t reserved22: 1;                  /*!< bit:     22 Reserved  */
        uint64_t x: 1;                           /*!< bit:     23 Non-standard extensions present  */
        uint64_t reserved24: 38;                 /*!< bit:     24..61 Reserved  */
        uint64_t mxl: 2;                         /*!< bit:     62..63 Machine XLEN  */
    };
} rv_misa_rv64_t;

static rv_target_t target;
rv_dcsr_t dcsr;
uint64_t dpc;

const uint32_t zero = 0; 
uint32_t result;
bool err_flag;
const char *err_msg;
uint32_t err_pc;
rv_hardware_breakpoint_t hardware_breakpoints[RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM];
rv_software_breakpoint_t software_breakpoints[RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM];

const char *rv_abstractcs_cmderr_str[8] = {
    "abs:0 (none)",
    "abs:1 (busy)",
    "abs:2 (not supported)",
    "abs:3 (exception)",
    "abs:4 (halt/resume)",
    "abs:5 (bus)",
    "abs:6 (FIXME)",
    "abs:7 (other)",
};

static void rv_set_error(const char *str)
{
    uint32_t pc;
    asm volatile (
            "mv %0, ra\n"
            :"=r"(pc)
            );
    err_flag = true;
    err_pc = pc;
    err_msg = str;
}

static void rv_dtm_sync(uint32_t reg)
{
    static uint32_t last_reg;
    uint32_t temp_in[2];
    uint32_t temp_out[2];

    if (reg != last_reg) {
        temp_in[0] = reg;
        rv_tap_shift_ir(temp_out, temp_in, 5);
    }
    last_reg = reg;
    
    temp_in[0] = target.dmi.data;
    switch (reg) {
        case RV_DTM_JTAG_REG_IDCODE:
            rv_tap_shift_dr(temp_out, temp_in, 32);
            target.dtm.idcode.value = temp_out[0];
            break;
        case RV_DTM_JTAG_REG_DTMCS:
            rv_tap_shift_dr(temp_out, temp_in, 32);
            target.dtm.dtmcs.value = temp_out[0];
            break;
        case RV_DTM_JTAG_REG_DMI:
            temp_in[0] = (target.dmi.data << 2) | (target.dmi.op & 0x3);
            temp_in[1] = (target.dmi.data >> 30) | (target.dmi.address << 2);
            rv_tap_shift_dr(temp_out, temp_in, 32 + 2 + target.dtm.dtmcs.abits);
            target.dmi.op = temp_out[0] & 0x3;
            target.dmi.data = (temp_out[0] >> 2) | (temp_out[1] << 30);
            break;
        default:
            break;
    }
    if (target.dtm.dtmcs.idle) {
        rv_tap_idle(target.dtm.dtmcs.idle);
    }
}

static void rv_dmi_read(uint32_t addr, uint32_t *out)
{
    uint32_t i;

    target.dmi.op = RV_DMI_OP_READ;
    target.dmi.data = 0x00;
    target.dmi.address = addr;
    rv_dtm_sync(RV_DTM_JTAG_REG_DMI);

    for (i = 0; i < RV_TARGET_CONFIG_DMI_RETRIES; i++) {
        target.dmi.op = RV_DMI_OP_NOP;
        target.dmi.data = 0x00;
        target.dmi.address = 0x00;
        rv_dtm_sync(RV_DTM_JTAG_REG_DMI);
        if(target.dmi.op == RV_DMI_RESULT_BUSY) {
            rv_tap_idle(32);
        } else {
            break;
        }
    }

    if(target.dmi.op != RV_DMI_RESULT_DONE) {
        target.dtm.dtmcs.dmireset = 1;
        target.dmi.data = target.dtm.dtmcs.value;
        rv_dtm_sync(RV_DTM_JTAG_REG_DTMCS);
    } else {
        *out = target.dmi.data;
    }

    result = target.dmi.op;
}

static void rv_dmi_write(uint32_t addr, uint32_t in)
{
    uint32_t i;

    target.dmi.op = RV_DMI_OP_WRITE;
    target.dmi.data = in;
    target.dmi.address = addr;
    rv_dtm_sync(RV_DTM_JTAG_REG_DMI);

    for (i = 0; i < RV_TARGET_CONFIG_DMI_RETRIES; i++) {
        target.dmi.op = RV_DMI_OP_NOP;
        target.dmi.data = 0x00;
        target.dmi.address = 0x00;
        rv_dtm_sync(RV_DTM_JTAG_REG_DMI);
        if(target.dmi.op == RV_DMI_RESULT_BUSY) {
            rv_tap_idle(32);
        } else {
            break;
        }
    }

    if(target.dmi.op != RV_DMI_RESULT_DONE) {
        target.dtm.dtmcs.dmireset = 1;
        target.dmi.data = target.dtm.dtmcs.value;
        rv_dtm_sync(RV_DTM_JTAG_REG_DTMCS);
    }

    result = target.dmi.op;
}

static void rv_register_read(uint32_t *reg, uint32_t regno)
{
    uint32_t xlen;
    if (regno == CSR_DCSR) {
        xlen = XLEN_RV32;
    } else {
        xlen = target.xlen;
    }
    err_flag = false;
    target.dm.command.value = 0;
    target.dm.command.reg.cmdtype = RV_DM_ABSTRACT_CMD_ACCESS_REG;
    if (XLEN_RV32 == xlen) {
        target.dm.command.reg.aarsize = 2;
    } else if (XLEN_RV64 == xlen) {
        target.dm.command.reg.aarsize = 3;
    } else {
        //TODO:
        return;
    }
    target.dm.command.reg.transfer = 1;
    target.dm.command.reg.regno = regno;
    rv_dmi_write(RV_DM_ABSTRACT_COMMAND, target.dm.command.value);

    rv_dmi_read(RV_DM_ABSTRACT_CONTROL_AND_STATUS, &target.dm.abstractcs.value);
    if (target.dm.abstractcs.cmderr) {
        rv_set_error(rv_abstractcs_cmderr_str[target.dm.abstractcs.cmderr]);
        target.dm.abstractcs.value = 0;
        target.dm.abstractcs.cmderr = 0x7;
        rv_dmi_write(RV_DM_ABSTRACT_CONTROL_AND_STATUS, target.dm.abstractcs.value);
        *reg = 0xffffffff;
    } else {
        if (XLEN_RV32 == xlen) {
            rv_dmi_read(RV_DM_ABSTRACT_DATA0, &target.dm.data[0]);
            reg[0] = target.dm.data[0];
        } else if (XLEN_RV64 == xlen) {
            rv_dmi_read(RV_DM_ABSTRACT_DATA0, &target.dm.data[0]);
            reg[0] = target.dm.data[0];
            rv_dmi_read(RV_DM_ABSTRACT_DATA1, &target.dm.data[1]);
            reg[1] = target.dm.data[1];
        } else {
            //TODO:
            return;
        }
    }
}

static void rv_register_write(uint32_t *reg, uint32_t regno)
{
    uint32_t xlen;
    if (regno == CSR_DCSR) {
        xlen = XLEN_RV32;
    } else {
        xlen = target.xlen;
    }
    err_flag = false;
    if (XLEN_RV32 == xlen) {
        target.dm.data[0] = reg[0];
        rv_dmi_write(RV_DM_ABSTRACT_DATA0, target.dm.data[0]);
    } else if (XLEN_RV64 == xlen) {
        target.dm.data[0] = reg[0];
        rv_dmi_write(RV_DM_ABSTRACT_DATA0, target.dm.data[0]);
        target.dm.data[1] = reg[1];
        rv_dmi_write(RV_DM_ABSTRACT_DATA1, target.dm.data[1]);
    } else {
        //TODO:
        return;
    }

    target.dm.command.value = 0;
    target.dm.command.reg.cmdtype = RV_DM_ABSTRACT_CMD_ACCESS_REG;
    if (XLEN_RV32 == xlen) {
        target.dm.command.reg.aarsize = 2;
    } else if (XLEN_RV64 == xlen) {
        target.dm.command.reg.aarsize = 3;
    } else {
        //TODO:
        return;
    }
    target.dm.command.reg.write = 1;
    target.dm.command.reg.transfer = 1;
    target.dm.command.reg.regno = regno;
    rv_dmi_write(RV_DM_ABSTRACT_COMMAND, target.dm.command.value);

    rv_dmi_read(RV_DM_ABSTRACT_CONTROL_AND_STATUS, &target.dm.abstractcs.value);
    if (target.dm.abstractcs.cmderr) {
        rv_set_error(rv_abstractcs_cmderr_str[target.dm.abstractcs.cmderr]);
        target.dm.abstractcs.value = 0;
        target.dm.abstractcs.cmderr = 0x7;
        rv_dmi_write(RV_DM_ABSTRACT_CONTROL_AND_STATUS, target.dm.abstractcs.value);
    }
}

static void rv_memory_read(uint8_t *mem, uint64_t addr, uint32_t len, uint32_t aamsize)
{
    uint32_t i;
    uint8_t *pbyte;
    uint16_t *phalfword;
    uint32_t *pword;

    err_flag = false;

    for (i = 0; i < len; i++) {
        if (XLEN_RV32 == target.xlen) {
            target.dm.data[1] = addr + (i << aamsize);
            rv_dmi_write(RV_DM_ABSTRACT_DATA1, target.dm.data[1]);
        } else if (XLEN_RV64 == target.xlen) {
            target.dm.data[2] = addr + (i << aamsize);
            rv_dmi_write(RV_DM_ABSTRACT_DATA2, target.dm.data[2]);
            target.dm.data[3] = addr >> 32;
            rv_dmi_write(RV_DM_ABSTRACT_DATA3, target.dm.data[3]);
        }

        target.dm.command.value = 0;
        target.dm.command.mem.cmdtype = RV_DM_ABSTRACT_CMD_ACCESS_MEM;
        target.dm.command.mem.aamsize = aamsize;
        rv_dmi_write(RV_DM_ABSTRACT_COMMAND, target.dm.command.value);

        rv_dmi_read(RV_DM_ABSTRACT_CONTROL_AND_STATUS, &target.dm.abstractcs.value);
        if (target.dm.abstractcs.cmderr) {
            rv_set_error(rv_abstractcs_cmderr_str[target.dm.abstractcs.cmderr]);
            target.dm.abstractcs.value = 0;
            target.dm.abstractcs.cmderr = 0x7;
            rv_dmi_write(RV_DM_ABSTRACT_CONTROL_AND_STATUS, target.dm.abstractcs.value);
            target.dm.data[0] = 0xffffffff;
            break;
        } else {
            rv_dmi_read(RV_DM_ABSTRACT_DATA0, &target.dm.data[0]);
        }

        switch(aamsize) {
            case 0:
                pbyte = (uint8_t*)mem;
                pbyte[i] = target.dm.data[0] & 0xff;
                break;
            case 1:
                phalfword = (uint16_t*)mem;
                phalfword[i] = target.dm.data[0] & 0xffff;
                break;
            case 2:
                pword = (uint32_t*)mem;
                pword[i] = target.dm.data[0];
                break;
            default:
                break;
        }
    }
}

static void rv_memory_write(const uint8_t* mem, uint64_t addr, uint32_t len, uint32_t aamsize)
{
    uint32_t i;
    const uint8_t* pbyte;
    const uint16_t* phalfword;
    const uint32_t* pword;

    err_flag = false;

    for (i = 0; i < len; i++) {
        if (XLEN_RV32 == target.xlen) {
            target.dm.data[1] = addr + (i << aamsize);
            rv_dmi_write(RV_DM_ABSTRACT_DATA1, target.dm.data[1]);
        } else if (XLEN_RV64 == target.xlen) {
            target.dm.data[2] = addr + (i << aamsize);
            rv_dmi_write(RV_DM_ABSTRACT_DATA2, target.dm.data[2]);
            target.dm.data[3] = addr >> 32;
            rv_dmi_write(RV_DM_ABSTRACT_DATA3, target.dm.data[3]);
        }

        switch(aamsize) {
            case RV_AAMSIZE_8BITS:
                pbyte = (const uint8_t*)mem;
                target.dm.data[0] = pbyte[i];
                break;
            case RV_AAMSIZE_16BITS:
                phalfword = (const uint16_t*)mem;
                target.dm.data[0] = phalfword[i];
                break;
            case RV_AAMSIZE_32BITS:
                pword = (const uint32_t*)mem;
                target.dm.data[0] = pword[i];
                break;
            default:
                break;
        }
        rv_dmi_write(RV_DM_ABSTRACT_DATA0, target.dm.data[0]);

        target.dm.command.value = 0;
        target.dm.command.mem.cmdtype = RV_DM_ABSTRACT_CMD_ACCESS_MEM;
        target.dm.command.mem.aamsize = aamsize;
        target.dm.command.mem.write = 1;
        rv_dmi_write(RV_DM_ABSTRACT_COMMAND, target.dm.command.value);

        rv_dmi_read(RV_DM_ABSTRACT_CONTROL_AND_STATUS, &target.dm.abstractcs.value);
        if (target.dm.abstractcs.cmderr) {
            rv_set_error(rv_abstractcs_cmderr_str[target.dm.abstractcs.cmderr]);
            target.dm.abstractcs.value = 0;
            target.dm.abstractcs.cmderr = 0x7;
            rv_dmi_write(RV_DM_ABSTRACT_CONTROL_AND_STATUS, target.dm.abstractcs.value);
            break;
        }
    }
}

static void rv_parse_watchpoint_inst(uint32_t inst, uint32_t* regno, uint32_t* offset)
{
    uint32_t opcode;
    int32_t offset_s;
    if ((inst & 0x3) == 0x3) {
        // 32 bits Instruction
        opcode = inst & 0x7f;
        switch(opcode) {
        case 0x03: // LOAD
        case 0x07: // LOAD-FP
            *regno = (inst >> 15) & 0x1f;
            offset_s = inst & 0xfff00000;
            offset_s = offset_s >> 20;
            break;
        case 0x23: // STORE
        case 0x27: // STORE-FP
            *regno = (inst >> 15) & 0x1f;
            offset_s = (inst & 0xfe000000) | ((inst << 13) & 0x01f00000);
            offset_s = offset_s >> 20;
            break;
        default:
            *regno = 0;
            offset_s = 0;
            break;
        }
    } else {
        // 16 bits Instruction, RV32 和 RV64 是不一样的，当前仅考虑 RV32
        opcode = ((inst & 0xe000) >> 11) | (inst & 0x3);
        switch(opcode) {
        case 0x4: // FLD
        case 0x8: // LW
        case 0xc: // FLW, LD
        case 0x14: // FSD, SQ
        case 0x18: // SW
        case 0x1c: // FSW, SD
            *regno = ((inst >> 7) & 0x7) + 8;
            break;

        case 0x6: // FLDSP, LQSP
        case 0xa: // LWSP
        case 0xe: // FLWSP, LDSP
        case 0x16: // FSDSP, SQSP
        case 0x1a: // SWSP
        case 0x1e: // FSWSP, SDSP
            *regno = 2;
            break;

        default:
            *regno = 0;
            break;
        }

        switch(opcode) {
        case 0x4: // FLD
        case 0x14: // FSD
            offset_s = ((inst & 0x1c00) >> 7) | ((inst & 0x60) << 1);
            break;

        case 0x8: // LW
        case 0xc: // FLW
        case 0x18: // SW
        case 0x1c: // FSW
            offset_s = ((inst & 0x1c00) >> 7) | ((inst & 0x40) >> 4) | ((inst & 0x20) << 1);
            break;

        case 0x6: // FLDSP
            offset_s = ((inst & 0x1000) >> 7) | ((inst & 0x60) >> 2) | ((inst & 0x1c) << 4);
            break;

        case 0xa: // LWSP
        case 0xe: // FLWSP
            offset_s = ((inst & 0x1000) >> 7) | ((inst & 0x70) >> 2) | ((inst & 0xc) << 4);
            break;

        case 0x16: // FSDSP
            offset_s = ((inst & 0x1c00) >> 7) | ((inst & 0x380) >> 1);
            break;

        case 0x1a: // SWSP
        case 0x1e: // FSWSP
            offset_s = ((inst & 0x1e00) >> 7) | ((inst & 0x180) >> 1);
            break;

        default:
            offset_s = 0;
            break;
        }
    }

    *offset = offset_s;
}

void rv_target_get_error(const char **str, uint32_t* pc)
{
    *str = err_msg;
    *pc = err_pc;
}

void rv_target_init(void)
{
    uint32_t i;

    for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
        hardware_breakpoints[i].type = rv_target_breakpoint_type_unused;
    }

    for(i = 0; i < RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM; i++) {
        software_breakpoints[i].type = rv_target_breakpoint_type_unused;
    }

    err_flag = false;
    err_pc = 0;
    err_msg = "no error";
    target.xlen = 0;
    target.flen = 0;

    rv_tap_init();
}

void rv_target_deinit(void)
{
    rv_tap_deinit();
}

uint32_t rv_target_xlen(void)
{
    return target.xlen;
}

uint32_t rv_target_flen(void)
{
    return target.flen;
}

void rv_target_set_interface(rv_target_interface_t interface)
{
    target.interface = interface;
}

void rv_target_init_post(rv_target_error_t *err)
{
    rv_tap_reset(32);

    if (TARGET_INTERFACE_CJTAG == target.interface) {
        rv_tap_oscan1_mode();
    }

    target.dtm.idcode.value = 0;
    target.dmi.data = target.dtm.idcode.value;
    rv_dtm_sync(RV_DTM_JTAG_REG_IDCODE);
    if (!target.dtm.idcode.reserved0) {
        *err = rv_target_error_line;
        return;
    }

    target.dtm.dtmcs.value = 0;
    target.dmi.data = target.dtm.dtmcs.value;
    rv_dtm_sync(RV_DTM_JTAG_REG_DTMCS);
    if (0 == target.dtm.dtmcs.version) {
        *err = rv_target_error_debug_module;
        return;
    }

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.dmactive = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
    if (result != RV_DMI_RESULT_DONE) {
        *err = rv_target_error_debug_module;
        return;
    }
    rv_dmi_read(RV_DM_DEBUG_MODULE_CONTROL, &target.dm.dmcontrol.value);
    if (result != RV_DMI_RESULT_DONE) {
        *err = rv_target_error_debug_module;
        return;
    }
    if (target.dm.dmcontrol.dmactive != 1) {
        *err = rv_target_error_debug_module;
        return;
    }

    return;
}

void rv_target_init_after_halted(rv_target_error_t *err)
{
    uint32_t i;

    /* get xlen flen */
    rv_misa_rv32_t misa32;
    rv_misa_rv64_t misa64;
    target.xlen = XLEN_RV64;
    rv_register_read((uint32_t*)&misa64, CSR_MISA);
    if (err_flag) {
        target.xlen = XLEN_RV32;
        rv_register_read((uint32_t*)&misa32, CSR_MISA);
        if (err_flag) {
            *err = rv_target_error_debug_module;
            return;
        }
        if (0x1 != misa32.mxl) {
            target.xlen = 0;
            *err = rv_target_error_debug_module;
            return;
        }
        target.flen = FLEN_SINGLE;
        if (misa32.d) {
            target.flen = FLEN_DOUBLE;
        }
    } else {
        if (0x2 != misa64.mxl) {
            target.xlen = 0;
            *err = rv_target_error_debug_module;
            return;
        }
        target.flen = FLEN_SINGLE;
        if (misa64.d) {
            target.flen = FLEN_DOUBLE;
        }
    }

    rv_register_read(&dcsr.value, CSR_DCSR);
    if (dcsr.xdebugver != 4) {
        *err = rv_target_error_compat;
        return;
    }

    /*
     * ebreak instructions in X-mode enter Debug Mode.
     */
    dcsr.ebreakm = 1;
    dcsr.ebreaks = 1;
    dcsr.ebreaku = 1;
    rv_register_write(&dcsr.value, CSR_DCSR);

    /*
     * clear all hardware breakpoints
     */
    for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
        rv_register_write(&i, CSR_TSELECT);
        rv_register_write((uint32_t*)&zero, CSR_TDATA1);
    }
}

void rv_target_fini_pre(void)
{
    /*
     * ebreak instructions in X-mode behave as described in the Privileged Spec.
     */
    rv_register_read(&dcsr.value, CSR_DCSR);
    dcsr.ebreakm = 0;
    dcsr.ebreaks = 0;
    dcsr.ebreaku = 0;
    rv_register_write(&dcsr.value, CSR_DCSR);

    /*
     * Disable debug module
     */
    target.dm.dmcontrol.value = 0;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
}

void rv_target_read_core_registers(void *regs)
{
    uint32_t i;

    for(i = 1; i < 32; i++) {
        if (XLEN_RV32 == target.xlen) {
            rv_register_read((uint32_t*)regs + i, i + 0x1000);
        } else if (XLEN_RV64 == target.xlen) {
            rv_register_read((uint32_t*)((uint64_t*)regs + i), i + 0x1000);
        }
    }
    if (XLEN_RV32 == target.xlen) {
        rv_register_read((uint32_t*)regs + 32, CSR_DPC);
    } else if (XLEN_RV64 == target.xlen) {
        rv_register_read((uint32_t*)((uint64_t*)regs + 32), CSR_DPC);
    }
}

void rv_target_write_core_registers(void *regs)
{
    uint32_t i;

    for(i = 1; i < 32; i++) {
        if (XLEN_RV32 == target.xlen) {
            rv_register_write((uint32_t*)regs + i, i + 0x1000);
        } else if (XLEN_RV64 == target.xlen) {
            rv_register_write((uint32_t*)((uint64_t*)regs + i), i + 0x1000);
        }
    }
    if (XLEN_RV32 == target.xlen) {
        rv_register_write((uint32_t*)regs + 32, CSR_DPC);
    } else if (XLEN_RV64 == target.xlen) {
        rv_register_write((uint32_t*)((uint64_t*)regs + 32), CSR_DPC);
    }
}

void rv_target_read_register(void *reg, uint32_t regno)
{
    if (regno <= 31) { // GPRs
        rv_register_read((uint32_t*)reg, regno + 0x1000);
    } else if (regno == 32) { // PC
        rv_register_read((uint32_t*)reg, CSR_DPC);
    } else if (regno <= 64) { // FPRs
        rv_register_read((uint32_t*)reg, regno + 0x1000 - 1);
    } else if (regno <= (0x1000 + 64)) { // CSRs
        rv_register_read((uint32_t*)reg, regno - 65);
    } else if (regno == 4161) {  // priv
        rv_register_read(&dcsr.value, CSR_DCSR);
        *(uint32_t*)reg = dcsr.prv;
    } else {
        *(uint32_t*)reg = 0xffffffff;
    }
}

void rv_target_write_register(void *reg, uint32_t regno)
{
    if (regno <= 31) { // GPRs
        rv_register_write((uint32_t*)reg, regno + 0x1000);
    } else if (regno == 32) { // PC
        rv_register_write((uint32_t*)reg, CSR_DPC);
    } else if (regno <= 64) { // FPRs
        rv_register_write((uint32_t*)reg, regno + 0x1000 - 1);
    } else if (regno <= (0x1000 + 64)) { // CSRs
        rv_register_write((uint32_t*)reg, regno - 65);
    } else if (regno == 4161) {  // priv
        rv_register_read(&dcsr.value, CSR_DCSR);
        dcsr.prv = *(uint32_t*)reg;
        rv_register_write(&dcsr.value, CSR_DCSR);
    }
}

void rv_target_read_memory(uint8_t* mem, uint64_t addr, uint32_t len)
{
    if (((uint32_t)mem & 3) == 0 && (addr & 3) == 0 && (len & 3) == 0) {
        rv_memory_read(mem, addr, len / 4, RV_AAMSIZE_32BITS);
    } else if (((uint32_t)mem & 1) == 0 && (addr & 1) == 0 && (len & 1) == 0) {
        rv_memory_read(mem, addr, len / 2, RV_AAMSIZE_16BITS);
    } else {
        rv_memory_read(mem, addr, len, RV_AAMSIZE_8BITS);
    }
}

void rv_target_write_memory(const uint8_t* mem, uint64_t addr, uint32_t len)
{
    if (((uint32_t)mem & 3) == 0 && (addr & 3) == 0 && (len & 3) == 0) {
        rv_memory_write(mem, addr, len / 4, RV_AAMSIZE_32BITS);
    } else if (((uint32_t)mem & 1) == 0 && (addr & 1) == 0 && (len & 1) == 0) {
        rv_memory_write(mem, addr, len / 2, RV_AAMSIZE_16BITS);
    } else {
        rv_memory_write(mem, addr, len, RV_AAMSIZE_8BITS);
    }
}

void rv_target_reset(void)
{
    uint32_t i;

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.dmactive = 1;
    target.dm.dmcontrol.haltreq = 1;
    target.dm.dmcontrol.setresethaltreq = 1;
    target.dm.dmcontrol.hartreset = 1;
    target.dm.dmcontrol.ndmreset = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);

    vTaskDelay(100 / portTICK_PERIOD_MS);

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.dmactive = 1;
    target.dm.dmcontrol.haltreq = 1;
    target.dm.dmcontrol.setresethaltreq = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);

    for(i = 0; i < 100; i++) {
        vTaskDelay(10 / portTICK_PERIOD_MS);
        rv_dmi_read(RV_DM_DEBUG_MODULE_STATUS, &target.dm.dmstatus.value);
        if (target.dm.dmstatus.allhalted) {
            break;
        }
    }

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.dmactive = 1;
    target.dm.dmcontrol.haltreq = 1;
    target.dm.dmcontrol.clrresethaltreq = 1;
    if (target.dm.dmstatus.allhavereset) {
        target.dm.dmcontrol.ackhavereset = 1;
    }
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
}

void rv_target_halt(void)
{
    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.haltreq = 1;
    target.dm.dmcontrol.dmactive = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
}

void rv_target_halt_check(rv_target_halt_info_t* halt_info)
{
    uint32_t i, has_watchpoint;
    uint32_t wp_inst;
    uint32_t wp_addr_base_regno;
    uint32_t wp_addr_offset;
    uint32_t mc_hit;

    rv_dmi_read(RV_DM_DEBUG_MODULE_STATUS, &target.dm.dmstatus.value);
    if (target.dm.dmstatus.allhalted) {
        rv_register_read(&dcsr.value, CSR_DCSR);
        if (dcsr.cause == RV_CSR_DCSR_CAUSE_EBREAK) {
            halt_info->reason = rv_target_halt_reason_software_breakpoint;
        } else if (dcsr.cause == RV_CSR_DCSR_CAUSE_TRIGGER) {
            halt_info->reason = rv_target_halt_reason_other;
#if 0
            for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
                if (hardware_breakpoints[i].type != rv_target_breakpoint_type_unused) {
                    if (XLEN_RV32 == target.xlen) {
                        target.tr32.tselect = i;
                        rv_register_write(&target.tr32.tselect, CSR_TSELECT);
                        rv_register_read(&target.tr32.tdata1.value, CSR_TDATA1);
                        mc_hit = target.tr32.tdata1.mc.hit;
                    } if (XLEN_RV64 == target.xlen) {
                        target.tr64.tselect = i;
                        rv_register_write((uint32_t*)&target.tr64.tselect, CSR_TSELECT);
                        rv_register_read((uint32_t*)&target.tr64.tdata1.value, CSR_TDATA1);
                        mc_hit = target.tr64.tdata1.mc.hit;
                    } else {
                        //TODO:
                        return;
                    }
                    if (mc_hit) {
                        if (hardware_breakpoints[i].type == rv_target_breakpoint_type_hardware) {
                            halt_info->reason = rv_target_halt_reason_hardware_breakpoint;
                        } else if (hardware_breakpoints[i].type == rv_target_breakpoint_type_write_watchpoint) {
                            halt_info->reason = rv_target_halt_reason_write_watchpoint;
                            halt_info->addr = hardware_breakpoints[i].addr;
                        } else if (hardware_breakpoints[i].type == rv_target_breakpoint_type_read_watchpoint) {
                            halt_info->reason = rv_target_halt_reason_read_watchpoint;
                            halt_info->addr = hardware_breakpoints[i].addr;
                        } else if (hardware_breakpoints[i].type == rv_target_breakpoint_type_access_watchpoint) {
                            halt_info->reason = rv_target_halt_reason_access_watchpoint;
                            halt_info->addr = hardware_breakpoints[i].addr;
                        } else {
                            halt_info->reason = rv_target_halt_reason_other;
                        }
                        break;
                    }
                }
            }
#endif
            rv_register_read((uint32_t*)&dpc, CSR_DPC);
            for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
                if (hardware_breakpoints[i].type == rv_target_breakpoint_type_hardware) {
                    if (hardware_breakpoints[i].addr == dpc) {
                        halt_info->reason = rv_target_halt_reason_hardware_breakpoint;
                        break;
                    }
                }
            }

            if (halt_info->reason == rv_target_halt_reason_other) {
                has_watchpoint = 0;
                for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
                    if (hardware_breakpoints[i].type == rv_target_breakpoint_type_write_watchpoint) {
                        has_watchpoint = 1;
                        break;
                    }else if (hardware_breakpoints[i].type == rv_target_breakpoint_type_read_watchpoint) {
                        has_watchpoint = 1;
                        break;
                    } else if (hardware_breakpoints[i].type == rv_target_breakpoint_type_access_watchpoint) {
                        has_watchpoint = 1;
                        break;
                    }
                }
                if (has_watchpoint) {
                    rv_memory_read((uint8_t*)&wp_inst, dpc, 4, RV_AAMSIZE_16BITS);
                    rv_parse_watchpoint_inst(wp_inst, &wp_addr_base_regno, &wp_addr_offset);
                    rv_register_read(&(halt_info->addr), wp_addr_base_regno + 0x1000);
                    halt_info->addr += wp_addr_offset;
                    if (halt_info->addr != 0xffffffff) {
                        for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
                            if (halt_info->addr == hardware_breakpoints[i].addr) {
                                if (hardware_breakpoints[i].type == rv_target_breakpoint_type_write_watchpoint) {
                                    halt_info->reason = rv_target_halt_reason_write_watchpoint;
                                }
                                if (hardware_breakpoints[i].type == rv_target_breakpoint_type_read_watchpoint) {
                                    halt_info->reason = rv_target_halt_reason_read_watchpoint;
                                }
                                if (hardware_breakpoints[i].type == rv_target_breakpoint_type_access_watchpoint) {
                                    halt_info->reason = rv_target_halt_reason_access_watchpoint;
                                }
                                break;
                            }
                        }
                    }
                }
            }
        } else {
            halt_info->reason = rv_target_halt_reason_other;
        }
    } else {
        halt_info->reason = rv_target_halt_reason_running;
    }
}

void rv_target_resume(void)
{
    rv_register_read(&dcsr.value, CSR_DCSR);
    dcsr.step = 0;
    rv_register_write(&dcsr.value, CSR_DCSR);

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.resumereq = 1;
    target.dm.dmcontrol.dmactive = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
}

void rv_target_step(void)
{
    rv_register_read(&dcsr.value, CSR_DCSR);
    dcsr.step = 1;
    rv_register_write(&dcsr.value, CSR_DCSR);

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.resumereq = 1;
    target.dm.dmcontrol.dmactive = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
}

void rv_target_insert_breakpoint(rv_target_breakpoint_type_t type, uint64_t addr, uint32_t kind, uint32_t* err)
{
    uint32_t i;
    const uint16_t c_ebreak = 0x9002;
    const uint32_t ebreak = 0x00100073;

    if (type == rv_target_breakpoint_type_software) {
        for(i = 0; i < RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM; i++) {
            if (software_breakpoints[i].type == rv_target_breakpoint_type_unused) {
                software_breakpoints[i].type = type;
                software_breakpoints[i].addr = addr;
                software_breakpoints[i].kind = kind;
                rv_memory_read((uint8_t*)&software_breakpoints[i].inst,
                                addr,
                                (kind == 2) ? 1 : 2,
                                RV_AAMSIZE_16BITS);
                rv_memory_write((kind == 2) ? (const uint8_t*)&c_ebreak : (const uint8_t*)&ebreak,
                                addr,
                                (kind == 2) ? 1 : 2,
                                RV_AAMSIZE_16BITS);
                break;
            }
        }
        if (i == RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM) {
            *err = 0x0e;
            return;
        }
    } else {
        for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
            if (hardware_breakpoints[i].type == rv_target_breakpoint_type_unused) {
                hardware_breakpoints[i].type = type;
                hardware_breakpoints[i].addr = addr;
                hardware_breakpoints[i].kind = kind;
                if (XLEN_RV32 == target.xlen) {
                    target.tr32.tselect = i;
                    rv_register_write(&target.tr32.tselect, CSR_TSELECT);
                    target.tr32.tdata1.value = 0;
                    target.tr32.tdata1.mc.type = 2;
                    target.tr32.tdata1.mc.dmode = 1;
                    target.tr32.tdata1.mc.action = 1;
                    target.tr32.tdata1.mc.m = 1;
                    target.tr32.tdata1.mc.s = 1;
                    target.tr32.tdata1.mc.u = 1;
                    switch(type) {
                        case rv_target_breakpoint_type_hardware:
                            target.tr32.tdata1.mc.execute = 1;
                            break;
                        case rv_target_breakpoint_type_write_watchpoint:
                            target.tr32.tdata1.mc.store = 1;
                            break;
                        case rv_target_breakpoint_type_read_watchpoint:
                            target.tr32.tdata1.mc.load = 1;
                            break;
                        case rv_target_breakpoint_type_access_watchpoint:
                            target.tr32.tdata1.mc.store = 1;
                            target.tr32.tdata1.mc.load = 1;
                            break;
                        default:
                            break;
                    }
                    switch(kind) {
                        case 1:
                            target.tr32.tdata1.mc.sizelo = 1;
                            break;
                        case 2:
                            target.tr32.tdata1.mc.sizelo = 2;
                            break;
                        case 4:
                            target.tr32.tdata1.mc.sizelo = 3;
                            break;
                        default:
                            break;
                    }
                    rv_register_write(&target.tr32.tdata1.value, CSR_TDATA1);
                    rv_register_write(&hardware_breakpoints[target.tr32.tselect].addr, CSR_TDATA2);
                } else if (XLEN_RV64 == target.xlen) {
                    target.tr64.tselect = i;
                    rv_register_write((uint32_t*)&target.tr64.tselect, CSR_TSELECT);
                    target.tr64.tdata1.value = 0;
                    target.tr64.tdata1.mc.type = 2;
                    target.tr64.tdata1.mc.dmode = 1;
                    target.tr64.tdata1.mc.action = 1;
                    target.tr64.tdata1.mc.m = 1;
                    target.tr64.tdata1.mc.s = 1;
                    target.tr64.tdata1.mc.u = 1;
                    switch(type) {
                        case rv_target_breakpoint_type_hardware:
                            target.tr64.tdata1.mc.execute = 1;
                            break;
                        case rv_target_breakpoint_type_write_watchpoint:
                            target.tr64.tdata1.mc.store = 1;
                            break;
                        case rv_target_breakpoint_type_read_watchpoint:
                            target.tr64.tdata1.mc.load = 1;
                            break;
                        case rv_target_breakpoint_type_access_watchpoint:
                            target.tr64.tdata1.mc.store = 1;
                            target.tr64.tdata1.mc.load = 1;
                            break;
                        default:
                            break;
                    }
                    switch(kind) {
                        case 1:
                            target.tr64.tdata1.mc.sizelo = 1;
                            break;
                        case 2:
                            target.tr64.tdata1.mc.sizelo = 2;
                            break;
                        case 4:
                            target.tr64.tdata1.mc.sizelo = 3;
                            break;
                        default:
                            break;
                    }
                    rv_register_write((uint32_t*)&target.tr64.tdata1.value, CSR_TDATA1);
                    rv_register_write(&hardware_breakpoints[target.tr64.tselect].addr, CSR_TDATA2);
                } else {
                    //TODO:
                    return;
                }
                break;
            }
        }
        if (i == RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM) {
            *err = 0x0e;
            return;
        }
    }
    *err = 0;
    return;
}

void rv_target_remove_breakpoint(rv_target_breakpoint_type_t type, uint64_t addr, uint32_t kind, uint32_t* err)
{
    uint32_t i;

    if (type == rv_target_breakpoint_type_software) {
        for(i = 0; i < RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM; i++) {
            if ((software_breakpoints[i].type == type) && 
                (software_breakpoints[i].addr == addr) && 
                (software_breakpoints[i].kind == kind)) {
                rv_memory_write((const uint8_t*)&software_breakpoints[i].inst,
                                addr,
                                (kind == 2) ? 1 : 2,
                                RV_AAMSIZE_16BITS);
                software_breakpoints[i].type = rv_target_breakpoint_type_unused;
                break;
            }
        }
        if (i == RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM) {
            *err = 0x0e;
            return;
        }
    } else {
        for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
            if ((hardware_breakpoints[i].type == type) && 
                (hardware_breakpoints[i].addr == addr) && 
                (hardware_breakpoints[i].kind == kind)) {
                if (XLEN_RV32 == target.xlen) {
                    target.tr32.tselect = i;
                    rv_register_write(&target.tr32.tselect, CSR_TSELECT);
                } else if (XLEN_RV64 == target.xlen) {
                    target.tr64.tselect = i;
                    rv_register_write((uint32_t*)&target.tr64.tselect, CSR_TSELECT);
                } else {
                    //TODO:
                    return;
                }
                rv_register_write((uint32_t*)&zero, CSR_TDATA1);
                hardware_breakpoints[i].type = rv_target_breakpoint_type_unused;
                break;
            }
        }
        if (i == RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM) {
            *err = 0x0e;
            return;
        }
    }
    *err = 0;
    return;
}

void rv_target_flash_erase(uint32_t addr, uint32_t len, uint32_t* err)
{}

void rv_target_flash_write(uint32_t addr, uint32_t len, uint8_t* buffer, uint32_t* err)
{}

void rv_target_flash_done(void)
{}