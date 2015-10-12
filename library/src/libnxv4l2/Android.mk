LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
# LOCAL_PRELINK_MODULE := false

SLSIAP_INCLUDE := $(TOP)/hardware/samsung_slsi/slsiap/include
LINUX_INCLUDE  := $(TOP)/linux/platform/$(TARGET_CPU_VARIANT2)/library/include

LOCAL_SHARED_LIBRARIES :=

LOCAL_STATIC_LIBRARIES :=

LOCAL_C_INCLUDES :=	\
	system/core/include/ion \
	$(SLSIAP_INCLUDE)		\
	$(LINUX_INCLUDE)

LOCAL_CFLAGS :=

LOCAL_SRC_FILES := \
	nxp-v4l2-media.cpp	\
	nxp-v4l2-dev.cpp	\
	nx_dsp.cpp

LOCAL_LDFLAGS +=

LOCAL_MODULE := libnx_dsp

LOCAL_MODULE_TAGS := optional

include $(BUILD_STATIC_LIBRARY)
