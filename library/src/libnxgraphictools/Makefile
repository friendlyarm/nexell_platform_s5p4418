########################################################################
# Get Linux Build Enviornment:
include ../../../build.env

LIB_INSTALL	:=	../../lib
INC_INSTALL	:= 	../../include

######################################################################
# Build options
INCLUDE		+= -I./ -I./include -I./include/khronos -I../../include

CFLAGS		+= -DGL_GLEXT_PROTOTYPES -fPIC

# Add Library Path
LIBRARY		+= -L./
LIBRARY		+= -L../../lib
LIBRARY 	+= -lnxvmem
LIBRARY 	+= -lpthread
# Add Library
#LIBRARY		+= -L./libs/
#LIBRARY 	+= -lEGL -lGLESv1_CM -lGLESv2 -lVR

CPPFLAGS	+= -DGL_GLEXT_PROTOTYPES -fPIC

######################################################################
# Target
COBJS		:=
# Graphic Tools Engine
CPPOBJS		:= src/vr_egl_runtime.o src/vr_deinterlace_shader.o src/vr_deinterlace.o
# Graphic Tools Service
CPPOBJS		+= src/NX_GTService.o src/NX_Queue.o src/NX_Semaphore.o
# Graphic Tools Engine Wrapper
CPPOBJS		+= src/nx_graphictools.o

LIBNAME		:= libnxgraphictools
TARGET		:= $(LIBNAME).so

######################################################################
# Build
OBJS	:= $(COBJS) $(CPPOBJS)

all: $(TARGET)

$(TARGET): depend $(OBJS)
	$(AR) $(ARFLAGS) $(LIBNAME).a $(OBJS)
	$(CC) $(LDFLAGS) -shared -Wl,-soname,$(SONAME) -o $@ $(OBJS) $(LIBRARY)

clean:
	rm -f src/*.o *.a *.so .depend

install:
	install -m 755 -d $(LIB_INSTALL)
	install -m 644 $(TARGET) $(LIB_INSTALL)
	install -m 644 $(LIBNAME).a $(LIB_INSTALL)
	install -m 644 ./libs/*.so $(LIB_INSTALL)

distclean: clean
	rm -f $(LIB_INSTALL)/$(TARGET)
	rm -f $(LIB_INSTALL)/$(LIBNAME).a
	rm -f $(LIB_INSTALL)/libEGL.so
	rm -f $(LIB_INSTALL)/libGLESv1_CM.so
	rm -f $(LIB_INSTALL)/libGLESv2.so
	rm -f $(LIB_INSTALL)/libVR.so

#########################################################################
# Dependency
ifeq (.depend,$(wildcard .depend))
include .depend
endif

SRCS := $(COBJS:.o=.c) $(CPPOBJS:.o=.cpp)
INCS := $(INCLUDE)
depend dep:
	$(CC) -M $(CPPFLAGS) $(INCS) $(SRCS) > .depend

