########################################################################
# Get Linux Build Enviornment:
include ../../../../build.env

######################################################################
# Build options
INCLUDE += -I./
INCLUDE += -I../../../include

LIBRARY	+= -L../../../lib
LIBRARY += -lnxvmem -lnxdeinterlace

######################################################################
# Target
COBJS := 
CPPOBJS	:= DeInterlaceTest.o

CFLAGS	+= -g -pthread -fpic

TARGET:= sw_deinterlace_test

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all:	$(TARGET)

$(TARGET):	depend $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBRARY)

clean:
	rm -f $(OBJS) $(TARGET) .depend

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp) $(APPOBJS:.o=.c)
INCS := $(INCLUDE)
depend dep:
	$(CC) -M $(CFLAGS) $(INCS) $(SRCS) > .depend
