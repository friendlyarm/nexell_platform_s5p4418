#!/bin/sh
#KDIR=/home/jonghooy/snow_leopard/ltib/rpm/BUILD/linux-3.0.35 \
#NAL_STORAGE_SRC=$PWD/drv_storage/arch_imx6/nalcode_storage_imx6.c 
#MODULES_DIR=$SDK_DIR/modules/zeroboot 
#CP_DIR=/nfs/boggle70/imx6 $SDK_DIR/out 
#DEBUG_CHECKSUM=y \
#DEBUG_FULL_RESTORE=y \

# sabreauto no lcd nand
#K_UART_VIRT=0xea8fe000 \
#K_UART_PHYS=0x021F0000 \

# sabreai lcd mmc
#K_UART_VIRT=0xc093e000 \
#K_UART_PHYS=0x02020000 \

#K_UART_VIRT=0xea8fe000 \
#K_UART_VIRT=0xea87e000 \

KDIR=$SDK_DIR/kernel/build_linux-3.0.35 \
MODULES_DIR=$PWD \
CP_DIR=/nfs/boggle70/imx6/ \
K_PHYS_OFFSET=0x10000000 \
K_UART_VIRT=0xc091a000 \
K_UART_PHYS=0x021e8000 \
DRV_STORAGE_SRC=mmc \
NAL_STORAGE_SRC=$PWD/drv_storage/arch_imx6/ \
NAL_STORAGE_OFFSET=0x0 \
NAL_STORAGE_SIZE=0x10000000 \
DEBUG_CHECKSUM=y \
make $1 || exit $?


echo "build success & copy to nfs"

