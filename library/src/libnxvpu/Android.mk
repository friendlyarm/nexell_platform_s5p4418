LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
# LOCAL_PRELINK_MODULE := false

SLSIAP_INCLUDE := $(TOP)/hardware/samsung_slsi/slsiap/include
LINUX_INCLUDE  := $(TOP)/linux/platform/$(TARGET_CPU_VARIANT2)/library/include

RATECONTROL_PATH := $(TOP)/linux/platform/$(TARGET_CPU_VARIANT2)/library/lib/ratecontrol

LOCAL_SHARED_LIBRARIES :=	\
	liblog \
	libcutils \
	libion \
	libion-nexell

LOCAL_STATIC_LIBRARIES := \
	libnxmalloc

LOCAL_C_INCLUDES := system/core/include/ion \
					$(SLSIAP_INCLUDE) \
					$(LINUX_INCLUDE)

LOCAL_CFLAGS :=

LOCAL_SRC_FILES := \
	parser_vld.c \
	nx_video_api.c

LOCAL_LDFLAGS += \
	-L$(RATECONTROL_PATH)	\
	-lnxvidrc_android

LOCAL_MODULE := libnx_vpu

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)

