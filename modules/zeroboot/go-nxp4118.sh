#!/bin/sh
#NALCODE_SOURCE=y \
#
#CP_DIR=$TOP/hardware/samsung_slsi/slsiap/prebuilt/modules/ \

TOP=/home/falinux/work/git/lollipop-avn-4418 \
ARCH=arm \
CROSS_COMPILE=$TOP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi- \
KDIR=$TOP/kernel \
MODULES_DIR=$PWD \
CP_DIR=/home/falinux/work/nxp_zeroboot/out \
K_PHYS_OFFSET=0x40000000 \
K_UART_VIRT=0xf00a1000 \
K_UART_PHYS=0xc00a1000 \
DRV_STORAGE_SRC=mmc \
NAL_STORAGE_SRC=$PWD/drv_storage/arch_nxp/ \
NAL_STORAGE_OFFSET=0x50000000 \
NAL_STORAGE_SIZE=0x10000000 \
make clean $1 || exit $?

TOP=/home/falinux/work/git/lollipop-avn-4418 \
ARCH=arm \
CROSS_COMPILE=$TOP/prebuilts/gcc/linux-x86/arm/arm-eabi-4.8/bin/arm-eabi- \
KDIR=$TOP/kernel \
MODULES_DIR=$PWD \
CP_DIR=/home/falinux/work/nxp_zeroboot/out \
K_PHYS_OFFSET=0x40000000 \
K_UART_VIRT=0xf00a1000 \
K_UART_PHYS=0xc00a1000 \
DRV_STORAGE_SRC=mmc \
NAL_STORAGE_SRC=$PWD/drv_storage/arch_nxp/ \
NAL_STORAGE_OFFSET=0x50000000 \
NAL_STORAGE_SIZE=0x10000000 \
NALCODE_SOURCE=y \
DEBUG_CHECKSUM=y \
make $1 || exit $?

echo "build success & copy to nfs"

#make ARCH=arm clean
#make ARCH=arm -j4
#cp nx_vpu.ko ../../../../../hardware/samsung_slsi/slsiap/prebuilt/modules/

scp drv_zero_snapshot/drv_zero_snapshot.ko drv_storage/drv_storage.ko nalcode/nalcode.bin boggle70@192.168.10.190:~/nxp/
