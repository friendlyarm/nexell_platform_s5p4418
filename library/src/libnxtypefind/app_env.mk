TOOL_SYS_ROOT	:= /opt/crosstools/arm-cortex_a9-eabi-4.7-eglibc-2.18/arm-cortex_a9-linux-gnueabi/sysroot

# Set GLIB path (library & include)
GLIB_INC 		:= $(TOOL_SYS_ROOT)/usr/include/glib-2.0
GLIB_LIB 		:= $(TOOL_SYS_ROOT)/usr/lib/glib-2.0

# Set GST path (library & include)
GST_INC 		:= $(TOOL_SYS_ROOT)/usr/include/gstreamer-0.10
GST_LIB 		:= $(TOOL_SYS_ROOT)/usr/lib/gstreamer-0.10
