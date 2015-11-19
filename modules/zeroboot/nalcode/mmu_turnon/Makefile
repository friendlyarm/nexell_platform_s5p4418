
.EXPORT_ALL_VARIABLES:

KDIR   ?= $(SDK_DIR)/kernel/build_your_kernel
CP_DIR ?= $(SDK_DIR)/out

TARGET 		= mmu_turnon

#################################################################
# 아키텍쳐의 설정 정보를 가져온다
# 컴파일 옵션과 메모리의 하드웨어 주소를 맞추어 준다
#################################################################
include $(KDIR)/.config

#################################################################
# linker script select
#################################################################
ifeq ($(CONFIG_CPU_S5PV210),y)
CPU_LINKER  = s5pv210
endif
ifeq ($(CONFIG_CPU_S3C6410),y)
CPU_LINKER  = s3c6410
endif
ifeq ($(CONFIG_CPU_S3C244X),y)
CPU_LINKER  = s3c2440
endif
ifeq ($(CONFIG_ARCH_OMAP4),y)
CPU_LINKER  = omap4
endif
ifeq ($(CONFIG_ARCH_TI814X),y)
CPU_LINKER  = am3874
endif

ifeq ($(CONFIG_CPU_EXYNOS4210),y)
CPU_LINKER  = exynos4210
endif

ifeq ($(CONFIG_ARCH_MX6),y)
CPU_LINKER  = imx6
endif

ifeq ($(CONFIG_ARCH_NXP5430),y)
CPU_LINKER  = nxp5430
endif

ifeq ($(CONFIG_ARCH_S5P4418),y)
CPU_LINKER  = s5p4418
endif

ASM_SOURCE	=  
ASM_SOURCE 	+= mmu_on_entry.S
ASM_SOURCE 	+= lib1funcs.S

C_SOURCE	=  
C_SOURCE	+= mmu_on_main.c
C_SOURCE    += uart.c
C_SOURCE	+= vsprintf.c
C_SOURCE	+= string.c
C_SOURCE	+= printf.c

#################################################################
# 아키텍쳐 의존적인 부분
#C_SOURCE	+= $(NAL_STORAGE_SRC)
ifeq ($(DRV_STORAGE_SRC),nand)
NAL_OBJ      = $(NAL_STORAGE_SRC)nalcode_storage_imx6.o
NAL_OBJ      += $(NAL_STORAGE_SRC)nalcode_mxs_nand.o
NAL_OBJ      += $(NAL_STORAGE_SRC)nalcode_apbh_dma.o
endif

ifeq ($(DRV_STORAGE_SRC),mmc)
	ifeq ($(CONFIG_ARCH_S5P4418),y)
	NAL_OBJ      = $(NAL_STORAGE_SRC)nalcode_mmc_storage_dw.o
	#NAL_OBJ      += $(NAL_STORAGE_SRC)nalcode_fsl_esdhc.o
	endif

	ifeq ($(CONFIG_ARCH_MX6),y)
	NAL_OBJ      = $(NAL_STORAGE_SRC)nalcode_mmc_storage_imx6.o
	NAL_OBJ      += $(NAL_STORAGE_SRC)nalcode_fsl_esdhc.o
	endif
endif
#################################################################

# 플래쉬 디바이스 

CC			= $(CROSS_COMPILE)gcc
LD			= $(CROSS_COMPILE)ld
OC			= $(CROSS_COMPILE)objcopy

ASM_OBJS  	= $(ASM_SOURCE:%.S=%.o)
C_OBJS    	= $(C_SOURCE:%.c=%.o)

SRCS		= $(ASM_SOURCE) $(C_SOURCE)	
OBJS 		= $(ASM_OBJS) $(C_OBJS) $(NAL_OBJ)

ASM_OPTION 	=  -c  
ASM_OPTION 	+=  -nostdinc
ASM_OPTION 	+= -I.
ASM_OPTION 	+= -D__ASSEMBLY__

C_OPTION 	= -DNAL_BUILD
C_OPTION 	+=  -c  
C_OPTION 	+= -nostdinc
C_OPTION 	+= -O2
C_OPTION 	+= -fno-strict-aliasing
C_OPTION 	+= -fno-common
C_OPTION 	+= -pipe
C_OPTION 	+= -fno-zero-initialized-in-bss
C_OPTION 	+= -msoft-float
C_OPTION 	+= -fno-builtin
C_OPTION 	+= -I.
C_OPTION 	+= -I../
C_OPTION 	+= -I../include/
C_OPTION 	+= -I$(MODULES_DIR)/include/
# use 32-bit enums yet the output is to use variable-size enums; use of enum values across objects may fail 에러 발생으로 아래 라인 추가
C_OPTION 	+= -fno-short-enums	
ifeq ($(CONFIG_CPU_ARM920T),y)
C_OPTION	+= -DARCH_V4
ASM_OPTION 	+= -DARCH_V4
endif
ifeq ($(CONFIG_CPU_V6),y)
C_OPTION	+= -DARCH_V6
ASM_OPTION	+= -DARCH_V6
endif
ifeq ($(CONFIG_CPU_V7),y)
C_OPTION	+= -march=armv7-a -DARCH_V7
ASM_OPTION	+= -march=armv7-a -DARCH_V7
	ifeq ($(CONFIG_ARCH_OMAP4),y)
		C_OPTION	+= -DARCH_OMAP4
		ASM_OPTION	+= -DARCH_OMAP4
	endif
	ifeq ($(CONFIG_ARCH_TI814X),y)
		C_OPTION	+= -DARCH_AM3874
		ASM_OPTION	+= -DARCH_AM3874
	endif
	ifeq ($(CONFIG_ARCH_MX6),y)
		C_OPTION	+= -DARCH_MX6
		ASM_OPTION	+= -DARCH_MX6
	endif
	ifeq ($(CONFIG_ARCH_S5P4418),y)
		C_OPTION	+= -DARCH_S5P4118
		ASM_OPTION	+= -DARCH_S5P4118
	endif
endif

ASM_OPTION	+= -DARG_REG_UART_PHYS=$(K_UART_PHYS)

ifeq ($(NALCODE_RELEASE_BUILD),y)
C_OPTION       += -DNAL_RELEASE
endif
C_OPTION	+= -DARG_REG_UART_PHYS=$(K_UART_PHYS)
C_OPTION	+= $(NAL_EXTRA_CFLAGS)
C_OPTION	+= -DNAL_PRE_LOAD

ifeq ($(DEBUG_CHECKSUM),y)
C_OPTION	+= -DDEBUG_4K_CHECKSUM
endif
ifeq ($(DEBUG_MEM_STORAGE),y)
C_OPTION	+= -DDEBUG_MEM_STORAGE
ASM_OPTION	+= -DDEBUG_MEM_STORAGE
endif

LD_OPTION   =  -static
LD_OPTION   += -nostdlib 
LD_OPTION   += -nostartfiles
LD_OPTION   += -nodefaultlibs
LD_OPTION   += -X
LD_OPTION   += -T ./$(TARGET)-$(CPU_LINKER)-ld-script
LD_OPTION   += -Map=memory.map


OC_OPTION   =  -O binary
OC_OPTION   += -R .note -R .comment
OC_OPTION   += -S

# arch dependent must append bootloader
GLOBAL_FLAG := -DARG_NAL_PHYS_OFFSET=$(K_PHYS_OFFSET) -DARG_NAL_PAGE_OFFSET=$(CONFIG_PAGE_OFFSET)

%.o:%.S
	@echo ">> Assembler compiling $< ..."
	$(CC)	$(ASM_OPTION) -o $@ $<

%.o:%.c
	@echo ">> C compiling $< ..."
	$(CC)	$(C_OPTION) $(GLOBAL_FLAG) -o $@ $<

ifeq ($(NALCODE_SOURCE),y)
all: $(OBJS)
else
all: $(NAL_OBJ)
endif

	@echo ">> MMU Link... ."
	@echo $(OBJS)
	$(LD)  	$(LD_OPTION)  $(OBJS) -o $(TARGET).elf32
	$(OC) 	$(OC_OPTION)  $(TARGET).elf32 $(TARGET).bin
	rm $(NAL_OBJ)
#	cp -a $(TARGET).bin $(CP_DIR)/$(TARGET).bin

clean:
ifeq ($(NALCODE_SOURCE),y)
	rm -f *.o
	rm -f $(OBJS)
endif
	rm -f memory.map
	rm -f $(TARGET).elf32
	rm -f $(TARGET).bin
	
dep: 
	$(CC) -M -nostdinc -I./ $(SRCS) > _depend		

ifeq (_depend,$(wildcard _depend))
include	_depend
endif

		
