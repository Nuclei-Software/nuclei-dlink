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

#ifndef __PORT_H__
#define __PORT_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "config.h"

void rvl_jtag_init(void);
void rvl_jtag_fini(void);
void rvl_jtag_srst_put(int srst);
void rvl_jtag_tms_put(int tms);
void rvl_jtag_tdi_put(int tdi);
void rvl_jtag_tck_put(int tck);
void rvl_jtag_tck_high_delay();
void rvl_jtag_tck_low_delay();
int rvl_jtag_tdo_get();

void rvl_transport_init(void);

#ifdef __cplusplus
}
#endif

#endif /* __PORT_H__ */
