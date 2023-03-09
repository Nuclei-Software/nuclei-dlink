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

#include "cdc_acm_core.h"
#include "gdb-packet.h"

#define USBD_VID                          0x28e9
#define USBD_PID                          0x018a

static uint32_t cdc_cmd = 0xFFU;

uint8_t usb_cmd_buffer[CDC_ACM_CMD_PACKET_SIZE];

__IO uint32_t cdc0_packet_sent = 1U;
__IO uint32_t cdc0_packet_receive = 1U;
__IO uint32_t cdc0_receive_length = 0U;

__IO uint32_t cdc1_packet_sent = 1U;
__IO uint32_t cdc1_packet_receive = 1U;
__IO uint32_t cdc1_receive_length = 0U;

//usbd_int_cb_struct *usbd_int_fops = NULL;

typedef struct
{
    uint32_t dwDTERate;   /* data terminal rate */
    uint8_t  bCharFormat; /* stop bits */
    uint8_t  bParityType; /* parity */
    uint8_t  bDataBits;   /* data bits */
}line_coding_struct;

line_coding_struct linecoding =
{
    115200, /* baud rate     */
    0x00,   /* stop bits - 1 */
    0x00,   /* parity - none */
    0x08    /* num of bits 8 */
};

/* note:it should use the C99 standard when compiling the below codes */
/* USB standard device descriptor */
const usb_desc_dev device_descriptor =
{
    .header =
    {
        .bLength                    = USB_DEV_DESC_LEN,
        .bDescriptorType            = USB_DESCTYPE_DEV
    },
    .bcdUSB                         = 0x0200,
    .bDeviceClass                   = 0xEF, /* Miscellaneous Device */
    .bDeviceSubClass                = 0x02, /* Common Class */
    .bDeviceProtocol                = 0x01, /* Interface Association */
    .bMaxPacketSize0                = USB_FS_EP0_MAX_LEN,
    .idVendor                       = USBD_VID,
    .idProduct                      = USBD_PID,
    .bcdDevice                      = 0x0100,
    .iManufacturer                  = 0x01,
    .iProduct                       = 0x02,
    .iSerialNumber                  = 0x03,
    .bNumberConfigurations          = 0x01
};

/* USB device configuration descriptor */
usb_descriptor_configuration_set_struct configuration_descriptor =
{
    .config =
    {
        .header =
        {
            .bLength                = USB_CFG_DESC_LEN,
            .bDescriptorType        = USB_DESCTYPE_CONFIG
        },
        .wTotalLength               = sizeof(usb_descriptor_configuration_set_struct),
        .bNumInterfaces             = 0x04,
        .bConfigurationValue        = 0x01,
        .iConfiguration             = 0x00,
        .bmAttributes               = 0x80,
        .bMaxPower                  = 0x32
    },

    .cdc[0].cdc_itf_association =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_itf_association),
            .bDescriptorType        = USB_DESCTYPE_ITF_ASSOCIATION
        },
        .bFirstInterface            = 0x00,
        .bInterfaceCount            = 0x02,
        .bFunctoinClass             = 0x02,
        .bFunctionSubClass          = 0x02,
        .bFunctionProtocol          = 0x00,
        .iFunction                  = 0x00
    },

    .cdc[0].cdc_loopback_interface =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_itf),
            .bDescriptorType        = USB_DESCTYPE_ITF
        },
        .bInterfaceNumber           = 0x00,
        .bAlternateSetting          = 0x00,
        .bNumEndpoints              = 0x01,
        .bInterfaceClass            = 0x02,
        .bInterfaceSubClass         = 0x02,
        .bInterfaceProtocol         = 0x00,
        .iInterface                 = 0x04
    },

    .cdc[0].cdc_loopback_header =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_header_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x00,
        .bcdCDC                     = 0x0110
    },

    .cdc[0].cdc_loopback_call_managment =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_call_managment_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x01,
        .bmCapabilities             = 0x00,
        .bDataInterface             = 0x01
    },

    .cdc[0].cdc_loopback_acm =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_acm_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x02,
        .bmCapabilities             = 0x02,
    },

    .cdc[0].cdc_loopback_union =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_union_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x06,
        .bMasterInterface           = 0x00,
        .bSlaveInterface0           = 0x01,
    },

    .cdc[0].cdc_loopback_cmd_endpoint =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_ep),
            .bDescriptorType        = USB_DESCTYPE_EP
        },
        .bEndpointAddress           = CDC0_ACM_CMD_EP,
        .bmAttributes               = 0x03,
        .wMaxPacketSize             = CDC_ACM_CMD_PACKET_SIZE,
        .bInterval                  = 0xFF
    },

    .cdc[0].cdc_loopback_data_interface =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_itf),
            .bDescriptorType        = USB_DESCTYPE_ITF
        },
        .bInterfaceNumber           = 0x01,
        .bAlternateSetting          = 0x00,
        .bNumEndpoints              = 0x02,
        .bInterfaceClass            = 0x0A,
        .bInterfaceSubClass         = 0x00,
        .bInterfaceProtocol         = 0x00,
        .iInterface                 = 0x00
    },

    .cdc[0].cdc_loopback_out_endpoint =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_ep),
            .bDescriptorType        = USB_DESCTYPE_EP
        },
        .bEndpointAddress           = CDC0_ACM_DATA_OUT_EP,
        .bmAttributes               = 0x02,
        .wMaxPacketSize             = CDC_ACM_DATA_PACKET_SIZE,
        .bInterval                  = 0x00
    },

    .cdc[0].cdc_loopback_in_endpoint =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_ep),
            .bDescriptorType        = USB_DESCTYPE_EP
        },
        .bEndpointAddress           = CDC0_ACM_DATA_IN_EP,
        .bmAttributes               = 0x02,
        .wMaxPacketSize             = CDC_ACM_DATA_PACKET_SIZE,
        .bInterval                  = 0x00
    },

    // CDC1

    .cdc[1].cdc_itf_association =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_itf_association),
            .bDescriptorType        = USB_DESCTYPE_ITF_ASSOCIATION
        },
        .bFirstInterface            = 0x02,
        .bInterfaceCount            = 0x02,
        .bFunctoinClass             = 0x02,
        .bFunctionSubClass          = 0x02,
        .bFunctionProtocol          = 0x00,
        .iFunction                  = 0x00
    },

    .cdc[1].cdc_loopback_interface =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_itf),
            .bDescriptorType        = USB_DESCTYPE_ITF
        },
        .bInterfaceNumber           = 0x02,
        .bAlternateSetting          = 0x00,
        .bNumEndpoints              = 0x01,
        .bInterfaceClass            = 0x02,
        .bInterfaceSubClass         = 0x02,
        .bInterfaceProtocol         = 0x00,
        .iInterface                 = 0x05
    },

    .cdc[1].cdc_loopback_header =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_header_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x00,
        .bcdCDC                     = 0x0110
    },

    .cdc[1].cdc_loopback_call_managment =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_call_managment_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x01,
        .bmCapabilities             = 0x00,
        .bDataInterface             = 0x03
    },

    .cdc[1].cdc_loopback_acm =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_acm_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x02,
        .bmCapabilities             = 0x02,
    },

    .cdc[1].cdc_loopback_union =
    {
        .header =
        {
            .bLength                = sizeof(usb_descriptor_union_function_struct),
            .bDescriptorType        = USB_DESCTYPE_CS_INTERFACE
        },
        .bDescriptorSubtype         = 0x06,
        .bMasterInterface           = 0x02,
        .bSlaveInterface0           = 0x03,
    },

    .cdc[1].cdc_loopback_cmd_endpoint =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_ep),
            .bDescriptorType        = USB_DESCTYPE_EP
        },
        .bEndpointAddress           = CDC1_ACM_CMD_EP,
        .bmAttributes               = 0x03,
        .wMaxPacketSize             = CDC_ACM_CMD_PACKET_SIZE,
        .bInterval                  = 0xFF
    },

    .cdc[1].cdc_loopback_data_interface =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_itf),
            .bDescriptorType        = USB_DESCTYPE_ITF
        },
        .bInterfaceNumber           = 0x03,
        .bAlternateSetting          = 0x00,
        .bNumEndpoints              = 0x02,
        .bInterfaceClass            = 0x0A,
        .bInterfaceSubClass         = 0x00,
        .bInterfaceProtocol         = 0x00,
        .iInterface                 = 0x00
    },

    .cdc[1].cdc_loopback_out_endpoint =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_ep),
            .bDescriptorType        = USB_DESCTYPE_EP
        },
        .bEndpointAddress           = CDC1_ACM_DATA_OUT_EP,
        .bmAttributes               = 0x02,
        .wMaxPacketSize             = CDC_ACM_DATA_PACKET_SIZE,
        .bInterval                  = 0x00
    },

    .cdc[1].cdc_loopback_in_endpoint =
    {
        .header =
        {
            .bLength                = sizeof(usb_desc_ep),
            .bDescriptorType        = USB_DESCTYPE_EP
        },
        .bEndpointAddress           = CDC1_ACM_DATA_IN_EP,
        .bmAttributes               = 0x02,
        .wMaxPacketSize             = CDC_ACM_DATA_PACKET_SIZE,
        .bInterval                  = 0x00
    },
};

/* USB language ID Descriptor */
const usb_desc_LANGID usbd_language_id_desc =
{
    .header =
    {
        .bLength = sizeof(usb_desc_LANGID),
        .bDescriptorType = USB_DESCTYPE_STR
    },
    .wLANGID = ENG_LANGID
};

static usb_descriptor_string usb_string_desc_serial =
{
    .header =
    {
        .bLength = 0,
        .bDescriptorType = USB_DESCTYPE_STR
    },
    .wString = {0}
};

void* const usbd_strings[STR_IDX_MAX] =
{
    [0x00] = (uint8_t *)&usbd_language_id_desc,
    [0x01] = USBD_STRING_DESC("Dlink"),
    [0x02] = USBD_STRING_DESC("Dlink Low Cost Scheme"),
    [0x03] = (uint8_t*) &usb_string_desc_serial,
    [0x04] = USBD_STRING_DESC("Dlink GDB Server"),
    [0x05] = USBD_STRING_DESC("Dlink COM Port"),
};

/*!
    \brief      Convert a C string to wide-character string
    \param[in]  dest: pointer to an array of wchar_t elements long enough to
                      contain the resulting sequence (at most, max wide characters)
    \param[in]  str: C string (without multibyte characters to be interpreted)
    \param[in]  max: maximum number of wchar_t characters to write to dest
    \param[out] none
    \retval     The number of wide characters written to dest, not including the
                eventual terminating null character.
*/
static size_t stowcs(wchar_t *dest, const char *str, size_t max)
{
    int i = 0;
    while ((*dest++ = *str++) && max--) i++;
    return i;
}

/*!
    \brief      fill an UBS serial string descriptor
    \param[in]  desc: pointer to USB string descriptor
    \param[in]  str: C string to write to the string descriptor
    \param[out] none
    \retval     none
*/
static void cdc_acm_set_string(usb_descriptor_string *desc, const char *str)
{
    int n = stowcs(desc->wString, str, sizeof(desc->wString));
    desc->header.bLength = sizeof(wchar_t) * n + 2;
}

/*!
    \brief      Get the USB string descriptor table of the CDC ACM device
    \param[in]  none
    \param[out] none
    \retval     none
*/
const void *cdc_acm_get_dev_strings_desc(void)
{
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "SN%08X", (unsigned int) (
            *(uint32_t*) 0x1FFFF7E8 ^
            *(uint32_t*) 0x1FFFF7EC ^
            *(uint32_t*) 0x1FFFF7F0));
    cdc_acm_set_string((usb_descriptor_string*)usbd_strings[0x03], tmp);
    return usbd_strings;
}

/*!
    \brief      initialize the CDC ACM device
    \param[in]  pudev: pointer to USB device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     USB device operation status
*/
uint8_t cdc_acm_init (usb_dev *pudev, uint8_t config_index)
{
    /* initialize the data Tx/Rx endpoint */
    usbd_ep_setup(pudev, &(configuration_descriptor.cdc[0].cdc_loopback_in_endpoint));
    usbd_ep_setup(pudev, &(configuration_descriptor.cdc[0].cdc_loopback_out_endpoint));

    usbd_ep_setup(pudev, &(configuration_descriptor.cdc[1].cdc_loopback_in_endpoint));
    usbd_ep_setup(pudev, &(configuration_descriptor.cdc[1].cdc_loopback_out_endpoint));

    return USBD_OK;
}

/*!
    \brief      de-initialize the CDC ACM device
    \param[in]  pudev: pointer to USB device instance
    \param[in]  config_index: configuration index
    \param[out] none
    \retval     USB device operation status
*/
uint8_t cdc_acm_deinit (usb_dev *pudev, uint8_t config_index)
{
    /* deinitialize the data Tx/Rx endpoint */
    usbd_ep_clear(pudev, CDC0_ACM_DATA_IN_EP);
    usbd_ep_clear(pudev, CDC0_ACM_DATA_OUT_EP);

    usbd_ep_clear(pudev, CDC1_ACM_DATA_IN_EP);
    usbd_ep_clear(pudev, CDC1_ACM_DATA_OUT_EP);
    return USBD_OK;
}

/*!
    \brief      handle CDC ACM data
    \param[in]  pudev: pointer to USB device instance
    \param[in]  rx_tx: data transfer direction:
        \arg        USBD_TX
        \arg        USBD_RX
    \param[in]  ep_id: endpoint identifier
    \param[out] none
    \retval     USB device operation status
*/
uint8_t cdc_acm_data_out_handler (usb_dev *pudev, uint8_t ep_id)
{
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    if ((EP0_OUT & 0x7F) == ep_id)
    {
        cdc_acm_ep0_data_out_handler(pudev);
    }
    else if ((CDC0_ACM_DATA_OUT_EP & 0x7F) == ep_id)
    {
        cdc0_packet_receive = 1;
        cdc0_receive_length = usbd_rxcount_get(pudev, CDC0_ACM_DATA_OUT_EP);
        return USBD_OK;
    }
    else if ((CDC1_ACM_DATA_OUT_EP & 0x7F) == ep_id)
    {
        cdc1_packet_receive = 1;
        cdc1_receive_length = usbd_rxcount_get(pudev, CDC1_ACM_DATA_OUT_EP);
        return USBD_OK;
    }
    return USBD_FAIL;
}

uint8_t cdc_acm_data_in_handler (usb_dev *pudev, uint8_t ep_id)
{
    BaseType_t xHigherPriorityTaskWoken = pdTRUE;
    if ((CDC0_ACM_DATA_IN_EP & 0x7F) == ep_id)
    {
        usb_transc *transc = &pudev->dev.transc_in[EP_ID(ep_id)];
        if ((transc->xfer_len % transc->max_len == 0) && (transc->xfer_len != 0)) {
            usbd_ep_send (pudev, ep_id, NULL, 0U);
        } else {
            cdc0_packet_sent = 1;
        }
        return USBD_OK;
    }
    else if ((CDC1_ACM_DATA_IN_EP & 0x7F) == ep_id)
    {
        usb_transc *transc = &pudev->dev.transc_in[EP_ID(ep_id)];
        if ((transc->xfer_len % transc->max_len == 0) && (transc->xfer_len != 0)) {
            usbd_ep_send (pudev, ep_id, NULL, 0U);
        } else {
            cdc1_packet_sent = 1;
        }
        return USBD_OK;
    }
    return USBD_FAIL;
}


/*!
    \brief      handle the CDC ACM class-specific requests
    \param[in]  pudev: pointer to USB device instance
    \param[in]  req: device class-specific request
    \param[out] none
    \retval     USB device operation status
*/
uint8_t cdc_acm_req_handler (usb_dev *pudev, usb_req *req)
{
    switch (req->bRequest)
    {
        case SEND_ENCAPSULATED_COMMAND:
            break;
        case GET_ENCAPSULATED_RESPONSE:
            break;
        case SET_COMM_FEATURE:
            break;
        case GET_COMM_FEATURE:
            break;
        case CLEAR_COMM_FEATURE:
            break;
        case SET_LINE_CODING:
            /* set the value of the current command to be processed */
            cdc_cmd = req->bRequest;
            /* enable EP0 prepare to receive command data packet */
            pudev->dev.transc_out[0].xfer_buf = usb_cmd_buffer;
            pudev->dev.transc_out[0].remain_len = req->wLength;
            break;
        case GET_LINE_CODING:
            usb_cmd_buffer[0] = (uint8_t)(linecoding.dwDTERate);
            usb_cmd_buffer[1] = (uint8_t)(linecoding.dwDTERate >> 8);
            usb_cmd_buffer[2] = (uint8_t)(linecoding.dwDTERate >> 16);
            usb_cmd_buffer[3] = (uint8_t)(linecoding.dwDTERate >> 24);
            usb_cmd_buffer[4] = linecoding.bCharFormat;
            usb_cmd_buffer[5] = linecoding.bParityType;
            usb_cmd_buffer[6] = linecoding.bDataBits;
            /* send the request data to the host */
            pudev->dev.transc_in[0].xfer_buf = usb_cmd_buffer;
            pudev->dev.transc_in[0].remain_len = req->wLength;
            break;
        case SET_CONTROL_LINE_STATE:
            break;
        case SEND_BREAK:
            break;
        default:
            break;
    }

    return USBD_OK;
}

/*!
    \brief      command data received on control endpoint
    \param[in]  pudev: pointer to USB device instance
    \param[out] none
    \retval     USB device operation status
*/
uint8_t cdc_acm_ep0_data_out_handler (usb_dev *pudev)
{
    if (SET_LINE_CODING == cdc_cmd) {
        /* process the command data */
        linecoding.dwDTERate = (uint32_t)(usb_cmd_buffer[0] |
                                        (usb_cmd_buffer[1] << 8) |
                                        (usb_cmd_buffer[2] << 16) |
                                        (usb_cmd_buffer[3] << 24));

        linecoding.bCharFormat = usb_cmd_buffer[4];
        linecoding.bParityType = usb_cmd_buffer[5];
        linecoding.bDataBits = usb_cmd_buffer[6];

        cdc_cmd = NO_CMD;

        usart_disable(UART_ITF);
        usart_baudrate_set(UART_ITF, linecoding.dwDTERate);
        usart_word_length_set(UART_ITF, (uint32_t)linecoding.bDataBits);
        usart_stop_bit_set(UART_ITF, (uint32_t)linecoding.bCharFormat);
        usart_parity_config(UART_ITF, (uint32_t)linecoding.bParityType);
        usart_enable(UART_ITF);
    }

    return USBD_OK;
}


usb_class_core usbd_cdc_cb = {
    .command         = NO_CMD,
    .alter_set       = 0,

    .init            = cdc_acm_init,
    .deinit          = cdc_acm_deinit,
    .req_proc        = cdc_acm_req_handler,
    .data_in         = cdc_acm_data_in_handler,
    .data_out        = cdc_acm_data_out_handler
};
