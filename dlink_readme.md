# Introduction

Dlink is developed based on RV-Link, and many new functions are added on its basis. At present, Dlink is only applicable to RV-START development board as a debugger. This document mainly introduces the new functions of Dlink, how to run Dlink in RV-START, how to connect hardware and how to use software.

# Repository

https://gito.corp.nucleisys.com/software/baremetal/nuclei-sdk.git

https://gito.corp.nucleisys.com/software/devtools/nuclei-dlink.git

https://gito.corp.nucleisys.com/software/devtools/dlink_gdbserver.git

# New-Functions

- RV32/RV64

- JTAG/C-JTAG

- abstract

- progbuf

- read/write register

- read/write memory

- read/write flash

- support openocd-flashloader

- software(32)/hardware(8) breakpoint

- support f/d/v/p extension

- support more csr register

# Connect-Hardware

![](hardware_connect.png)

| PC  | Dlink  | Dlink       | Board |
| --- | ------ | ----------- | ----- |
| USB | Type-C | GPIOA_Pin4  | TCK   |
|     |        | GPIOB_Pin15 | TMS   |
|     |        | GPIOB_Pin14 | TDI   |
|     |        | GPIOB_Pin13 | TDO   |
|     |        | GND         | GND   |

# Use-Software

## Clone Repository

## Compile operation

```bash
cd  nuclei_dlink
# Suppose three git repositories are in the same directory
# If it is not please modify Makefile
make clean all upload
```

```bash
cd dlnk_gdbserver
# Install QT and compile run dlink_gdbserver
qtcreator dlink_gdbserver.pro &
# Or use the release version of dlink_gdbserver
./dlink_gdbserver
# Make sure your profile is correct
# then select dlink_gdbserver.cfg on the screen and click connect
```

```bash
cd nuclei-sdk
make SOC=demosoc clean all run_gdb
# Let's start debugging
```

# dlink_gdbserver.cfg

```textile
gdb port 3333

# The value must be modified according to the actual situation
serial port /dev/ttyUSB1
# It should not be modified unless you have modified board.c
serial baud 115200

# The value must be modified according to the system configuration
transport select jtag
# transport select cjtag

# The value must be modified according to the system configuration
workarea addr 0x80000000
workarea size 0x1000
workarea backup true

# The value must be modified according to the system configuration
flash spi_base 0x10014000
flash xip_base 0x20000000
flash xip_size 0x10000000
flash block_size 0x10000
# flash loader_path /home/nuclei/Git/openocd-flashloader/build/rv32/loader.bin
flash loader_path /home/nuclei/Git/openocd-flashloader/build/rv64/loader.bin



```
