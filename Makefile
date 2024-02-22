TARGET = dlink
RTOS = FreeRTOS

NUCLEI_SDK_ROOT ?= ../nuclei-sdk

COMMON_FLAGS := -Os -fshort-wchar
COMMON_FLAGS += -DUSE_USB_FS

SRCDIRS = . \
        arch/riscv/source \
        flash/source \
        server/gdb-server/source \
        soc/gd32vf103/gd32vf103v_rvstar

INCDIRS = . \
        arch/riscv/include \
        flash/include \
        server/gdb-server/include \
        soc \
        soc/gd32vf103/gd32vf103v_rvstar

SOC := gd32vf103
SYSCLK := 96000000
USB_DRIVER := device

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base
