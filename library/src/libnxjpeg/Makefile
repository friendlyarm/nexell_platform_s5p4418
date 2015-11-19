########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build Options

# Prevent: "warning: creating a DT_TEXTREL in object"
CFLAGS	:= -O2 -g -fPIC
INCLUDE += -I./
LIBRARY	+= -lturbojpeg

######################################################################
# Target
COBJS  	:= nx_jpeg.o
CPPOBJS	:=

LIBNAME := libnxjpeg
TARGET	:= $(LIBNAME).so

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all: $(TARGET)

$(TARGET): depend $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(TARGET) -o $@ $(OBJS) $(LIBRARY)

clean:
	rm -f *.o *.a *.so *.so.* .depend

install:
	install -m 755 -d $(LIB_INSTALL)
	install -m 644 $(TARGET) $(LIB_INSTALL)
	cp ./libs/*.so* $(LIB_INSTALL) -a
	
distclean: clean
	rm -f $(LIB_INSTALL)/$(TARGET)
	rm -f $(LIB_INSTALL)/libturbojpeg.so
	rm -f $(LIB_INSTALL)/libturbojpeg.so.0
	rm -f $(LIB_INSTALL)/libturbojpeg.so.0.0.0
	
#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC)  -M  $(CFLAGS)   $(INCS) $(SRCS) > .depend
