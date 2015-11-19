########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
CFLAGS	+= -mfpu=neon -fPIC -O3
CFLAGS	+= -DCAN_COMPILE_ARM -D__ARM_NEON__
AFLAGS	+= -mfpu=neon

INCLUDE += -I./ -I../../include -I$(KERNDIR)/include

######################################################################
# Target
COBJS  	:=
COBJS  	+= nx_deinterlace.o
COBJS  	+= algo_basic.o
COBJS  	+= algo_x.o
COBJS  	+= merge.o

AOBJS	:=
AOBJS	+= merge_arm.o

LIBNAME	:= libnxdeinterlace
TARGET  := $(LIBNAME).so

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS) $(AOBJS)

all: $(TARGET)

$(TARGET):	depend $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME).a $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(TARGET) -o $@ $(OBJS) $(LIBRARY)

install:
	install -m 755 -d $(LIB_INSTALL)
	install -m 644 $(TARGET) $(LIB_INSTALL)
	install -m 644 $(LIBNAME).a $(LIB_INSTALL)

clean:
	@find $(OBJTREE) -type f \
		\( -name '*.o'  -o -name $(TARGET) -o -name '*.a' -o -name '*.so*' -o \
		-name '*.bin' -o -name '*.map' -o -name 'core' -o -name '*~' -o -name \
		'.depend' \) -print | xargs rm -f

distclean: clean
	rm -f $(LIB_INSTALL)/$(TARGET)
	rm -f $(LIB_INSTALL)/$(LIBNAME).a

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(AOBJS:.o=.s) $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
