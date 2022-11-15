TARGET = dlink
RTOS = FreeRTOS

NUCLEI_SDK_ROOT ?= ../nuclei-sdk

COMMON_FLAGS := -O3

SRCDIRS = . \
        arch/riscv/source \
        helper/source \
        server/gdb-server/source \
        soc/gd32vf103/gd32vf103v_rvstar

INCDIRS = . \
        arch/riscv/include \
        helper/include \
        server/gdb-server/include \
        soc \
        soc/gd32vf103/gd32vf103v_rvstar

SOC := gd32vf103
SYSCLK := 108000000
CLKSRC := irc8m

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base
