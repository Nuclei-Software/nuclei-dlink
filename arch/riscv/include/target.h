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

#ifndef __TARGET_H__
#define __TARGET_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"

#if RVL_TARGET_CONFIG_REG_WIDTH == 32
typedef uint32_t rvl_target_reg_t;
#elif RVL_TARGET_CONFIG_REG_WIDTH == 64
typedef uint64_t rvl_target_ret_t; /* FIX ME rvl_target_ret_t not used */
#else
#error RVL_TARGET_CONFIG_REG_WIDTH is not defined
#endif

#if RVL_TARGET_CONFIG_ADDR_WIDTH == 32
typedef uint32_t rvl_target_addr_t;
#elif RVL_TARGET_CONFIG_ADDR_WIDTH == 64
typedef uint64_t rvl_target_addr_t;
#else
#error RVL_TARGET_CONFIG_ADDR_WIDTH is not defined
#endif

typedef enum
{
    rvl_target_halt_reason_running = 0,
    rvl_target_halt_reason_hardware_breakpoint = 1,
    rvl_target_halt_reason_software_breakpoint = 2,
    rvl_target_halt_reason_write_watchpoint = 3,
    rvl_target_halt_reason_read_watchpoint = 4,
    rvl_target_halt_reason_access_watchpoint = 5,
    rvl_target_halt_reason_other = 6,
} rvl_target_halt_reason_t;

typedef struct
{
    rvl_target_halt_reason_t reason;
    rvl_target_addr_t addr;
} rvl_target_halt_info_t;

typedef enum
{
    /*
     * Consistent with the type field of the z/Z command in gdb Remote Serial Protocol
     */
    rvl_target_breakpoint_type_software = 0,
    rvl_target_breakpoint_type_hardware = 1,
    rvl_target_breakpoint_type_write_watchpoint = 2,
    rvl_target_breakpoint_type_read_watchpoint = 3,
    rvl_target_breakpoint_type_access_watchpoint = 4,
    rvl_target_breakpoint_type_unused = 5,
} rvl_target_breakpoint_type_t;

typedef enum
{
    rvl_target_error_none = 0,

    /* The connection line problem, the JTAG line is not connected properly,
     * there is no power supply, the logic level does not match, etc. */
    rvl_target_error_line = 1,

    /* Compatibility issues */
    rvl_target_error_compat = 2,

    /* Problems with Debug Module */
    rvl_target_error_debug_module = 3,

    /* The target is protected and must be unprotected before subsequent
     * operations can be performed */
    rvl_target_error_protected = 4,

    /* Another error occurred */
    rvl_target_error_other = 255,
} rvl_target_error_t;

typedef enum {
    rvl_target_memory_type_ram = 0,
    rvl_target_memory_type_flash,
} rvl_target_memory_type_t;

typedef struct {
    rvl_target_memory_type_t type;
    rvl_target_addr_t start;
    rvl_target_addr_t length;
    rvl_target_addr_t blocksize;
} rvl_target_memory_t;

void riscv_target_init(void);
void riscv_target_init_post(rvl_target_error_t *err);
void riscv_target_init_after_halted(rvl_target_error_t *err);
void riscv_target_fini_pre(void);
void riscv_target_fini(void);
uint32_t riscv_target_get_idcode(void);

void rvl_target_set_error(const char* str);
void rvl_target_get_error(const char** str, uint32_t* pc);
void rvl_target_clr_error(void);

void rvl_target_reset(void);
void rvl_target_halt(void);
void rvl_target_halt_check(rvl_target_halt_info_t* halt_info);
void rvl_target_resume(void);
void rvl_target_step(void);

void rvl_target_read_core_registers(rvl_target_reg_t* regs);
void rvl_target_write_core_registers(const rvl_target_reg_t* regs);

void rvl_target_read_register(rvl_target_reg_t* reg, int regno);
void rvl_target_write_register(rvl_target_reg_t reg, int regno);

void rvl_target_read_memory(uint8_t* mem, rvl_target_addr_t addr, size_t len);
void rvl_target_write_memory(const uint8_t* mem, rvl_target_addr_t addr, size_t len);

void rvl_target_insert_breakpoint(rvl_target_breakpoint_type_t type, rvl_target_addr_t addr, int kind, int* err);
void rvl_target_remove_breakpoint(rvl_target_breakpoint_type_t type,rvl_target_addr_t addr, int kind, int* err);

const char* rvl_target_get_target_xml(void);
size_t rvl_target_get_target_xml_len(void);

const char *rvl_target_get_name(void);

void rvl_target_flash_erase(rvl_target_addr_t addr, size_t len, int* err);
void rvl_target_flash_write(rvl_target_addr_t addr, size_t len, uint8_t* buffer, int* err);
void rvl_target_flash_done(void);

#ifdef __cplusplus
}
#endif

#endif /* __TARGET_H__ */
