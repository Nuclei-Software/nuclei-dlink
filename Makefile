TARGET = dlink
RTOS = FreeRTOS

NUCLEI_SDK_ROOT ?= ../nuclei-sdk

SOC := gd32vf103
BOARD ?= gd32vf103c_dlink
SYSCLK := 96000000
USB_DRIVER := device

COMMON_FLAGS := -Os -fshort-wchar
COMMON_FLAGS += -DUSE_USB_FS

SRCDIRS = . \
        arch/riscv/source \
        flash/source \
        server/gdb-server/source \
        soc/gd32vf103/$(BOARD)

INCDIRS = . \
        arch/riscv/include \
        flash/include \
        server/gdb-server/include \
        soc \
        soc/gd32vf103/$(BOARD)

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base
