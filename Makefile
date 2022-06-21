TARGET = rvlink

NUCLEI_SDK_ROOT ?= ../nuclei-sdk

COMMON_FLAGS := -O0 -DRVL_TARGET_GD32VF103 \
	-DRVL_LINK_RVSTAR -DUSING_NUCLEI_SDK \
	-DRVL_SERIAL_USING_UART4_FULL_REMAP \
	-DUSE_USB_FS -DRVL_DEBUG_EN \
	-DRVL_APP_CONFIG_GDB_SERVER_VERSION=\"0.3\"

SRCDIRS = lib/pt rv-link rv-link/link rv-link/target rv-link/details \
		  rv-link/gdb-server rv-link/gdb-server/details \
		  rv-link/link/arch/gd32vf103 \
		  rv-link/link/arch/gd32vf103/details \
		  rv-link/link/arch/gd32vf103/rvstar \
		  rv-link/target/arch/riscv \
		  rv-link/target/arch/riscv \
		  rv-link/target/family/gd32vf103

INCDIRS = . lib rv-link rv-link/link/arch/gd32vf103/details

EXCLUDE_SRCS = rv-link/link/arch/gd32vf103/details/system_gd32vf103.c \
				rv-link/link/arch/gd32vf103/details/gd32vf103_usb_hw_nuclei_sdk.c
				#rv-link/link/arch/gd32vf103/details/gd32vf103_usb_hw_gd32vf103_sdk.c \
				#$(NUCLEI_SDK_ROOT)//SoC/gd32vf103/Common/Source/Drivers/Usb/gd32vf103_usb_hw.c \

SOC := gd32vf103
SYSCLK := 96000000
USB_DRIVER := device

include $(NUCLEI_SDK_ROOT)/Build/Makefile.base
