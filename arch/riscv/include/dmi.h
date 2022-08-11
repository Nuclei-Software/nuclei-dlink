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

#ifndef __DMI_H__
#define __DMI_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"

/*
 * Debug Module Interface (DMI)
 */

#define RISCV_DMI_OP_NOP        0
#define RISCV_DMI_OP_READ       1
#define RISCV_DMI_OP_WRITE      2

#define RISCV_DMI_RESULT_DONE   0
#define RISCV_DMI_RESULT_FAIL   2
#define RISCV_DMI_RESULT_BUSY   3

#ifdef RISCV_DEBUG_SPEC_VERSION_V0P13
    typedef uint32_t rvl_dmi_reg_t;
#else
    typedef uint64_t rvl_dmi_reg_t;
#endif

void rvl_dmi_init(void);
void rvl_dmi_fini(void);
void rvl_dmi_nop(void);
void rvl_dmi_read(uint32_t addr, rvl_dmi_reg_t* data, uint32_t *result);
void rvl_dmi_write(uint32_t addr, rvl_dmi_reg_t data, uint32_t *result);
void rvl_dmi_read_vector(uint32_t start_addr, rvl_dmi_reg_t* buffer, uint32_t len, uint32_t *result);
void rvl_dmi_write_vector(uint32_t start_addr, const rvl_dmi_reg_t* buffer, uint32_t len, uint32_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __DMI_H__ */
