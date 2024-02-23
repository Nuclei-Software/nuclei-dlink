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

#ifndef CDC_ACM_CORE_H
#define CDC_ACM_CORE_H

#include "usbd_enum.h"
#include "usb_ch9_std.h"
#include "usbd_transc.h"
#include "usb-serial.h"

#define USB_DESCTYPE_CS_INTERFACE               0x24
#define USB_CDC_ACM_CONFIG_DESC_SIZE            0x43

#define CDC_ACM_DESC_SIZE                       0x3A

#define CDC_ACM_DESC_TYPE                       0x21

#define SEND_ENCAPSULATED_COMMAND               0x00
#define GET_ENCAPSULATED_RESPONSE               0x01
#define SET_COMM_FEATURE                        0x02
#define GET_COMM_FEATURE                        0x03
#define CLEAR_COMM_FEATURE                      0x04
#define SET_LINE_CODING                         0x20
#define GET_LINE_CODING                         0x21
#define SET_CONTROL_LINE_STATE                  0x22
#define SEND_BREAK                              0x23
#define NO_CMD                                  0xFF

#pragma pack(1)

typedef struct
{
    usb_desc_header header;  /*!< descriptor header, including type and size. */
    uint8_t  bDescriptorSubtype;          /*!< bDescriptorSubtype: header function descriptor */
    uint16_t  bcdCDC;                     /*!< bcdCDC: low byte of spec release number (CDC1.10) */
} usb_descriptor_header_function_struct;

typedef struct
{
    usb_desc_header header;  /*!< descriptor header, including type and size. */
    uint8_t  bDescriptorSubtype;          /*!< bDescriptorSubtype:  call management function descriptor */
    uint8_t  bmCapabilities;              /*!< bmCapabilities: D0 is reset, D1 is ignored */
    uint8_t  bDataInterface;              /*!< bDataInterface: 1 interface used for call management */
} usb_descriptor_call_managment_function_struct;

typedef struct
{
    usb_desc_header header;  /*!< descriptor header, including type and size. */
    uint8_t  bDescriptorSubtype;          /*!< bDescriptorSubtype: abstract control management desc */
    uint8_t  bmCapabilities;              /*!< bmCapabilities: D1 */
} usb_descriptor_acm_function_struct;

typedef struct
{
    usb_desc_header header;  /*!< descriptor header, including type and size. */
    uint8_t  bDescriptorSubtype;          /*!< bDescriptorSubtype: union func desc */
    uint8_t  bMasterInterface;            /*!< bMasterInterface: communication class interface */
    uint8_t  bSlaveInterface0;            /*!< bSlaveInterface0: data class interface */
} usb_descriptor_union_function_struct;

#pragma pack()

typedef struct
{
    usb_desc_itf_association                            cdc_itf_association;
    usb_desc_itf                                        cdc_loopback_interface;
    usb_descriptor_header_function_struct               cdc_loopback_header;
    usb_descriptor_call_managment_function_struct       cdc_loopback_call_managment;
    usb_descriptor_acm_function_struct                  cdc_loopback_acm;
    usb_descriptor_union_function_struct                cdc_loopback_union;
    usb_desc_ep                                         cdc_loopback_cmd_endpoint;
    usb_desc_itf                                        cdc_loopback_data_interface;
    usb_desc_ep                                         cdc_loopback_out_endpoint;
    usb_desc_ep                                         cdc_loopback_in_endpoint;
} usb_descriptor_cdc_configuration_set_struct;

typedef struct
{
    usb_desc_config                             config;
    usb_descriptor_cdc_configuration_set_struct cdc[2];
} usb_descriptor_configuration_set_struct;

typedef struct {
    usb_desc_header header;
    wchar_t wString[16];
} usb_descriptor_string;

extern void* const usbd_strings[STR_IDX_MAX];
extern const usb_desc_dev device_descriptor;
extern usb_descriptor_configuration_set_struct configuration_descriptor;

extern usb_class_core usbd_cdc_cb;

/* function declarations */
/* initialize the CDC ACM device */
uint8_t cdc_acm_init(usb_dev *pudev, uint8_t config_index);
/* de-initialize the CDC ACM device */
uint8_t cdc_acm_deinit(usb_dev *pudev, uint8_t config_index);
/* handle the CDC ACM class-specific requests */
uint8_t cdc_acm_req_handler(usb_dev *pudev, usb_req *req);
/* handle CDC ACM data */
uint8_t cdc_acm_data_in_handler(usb_dev *pudev, uint8_t ep_id);
uint8_t cdc_acm_data_out_handler(usb_dev *pudev, uint8_t ep_id);
const void *cdc_acm_get_dev_strings_desc(void);
/* command data received on control endpoint */
uint8_t cdc_acm_ep0_data_out_handler(usb_dev  *pudev);

#endif  /* CDC_ACM_CORE_H */
