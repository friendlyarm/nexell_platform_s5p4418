ifeq ($(TARGET_CPU_VARIANT2),s5p4418)

LOCAL_PATH := $(call my-dir)

##############################################################################
##
##     Graphic Library Shared Library
##

include $(CLEAR_VARS)
# LOCAL_PRELINK_MODULE := false

NX_HW_TOP        := $(TOP)/hardware/samsung_slsi/slsiap/
NX_HW_INCLUDE    := $(NX_HW_TOP)/include
NX_LINUX_INCLUDE := $(TOP)/linux/platform/s5p4418/library/include

LOCAL_SHARED_LIBRARIES :=	\
	liblog \
	libcutils \
	libion-nexell \
	libion

LOCAL_STATIC_LIBRARIES :=	\
	libnxmalloc

LOCAL_LDFLAGS += \
	-L$(NX_HW_TOP)/prebuilt/library -lEGL_vr -lGLESv1_CM_vr -lVR -lGLESv2_vr

LOCAL_C_INCLUDES :=	$(TOP)/system/core/include/ion \
					$(NX_HW_INCLUDE) \
					$(NX_LINUX_INCLUDE) \
					$(LOCAL_PATH)/src \
					$(LOCAL_PATH)/include/khronos

LOCAL_CFLAGS := -DGL_GLEXT_PROTOTYPES=1

LOCAL_SRC_FILES := \
	src/nx_graphictools.cpp \
	src/vr_egl_runtime.cpp \
	src/vr_deinterlace_shader.cpp \
	src/vr_deinterlace.cpp \
	src/NX_Queue.cpp \
	src/NX_Semaphore.cpp \
	src/NX_GTService.cpp \

LOCAL_MODULE := libnxgraphictools

LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)


##############################################################################
##
##     Graphic Library Test Application
##

include $(CLEAR_VARS)
NX_HW_TOP        := $(TOP)/hardware/samsung_slsi/slsiap/
NX_HW_INCLUDE    := $(NX_HW_TOP)/include
NX_LINUX_INCLUDE := $(TOP)/linux/platform/s5p4418/library/include

LOCAL_SHARED_LIBRARIES :=	\
	liblog \
	libcutils \
	libnxgraphictools \
	libion-nexell \
	libion \
	libnx_vpu

LOCAL_STATIC_LIBRARIES :=	\
	libnxmalloc

LOCAL_C_INCLUDES :=	$(TOP)/system/core/include/ion \
					$(NX_HW_INCLUDE) \
					$(LOCAL_PATH)/src \
					$(NX_LINUX_INCLUDE)

LOCAL_CFLAGS := 

LOCAL_SRC_FILES := \
	test_graphicutil.cpp

LOCAL_MODULE := graphicutil_test

LOCAL_MODULE_TAGS := optional

include $(BUILD_EXECUTABLE)


###############################################################################
###
###     Graphic Library Test Application for Surface Test
###
#
#include $(CLEAR_VARS)
#NX_HW_TOP := $(TOP)/hardware/nexell/pyrope/
#NX_HW_INCLUDE := $(NX_HW_TOP)/include
#
#LOCAL_SHARED_LIBRARIES :=	\
#	liblog \
#	libcutils \
#	libnxgraphictools \
#	libcutils libbinder libutils libgui libui libv4l2-nexell
#
#LOCAL_STATIC_LIBRARIES :=	\
#	libnxmalloc
#
#LOCAL_C_INCLUDES :=	$(TOP)/system/core/include/ion \
#					$(NX_HW_INCLUDE) \
#					$(LOCAL_PATH)/src \
#
#LOCAL_CFLAGS := 
#
#LOCAL_SRC_FILES := \
#	test_surface.cpp
#
#LOCAL_MODULE := graphic_surface_test
#
#LOCAL_MODULE_TAGS := optional
#
#include $(BUILD_EXECUTABLE)

endif
