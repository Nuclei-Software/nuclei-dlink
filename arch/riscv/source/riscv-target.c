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
#include "encoding.h"
#include "opcodes.h"
#include "config.h"

typedef struct {
    rv_dm_t dm;
    rv_tr32_t tr32;
    rv_tr64_t tr64;
    rv_dtm_t dtm;
    rv_dmi_t dmi;
    rv_misa_rv32_t misa;
    uint64_t vlenb;
    rv_target_interface_t interface;
} rv_target_t;

static rv_target_t target;
rv_dcsr_t dcsr;
uint64_t dpc;

uint32_t zero = 0; 
uint32_t result;
bool err_flag;
const char *err_msg;
uint32_t err_pc;
uint64_t save_vtype, save_vl;
rv_hardware_breakpoint_t hardware_breakpoints[RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM];
rv_software_breakpoint_t software_breakpoints[RV_TARGET_CONFIG_SOFTWARE_BREAKPOINT_NUM];
uint32_t rv_target_dr_post;
uint32_t rv_target_dr_pre;
uint32_t rv_target_ir_post;
uint32_t rv_target_ir_pre;

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
        rv_tap_shift_ir(temp_out, temp_in, 5, rv_target_ir_post, rv_target_ir_pre);
    }
    last_reg = reg;
    
    temp_in[0] = target.dmi.data;
    switch (reg) {
        case RV_DTM_JTAG_REG_IDCODE:
            rv_tap_shift_dr(temp_out, temp_in, 32, rv_target_dr_post, rv_target_dr_pre);
            target.dtm.idcode.value = temp_out[0];
            break;
        case RV_DTM_JTAG_REG_DTMCS:
            rv_tap_shift_dr(temp_out, temp_in, 32, rv_target_dr_post, rv_target_dr_pre);
            target.dtm.dtmcs.value = temp_out[0];
            break;
        case RV_DTM_JTAG_REG_DMI:
            temp_in[0] = (target.dmi.data << 2) | (target.dmi.op & 0x3);
            temp_in[1] = (target.dmi.data >> 30) | (target.dmi.address << 2);
            rv_tap_shift_dr(temp_out, temp_in, 32 + 2 + target.dtm.dtmcs.abits, rv_target_dr_post, rv_target_dr_pre);
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

static void rv_prep_for_register_access(uint32_t regno)
{
    uint64_t mstatus;
    if ((regno >= RV_REG_FT0) && (regno <= RV_REG_FT11)) {
        rv_target_read_register(&mstatus, RV_REG_MSTATUS);
        mstatus |= MSTATUS_FS;
        rv_target_write_register(&mstatus, RV_REG_MSTATUS);
    }
    if (((regno >= RV_REG_V0) && (regno <= RV_REG_V31)) || 
        ((regno >= RV_REG_VSTART) && (regno <= RV_REG_VCSR)) ||
        ((regno >= RV_REG_VL) && (regno <= RV_REG_VLENB))) {
        rv_target_read_register(&mstatus, RV_REG_MSTATUS);
        mstatus |= MSTATUS_VS;
        rv_target_write_register(&mstatus, RV_REG_MSTATUS);
    }
}

static void rv_cleanup_after_register_access(uint32_t regno)
{
    uint64_t mstatus;
    if ((regno >= RV_REG_FT0) && (regno <= RV_REG_FT11)) {
        rv_target_read_register(&mstatus, RV_REG_MSTATUS);
        mstatus &= ~MSTATUS_FS;
        rv_target_write_register(&mstatus, RV_REG_MSTATUS);
    }
    if (((regno >= RV_REG_V0) && (regno <= RV_REG_V31)) || 
        ((regno >= RV_REG_VSTART) && (regno <= RV_REG_VCSR)) ||
        ((regno >= RV_REG_VL) && (regno <= RV_REG_VLENB))) {
        rv_target_read_register(&mstatus, RV_REG_MSTATUS);
        mstatus &= ~MSTATUS_VS;
        rv_target_write_register(&mstatus, RV_REG_MSTATUS);
    }
}

static void rv_register_read(uint32_t *reg, uint32_t regno)
{
    uint32_t mxl;
    if (regno == (RV_REG_DCSR - RV_REG_CSR0)) {
        mxl = MXL_RV32;
    } else {
        mxl = target.misa.mxl;
    }
    err_flag = false;
    target.dm.command.value = 0;
    target.dm.command.reg.cmdtype = RV_DM_ABSTRACT_CMD_ACCESS_REG;
    if (MXL_RV32 == mxl) {
        target.dm.command.reg.aarsize = 2;
    } else if (MXL_RV64 == mxl) {
        target.dm.command.reg.aarsize = 3;
    }
    target.dm.command.reg.transfer = 1;
    target.dm.command.reg.regno = regno;
    rv_dmi_write(RV_DM_ABSTRACT_COMMAND, target.dm.command.value);

    do {
        rv_dmi_read(RV_DM_ABSTRACT_CONTROL_AND_STATUS, &target.dm.abstractcs.value);
        if (target.dm.abstractcs.cmderr) {
            rv_set_error(rv_abstractcs_cmderr_str[target.dm.abstractcs.cmderr]);
            target.dm.abstractcs.value = 0;
            target.dm.abstractcs.cmderr = 0x7;
            rv_dmi_write(RV_DM_ABSTRACT_CONTROL_AND_STATUS, target.dm.abstractcs.value);
            *reg = 0xffffffff;
        }
    } while(target.dm.abstractcs.busy);

    if (MXL_RV32 == mxl) {
        rv_dmi_read(RV_DM_ABSTRACT_DATA0, &target.dm.data[0]);
        reg[0] = target.dm.data[0];
    } else if (MXL_RV64 == mxl) {
        rv_dmi_read(RV_DM_ABSTRACT_DATA0, &target.dm.data[0]);
        reg[0] = target.dm.data[0];
        rv_dmi_read(RV_DM_ABSTRACT_DATA1, &target.dm.data[1]);
        reg[1] = target.dm.data[1];
    }
}

static void rv_register_write(uint32_t *reg, uint32_t regno)
{
    uint32_t mxl;
    if (regno == (RV_REG_DCSR - RV_REG_CSR0)) {
        mxl = MXL_RV32;
    } else {
        mxl = target.misa.mxl;
    }
    err_flag = false;
    if (MXL_RV32 == mxl) {
        target.dm.data[0] = reg[0];
        rv_dmi_write(RV_DM_ABSTRACT_DATA0, target.dm.data[0]);
    } else if (MXL_RV64 == mxl) {
        target.dm.data[0] = reg[0];
        rv_dmi_write(RV_DM_ABSTRACT_DATA0, target.dm.data[0]);
        target.dm.data[1] = reg[1];
        rv_dmi_write(RV_DM_ABSTRACT_DATA1, target.dm.data[1]);
    }

    target.dm.command.value = 0;
    target.dm.command.reg.cmdtype = RV_DM_ABSTRACT_CMD_ACCESS_REG;
    if (MXL_RV32 == mxl) {
        target.dm.command.reg.aarsize = 2;
    } else if (MXL_RV64 == mxl) {
        target.dm.command.reg.aarsize = 3;
    }
    target.dm.command.reg.write = 1;
    target.dm.command.reg.transfer = 1;
    target.dm.command.reg.regno = regno;
    rv_dmi_write(RV_DM_ABSTRACT_COMMAND, target.dm.command.value);

    do {
        rv_dmi_read(RV_DM_ABSTRACT_CONTROL_AND_STATUS, &target.dm.abstractcs.value);
        if (target.dm.abstractcs.cmderr) {
            rv_set_error(rv_abstractcs_cmderr_str[target.dm.abstractcs.cmderr]);
            target.dm.abstractcs.value = 0;
            target.dm.abstractcs.cmderr = 0x7;
            rv_dmi_write(RV_DM_ABSTRACT_CONTROL_AND_STATUS, target.dm.abstractcs.value);
        }
    } while(target.dm.abstractcs.busy);
}

static void rv_memory_read(uint8_t *mem, uint64_t addr, uint32_t len, uint32_t aamsize)
{
    uint32_t i;
    uint8_t *pbyte;
    uint16_t *phalfword;
    uint32_t *pword;

    err_flag = false;

    for (i = 0; i < len; i++) {
        if (MXL_RV32 == target.misa.mxl) {
            target.dm.data[1] = addr + (i << aamsize);
            rv_dmi_write(RV_DM_ABSTRACT_DATA1, target.dm.data[1]);
        } else if (MXL_RV64 == target.misa.mxl) {
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
            case RV_AAMSIZE_8BITS:
                pbyte = (uint8_t*)mem;
                pbyte[i] = target.dm.data[0] & 0xff;
                break;
            case RV_AAMSIZE_16BITS:
                phalfword = (uint16_t*)mem;
                phalfword[i] = target.dm.data[0] & 0xffff;
                break;
            case RV_AAMSIZE_32BITS:
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
        if (MXL_RV32 == target.misa.mxl) {
            target.dm.data[1] = addr + (i << aamsize);
            rv_dmi_write(RV_DM_ABSTRACT_DATA1, target.dm.data[1]);
        } else if (MXL_RV64 == target.misa.mxl) {
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

void rv_program_exec(uint32_t* inst, uint32_t num)
{
    for (int i = 0; i < num; i++) {
        rv_dmi_write(RV_DM_PROGRAM_BUFFER0 + i, inst[i]);
    }
    rv_dmi_write(RV_DM_PROGRAM_BUFFER0 + num, ebreak());

    target.dm.command.value = 0;
    target.dm.command.reg.aarsize = 2;
    target.dm.command.reg.postexec = 1;
    target.dm.command.reg.transfer = 0;
    target.dm.command.reg.regno = 0x1000;
    rv_dmi_write(RV_DM_ABSTRACT_COMMAND, target.dm.command.value);

    do {
        rv_dmi_read(RV_DM_ABSTRACT_CONTROL_AND_STATUS, &target.dm.abstractcs.value);
        if (target.dm.abstractcs.cmderr) {
            rv_set_error(rv_abstractcs_cmderr_str[target.dm.abstractcs.cmderr]);
            target.dm.abstractcs.value = 0;
            target.dm.abstractcs.cmderr = 0x7;
            rv_dmi_write(RV_DM_ABSTRACT_CONTROL_AND_STATUS, target.dm.abstractcs.value);
        }
    } while(target.dm.abstractcs.busy);
}

static void rv_register_write_buf(void *reg, uint32_t regno)
{
    uint32_t inst[2];
    uint32_t inst_num;
    uint64_t save_fp, save_s1;

    rv_target_read_register(&save_fp, RV_REG_FP);

    if (regno >= RV_REG_FT0 && regno <= RV_REG_FT11) {
        if (target.misa.d && (target.misa.mxl == MXL_RV32)) {// RV32 D
            uint64_t scratch_addr;
            rv_dmi_read(RV_DM_HALT_INFO, &target.dm.hartinfo.value);
            scratch_addr = target.dm.hartinfo.dataaddr;
            inst[0] = fld(regno - RV_REG_FT0, RV_REG_FP, 0);
            inst_num = 1;
            rv_target_write_register(&scratch_addr, RV_REG_FP);
            rv_memory_write(reg, scratch_addr, 8, RV_AAMSIZE_32BITS);
            rv_program_exec(inst, inst_num);
        } else if (target.misa.d) {// RV64 D
            inst[0] = fmv_d_x(regno - RV_REG_FT0, RV_REG_FP);
            inst_num = 1;
            rv_target_write_register(reg, RV_REG_FP);
            rv_program_exec(inst, inst_num);
        } else {// RV32/RV64 F
            inst[0] = fmv_w_x(regno - RV_REG_FT0, RV_REG_FP);
            inst_num = 1;
            rv_target_write_register(reg, RV_REG_FP);
            rv_program_exec(inst, inst_num);
        }
    } else if (regno == RV_REG_VL) {
        rv_target_read_register(&save_s1, RV_REG_S1);
        inst[0] = csrr(RV_REG_S1, RV_REG_VTYPE - RV_REG_CSR0);
        inst[1] = vsetvl(RV_REG_ZERO, RV_REG_FP, RV_REG_S1);
        inst_num = 2;
        rv_target_write_register(reg, RV_REG_FP);
        rv_program_exec(inst, inst_num);
        rv_target_write_register(&save_s1, RV_REG_S1);
    } else if (regno == RV_REG_VTYPE) {
        rv_target_read_register(&save_s1, RV_REG_S1);
        inst[0] = csrr(RV_REG_S1, RV_REG_VL - RV_REG_CSR0);
        inst[1] = vsetvl(RV_REG_ZERO, RV_REG_S1, RV_REG_FP);
        inst_num = 2;
        rv_target_write_register(reg, RV_REG_FP);
        rv_program_exec(inst, inst_num);
        rv_target_write_register(&save_s1, RV_REG_S1);
    } else if (regno >= RV_REG_CSR0 && regno <= (4095 + RV_REG_CSR0)) {
        inst[0] = csrrw(RV_REG_ZERO, RV_REG_FP, regno - RV_REG_CSR0);
        inst_num = 1;
        rv_target_write_register(reg, RV_REG_FP);
        rv_program_exec(inst, inst_num);
    } else if (RV_REG_V0 < regno < RV_REG_V31) {
        uint64_t xlen, debug_vl, encoded_vsew;
        xlen = target.misa.mxl * 32;
        if (MXL_RV32 == target.misa.mxl) {
            encoded_vsew = 2 << 3;
        } else if (MXL_RV64 == target.misa.mxl) {
            encoded_vsew = 3 << 3;
        }
        debug_vl = ((target.vlenb * 8) + xlen - 1) / xlen;
        rv_register_write_buf(&encoded_vsew, RV_REG_VTYPE);
        rv_register_write_buf(&debug_vl, RV_REG_VL);
        inst[0] = vslide1down_vx(regno - RV_REG_V0, regno - RV_REG_V0, RV_REG_FP, true);
        inst_num = 1;
        for (int i = 0; i < debug_vl; i++) {
            if (MXL_RV32 == target.misa.mxl) {
                rv_target_write_register((uint32_t*)reg + i, RV_REG_FP);
            } else if (MXL_RV64 == target.misa.mxl) {
                rv_target_write_register((uint64_t*)reg + i, RV_REG_FP);
            }
            rv_program_exec(inst, inst_num);
        }
    }

    rv_target_write_register(&save_fp, RV_REG_FP);
}

static void rv_register_read_buf(void *reg, uint32_t regno)
{
    uint32_t inst[2];
    uint32_t inst_num;
    uint64_t save_fp;

    rv_target_read_register(&save_fp, RV_REG_FP);

    if (regno >= RV_REG_FT0 && regno <= RV_REG_FT11) {
        if (target.misa.d && (target.misa.mxl == MXL_RV32)) {// RV32 D
            uint64_t scratch_addr;
            rv_dmi_read(RV_DM_HALT_INFO, &target.dm.hartinfo.value);
            scratch_addr = target.dm.hartinfo.dataaddr;
            inst[0] = fsd(regno - RV_REG_FT0, RV_REG_FP, 0);
            inst_num = 1;
            rv_target_write_register(&scratch_addr, RV_REG_FP);
            rv_program_exec(inst, inst_num);
            rv_memory_read(reg, scratch_addr, 8, RV_AAMSIZE_32BITS);
        } else if (target.misa.d) {// RV64 D
            inst[0] = fmv_x_d(RV_REG_FP, regno - RV_REG_FT0);
            inst_num = 1;
            rv_program_exec(inst, inst_num);
            rv_target_read_register(reg, RV_REG_FP);
        } else {// RV32/RV64 F
            inst[0] = fmv_x_w(RV_REG_FP, regno - RV_REG_FT0);
            inst_num = 1;
            rv_program_exec(inst, inst_num);
            rv_target_read_register(reg, RV_REG_FP);
        }
    } else if (regno >= RV_REG_CSR0 && regno <= (4095 + RV_REG_CSR0)) {
        inst[0] = csrrs(RV_REG_FP, RV_REG_ZERO, regno - RV_REG_CSR0);
        inst_num = 1;
        rv_program_exec(inst, inst_num);
        rv_target_read_register(reg, RV_REG_FP);
    } else if (RV_REG_V0 < regno < RV_REG_V31) {
        uint64_t xlen, debug_vl, encoded_vsew;
        xlen = target.misa.mxl * 32;
        if (MXL_RV32 == target.misa.mxl) {
            encoded_vsew = 2 << 3;
        } else if (MXL_RV64 == target.misa.mxl) {
            encoded_vsew = 3 << 3;
        }
        debug_vl = ((target.vlenb * 8) + xlen - 1) / xlen;
        rv_register_write_buf(&encoded_vsew, RV_REG_VTYPE);
        rv_register_write_buf(&debug_vl, RV_REG_VL);
        inst[0] = vmv_x_s(RV_REG_FP, regno - RV_REG_V0);
        inst[1] = vslide1down_vx(regno - RV_REG_V0, regno - RV_REG_V0, RV_REG_FP, true);
        inst_num = 2;
        for (int i = 0; i < debug_vl; i++) {
            rv_program_exec(inst, inst_num);
            if (MXL_RV32 == target.misa.mxl) {
                rv_target_read_register((uint32_t*)reg + i, RV_REG_FP);
            } else if (MXL_RV64 == target.misa.mxl) {
                rv_target_read_register((uint64_t*)reg + i, RV_REG_FP);
            }
        }
    }

    rv_target_write_register(&save_fp, RV_REG_FP);
}

static void rv_prep_for_vector_access()
{
    //prep for vector access
    rv_register_read_buf(&save_vtype, RV_REG_VTYPE);
    rv_register_read_buf(&save_vl, RV_REG_VL);
}

static void rv_cleanup_for_vector_access()
{
    //cleanup after vector access
    rv_register_write_buf(&save_vtype, RV_REG_VTYPE);
    rv_register_write_buf(&save_vl, RV_REG_VL);
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
    rv_target_dr_post = 0;
    rv_target_dr_pre = 0;
    rv_target_ir_post = 0;
    rv_target_ir_pre = 0;
    target.misa.value = 0;
    target.vlenb = 0;

    rv_tap_init();
}

void rv_target_deinit(void)
{
    rv_tap_deinit();
}

uint32_t rv_target_misa(void)
{
    return target.misa.value;
}

uint32_t rv_target_mxl(void)
{
    return target.misa.mxl;
}

uint64_t rv_target_vlenb(void)
{
    return target.vlenb;
}

void rv_target_set_interface(rv_target_interface_t interface)
{
    target.interface = interface;
}

void rv_target_init_post(rv_target_error_t *err)
{
    rv_tap_reset(32);

    if (TARGET_INTERFACE_CJTAG == target.interface) {
        rv_tap_oscan1_mode(rv_target_dr_post, rv_target_dr_pre, rv_target_ir_post, rv_target_ir_pre);
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

    /* get misa */
    rv_misa_rv32_t misa32;
    rv_misa_rv64_t misa64;
    target.misa.mxl = MXL_RV64;
    rv_target_read_register(&misa64, RV_REG_MISA);
    if (err_flag) {
        target.misa.mxl = MXL_RV32;
        rv_target_read_register(&misa32, RV_REG_MISA);
        if (err_flag) {
            *err = rv_target_error_debug_module;
            return;
        }
        if (0x1 != misa32.mxl) {
            target.misa.value = 0;
            *err = rv_target_error_debug_module;
            return;
        }
        target.misa.value = misa32.value;
    } else {
        if (0x2 != misa64.mxl) {
            target.misa.value = 0;
            *err = rv_target_error_debug_module;
            return;
        }
        target.misa.value = misa64.value;
        target.misa.mxl = misa64.mxl;
    }

    rv_target_read_register(&target.vlenb, RV_REG_VLENB);

    rv_target_read_register(&dcsr.value, RV_REG_DCSR);
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
    rv_target_write_register(&dcsr.value, RV_REG_DCSR);

    /*
     * clear all hardware breakpoints
     */
    for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
        rv_target_write_register(&i, RV_REG_TSELECT);
        rv_target_write_register(&zero, RV_REG_TDATA1);
    }
}

void rv_target_fini_pre(void)
{
    /*
     * ebreak instructions in X-mode behave as described in the Privileged Spec.
     */
    rv_target_read_register(&dcsr.value, RV_REG_DCSR);
    dcsr.ebreakm = 0;
    dcsr.ebreaks = 0;
    dcsr.ebreaku = 0;
    rv_target_write_register(&dcsr.value, RV_REG_DCSR);

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
        if (MXL_RV32 == target.misa.mxl) {
            rv_target_read_register((uint32_t*)regs + i, i);
        } else if (MXL_RV64 == target.misa.mxl) {
            rv_target_read_register((uint64_t*)regs + i, i);
        }
    }
    if (MXL_RV32 == target.misa.mxl) {
        rv_target_read_register((uint32_t*)regs + 32, RV_REG_DPC);
    } else if (MXL_RV64 == target.misa.mxl) {
        rv_target_read_register((uint64_t*)regs + 32, RV_REG_DPC);
    }
}

void rv_target_write_core_registers(void *regs)
{
    uint32_t i;

    for(i = 1; i < 32; i++) {
        if (MXL_RV32 == target.misa.mxl) {
            rv_target_write_register((uint32_t*)regs + i, i);
        } else if (MXL_RV64 == target.misa.mxl) {
            rv_target_write_register((uint64_t*)regs + i, i);
        }
    }
    if (MXL_RV32 == target.misa.mxl) {
        rv_target_write_register((uint32_t*)regs + 32, RV_REG_DPC);
    } else if (MXL_RV64 == target.misa.mxl) {
        rv_target_write_register((uint64_t*)regs + 32, RV_REG_DPC);
    }
}

void rv_target_read_register(void *reg, uint32_t regno)
{
    rv_prep_for_register_access(regno);
    if (regno >= RV_REG_ZERO && regno < RV_REG_PC) {
        rv_register_read((uint32_t*)reg, 0x1000 + regno - RV_REG_ZERO);
    } else if (regno == RV_REG_PC) {
        rv_target_read_register(reg, RV_REG_DPC);
    } else if (regno >= RV_REG_FT0 && regno <= RV_REG_FT11) {
        rv_register_read((uint32_t*)reg, 0x1020 + regno - RV_REG_FT0);
        /* abstract fail try progbuf */
        if (err_flag) {
            rv_register_read_buf(reg, regno);
        }
    } else if (regno >= RV_REG_CSR0 && regno <= (4095 + RV_REG_CSR0)) {
        rv_register_read((uint32_t*)reg, regno - RV_REG_CSR0);
        /* abstract fail try progbuf */
        if (err_flag) {
            rv_register_read_buf(reg, regno);
        }
    } else if (regno == RV_REG_PRIV) {
        rv_target_read_register(&dcsr.value, RV_REG_DCSR);
        *(uint32_t*)reg = dcsr.prv;
    } else if (regno >= RV_REG_V0 && regno <= RV_REG_V31) {
        rv_prep_for_vector_access();
        rv_register_read_buf(reg, regno);
        rv_cleanup_for_vector_access();
    } else {
        *(uint32_t*)reg = 0xffffffff;
    }
    rv_cleanup_after_register_access(regno);
}

void rv_target_write_register(void *reg, uint32_t regno)
{
    rv_prep_for_register_access(regno);
    if (regno >= RV_REG_ZERO && regno < RV_REG_PC) {
        rv_register_write((uint32_t*)reg, 0x1000 + regno - RV_REG_ZERO);
    } else if (regno == RV_REG_PC) {
        rv_target_write_register(reg, RV_REG_DPC);
    } else if (regno >= RV_REG_FT0 && regno <= RV_REG_FT11) {
        rv_register_write((uint32_t*)reg, 0x1020 + regno - RV_REG_FT0);
        /* abstract fail try progbuf */
        if (err_flag) {
            rv_register_write_buf(reg, regno);
        }
    } else if (regno >= RV_REG_CSR0 && regno <= (4095 + RV_REG_CSR0)) {
        rv_register_write((uint32_t*)reg, regno - RV_REG_CSR0);
        /* abstract fail try progbuf */
        if (err_flag) {
            rv_register_write_buf(reg, regno);
        }
    } else if (regno == RV_REG_PRIV) {
        rv_target_read_register(&dcsr.value, RV_REG_DCSR);
        dcsr.prv = *(uint32_t*)reg;
        rv_target_write_register(&dcsr.value, RV_REG_DCSR);
    } else if (regno >= RV_REG_V0 && regno <= RV_REG_V31) {
        rv_prep_for_vector_access();
        rv_register_write_buf(reg, regno);
        rv_cleanup_for_vector_access();
    }
    rv_cleanup_after_register_access(regno);
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
    uint64_t tselect;

    rv_dmi_read(RV_DM_DEBUG_MODULE_STATUS, &target.dm.dmstatus.value);
    if (target.dm.dmstatus.allhalted) {
        rv_target_read_register(&dcsr.value, RV_REG_DCSR);
        if (dcsr.cause == RV_CSR_DCSR_CAUSE_EBREAK) {
            halt_info->reason = rv_target_halt_reason_software_breakpoint;
        } else if (dcsr.cause == RV_CSR_DCSR_CAUSE_TRIGGER) {
            halt_info->reason = rv_target_halt_reason_other;
            rv_target_read_register(&tselect, RV_REG_TSELECT);
            for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
                if (hardware_breakpoints[i].type != rv_target_breakpoint_type_unused) {
                    if (MXL_RV32 == target.misa.mxl) {
                        target.tr32.tselect = i;
                        rv_target_write_register(&target.tr32.tselect, RV_REG_TSELECT);
                        rv_target_read_register(&target.tr32.tdata1.value, RV_REG_TDATA1);
                        mc_hit = target.tr32.tdata1.mc.hit;
                    } if (MXL_RV64 == target.misa.mxl) {
                        target.tr64.tselect = i;
                        rv_target_write_register((uint32_t*)&target.tr64.tselect, RV_REG_TSELECT);
                        rv_target_read_register(&target.tr64.tdata1.value, RV_REG_TDATA1);
                        mc_hit = target.tr64.tdata1.mc.hit;
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
            rv_target_write_register(&tselect, RV_REG_TSELECT);
            rv_target_read_register(&dpc, RV_REG_DPC);
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
                    rv_target_read_register(&(halt_info->addr), wp_addr_base_regno);
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
    rv_target_read_register(&dcsr.value, RV_REG_DCSR);
    dcsr.step = 0;
    rv_target_write_register(&dcsr.value, RV_REG_DCSR);

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.resumereq = 1;
    target.dm.dmcontrol.dmactive = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
}

void rv_target_step(void)
{
    rv_target_read_register(&dcsr.value, RV_REG_DCSR);
    dcsr.step = 1;
    rv_target_write_register(&dcsr.value, RV_REG_DCSR);

    target.dm.dmcontrol.value = 0;
    target.dm.dmcontrol.resumereq = 1;
    target.dm.dmcontrol.dmactive = 1;
    rv_dmi_write(RV_DM_DEBUG_MODULE_CONTROL, target.dm.dmcontrol.value);
}

void rv_target_insert_breakpoint(rv_target_breakpoint_type_t type, uint64_t addr, uint32_t kind, uint32_t* err)
{
    uint32_t i;
    uint64_t tselect;
    uint64_t tselect_rd, tdata1_rd;
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
        tselect_rd = 0;
        tdata1_rd = 0;
        rv_target_read_register(&tselect, RV_REG_TSELECT);
        for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
            if (hardware_breakpoints[i].type == rv_target_breakpoint_type_unused) {
                hardware_breakpoints[i].type = type;
                hardware_breakpoints[i].addr = addr;
                hardware_breakpoints[i].kind = kind;
                if (MXL_RV32 == target.misa.mxl) {
                    target.tr32.tselect = i;
                    rv_target_write_register(&target.tr32.tselect, RV_REG_TSELECT);
                    rv_target_read_register(&tselect_rd, RV_REG_TSELECT);
                    if (target.tr32.tselect != tselect_rd) {
                        *err = 0x0e;
                        return;
                    }
                    rv_target_read_register(&target.tr32.tdata1.value, RV_REG_TDATA1);
                    target.tr32.tdata1.mc.dmode = 1;
                    target.tr32.tdata1.mc.action = 1;
                    target.tr32.tdata1.mc.match = 0;
                    target.tr32.tdata1.mc.m = 1;
                    if (target.misa.s) {
                        target.tr32.tdata1.mc.s = 1;
                    }
                    if (target.misa.u) {
                        target.tr32.tdata1.mc.u = 1;
                    }
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
                    rv_target_write_register(&target.tr32.tdata1.value, RV_REG_TDATA1);
                    rv_target_read_register(&tdata1_rd, RV_REG_TDATA1);
                    if (target.tr32.tdata1.value != tdata1_rd) {
                        *err = 0x0e;
                        return;
                    }
                    rv_target_write_register(&hardware_breakpoints[target.tr32.tselect].addr, RV_REG_TDATA2);
                } else if (MXL_RV64 == target.misa.mxl) {
                    target.tr64.tselect = i;
                    rv_target_write_register(&target.tr64.tselect, RV_REG_TSELECT);
                    rv_target_read_register(&tselect_rd, RV_REG_TSELECT);
                    if (target.tr64.tselect != tselect_rd) {
                        *err = 0x0e;
                        return;
                    }
                    rv_target_read_register(&target.tr64.tdata1.value, RV_REG_TDATA1);
                    target.tr64.tdata1.mc.dmode = 1;
                    target.tr64.tdata1.mc.action = 1;
                    target.tr64.tdata1.mc.match = 0;
                    target.tr64.tdata1.mc.m = 1;
                    if (target.misa.s) {
                        target.tr64.tdata1.mc.s = 1;
                    }
                    if (target.misa.u) {
                        target.tr64.tdata1.mc.u = 1;
                    }
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
                    rv_target_write_register(&target.tr64.tdata1.value, RV_REG_TDATA1);
                    rv_target_read_register(&tdata1_rd, RV_REG_TDATA1);
                    if (target.tr64.tdata1.value != tdata1_rd) {
                        *err = 0x0e;
                        return;
                    }
                    rv_target_write_register(&hardware_breakpoints[target.tr64.tselect].addr, RV_REG_TDATA2);
                }
                break;
            }
        }
        rv_target_write_register(&tselect, RV_REG_TSELECT);
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
    uint64_t tselect;
    uint64_t tselect_rd;

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
        tselect_rd = 0;
        rv_target_read_register(&tselect, RV_REG_TSELECT);
        for(i = 0; i < RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM; i++) {
            if ((hardware_breakpoints[i].type == type) && 
                (hardware_breakpoints[i].addr == addr) && 
                (hardware_breakpoints[i].kind == kind)) {
                if (MXL_RV32 == target.misa.mxl) {
                    target.tr32.tselect = i;
                    rv_target_write_register(&target.tr32.tselect, RV_REG_TSELECT);
                    rv_target_read_register(&tselect_rd, RV_REG_TSELECT);
                    if (target.tr32.tselect != tselect_rd) {
                        *err = 0x0e;
                        return;
                    }
                } else if (MXL_RV64 == target.misa.mxl) {
                    target.tr64.tselect = i;
                    rv_target_write_register(&target.tr64.tselect, RV_REG_TSELECT);
                    rv_target_read_register(&tselect_rd, RV_REG_TSELECT);
                    if (target.tr64.tselect != tselect_rd) {
                        *err = 0x0e;
                        return;
                    }
                }
                rv_target_write_register(&zero, RV_REG_TDATA1);
                hardware_breakpoints[i].type = rv_target_breakpoint_type_unused;
                break;
            }
        }
        rv_target_write_register(&tselect, RV_REG_TSELECT);
        if (i == RV_TARGET_CONFIG_HARDWARE_BREAKPOINT_NUM) {
            *err = 0x0e;
            return;
        }
    }
    *err = 0;
    return;
}
