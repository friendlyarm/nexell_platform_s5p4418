#
# kernel 2.6 driver Makefile
#

obj-m  += drv_storage.o
                     
drv_storage-objs := zb_storage.o 


#################################################################
# 아키텍쳐 의존적인 부분
ARCH_DIRS := arch_nxp

ifeq ($(DRV_STORAGE_SRC),mmc)
	ifeq ($(CONFIG_ARCH_S5P4418),y)
	drv_storage-objs		+= arch_nxp/mmc-dw.o
	endif

	ifeq ($(CONFIG_ARCH_MX6),y)
	drv_storage-objs		+= arch_imx6/mmc-imx6.o
	drv_storage-objs		+= arch_imx6/fsl_esdhc.o
	endif
endif
#################################################################


PWD    := $(shell pwd)
INCLUDEDIR += -I. 
INCLUDEDIR += -I$(KDIR)/include
EXTRA_CFLAGS += -I$(SUBDIRS)/../include/ -I$(SUBDIRS)/include/ -I$(SUBDIRS)/
EXTRA_CFLAGS += $(NAL_EXTRA_CFLAGS)
LDFLAGS+=-Map=$@.map -s -S
default:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules #V=1 
	cp drv_storage.ko	$(CP_DIR)/
	
clean :	
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
	

