#!/bin/sh


# load kernel module
#
insmod drv_zero_snapshot.ko
insmod drv_storage.ko

# load nalcode
#
cat nalcode.bin > /proc/nalcode


# remove file
#
rm nalcode.bin
rm drv_zero_snapshot.ko
rm drv_storage.ko

# hw list
#  don't save and restore
#
HWLIST=/proc/zb_hwlist

#echo 'lcd=0'      > $HWLIST
echo 'info:fb=0x1e0000@0x21200000' > $HWLIST

#echo 'systimer=0' > $HWLIST
#echo 'pwmtimer=0' > $HWLIST
#echo 'rtctimer=0' > $HWLIST

#echo 'sysclk=0'   > $HWLIST
#echo 'vic0=0'     > $HWLIST
#echo 'vic1=0'     > $HWLIST
#echo 'vic2=0'     > $HWLIST
#echo 'vic3=0'     > $HWLIST

#echo 'gpio=0'     > $HWLIST
#echo 'tsadc=0'    > $HWLIST
#echo 'srom=0'     > $HWLIST
#echo 'i2c0=0'     > $HWLIST
#echo 'i2c1=0'     > $HWLIST
#echo 'i2c2=0'     > $HWLIST
#echo 'ac97=0'     > $HWLIST

#echo 'mmc0=0'     > $HWLIST
echo 'mmc1=0'     > $HWLIST
echo 'mmc2=0'     > $HWLIST
echo 'mmc3=0'     > $HWLIST

echo 'hdmi=0'     > $HWLIST
echo 'video=0'     > $HWLIST
echo 'mixer=0'     > $HWLIST

# if need, Applications may be executed on
#     
umount /app
umount /mnt/nfs
ifconfig eth0 down

sleep 1

# make to zeroboot image
#
echo 'start' > /proc/zeroboot_trigger


# wait until make the zeroboot image
#
./app-wait-zeroboot 30 


# below user app or script
#
if [ -f /proc/zb_active_late ]; then
	echo "usb" > /proc/zb_active_late
fi

cd /root
mount -t yaffs2 -o noatime /dev/mtdblock2 /app

if [ -f /app/run.sh ]; then
	/app/run.sh
fi
# end
