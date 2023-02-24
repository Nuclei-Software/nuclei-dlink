/*
 * Copyright (c) 2019 zoomdy@163.com
 * Copyright (c) 2020, Micha Hoiting <micha.hoiting@gmail.com>
 * Copyright (c) 2022 Nuclei Limited. All rights reserved.
 *
 * Dlink is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR
 * PURPOSE.
 * See the Mulan PSL v1 for more details.
 */

#ifndef __USB_CONF_H
#define __USB_CONF_H

#include "gd32vf103.h"
#include "port.h"

#include <stddef.h>

#ifdef USE_USB_FS
    #define USB_FS_CORE
#endif

#ifdef USE_USB_HS
    #define USB_HS_CORE
#endif

#ifdef USB_FS_CORE
    #define RX_FIFO_FS_SIZE                         128
    #define TX0_FIFO_FS_SIZE                        64
    #define TX1_FIFO_FS_SIZE                        128
    #define TX2_FIFO_FS_SIZE                        0
    #define TX3_FIFO_FS_SIZE                        0
#endif /* USB_FS_CORE */

#ifdef USB_HS_CORE
    #define RX_FIFO_HS_SIZE                          512
    #define TX0_FIFO_HS_SIZE                         128
    #define TX1_FIFO_HS_SIZE                         372
    #define TX2_FIFO_HS_SIZE                         0
    #define TX3_FIFO_HS_SIZE                         0
    #define TX4_FIFO_HS_SIZE                         0
    #define TX5_FIFO_HS_SIZE                         0

    #ifdef USE_ULPI_PHY
        #define USB_OTG_ULPI_PHY_ENABLED
    #endif

    #ifdef USE_EMBEDDED_PHY
        #define USB_OTG_EMBEDDED_PHY_ENABLED
    #endif

    #define USB_OTG_HS_INTERNAL_DMA_ENABLED
    #define USB_OTG_HS_DEDICATED_EP1_ENABLED
#endif /* USB_HS_CORE */

#define USB_SOF_OUTPUT              1
#define USB_LOW_POWER               1

//#define VBUS_SENSING_ENABLED

//#define USE_HOST_MODE
#define USE_DEVICE_MODE
//#define USE_OTG_MODE

#ifndef USB_FS_CORE
    #ifndef USB_HS_CORE
        #error "USB_HS_CORE or USB_FS_CORE should be defined"
    #endif
#endif

#ifndef USE_DEVICE_MODE
    #ifndef USE_HOST_MODE
        #error "USE_DEVICE_MODE or USE_HOST_MODE should be defined"
    #endif
#endif

#ifndef USE_USB_HS
    #ifndef USE_USB_FS
        #error "USE_USB_HS or USE_USB_FS should be defined"
    #endif
#endif

/****************** C Compilers dependant keywords ****************************/
/* In HS mode and when the DMA is used, all variables and data structures dealing
   with the DMA during the transaction process should be 4-bytes aligned */
#ifdef USB_OTG_HS_INTERNAL_DMA_ENABLED
    #if defined   (__GNUC__)            /* GNU Compiler */
        #define __ALIGN_END __attribute__ ((aligned(4)))
        #define __ALIGN_BEGIN
    #endif                              /* __GNUC__ */
#else
    #define __ALIGN_BEGIN
    #define __ALIGN_END
#endif /* USB_OTG_HS_INTERNAL_DMA_ENABLED */


#endif /* __USB_CONF_H */
