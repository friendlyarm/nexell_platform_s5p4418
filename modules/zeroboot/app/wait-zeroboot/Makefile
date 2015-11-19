#
# 어플용 Makefile
#

CROSS_PREFIX   = $(CROSS_COMPILE)
CP_DIR          = $(SDK_DIR)/out

## 사용자 소스
TARGET			= app-wait-zeroboot
SRCS			= wait-zeroboot.c


## 인클루드 디렉토리
DIR_INC        = ../../include/

INCLUDEDIRS		+= -I. -I$(DIR_INC)  

OBJS			= $(SRCS:.c=.o)
CFLAGS			= $(INCLUDEDIRS) -o -W -Wall -O2
LDFLAGS			= $(LIVDIRS)
LIBS		    = -lm    

#---------------------------------------------------------------------
CC				=	$(CROSS_PREFIX)gcc
CXX			 	=	$(CROSS_PREFIX)g++
AR				=	$(CROSS_PREFIX)ar rc
AR2				=	$(CROSS_PREFIX)ranlib
RANLIB			=	$(CROSS_PREFIX)ranlib
LD				=	$(CROSS_PREFIX)ld
NM				=	$(CROSS_PREFIX)nm
STRIP			=	$(CROSS_PREFIX)strip
OBJCOPY		 	=	$(CROSS_PREFIX)objcopy
CP	= cp
MV	= mv
#--------------------------------------------------------------------

all : $(TARGET)
	cp $^ $(CP_DIR)/

$(TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@ $(LIBS) 
	$(NM) $(TARGET) > $(TARGET).map

%.o:%.c
	@echo "Compiling $< ..."
	$(CC) -c $(CFLAGS) -o $@ $<

%.o:%.cc
	@echo "C++ compiling $< ..."
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) -o $@ $<

dep :
	$(CC)	-M	$(INCLUDEDIRS)	$(SRCS) > .depend

clean:
	rm -f *.bak
	rm -f *.map
	rm -f *.o
	rm -f $(OBJS)
	rm -f $(TARGET) core

distclean: clean
	rm -rf .depend	

ifeq (.depend,$(wildcard .depend))
include .depend
endif

