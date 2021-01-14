#ifndef __RV_LINK_LINK_ARCH_GD32VF103C_DETAILS_GD32VF103_HW_H__
#define __RV_LINK_LINK_ARCH_GD32VF103C_DETAILS_GD32VF103_HW_H__
/*!
    Copyright (c) 2019, GigaDevice Semiconductor Inc.
    Copyright (c) 2021, Micha Hoiting <micha.hoiting@gmail.com>

    \file  rv-link/link/arch/gd32vf103c/details/gd32vf103_hw.h
    \brief the header file of the HW functions

    Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors
       may be used to endorse or promote products derived from this software without
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
OF SUCH DAMAGE.
*/

/* system library header file includes */
#include <stdint.h>

/* other library header file includes */
#include <gd32vf103-sdk/GD32VF103_standard_peripheral/gd32vf103.h>
#include <gd32vf103-sdk/GD32VF103_usbfs_driver/Include/drv_usb_hw.h>

extern __IO uint32_t usbfs_prescaler;

void usb_timer_irq(void);

#endif /* __RV_LINK_LINK_ARCH_GD32VF103C_DETAILS_GD32VF103_HW_H__ */
