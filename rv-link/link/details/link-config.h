#ifndef __RV_LINK_LINK_DETAILS_LINK_CONFIG_H__
#define __RV_LINK_LINK_DETAILS_LINK_CONFIG_H__
/**
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 *
 * \file  rv-link/link/details/link-config.h
 * \brief Top level link configuration header file for supported boards.
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

/* other project header file includes */
#if defined(RVL_LINK_GD32VF103C_START)
#include <rv-link/link/arch/gd32vf103/gd32vf103c-start/link-config.h>
#elif defined(RVL_LINK_LONGAN_NANO)
#include <rv-link/link/arch/gd32vf103/longan-nano/link-config.h>
#elif defined(RVL_LINK_LONGAN_NANO_ALT)
#include <rv-link/link/arch/gd32vf103/longan-nano-alt/link-config.h>
#elif defined(RVL_LINK_RVL_PROBE)
#include <rv-link/link/arch/gd32vf103/rvl-probe/link-config.h>
#elif defined(RVL_LINK_RVSTAR)
#include <rv-link/link/arch/gd32vf103/rvstar/link-config.h>
#else
#error No RVL_LINK_X defined
#endif

#endif /* __RV_LINK_LINK_DETAILS_LINK_CONFIG_H__ */
