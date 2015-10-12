########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

#KERNDIR	:= /home/ray/devel/android/jb-mr1.1/kernel

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
INCLUDE += -I./ -I../../include -I$(KERNDIR)/include
INCLUDE += -I$(KERNDIR)/arch/arm/mach-s5p4418/include/

LIBRARY += -L../../lib/
LIBRARY += -lnxvmem

######################################################################
# Target
COBJS  	:= 
COBJS  	+= libnxscaler.o
CPPOBJS	:=  

CFLAGS	+= -g

LIBNAME	:= libnxscaler
TARGET  := libnxscaler.so

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all: $(TARGET)

$(TARGET):	depend $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME).a $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(TARGET) -o $@ $(OBJS)

install:
	install -m 755 -d $(LIB_INSTALL)
	install -m 644 $(TARGET) $(LIB_INSTALL)
	install -m 644 $(LIBNAME).a $(LIB_INSTALL)

clean:
	rm -f *.o *.so *.a .depend

distclean: clean
	rm -f $(LIB_INSTALL)/$(TARGET)
	rm -f $(LIB_INSTALL)/$(LIBNAME).a

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) $(APPOBJS:.o=.c)
INCS := $(INCLUDE)
depend dep:
	$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
