########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
CFLAGS	+= -O2 -g -fpic
INCLUDE += -I./ -I./$(INC_INSTALL)
LIBRARY	+= -L$(LIB_INSTALL) -lnxgpio -lnxpwm

######################################################################
# Target
COBJS  	:= 
COBJS  	+= nx_iso7816.o
CPPOBJS	:=  

LIBNAME	:= libnxiso7816
TARGET  := $(LIBNAME).so

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
