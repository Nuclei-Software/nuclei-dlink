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

#ifndef __ASSERT_H__
#define __ASSERT_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

#ifdef RV_ASSERT_EN
#define rv_assert(cond)                                                \
do {                                                                    \
  if (!(cond)) {                                                        \
    rv_assert_handler(#cond, __FUNCTION__, __LINE__);                  \
  }                                                                     \
} while(0)
#else
#define rv_assert(cond) ((void)0)
#endif

void rv_assert_handler(const char *cond, const char *func, uint32_t line);

#ifdef __cplusplus
}
#endif

#endif /* __ASSERT_H__ */
