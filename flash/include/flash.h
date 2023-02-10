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

#ifndef __FLASH_H__
#define __FLASH_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "port.h"
#include "riscv-target.h"

#define RETURN_FLASH_WIP_ERROR        (0x1 << 0)
#define RETURN_FLASH_ERASE_ERROR      (0x1 << 1)
#define RETURN_FLASH_WRITE_ERROR      (0x1 << 2)
#define RETURN_FLASH_READ_ERROR       (0x1 << 3)
#define RETURN_SPI_TX_ERROR           (0x1 << 4)
#define RETURN_SPI_RX_ERROR           (0x1 << 5)

int flash_init(uint32_t nuspi_base);
int flash_erase(uint32_t nuspi_base, uint32_t start_addr, uint32_t end_addr);
int flash_write(uint32_t nuspi_base, uint8_t* buffer, uint32_t offset, uint32_t count);
int flash_read(uint32_t nuspi_base, uint8_t* buffer, uint32_t offset, uint32_t count);

#ifdef __cplusplus
}
#endif

#endif /* __FLASH_H__ */
