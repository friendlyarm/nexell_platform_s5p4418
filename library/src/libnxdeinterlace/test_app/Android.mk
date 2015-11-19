LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES:= \
	DeInterlaceTest.cpp

LOCAL_C_INCLUDES += \
	frameworks/native/include	\
	system/core/include		\
	hardware/libhardware/include	\
	hardware/samsung_slsi/slsiap/include	\
	linux/platform/$(TARGET_CPU_VARIANT2)/library/include	\
	system/core/include/ion	\
	$(LOCAL_PATH)

LOCAL_STATIC_LIBRARIES := libnxmalloc

LOCAL_SHARED_LIBRARIES := \
	libcutils		\
	libbinder		\
	libutils		\
	libgui			\
	libui			\
	libion			\
	libion-nexell		\
	libnx_deinterlace

LOCAL_LDFLAGS += \
	-Llinux/platform/$(TARGET_CPU_VARIANT2)/library/lib

LOCAL_MODULE:= deinterlace_tests

include $(BUILD_EXECUTABLE)
