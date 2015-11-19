#!/bin/sh
#NALCODE_SOURCE=y \
#

ARCH=arm \
CROSS_COMPILE=arm-eabi- \
KDIR=$TOP/kernel \
MODULES_DIR=$PWD \
CP_DIR=$TOP/hardware/samsung_slsi/slsiap/prebuilt/modules/ \
K_PHYS_OFFSET=0x40000000 \
K_UART_VIRT=0xf00a1000 \
K_UART_PHYS=0xc00a1000 \
DRV_STORAGE_SRC=mmc \
NAL_STORAGE_SRC=$PWD/drv_storage/arch_nxp/ \
NAL_STORAGE_OFFSET=0x50000000 \
NAL_STORAGE_SIZE=0x10000000 \
make clean $1 || exit $?


ARCH=arm \
CROSS_COMPILE=arm-eabi- \
KDIR=$TOP/kernel \
MODULES_DIR=$PWD \
CP_DIR=$TOP/hardware/samsung_slsi/slsiap/prebuilt/modules/ \
K_PHYS_OFFSET=0x40000000 \
K_UART_VIRT=0xf00a1000 \
K_UART_PHYS=0xc00a1000 \
DRV_STORAGE_SRC=mmc \
NAL_STORAGE_SRC=$PWD/drv_storage/arch_nxp/ \
NAL_STORAGE_OFFSET=0x50000000 \
NAL_STORAGE_SIZE=0x10000000 \
DEBUG_CHECKSUM=y \
make $1 || exit $?

echo "build success & copy to nfs"

#make ARCH=arm clean
#make ARCH=arm -j4
#cp nx_vpu.ko ../../../../../hardware/samsung_slsi/slsiap/prebuilt/modules/
