########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
CFLAGS		+= -fpic
INCLUDE		+= -I./ -I../../include -I$(KERNDIR)/include
LIBRARY		+= -L./

######################################################################
# Target
COBJS			:= ion.o
CPPOBJS			:= 

ION_OBJS		:= ion.o
ION_LIBNAME		:= libion
ION_TARGET		:= $(ION_LIBNAME).a

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all: $(ION_TARGET)

$(ION_TARGET): depend $(ION_OBJS)
	$(AR) $(ARFLAGS) $(ION_LIBNAME).a $(ION_OBJS)

clean:
	rm -f *.o *.a *.so .depend

install:
	install -m 755 -d $(LIB_INSTALL)
	install -m 644 $(ION_TARGET) $(LIB_INSTALL)

distclean: clean
	rm -f $(LIB_INSTALL)/$(ION_LIBNAME).a 

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC)  -M  $(CFLAGS)   $(INCS) $(SRCS) > .depend

