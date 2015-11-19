#
# kernel 2.6 driver Makefile
#

include $(KDIR)/.config

obj-m  += drv_zero_snapshot.o
                     
                     
drv_zero_snapshot-objs := drv_zero_snapshot_main.o zb_debug.o \
   							zb_used_memory.o zb_zbi.o reg_save_restore.o \
							zb_proc.o zb_trigger.o zb_blk.o zb_app_inf.o

#ifeq ($(CONFIG_FALINUX_ZEROBOOT_MEM_STORAGE),y)
#drv_zero_snapshot-objs += zb_mem_half_load.o
#endif

REST_DIRS := ext_io
ifeq ($(CONFIG_MACH_EZS3C6410),y)
USE_AX88796B=y
endif
ifeq ($(CONFIG_ARCH_S5PV210),y)
USE_AX88796B=y
endif

ifeq ($(USE_AX88796B),y)
drv_zero_snapshot-objs += ext_io/ax88796b.o
endif


#################################################################
# 아키텍쳐 의존적인 부분
ifeq ($(CONFIG_ARCH_MX6),y)
ARCH_DIRS := arch_imx6
		drv_zero_snapshot-objs += arch_imx6/zb_mmu_imx6.o \
								  arch_imx6/zb_cpu_imx6.o \
								  arch_imx6/zb_cpu_imx6_regdata.o
								  
endif

ifeq ($(CONFIG_ARCH_S5P4418),y)
ARCH_DIRS := arch_nxp4418
		drv_zero_snapshot-objs += arch_nxp4418/zb_mmu_nxp4418.o \
								  arch_nxp4418/zb_cpu_nxp4418.o \
								  arch_nxp4418/zb_cpu_nxp4418_regdata.o
endif

#################################################################


PWD    := $(shell pwd)
INCLUDEDIR += -I$(KDIR)/include -I. 
EXTRA_CFLAGS += -I$(SUBDIRS)/../include/ -I$(SUBDIRS)/include/ -I$(SUBDIRS)/
EXTRA_CFLAGS += $(NAL_EXTRA_CFLAGS)
ifeq ($(DEBUG_CHECKSUM),y)
EXTRA_CFLAGS += -DDEBUG_4K_CHECKSUM
endif
ifeq ($(DEBUG_FULL_RESTORE),y)
EXTRA_CFLAGS += -DFULL_RESTORE_AT_BOOT
endif

LDFLAGS+=-Map=$@.map -s -S

default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules #V=1
	cp drv_zero_snapshot.ko	$(CP_DIR)/
	
clean :	
	echo $(ARCH_DIRS)
	rm -rf *.ko
	rm -rf *.mod.*
	rm -rf .*.cmd
	rm -rf *.o
	rm -rf .tmp_versions
	rm -rf Module.symvers
	rm -rf modules.order
	for i in $(ARCH_DIRS) ; do \
		if [ -d $$i ]; then make -C $$i clean; fi \
	done
	for i in $(REST_DIRS) ; do \
		if [ -d $$i ]; then make -C $$i clean; fi \
	done


