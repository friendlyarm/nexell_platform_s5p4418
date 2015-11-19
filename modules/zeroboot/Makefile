#
#  << zeroboot Makefile >>
#  
#     ver 0.1.0
#
#   PLZ You should use the make-arch.sh file  
#      make-arch.sh :  make-s3c6410.sh  or  copy file
#                      make-s5pv210.sh  or  copy file
#                      etc
#

.EXPORT_ALL_VARIABLES:

DIRS += drv_storage/ 
DIRS += drv_zero_snapshot/ 

KDIR   ?= $(SDK_DIR)/kernel/build_your_kernel
CP_DIR ?= $(SDK_DIR)/out

ifdef NAL_STORAGE_OFFSET
NAL_EXTRA_CFLAGS ?= -DARG_ZB_STORAGE_OFFSET=$(NAL_STORAGE_OFFSET)
NAL_EXTRA_CFLAGS += -DARG_ZB_STORAGE_SIZE=$(NAL_STORAGE_SIZE)
endif

all: 
	for i in $(DIRS) ; do make -C $$i || exit $? ; done
	if [ -d nalcode ]; then make -C nalcode || exit $? ; fi

clean:
	for i in $(DIRS) ; do make -C $$i clean; done
	if [ -d nalcode ]; then make -C nalcode clean ; fi

distclean:
	for i in $(DIRS) ; do make -C $$i clean; done
	if [ -d nalcode ]; then make -C nalcode clean ; fi
	rm -f cscope.*
	rm -f tags

