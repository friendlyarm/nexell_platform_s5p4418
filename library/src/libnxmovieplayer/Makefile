########################################################################
# Get Linux Build Enviornment:
ifndef TOPDIR
TOPDIR	:= ../../..
endif

include $(TOPDIR)/build.env
include ./app_env.mk

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
INCLUDE += -I./ -I$(TOPDIR)/inc
INCLUDE += -I$(GLIB_INC) -I$(GLIB_LIB)/include
INCLUDE += -I$(GST_INC)
#INCLUDE += -I./ -I$(LIBSDIR)/include

LIBRARY	+= -lstdc++ -lpthread -lgobject-2.0 -lm
LIBRARY += -L$(GLIB_LIB) -lglib-2.0 
LIBRARY += -L$(GST_LIB) -lgstreamer-0.10 
#LIBRARY += -L$(LIBSDIR)/lib -lnxtypefind

######################################################################
# Target
COBJS  	:= NX_MoviePlay.o
COBJS  	+= NX_Util.o

CPPOBJS	:=  

CFLAGS		+= -fPIC
CPPFLAGS	+= -fPIC

LIBNAME	:= libnxmovplayer
TARGET	:= $(LIBNAME).so

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all: $(TARGET)

$(TARGET):	depend $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME).a $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(TARGET) -o $@ $(OBJS)

install:
	install -m 755 -d $(LIB_INSTALL)
	install -m 644 $(LIBNAME).a $(LIB_INSTALL)
	install -m 644 $(TARGET) $(LIB_INSTALL)

clean:
	rm -f *.o *.a *.so *.so.* .depend

distclean: clean
	rm -f $(LIB_INSTALL)/$(LIBNAME).a
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
