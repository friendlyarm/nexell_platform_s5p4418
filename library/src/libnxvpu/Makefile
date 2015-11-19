########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
INCLUDE += -I./ -I../../include -I$(KERNDIR)/include
#INCLUDE	+= -I../libhevc/common -I../libhevc/decoder

LIBRARY += -L../../lib/
LIBRARY += -lnxvmem
LIBRARY += -lnxvidrc
#LIBRARY += -lhevcdec

######################################################################
# Target
COBJS  	:= 
COBJS  	+= parser_vld.o nx_video_api.o
CPPOBJS	:=  

#CFLAGS	+= -DHEVC_DEC
CFLAGS	+= -fPIC

LDFLAGS := -Wl,-Bsymbolic

LIBNAME	:= libnxvpu
TARGET  := libnxvpu.so

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all: $(TARGET)

$(TARGET):	depend $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME).a $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(TARGET) -o $@ $(OBJS) $(LIBRARY)

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
