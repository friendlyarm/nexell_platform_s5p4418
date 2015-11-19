
.EXPORT_ALL_VARIABLES:

KDIR   ?= $(SDK_DIR)/kernel/build_your_kernel
CP_DIR ?= $(SDK_DIR)/out

TARGET 		= nalcode

#################################################################
# 아키텍쳐의 설정 정보를 가져온다
# 컴파일 옵션과 메모리의 하드웨어 주소를 맞추어 준다
#################################################################
include $(KDIR)/.config

#################################################################
# linker script select
#################################################################
ifeq ($(CONFIG_PAGE_OFFSET),0x40000000)
CPU_LINKER  = 40000000
endif
ifeq ($(CONFIG_PAGE_OFFSET),0x80000000)
CPU_LINKER  = 80000000
endif
ifeq ($(CONFIG_PAGE_OFFSET),0xC0000000)
CPU_LINKER  = C0000000
endif

ASM_SOURCE	=  
ASM_SOURCE 	+= fault_entry.S

C_SOURCE	=  
C_SOURCE	+= fault_main.c 
C_SOURCE	+= uart.c 

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

#################################################################
# 릴리즈 버전 빌드
#################################################################
ifeq ($(NALCODE_RELEASE_BUILD),y)
ASM_SOURCE 	+= $(KDIR)/nalsecure.S
endif


C_SOURCE	+= string.c
C_SOURCE	+= printf.c
C_SOURCE	+= nal_m2n.c

# 플래쉬 디바이스 
CC			= $(CROSS_COMPILE)gcc
LD			= $(CROSS_COMPILE)ld
OC			= $(CROSS_COMPILE)objcopy
STRIP		= $(CROSS_COMPILE)strip

ASM_OBJS  	= $(ASM_SOURCE:%.S=%.o)
C_OBJS    	= $(C_SOURCE:%.c=%.o)

SRCS		= $(ASM_SOURCE) $(C_SOURCE)	
OBJS 		= $(ASM_OBJS) $(C_OBJS)  $(NAL_OBJ)

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
endif
ifeq ($(CONFIG_CPU_V7),y)
C_OPTION	+= -march=armv7-a -DARCH_V7
ASM_OPTION	+= -march=armv7-a -DARCH_V7
	ifeq ($(CONFIG_ARCH_OMAP4),y)
		C_OPTION	+= -DARCH_OMAP4
	endif
	ifeq ($(CONFIG_ARCH_TI814X),y)
		C_OPTION	+= -DARCH_AM3874
	endif
	ifeq ($(CONFIG_ARCH_MX6),y)
		C_OPTION	+= -DARCH_MX6
	endif
	ifeq ($(CONFIG_ARCH_S5P4418),y)
		C_OPTION	+= -DARCH_S5P4118
	endif
endif

C_OPTION	+= -DARG_REG_UART_VIRT=$(K_UART_VIRT)
ASM_OPTION	+= -DARG_REG_UART_VIRT=$(K_UART_VIRT)
C_OPTION	+= $(NAL_EXTRA_CFLAGS)

ifeq ($(DEBUG_CHECKSUM),y)
C_OPTION	+= -DDEBUG_4K_CHECKSUM
endif
ifeq ($(DEBUG_FULL_RESTORE),y)
C_OPTION	+= -DFULL_RESTORE_AT_BOOT
ASM_OPTION	+= -DFULL_RESTORE_AT_BOOT
endif

ifeq ($(BACKUP_L2),y)
C_OPTION 	+= -DBACKUP_L2
endif

ifeq ($(CONFIG_SMP),y)
C_OPTION	+= -DCONFIG_SMP
ASM_OPTION	+= -DCONFIG_SMP
endif

ifeq ($(CONFIG_CACHE_L2X0),y)
C_OPTION	+= -DCONFIG_CACHE_L2X0
ASM_OPTION	+= -DCONFIG_CACHE_L2X0
endif

#################################################################
# build only debugging version
#################################################################
ifeq ($(NALCODE_RELEASE_BUILD),y)
C_OPTION	+= -DNAL_RELEASE
else
ASM_SOURCE 	+= lib1funcs.S
C_SOURCE	+= vsprintf.c
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
	$(CC)	$(ASM_OPTION) $(GLOBAL_FLAG) -o $@ $<

%.o:%.c
	@echo ">> C compiling $< ..."
	$(CC)	$(C_OPTION) $(GLOBAL_FLAG) -o $@ $<

ifeq ($(NALCODE_SOURCE),y)
all: $(OBJS)
else
all: $(NAL_OBJ)
endif

ifeq ($(NALCODE_RELEASE_BUILD),y)
	@echo ""
	@echo ""
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
	@echo ">> You try build Nalcode Release Version"
	@echo -ne ">> Please enter for confirm : ENTER"
	@read $(DUMMY)
	@echo ">> OK, Continue Building....."
	@echo ">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>"
endif
	@echo ">> NAL Link... ."
	$(LD)  	$(LD_OPTION)  $(OBJS) -o $(TARGET).elf32
	$(OC) 	$(OC_OPTION)  $(TARGET).elf32 $(TARGET).bin_
	rm $(NAL_OBJ)
	
	make -C mmu_turnon/ || exit $?

	dd if=$(TARGET).bin_ of=$(TARGET).bin bs=1k conv=sync
	dd if=mmu_turnon/mmu_turnon.bin of=$(TARGET).bin bs=1k seek=32
	rm $(TARGET).bin_
	cp -a $(TARGET).bin $(CP_DIR)/$(TARGET).bin

clean:
	make clean -C mmu_turnon/
ifeq ($(NALCODE_SOURCE),y)
	rm -f *.o
	rm -f $(OBJS)
	rm -f $(ASM_OBJS)
endif
	rm -f memory.map
	rm -f $(TARGET).elf32
	rm -f $(TARGET).bin

dep: 
	$(CC) -M -nostdinc -I./ $(SRCS) > _depend		

ifeq (_depend,$(wildcard _depend))
include	_depend
endif

		
