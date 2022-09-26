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
#include <stdint.h>
#include <stddef.h>
#include "config.h"

#define MXL_RV32                   (1)
#define MXL_RV64                   (2)

/*
 * Debug Transport Module (DTM)
 */
#define RV_DTM_JTAG_REG_IDCODE       (0x01)
#define RV_DTM_JTAG_REG_DTMCS        (0x10)
#define RV_DTM_JTAG_REG_DMI          (0x11)

#define RV_DTMCS_DMI_RESET           (1 << 16)
#define RV_DTMCS_DMI_HARD_RESET      (1 << 17)

/*
 * Debug Module Interface (DMI)
 */

#define RV_DMI_OP_NOP                (0)
#define RV_DMI_OP_READ               (1)
#define RV_DMI_OP_WRITE              (2)

#define RV_DMI_RESULT_DONE           (0)
#define RV_DMI_RESULT_FAIL           (2)
#define RV_DMI_RESULT_BUSY           (3)

typedef enum {
    /*
     * Consistent with the type field of the z/Z command in gdb Remote Serial Protocol
     */
    rv_target_breakpoint_type_software = 0,
    rv_target_breakpoint_type_hardware = 1,
    rv_target_breakpoint_type_write_watchpoint = 2,
    rv_target_breakpoint_type_read_watchpoint = 3,
    rv_target_breakpoint_type_access_watchpoint = 4,
    rv_target_breakpoint_type_unused = 5,
} rv_target_breakpoint_type_t;

typedef struct {
    rv_target_breakpoint_type_t type;
    uint32_t addr;
    uint32_t kind;
} rv_hardware_breakpoint_t;

typedef struct {
    rv_target_breakpoint_type_t type;
    uint32_t addr;
    uint32_t kind;
    uint32_t inst;
} rv_software_breakpoint_t;

typedef enum {
    rv_target_error_none = 0,
    /* The connection line problem, the JTAG line is not connected properly,
     * there is no power supply, the logic level does not match, etc. */
    rv_target_error_line = 1,
    /* Compatibility issues */
    rv_target_error_compat = 2,
    /* Problems with Debug Module */
    rv_target_error_debug_module = 3,
    /* The target is protected and must be unprotected before subsequent
     * operations can be performed */
    rv_target_error_protected = 4,
    /* Another error occurred */
    rv_target_error_other = 255,
} rv_target_error_t;

// Abstract Command (command)
#define RV_DM_ABSTRACT_CMD_ACCESS_REG        (0)
#define RV_DM_ABSTRACT_CMD_QUICK_ACCESS      (1)
#define RV_DM_ABSTRACT_CMD_ACCESS_MEM        (2)

#define RV_AAMSIZE_8BITS                     (0)
#define RV_AAMSIZE_16BITS                    (1)
#define RV_AAMSIZE_32BITS                    (2)
#define RV_AAMSIZE_64BITS                    (3)
#define RV_AAMSIZE_128BITS                   (4)

#define RV_CSR_DCSR_CAUSE_EBREAK             (1)
#define RV_CSR_DCSR_CAUSE_TRIGGER            (2)
#define RV_CSR_DCSR_CAUSE_HALT_REQ           (3)
#define RV_CSR_DCSR_CAUSE_STEP               (4)
#define RV_CSR_DCSR_CAUSE_RESET_HALT_REQ     (5)

typedef enum{
    rv_target_halt_reason_running = 0,
    rv_target_halt_reason_hardware_breakpoint = 1,
    rv_target_halt_reason_software_breakpoint = 2,
    rv_target_halt_reason_write_watchpoint = 3,
    rv_target_halt_reason_read_watchpoint = 4,
    rv_target_halt_reason_access_watchpoint = 5,
    rv_target_halt_reason_other = 6,
} rv_target_halt_reason_t;

typedef struct
{
    rv_target_halt_reason_t reason;
    uint32_t addr;
} rv_target_halt_info_t;

typedef enum {
    TARGET_INTERFACE_JTAG,
    TARGET_INTERFACE_CJTAG,
    TARGET_INTERFACE_TWDI,
    TARGET_INTERFACE_NW,
    TARGET_INTERFACE_MAX,
} rv_target_interface_t;

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

void rv_target_get_error(const char **str, uint32_t* pc);
void rv_target_init(void);
void rv_target_deinit(void);
uint32_t rv_target_misa(void);
uint32_t rv_target_mxl(void);
uint64_t rv_target_vlenb(void);
void rv_target_set_interface(rv_target_interface_t interface);
void rv_target_init_post(rv_target_error_t *err);
void rv_target_init_after_halted(rv_target_error_t *err);
void rv_target_fini_pre(void);
void rv_target_read_core_registers(void *regs);
void rv_target_write_core_registers(void *regs);
void rv_target_read_register(void *reg, uint32_t regno);
void rv_target_write_register(void *reg, uint32_t regno);
void rv_target_read_memory(uint8_t *mem, uint64_t addr, uint32_t len);
void rv_target_write_memory(const uint8_t *mem, uint64_t addr, uint32_t len);
void rv_target_reset(void);
void rv_target_halt(void);
void rv_target_halt_check(rv_target_halt_info_t *halt_info);
void rv_target_resume(void);
void rv_target_step(void);
void rv_target_insert_breakpoint(rv_target_breakpoint_type_t type, uint64_t addr, uint32_t kind, uint32_t *err);
void rv_target_remove_breakpoint(rv_target_breakpoint_type_t type, uint64_t addr, uint32_t kind, uint32_t *err);
void rv_target_flash_erase(uint32_t addr, uint32_t len, uint32_t *err);
void rv_target_flash_write(uint32_t addr, uint32_t len, uint8_t *buffer, uint32_t *err);
void rv_target_flash_done(void);

/*===============================DM============================================*/
/*==== debug module register ====*/
typedef uint32_t rv_dm_reg_t;
#define RV_DM_ABSTRACT_DATA0                    (0x04)/* Abstract Data 0 (data0) */
#define RV_DM_ABSTRACT_DATA1                    (0x05)/* Abstract Data 1 (data1) */
#define RV_DM_ABSTRACT_DATA2                    (0x06)/* Abstract Data 2 (data2) */
#define RV_DM_ABSTRACT_DATA3                    (0x07)/* Abstract Data 3 (data3) */
#define RV_DM_ABSTRACT_DATA4                    (0x08)/* Abstract Data 4 (data4) */
#define RV_DM_ABSTRACT_DATA5                    (0x09)/* Abstract Data 5 (data5) */
#define RV_DM_ABSTRACT_DATA6                    (0x0A)/* Abstract Data 6 (data6) */
#define RV_DM_ABSTRACT_DATA7                    (0x0B)/* Abstract Data 7 (data7) */
#define RV_DM_ABSTRACT_DATA8                    (0x0C)/* Abstract Data 8 (data8) */
#define RV_DM_ABSTRACT_DATA9                    (0x0D)/* Abstract Data 9 (data9) */
#define RV_DM_ABSTRACT_DATA10                   (0x0E)/* Abstract Data 10 (data10) */
#define RV_DM_ABSTRACT_DATA11                   (0x0F)/* Abstract Data 11 (data11) */
#define RV_DM_DEBUG_MODULE_CONTROL              (0x10)/* Debug Module Control (dmcontrol) */
#define RV_DM_DEBUG_MODULE_STATUS               (0x11)/* Debug Module Status (dmstatus) */
#define RV_DM_HALT_INFO                         (0x12)/* Hart Info (hartinfo) */
#define RV_DM_HALT_SUMMARY1                     (0x13)/* Halt Summary 1 (haltsum1) */
#define RV_DM_HALT_ARRAY_WINDOW_SELECT          (0x14)/* Hart Array Window Select (hawindowsel) */
#define RV_DM_HALT_ARRAY_WINDOW                 (0x15)/* Hart Array Window (hawindow) */
#define RV_DM_ABSTRACT_CONTROL_AND_STATUS       (0x16)/* Abstract Control and Status (abstractcs)  */
#define RV_DM_ABSTRACT_COMMAND                  (0x17)/* Abstract Command (command) */
#define RV_DM_ABSTRACT_COMMAND_AUTOEXEC         (0x18)/* Abstract Command Autoexec (abstractauto) */
#define RV_DM_CONFIGURATION_STRING_POINTER0     (0x19)/* Configuration String Pointer 0 (confstrptr0) */
#define RV_DM_CONFIGURATION_STRING_POINTER1     (0x1A)/* Configuration String Pointer 1 (confstrptr1) */
#define RV_DM_CONFIGURATION_STRING_POINTER2     (0x1B)/* Configuration String Pointer 2 (confstrptr2) */
#define RV_DM_CONFIGURATION_STRING_POINTER3     (0x1C)/* Configuration String Pointer 3 (confstrptr3) */
#define RV_DM_NEXT_DEBUG_MODULE                 (0x1D)/* Next Debug Module (nextdm) */
#define RV_DM_PROGRAM_BUFFER0                   (0x20)/* Program Buffer 0 (progbuf0) */
#define RV_DM_PROGRAM_BUFFER1                   (0x21)/* Program Buffer 1 (progbuf1) */
#define RV_DM_PROGRAM_BUFFER2                   (0x22)/* Program Buffer 2 (progbuf2) */
#define RV_DM_PROGRAM_BUFFER3                   (0x23)/* Program Buffer 3 (progbuf3) */
#define RV_DM_PROGRAM_BUFFER4                   (0x24)/* Program Buffer 4 (progbuf4) */
#define RV_DM_PROGRAM_BUFFER5                   (0x25)/* Program Buffer 5 (progbuf5) */
#define RV_DM_PROGRAM_BUFFER6                   (0x26)/* Program Buffer 6 (progbuf6) */
#define RV_DM_PROGRAM_BUFFER7                   (0x27)/* Program Buffer 7 (progbuf7) */
#define RV_DM_PROGRAM_BUFFER8                   (0x28)/* Program Buffer 8 (progbuf8) */
#define RV_DM_PROGRAM_BUFFER9                   (0x29)/* Program Buffer 9 (progbuf9) */
#define RV_DM_PROGRAM_BUFFER10                  (0x2A)/* Program Buffer 10 (progbuf10) */
#define RV_DM_PROGRAM_BUFFER11                  (0x2B)/* Program Buffer 11 (progbuf11) */
#define RV_DM_PROGRAM_BUFFER12                  (0x2C)/* Program Buffer 12 (progbuf12) */
#define RV_DM_PROGRAM_BUFFER13                  (0x2D)/* Program Buffer 13 (progbuf13) */
#define RV_DM_PROGRAM_BUFFER14                  (0x2E)/* Program Buffer 14 (progbuf14) */
#define RV_DM_PROGRAM_BUFFER15                  (0x2F)/* Program Buffer 15 (progbuf15) */
#define RV_DM_AUTHENTICATION_DATA               (0x30)/* Authentication Data (authdata) */
#define RV_DM_HALT_SUMMARY2                     (0x34)/* Halt Summary 2 (haltsum2) */
#define RV_DM_HALT_SUMMARY3                     (0x35)/* Halt Summary 3 (haltsum3) */
#define RV_DM_SYSTEM_BUS_ADDRESS3               (0x37)/* System Bus Address 127:96 (sbaddress3) */
#define RV_DM_ACCESS_CONTROL_AND_STATUS         (0x38)/* System Bus Access Control and Status (sbcs) */
#define RV_DM_SYSTEM_BUS_ADDRESS0               (0x39)/* System Bus Address 31:0 (sbaddress0) */
#define RV_DM_SYSTEM_BUS_ADDRESS1               (0x3A)/* System Bus Address 63:32 (sbaddress1) */
#define RV_DM_SYSTEM_BUS_ADDRESS2               (0x3B)/* System Bus Address 95:64 (sbaddress2) */
#define RV_DM_SYSTEM_BUS_DATA0                  (0x3C)/* System Bus Data 31:0 (sbdata0) */
#define RV_DM_SYSTEM_BUS_DATA1                  (0x3D)/* System Bus Data 63:32 (sbdata1) */
#define RV_DM_SYSTEM_BUS_DATA2                  (0x3E)/* System Bus Data 95:64 (sbdata2) */
#define RV_DM_SYSTEM_BUS_DATA3                  (0x3F)/* System Bus Data 127:96 (sbdata3) */
#define RV_DM_HALT_SUMMARY0                     (0x40)/* Halt Summary 0 (haltsum0) */

typedef union {
    rv_dm_reg_t value;
    struct {
        rv_dm_reg_t dmactive: 1;
        rv_dm_reg_t ndmreset: 1;
        rv_dm_reg_t clrresethaltreq: 1;
        rv_dm_reg_t setresethaltreq: 1;
        rv_dm_reg_t reserved4: 2;
        rv_dm_reg_t hartselhi: 10;
        rv_dm_reg_t hartsello: 10;
        rv_dm_reg_t hasel: 1;
        rv_dm_reg_t reserved27: 1;
        rv_dm_reg_t ackhavereset: 1;
        rv_dm_reg_t hartreset: 1;
        rv_dm_reg_t resumereq: 1;
        rv_dm_reg_t haltreq: 1;
    };
} rv_debug_module_control_t;

typedef union {
    rv_dm_reg_t value;
    struct {
        rv_dm_reg_t version: 4;
        rv_dm_reg_t confstrptrvalid: 1;
        rv_dm_reg_t hasresethaltreq: 1;
        rv_dm_reg_t authbusy: 1;
        rv_dm_reg_t authenticated: 1;
        rv_dm_reg_t anyhalted: 1;
        rv_dm_reg_t allhalted: 1;
        rv_dm_reg_t anyrunning: 1;
        rv_dm_reg_t allrunning: 1;
        rv_dm_reg_t anyunavail: 1;
        rv_dm_reg_t allunavail: 1;
        rv_dm_reg_t anynonexistent: 1;
        rv_dm_reg_t allnonexistent: 1;
        rv_dm_reg_t anyresumeack: 1;
        rv_dm_reg_t allresumeack: 1;
        rv_dm_reg_t anyhavereset: 1;
        rv_dm_reg_t allhavereset: 1;
        rv_dm_reg_t reserved20: 2;
        rv_dm_reg_t impebreak: 1;
        rv_dm_reg_t reserved23: 9;
    };
} rv_debug_module_status_t;

typedef union {
    rv_dm_reg_t value;
    struct {
        rv_dm_reg_t dataaddr: 12;
        rv_dm_reg_t datasize: 4;
        rv_dm_reg_t dataaccess: 1;
        rv_dm_reg_t reserved17: 3;
        rv_dm_reg_t nscratch: 4;
        rv_dm_reg_t reserved24: 8;
    };
} rv_hart_info_t;

typedef union {
    rv_dm_reg_t value;
    struct {
        rv_dm_reg_t hawindowsel: 15;
        rv_dm_reg_t reserved15: 17;
    };
} rv_halt_array_window_select_t;

typedef union {
    rv_dm_reg_t value;
    struct {
        rv_dm_reg_t datacount: 4;
        rv_dm_reg_t reserved4: 4;
        rv_dm_reg_t cmderr: 3;
        rv_dm_reg_t reserved11: 1;
        rv_dm_reg_t busy: 1;
        rv_dm_reg_t reserved13: 11;
        rv_dm_reg_t progbufsize: 5;
        rv_dm_reg_t reserved29: 3;
    };
} rv_abstract_control_and_status_t;

typedef union {
    rv_dm_reg_t value;
    union {
        struct {
            rv_dm_reg_t regno: 16;
            rv_dm_reg_t write: 1;
            rv_dm_reg_t transfer: 1;
            rv_dm_reg_t postexec: 1;
            rv_dm_reg_t aarpostincrement: 1;
            rv_dm_reg_t aarsize: 3;
            rv_dm_reg_t reserved23: 1;
            rv_dm_reg_t cmdtype: 8;
        } reg;
        struct {
            rv_dm_reg_t reserved0: 14;
            rv_dm_reg_t target_specific: 2;
            rv_dm_reg_t write: 1;
            rv_dm_reg_t reserved17: 2;
            rv_dm_reg_t aampostincrement: 1;
            rv_dm_reg_t aamsize: 3;
            rv_dm_reg_t aamvirtual: 1;
            rv_dm_reg_t cmdtype: 8;
        } mem;
    };
} rv_abstract_command_t;

typedef union {
    rv_dm_reg_t value;
    struct {
        rv_dm_reg_t autoexecdata: 12;
        rv_dm_reg_t reserved12: 4;
        rv_dm_reg_t autoexecprogbuf: 16;
    };
} rv_abstract_command_autoexec_t;

typedef union {
    rv_dm_reg_t value;
    struct {
        rv_dm_reg_t sbaccess8: 1;
        rv_dm_reg_t sbaccess16: 1;
        rv_dm_reg_t sbaccess32: 1;
        rv_dm_reg_t sbaccess64: 1;
        rv_dm_reg_t sbaccess128: 1;
        rv_dm_reg_t sbasize: 7;
        rv_dm_reg_t sberror: 3;
        rv_dm_reg_t sbreadondata: 1;
        rv_dm_reg_t sbautoincrement: 1;
        rv_dm_reg_t sbaccess: 3;
        rv_dm_reg_t sbreadonaddr: 1;
        rv_dm_reg_t sbbusy: 1;
        rv_dm_reg_t sbbusyerror: 1;
        rv_dm_reg_t reserved23: 6;
        rv_dm_reg_t sbversion: 3;
    };
} rv_system_bus_access_control_and_status_t;

typedef struct {
    /*==== debug module register ====*/
    rv_dm_reg_t data[12];
    rv_debug_module_control_t dmcontrol;
    rv_debug_module_status_t dmstatus;
    rv_hart_info_t hartinfo;
    rv_dm_reg_t haltsum1;
    rv_halt_array_window_select_t hawindowsel;
    rv_dm_reg_t hawindow;
    rv_abstract_control_and_status_t abstractcs;
    rv_abstract_command_t command;
    rv_abstract_command_autoexec_t abstractauto;
    rv_dm_reg_t confstrptr[4];
    rv_dm_reg_t nextdm;
    rv_dm_reg_t progbuf[16];
    rv_dm_reg_t authdata;
    rv_dm_reg_t haltsum2;
    rv_dm_reg_t haltsum3;
    rv_dm_reg_t sbaddress3;
    rv_system_bus_access_control_and_status_t sbcs;
    rv_dm_reg_t sbaddress[3];
    rv_dm_reg_t sbdata[4];
    rv_dm_reg_t haltsum0;
} rv_dm_t;

/*====================================TR32=======================================*/
/*==== trigger register ====*/
typedef uint32_t rv_tr32_reg_t;
typedef union {
    rv_tr32_reg_t value;
    union {
        /* mcontrol while type=2 */
        struct {
            rv_tr32_reg_t load: 1;
            rv_tr32_reg_t store: 1;
            rv_tr32_reg_t execute: 1;
            rv_tr32_reg_t u: 1;
            rv_tr32_reg_t s: 1;
            rv_tr32_reg_t reserved5: 1;
            rv_tr32_reg_t m: 1;
            rv_tr32_reg_t match: 4;
            rv_tr32_reg_t chain: 1;
            rv_tr32_reg_t action: 4;
            rv_tr32_reg_t sizelo: 2;
            rv_tr32_reg_t timing: 1;
            rv_tr32_reg_t select: 1;
            rv_tr32_reg_t hit: 1;
            rv_tr32_reg_t maskmax: 6;
            rv_tr32_reg_t dmode: 1;
            rv_tr32_reg_t type: 4;
        } mc;
        /* icount while type=2 */
        struct {
            rv_tr32_reg_t action: 6;
            rv_tr32_reg_t u: 1;
            rv_tr32_reg_t s: 1;
            rv_tr32_reg_t reserved8: 1;
            rv_tr32_reg_t m: 1;
            rv_tr32_reg_t count: 14;
            rv_tr32_reg_t hit: 1;
            rv_tr32_reg_t reserved25: (32 - 30);
            rv_tr32_reg_t dmode: 1;
            rv_tr32_reg_t type: 4;
        } ic;
        /* itrigger while type=4 
            etrigger while type=5 */
        struct {
            rv_tr32_reg_t action: 6;
            rv_tr32_reg_t u: 1;
            rv_tr32_reg_t s: 1;
            rv_tr32_reg_t reserved8: 1;
            rv_tr32_reg_t m: 1;
            rv_tr32_reg_t reserved10: (32 - 16);
            rv_tr32_reg_t hit: 1;
            rv_tr32_reg_t dmode: 1;
            rv_tr32_reg_t type: 4;
        } iet;
    };
} rv_trigger32_data1_t;

typedef union {
    rv_tr32_reg_t value;
    /* textra32 while type=2/3/4/5 */
    struct {
        rv_tr32_reg_t sselect: 2;
        rv_tr32_reg_t svalue: 16;
        rv_tr32_reg_t mc_reserved18: 7;
        rv_tr32_reg_t mselect: 1;
        rv_tr32_reg_t mvalue: 6;
    };
} rv_trigger32_data3_t;

typedef union {
    rv_tr32_reg_t value;
    struct {
        rv_tr32_reg_t info: 16;
        rv_tr32_reg_t reserved16: (32 - 16);
    };
} rv_tinfo32_t;

typedef union {
    rv_tr32_reg_t value;
    struct {
        rv_tr32_reg_t reserved0: 3;
        rv_tr32_reg_t mte: 1;
        rv_tr32_reg_t reserved4: 3;
        rv_tr32_reg_t mpte: 1;
        rv_tr32_reg_t reserved16: (32 - 8);
    };
} rv_tcontrol32_t;

typedef struct {
    /*==== trigger register ====*/
    rv_tr32_reg_t tselect;
    rv_trigger32_data1_t tdata1;
    rv_tr32_reg_t tdata2;
    rv_trigger32_data3_t tdata3;
    rv_tinfo32_t tinfo;
    rv_tcontrol32_t tcontrol;
    rv_tr32_reg_t mcontext;
    rv_tr32_reg_t scontext;
} rv_tr32_t;

/*====================================TR64=======================================*/
/*==== trigger register ====*/
typedef uint64_t rv_tr64_reg_t;
typedef union {
    rv_tr64_reg_t value;
    union {
        /* mcontrol while type=2 */
        struct {
            rv_tr64_reg_t load: 1;
            rv_tr64_reg_t store: 1;
            rv_tr64_reg_t execute: 1;
            rv_tr64_reg_t u: 1;
            rv_tr64_reg_t s: 1;
            rv_tr64_reg_t reserved5: 1;
            rv_tr64_reg_t m: 1;
            rv_tr64_reg_t match: 4;
            rv_tr64_reg_t chain: 1;
            rv_tr64_reg_t action: 4;
            rv_tr64_reg_t sizelo: 2;
            rv_tr64_reg_t timing: 1;
            rv_tr64_reg_t select: 1;
            rv_tr64_reg_t hit: 1;
            rv_tr64_reg_t sizehi: 2;
            rv_tr64_reg_t reserved23: (64 - 34);
            rv_tr64_reg_t maskmax: 6;
            rv_tr64_reg_t dmode: 1;
            rv_tr64_reg_t type: 4;
        } mc;
        /* icount while type=2 */
        struct {
            rv_tr64_reg_t action: 6;
            rv_tr64_reg_t u: 1;
            rv_tr64_reg_t s: 1;
            rv_tr64_reg_t reserved8: 1;
            rv_tr64_reg_t m: 1;
            rv_tr64_reg_t count: 14;
            rv_tr64_reg_t hit: 1;
            rv_tr64_reg_t reserved25: (64 - 30);
            rv_tr64_reg_t dmode: 1;
            rv_tr64_reg_t type: 4;
        } ic;
        /* itrigger while type=4 
            etrigger while type=5 */
        struct {
            rv_tr64_reg_t action: 6;
            rv_tr64_reg_t u: 1;
            rv_tr64_reg_t s: 1;
            rv_tr64_reg_t reserved8: 1;
            rv_tr64_reg_t m: 1;
            rv_tr64_reg_t reserved10: (64 - 16);
            rv_tr64_reg_t hit: 1;
            rv_tr64_reg_t dmode: 1;
            rv_tr64_reg_t type: 4;
        } iet;
    };
} rv_trigger64_data1_t;

typedef union {
    rv_tr64_reg_t value;
    /* textra64 while type=2/3/4/5 */
    struct {
        rv_tr64_reg_t sselect: 2;
        rv_tr64_reg_t svalue: 34;
        rv_tr64_reg_t mc_reserved36: 14;
        rv_tr64_reg_t mselect: 1;
        rv_tr64_reg_t mvalue: 13;
    };
} rv_trigger64_data3_t;

typedef union {
    rv_tr64_reg_t value;
    struct {
        rv_tr64_reg_t info: 16;
        rv_tr64_reg_t reserved16: (64 - 16);
    };
} rv_tinfo64_t;

typedef union {
    rv_tr64_reg_t value;
    struct {
        rv_tr64_reg_t reserved0: 3;
        rv_tr64_reg_t mte: 1;
        rv_tr64_reg_t reserved4: 3;
        rv_tr64_reg_t mpte: 1;
        rv_tr64_reg_t reserved16: (64 - 8);
    };
} rv_tcontrol64_t;

typedef struct {
    /*==== trigger register ====*/
    rv_tr64_reg_t tselect;
    rv_trigger64_data1_t tdata1;
    rv_tr64_reg_t tdata2;
    rv_trigger64_data3_t tdata3;
    rv_tinfo64_t tinfo;
    rv_tcontrol64_t tcontrol;
    rv_tr64_reg_t mcontext;
    rv_tr64_reg_t scontext;
} rv_tr64_t;

/*================================DTM===========================================*/
/*==== debug transport module register ====*/
typedef uint32_t rv_dtm_reg_t;
typedef union {
    rv_dtm_reg_t value;
    struct {
        rv_dtm_reg_t reserved0: 1;
        rv_dtm_reg_t manufld: 11;
        rv_dtm_reg_t partnumber: 16;
        rv_dtm_reg_t version: 4;
    };
} rv_idcode_t;

typedef union {
    rv_dtm_reg_t value;
    struct {
        rv_dtm_reg_t version: 4;
        rv_dtm_reg_t abits: 6;
        rv_dtm_reg_t dmistat: 2;
        rv_dtm_reg_t idle: 3;
        rv_dtm_reg_t reserved15: 1;
        rv_dtm_reg_t dmireset: 1;
        rv_dtm_reg_t dmihardreset: 1;
        rv_dtm_reg_t reserved18: 14;
    };
} rv_dtm_control_and_status_t;

typedef struct {
    /*==== debug transport module register ====*/
    rv_idcode_t idcode;
    rv_dtm_control_and_status_t dtmcs;
} rv_dtm_t;

/*==================================DMI=========================================*/
/*==== debug module interface access register ====*/
typedef uint32_t rv_dmi_reg_t;
typedef struct {
    rv_dmi_reg_t op;
    rv_dmi_reg_t data;
    rv_dmi_reg_t address;
} rv_dmi_t;
