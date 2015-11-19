/*-----------------------------------------------------------------------------
  파 일 : zb_cpu_s5pv210.c
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-01-04
  주 의 :

-------------------------------------------------------------------------------*/
#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h> 
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/ioport.h>
#include <linux/slab.h>     // kmalloc() 
#include <linux/poll.h>     
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/pm.h>
#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>
//#include <mach/map.h>
#include <linux/time.h>			
#include <linux/timer.h>		
#include <linux/clk.h>
#include <asm/mach/map.h>
//#include <plat/regs-timer.h>
//#include <plat/clock.h>

#include <reg_save_restore.h>
#include <zbi.h>
#include <zb_debug.h>
#include <zb_cpu.h>
#include <zb_used_memory.h>

#define STRUCT
#include <zb_nalfuncs.h>
#undef STRUCT

/// @{
/// @brief  외부 함수,변수정의
///-----------------------------------------------------------------------------
extern void zeroboot_boot_call( void ); 					// arch/arm/mach-s5pv210/mach-ezs5pv210.c
extern void zero_recover_mx6_change_ddr_freq(void);

#include "zb_cpu_nxp4418_regdata.h"
/// @}


/// @{
/// @brief  로컬 변수정의
///-----------------------------------------------------------------------------



static zbhw_list_t cpu_hwlist[] = {
		{	.name = "ARM MP",	.regs = regv_armmp  ,	.phys_base = 0x00A00000,	.phys_size = 0x2000,	.active = 1,	},
//		{	.name = "Snooping Controller Unit",	.regs = regv_scu  ,	.phys_base = 0x00A00000,	.phys_size = 0x100,	.active = 1,	},
//		{	.name = "Interrupt Controller Interface",	.regs = regv_ici  ,	.phys_base = 0x00A00000,	.phys_size = 0x2000,	.active = 1,	},
//		{	.name = "Global Timer",	.regs = regv_gtimer  ,	.phys_base = 0x00A00200,	.phys_size = 0x1000,	.active = 1,	},
//		{	.name = "Private Timer and Watchdog",	.regs = regv_ptimer_watchdog  ,	.phys_base = 0x00A00600,	.phys_size = 0x1000,	.active = 1,	},
//		{	.name = "Interrupt Distubutor",	.regs = regv_interrupt_distibutor  ,	.phys_base = 0x00A01000,	.phys_size = 0x1000,	.active = 1,	},
//		{	.name = "Enhanced Periodic Interrupt Timer",	.regs = regv_epit  ,	.phys_base = 0x020D0000,	.phys_size = 0x5000,	.active = 1,	},
		{	.name = "General Purpose Timer",	.regs = regv_gpt  ,	.phys_base = 0x02098000,	.phys_size = 0x1000,	.active = 1,	},
		{	.name = "UART1",	.regs = regv_uart1  ,	.phys_base = 0x02020000,	.phys_size = 0x10000,	.active = 1,	},
		{	.name = "UART2",	.regs = regv_uart2  ,	.phys_base = 0x021e8000,	.phys_size = 0x10000,	.active = 1,	},
		{	.name = "UART3",	.regs = regv_uart3  ,	.phys_base = 0x021ec000,	.phys_size = 0x10000,	.active = 1,	},
		{	.name = "UART4",	.regs = regv_uart4  ,	.phys_base = 0x021f0000,	.phys_size = 0x10000,	.active = 1,	},
		{	.name = "UART5",	.regs = regv_uart5  ,	.phys_base = 0x021f4000,	.phys_size = 0x10000,	.active = 1,	},
};

//#define	ZB_MEM_BLK_SIZE		(SZ_256M + SZ_128M + SZ_32M)
#define	ZB_MEM_BLK_SIZE		(SZ_512M)
#define	MEM_STORAGE_BASE	(SZ_1G - ZB_MEM_BLK_SIZE)
static struct zb_reserved_mem cpu_rev_mem[] = {
//	{ .base = 0x22a00000, .size = 0x1000000 },
	//{ .base = 0x74000000 + 0x708000 , .size = 0x0c000000 - 0x708000},	// NXP/CMA reserved
//	{ .base = 0x74000000 + 0x1563000, .size = 0x0c000000 - 0x1563000},	// NXP/CMA reserved

#ifdef CONFIG_FALINUX_ZEROBOOT_MEM_STORAGE
	{ .base = 0x40000000 + MEM_STORAGE_BASE , .size = ZB_MEM_BLK_SIZE},	// mem block
#endif
	{
		.base = 0x40000000 + 0x0D000000, 
		.size = SZ_16M,
	},	// u-boot area
	{ .base = RESERVED_MEM_END_BASE, .size = 0 }
};





/*------------------------------------------------------------------------------
  @brief   레지스터그룹에서 해당하는 레지스터의 저장값을 얻는다.
  @remark  
*///----------------------------------------------------------------------------
static u32 get_match_regval( reg_t *regs, u32 find_addr )
{
	while( 0xFFFFFFFF != regs->addr )
	{
		if ( regs->addr == find_addr )
		{
			return regs->val; 
		}
		regs ++;
	}	
	
	return 0;
}

void imx6_gpt_pause(void *iomem)
{
	unsigned int temp;
	temp = __raw_readl(iomem+0x0000);
	printk("TIMER PAUSE : %8x\n", temp);
	//__raw_writel(iomem,temp & 0xFFFE);
	__raw_writel(temp & 0xFFFE,iomem);

}

void imx6_gpt_resume(void *iomem)
{
	unsigned int timer_control;
	unsigned int temp;
	timer_control = __raw_readl(iomem + 0x0000);
	printk("TIMER Resume : %8x\n", temp);
	//__raw_writel(iomem,temp | 0x1);
	temp = __raw_readl(iomem + 0x0024); // READ Timer Counter
	printk("TIMER Cnt : %8x\n", temp);
	__raw_writel(temp + 0x1000,iomem + 0x0010); // SET TO Output compare Register
	__raw_writel(timer_control | 0x1,iomem); // Now Start Timer

}

struct zb_ion_used_mem {
	unsigned long phys;
	unsigned long len;
};

#define ZB_IO_MEM_SIZE  SZ_128K
#define ZB_ION_COUNT    (ZB_IO_MEM_SIZE / (sizeof(struct zb_ion_used_mem)))
extern unsigned long zb_get_dma_mem(void); 
extern int zb_dma_count;

#if 0
void zb_add_platform_manage_area(void) 
{ 
	// set ion nxp GL memoryr 
    struct zb_ion_used_mem *imem; 
    unsigned long idx, hit; 

    // search free idx
	imem = (struct zb_ion_used_mem *)zb_get_dma_mem();
	for (idx = 0, hit = 0; idx < ZB_ION_COUNT; idx++, imem++) {
		if (!imem->phys || imem->phys == 0xffffffff)
			continue;

		NAL_CALL(m2n_set_mark_area_of_preloading, imem->phys, imem->len, 0);
		printk("add ion memory physcal at 0x%lx, len 0x%lx\n", imem->phys, imem->len);
		hit++;
		if (hit > (zb_dma_count+1)) {
			printk("unmatched ion count hit 0x%x, zb_dma_count 0x%x\n", hit, zb_dma_count);
			panic("boggle70 bug\n");
		}	
	}
}
#else
static int get_used_info_dma_used(unsigned long phys)
{
    unsigned long dividend, remainder, order;
    unsigned char *imem = zb_get_dma_mem();
    unsigned char mask, ret;

    if (!imem) {
        printk("Fail!!! dma used mem is NULL\n");
        return;
    }

    phys &= ~(PAGE_SIZE -1);
    order = phys / PAGE_SIZE;

    dividend = order >> 8;
    remainder = order & 0x7;
    mask = 1 << remainder;

	ret = imem[dividend] & mask;

	return ret == 0 ? 	0 : 1;
}

void zb_add_platform_manage_area(void) 
{ 
	// set ion nxp GL memoryr 
	unsigned long phys_base, phys_max;
	int ret;

    // search free idx
	phys_base = PHYS_OFFSET;
	phys_max = PHYS_OFFSET + SZ_1G;

	for (phys_base = PHYS_OFFSET; phys_base < phys_max; phys_base += PAGE_SIZE) {
		ret = get_used_info_dma_used(phys_base);

		if (ret) {
			NAL_CALL(m2n_set_mark_area_of_preloading, phys_base, PAGE_SIZE, 0);
//			printk("zb add dma memory phys 0x%x\n", phys_base);
		}
	}
}
#endif

/*------------------------------------------------------------------------------
  @brief   I/O 처리 초기화 
  @remark  
*///----------------------------------------------------------------------------
void zb_io_init( void  )
{
	zb_registe_reserved_mem( cpu_rev_mem );

	zeroboot_checkin_hw_list( cpu_hwlist, sizeof(cpu_hwlist)/sizeof(cpu_hwlist[0]) );
}

/*------------------------------------------------------------------------------
  @brief   I/O 처리 종료
  @remark  
*///----------------------------------------------------------------------------
void zb_io_free( void  )
{
	zeroboot_checkout_hw_list();
}

/*------------------------------------------------------------------------------
  @brief   I/O 상태를 저장한다.
  @remark  
*///----------------------------------------------------------------------------
void zb_io_save( void  )
{
	zbi_t    *zbi;
	//zbhw_list_t *gpc = zeroboot_get_hw_list( "General Purpose Timer" );
	zbhw_list_t *armmp = zeroboot_get_hw_list( "ARM MP" );

	//if ( gpc != NULL ) imx6_gpt_pause(gpc->iomem);
	//	else printk("GPC Pointer is Null !!!!!!!\n");

	
	zeroboot_save_hw_list();

	zbi = get_zbi_base();
	if (!zbi) {
		printk("BUG... zbi base is NULL\n");
	} else {
		zbi->armmp_base = armmp->iomem;
	}
}




/*------------------------------------------------------------------------------
  @brief   I/O 상태를 복구한다.
  @remark  
*///----------------------------------------------------------------------------
void zb_io_restore( void  )
{
	zbhw_list_t *gpc = zeroboot_get_hw_list( "General Purpose Timer" );
	zbhw_list_t *scu = zeroboot_get_hw_list( "Snooping Controller Unit" );

	zeroboot_restore_hw_list();

	zeroboot_restore_late_hw_list();

//	zero_recover_mx6_change_ddr_freq();
	if ( gpc != NULL ) imx6_gpt_resume(gpc->iomem);
	else printk("GPC Pointer is Null !!!!!!!\n");

	if (scu) {
		__raw_writel(0x01, scu->iomem);		// scu disable
		__raw_writel(0xfff, scu->iomem+0x54);	// snsac(scu non-secure access control) 
	} else {
		printk("scu is not implements\n");
	}
}
/*------------------------------------------------------------------------------
  @brief   I/O 상태를 복구한다.
  @remark  인터럽트가 활성화 된 후 호출된다.
*///----------------------------------------------------------------------------
void zb_io_restore_with_irq( void  )
{
	printk("call zb_io_restore_with_irq called at 1 sec.. after\n");
	//zeroboot_show_hw_list();

}

/*------------------------------------------------------------------------------
  @brief   레지스터 내용을 화면에 출력한다. 
  @remark  
*///----------------------------------------------------------------------------
void zb_io_show_all( void )
{
	zeroboot_show_hw_list();
	// zeroboot_show_reg(  iomem_sample, regv_sample );
}



