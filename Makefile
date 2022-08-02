TARGET = dlink

NUCLEI_SDK_ROOT ?= ../nuclei-sdk

COMMON_FLAGS := -O0 \
	-DRVL_DEBUG_EN \
	-DRVL_TARGET_RISCV_DEBUG_SPEC_V0P13

SRCDIRS = . \
		lib/pt \
		arch/riscv/source \
		helper/source \
		server/gdb-server/source \
		soc/gd32vf103/gd32vf103v_rvstar

INCDIRS = . \
		lib/pt \
		arch/riscv/include \
		helper/include \
		server/gdb-server/include \
		soc \
		soc/gd32vf103/gd32vf103v_rvstar

SOC := gd32vf103
SYSCLK := 96000000

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base
