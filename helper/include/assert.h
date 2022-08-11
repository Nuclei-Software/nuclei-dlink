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

#ifndef __ASSERT_H__
#define __ASSERT_H__

#ifdef __cplusplus
 extern "C" {
#endif

#include <stdint.h>

#ifdef RVL_ASSERT_EN
#define rvl_assert(cond)                                                \
do {                                                                    \
  if (!(cond)) {                                                        \
    rvl_assert_handler(#cond, __FUNCTION__, __LINE__);                  \
  }                                                                     \
} while(0)
#else
#define rvl_assert(cond) ((void)0)
#endif

void rvl_assert_handler(const char *cond, const char *func, uint32_t line);

#ifdef __cplusplus
}
#endif

#endif /* __ASSERT_H__ */
