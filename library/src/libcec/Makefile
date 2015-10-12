########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build Options

# Prevent: "warning: creating a DT_TEXTREL in object"
CFLAGS	+= -O2 -g -fpic
INCLUDE += -I./
LIBRARY	+= -L./

######################################################################
# Target
COBJS  	:= libcec.o
CPPOBJS	:=

LIBNAME := libcec
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
	install -m 644 libcec.h $(INC_INSTALL)

distclean: clean
	rm -f $(LIB_INSTALL)/$(TARGET)

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC)  -M  $(CFLAGS)   $(INCS) $(SRCS) > .depend
