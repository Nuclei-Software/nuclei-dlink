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
#include "flash.h"

/* Register offsets */
#define NUSPI_REG_SCKMODE           (0x04)
#define NUSPI_REG_FORCE             (0x0C)
#define NUSPI_REG_VERSION           (0x1C)
#define NUSPI_REG_CSMODE            (0x18)
#define NUSPI_REG_FMT               (0x40)
#define NUSPI_REG_TXDATA            (0x48)
#define NUSPI_REG_RXDATA            (0x4C)
#define NUSPI_REG_FCTRL             (0x60)
#define NUSPI_REG_FFMT              (0x64)
#define NUSPI_REG_IP                (0x74)
#define NUSPI_REG_STATUS            (0x7C)
#define NUSPI_REG_RXEDGE            (0x80)
/* IP register */
#define NUSPI_IP_TXWM               (0x1)
/* FCTRL register */
#define NUSPI_FCTRL_EN              (0x1)
#define NUSPI_INSN_CMD_EN           (0x1)
/* FMT register */
#define NUSPI_FMT_DIR(x)            (((x) & 0x1) << 3)
/* STATUS register */
#define NUSPI_STAT_BUSY             (0x1 << 0)
#define NUSPI_STAT_TXFULL           (0x1 << 4)
#define NUSPI_STAT_RXEMPTY          (0x1 << 5)
#define NUSPI_CSMODE_AUTO           (0)
#define NUSPI_CSMODE_HOLD           (2)
#define NUSPI_DIR_RX                (0)
#define NUSPI_DIR_TX                (1)

#define NUSPI_TX_TIMES_OUT          (500)
#define NUSPI_RX_TIMES_OUT          (500)

/*==== FLASH ====*/
#define SPIFLASH_BSY            0
#define SPIFLASH_BSY_BIT        (1 << SPIFLASH_BSY) /* WIP Bit of SPI SR */
/* SPI Flash Commands */
#define SPIFLASH_READ_ID        0x9F /* Read Flash Identification */
#define SPIFLASH_READ_STATUS    0x05 /* Read Status Register */
#define SPIFLASH_WRITE_ENABLE   0x06 /* Write Enable */
#define SPIFLASH_PAGE_PROGRAM   0x02 /* Page Program */
#define SPIFLASH_PAGE_SIZE      0x100
#define SPIFLASH_READ           0x03 /* Normal Read */
#define SPIFLASH_BLOCK_ERASE    0xD8 /* Block Erase */
#define SPIFLASH_BLOCK_SIZE     0x10000

static volatile uint8_t is_nuspi = 0;

static inline void nuspi_read_reg(uint32_t nuspi_addr, uint32_t offset, uint32_t *value)
{
    rv_target_read_memory((uint8_t*)value, nuspi_addr + offset, 4);
}

static inline void nuspi_write_reg(uint32_t nuspi_addr, uint32_t offset, uint32_t value)
{
    rv_target_write_memory((const uint8_t*)&value, nuspi_addr + offset, 4);
}

static inline void nuspi_set_dir(uint32_t nuspi_addr, uint32_t dir)
{
    uint32_t fmt = 0;
    nuspi_read_reg(nuspi_addr, NUSPI_REG_FMT, &fmt);
    nuspi_write_reg(nuspi_addr, NUSPI_REG_FMT, (fmt & ~(NUSPI_FMT_DIR(0xFFFFFFFF))) | NUSPI_FMT_DIR(dir));
}

void nuspi_init(uint32_t nuspi_addr)
{
    uint32_t temp = 0;
    nuspi_read_reg(nuspi_addr, NUSPI_REG_VERSION, &temp);
    if (temp >= 0x10100) {
        is_nuspi = 1;
    }
    /* clear rxfifo */
    while (1) {
        if (is_nuspi) {
            nuspi_read_reg(nuspi_addr, NUSPI_REG_RXDATA, &temp);
            nuspi_read_reg(nuspi_addr, NUSPI_REG_STATUS, &temp);
            if (temp & NUSPI_STAT_RXEMPTY) {
                break;
            }
        } else {
            nuspi_read_reg(nuspi_addr, NUSPI_REG_RXDATA, &temp);
            if (temp >> 31) {
                break;
            }
        }
    }
    /* init register */
    nuspi_write_reg(nuspi_addr, NUSPI_REG_SCKMODE, 0x0);
    nuspi_write_reg(nuspi_addr, NUSPI_REG_FORCE, 0x3);
    nuspi_write_reg(nuspi_addr, NUSPI_REG_FCTRL, 0x0);
    nuspi_write_reg(nuspi_addr, NUSPI_REG_FMT, 0x80008);
    nuspi_write_reg(nuspi_addr, NUSPI_REG_FFMT, 0x30007);
    nuspi_write_reg(nuspi_addr, NUSPI_REG_RXEDGE, 0x0);
}

void nuspi_hw(uint32_t nuspi_addr, bool sel)
{
    uint32_t fctrl = 0;
    nuspi_read_reg(nuspi_addr, NUSPI_REG_FCTRL, &fctrl);
    if (sel) {
        nuspi_write_reg(nuspi_addr, NUSPI_REG_FCTRL, fctrl | NUSPI_FCTRL_EN);
    } else {
        nuspi_write_reg(nuspi_addr, NUSPI_REG_FCTRL, fctrl & ~NUSPI_FCTRL_EN);
    }
}

void nuspi_cs(uint32_t nuspi_addr, bool sel)
{
    if (sel) {
        nuspi_write_reg(nuspi_addr, NUSPI_REG_CSMODE, NUSPI_CSMODE_HOLD);
    } else {
        nuspi_write_reg(nuspi_addr, NUSPI_REG_CSMODE, NUSPI_CSMODE_AUTO);
    }
}

int nuspi_tx(uint32_t nuspi_addr, uint8_t *in, uint32_t len)
{
    uint32_t times_out = 0;
    uint32_t value = 0;
    nuspi_set_dir(nuspi_addr, NUSPI_DIR_TX);
    for (int i = 0;i < len;i++) {
        times_out = NUSPI_TX_TIMES_OUT;
        while (times_out--) {
            if (is_nuspi) {
                nuspi_read_reg(nuspi_addr, NUSPI_REG_STATUS, &value);
                if (!(value & NUSPI_STAT_TXFULL)) {
                    break;
                }
            } else {
                nuspi_read_reg(nuspi_addr, NUSPI_REG_TXDATA, &value);
                if (!(value >> 31)) {
                    break;
                }
            }
        }
        if (0 >= times_out) {
            return RETURN_SPI_TX_ERROR;
        }
        nuspi_write_reg(nuspi_addr, NUSPI_REG_TXDATA, in[i]);
    }
    times_out = NUSPI_TX_TIMES_OUT;
    while (times_out--) {
        if (is_nuspi) {
            nuspi_read_reg(nuspi_addr, NUSPI_REG_STATUS, &value);
            if (0 == (value & NUSPI_STAT_BUSY)) {
                break;
            }
        } else {
            nuspi_read_reg(nuspi_addr, NUSPI_REG_IP, &value);
            if (value & NUSPI_IP_TXWM) {
                break;
            }
        }
    }
    if (0 >= times_out) {
        return RETURN_SPI_TX_ERROR;
    }
    return 0;
}

int nuspi_rx(uint32_t nuspi_addr, uint8_t *out, uint32_t len)
{
    uint32_t times_out = 0;
    uint32_t value = 0;
    nuspi_set_dir(nuspi_addr, NUSPI_DIR_RX);
    for (int i = 0;i < len;i++) {
        times_out = NUSPI_RX_TIMES_OUT;
        nuspi_write_reg(nuspi_addr, NUSPI_REG_TXDATA, 0x00);
        while (times_out--) {
            if (is_nuspi) {
                nuspi_read_reg(nuspi_addr, NUSPI_REG_STATUS, &value);
                if (!(value & NUSPI_STAT_RXEMPTY)) {
                    break;
                }
            } else {
                nuspi_read_reg(nuspi_addr, NUSPI_REG_RXDATA, &value);
                if (!(value >> 31)) {
                    break;
                }
            }
        }
        if (0 >= times_out) {
            return RETURN_SPI_RX_ERROR;
        }
        if (is_nuspi) {
            nuspi_read_reg(nuspi_addr, NUSPI_REG_RXDATA, &value);
        }
        out[i] = value & 0xff;
    }
    return 0;
}

static inline int flash_wip(uint32_t nuspi_addr)
{
    int retval = 0;
    uint8_t value = 0;
    /* read status */
    nuspi_cs(nuspi_addr, true);
    value = SPIFLASH_READ_STATUS;
    retval |= nuspi_tx(nuspi_addr, &value, 1);
    while (1) {
        retval |= nuspi_rx(nuspi_addr, &value, 1);
        if ((value & SPIFLASH_BSY_BIT) == 0) {
            break;
        }
    }
    nuspi_cs(nuspi_addr, false);
    if (retval) {
        return retval | RETURN_FLASH_WIP_ERROR;
    }
    return retval;
}

int flash_init(uint32_t nuspi_addr)
{
    int retval = 0;
    uint8_t value[3] = {0};
    nuspi_init(nuspi_addr);
    /* read flash id */
    nuspi_hw(nuspi_addr, false);
    nuspi_cs(nuspi_addr, true);
    value[0] = SPIFLASH_READ_ID;
    retval |= nuspi_tx(nuspi_addr, value, 1);
    retval |= nuspi_rx(nuspi_addr, value, 3);
    nuspi_cs(nuspi_addr, false);
    nuspi_hw(nuspi_addr, true);
    retval = (value[2] << 16) | (value[1] << 8) | value[0];
    return retval;
}

int flash_erase(uint32_t nuspi_addr, uint32_t start_addr, uint32_t end_addr)
{
    int retval = 0;
    uint8_t value[4] = {0};
    uint32_t curr_addr = start_addr;
    uint32_t sector_num = (end_addr - start_addr) / SPIFLASH_BLOCK_SIZE;
    /* erase flash */
    nuspi_hw(nuspi_addr, false);
    for (int i = 0;i < sector_num;i++) {
        /* send write enable cmd */
        nuspi_cs(nuspi_addr, true);
        value[0] = SPIFLASH_WRITE_ENABLE;
        retval |= nuspi_tx(nuspi_addr, value, 1);
        nuspi_cs(nuspi_addr, false);
        /* send erase cmd and addr*/
        nuspi_cs(nuspi_addr, true);
        value[0] = SPIFLASH_BLOCK_ERASE;
        value[1] = curr_addr >> 16;
        value[2] = curr_addr >> 8;
        value[3] = curr_addr >> 0;
        retval |= nuspi_tx(nuspi_addr, value, 4);
        nuspi_cs(nuspi_addr, false);
        retval |= flash_wip(nuspi_addr);
        curr_addr += SPIFLASH_BLOCK_SIZE;
    }
    nuspi_hw(nuspi_addr, true);
    if (retval) {
        return retval | RETURN_FLASH_ERASE_ERROR;
    }
    return retval;
}

int flash_write(uint32_t nuspi_addr, uint8_t* buffer, uint32_t offset, uint32_t count)
{
    int retval = 0;
    uint8_t value[4] = {0};
    uint32_t cur_offset = offset % SPIFLASH_PAGE_SIZE;
    uint32_t cur_count = 0;
    /* write flash */
    nuspi_hw(nuspi_addr, false);
    while (count > 0) {
        if ((cur_offset + count) >= SPIFLASH_PAGE_SIZE) {
            cur_count = SPIFLASH_PAGE_SIZE - cur_offset;
        } else {
            cur_count = count;
        }
        cur_offset = 0;
        /* send write enable cmd */
        nuspi_cs(nuspi_addr, true);
        value[0] = SPIFLASH_WRITE_ENABLE;
        retval |= nuspi_tx(nuspi_addr, value, 1);
        nuspi_cs(nuspi_addr, false);
        /* send write cmd and addr*/
        nuspi_cs(nuspi_addr, true);
        value[0] = SPIFLASH_PAGE_PROGRAM;
        value[1] = offset >> 16;
        value[2] = offset >> 8;
        value[3] = offset >> 0;
        retval |= nuspi_tx(nuspi_addr, value, 4);
        retval |= nuspi_tx(nuspi_addr, buffer, cur_count);
        nuspi_cs(nuspi_addr, false);
        retval |= flash_wip(nuspi_addr);
        buffer += cur_count;
        offset += cur_count;
        count -= cur_count;
    }
    nuspi_hw(nuspi_addr, true);
    if (retval) {
        return retval | RETURN_FLASH_WRITE_ERROR;
    }
    return retval;
}

int flash_read(uint32_t nuspi_addr, uint8_t* buffer, uint32_t offset, uint32_t count)
{
    int retval = 0;
    uint8_t value[4] = {0};
    /* read flash */
    nuspi_hw(nuspi_addr, false);
    /* send read cmd and addr*/
    nuspi_cs(nuspi_addr, true);
    value[0] = SPIFLASH_READ;
    value[1] = offset >> 16;
    value[2] = offset >> 8;
    value[3] = offset >> 0;
    retval |= nuspi_tx(nuspi_addr, value, 4);
    retval |= nuspi_rx(nuspi_addr, buffer, count);
    nuspi_cs(nuspi_addr, false);
    retval |= flash_wip(nuspi_addr);
    nuspi_hw(nuspi_addr, true);
    if (retval) {
        return retval | RETURN_FLASH_READ_ERROR;
    }
    return retval;
}
