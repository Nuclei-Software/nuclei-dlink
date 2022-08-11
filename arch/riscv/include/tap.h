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

#ifndef __TAP_H__
#define __TAP_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"

void rvl_tap_init(void);
void rvl_tap_fini(void);
void rvl_tap_shift(uint32_t* old, uint32_t *new, size_t len, uint8_t pre, uint8_t post);
void rvl_tap_shift_ir(uint32_t* old_ir, uint32_t* new_ir, size_t ir_len);
void rvl_tap_shift_dr(uint32_t* old_dr, uint32_t* new_dr, size_t dr_len);
void rvl_tap_go_idle(void);
void rvl_tap_run(uint32_t ticks);

#ifdef RVL_TARGET_CONFIG_TAP_DYN
void rvl_tap_config(uint8_t ir_pre, uint8_t ir_post, uint8_t dr_pre, uint8_t dr_post);
#endif /* RVL_TARGET_CONFIG_TAP_DYN */

#ifdef __cplusplus
}
#endif

#endif /* __TAP_H__ */
