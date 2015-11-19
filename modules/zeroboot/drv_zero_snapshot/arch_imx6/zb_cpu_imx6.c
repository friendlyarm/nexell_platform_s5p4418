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
extern void vfp_enable(void *unused);
extern void zeroboot_boot_call( void ); 					// arch/arm/mach-s5pv210/mach-ezs5pv210.c
extern void zero_recover_mx6_change_ddr_freq(void);

#include "zb_cpu_imx6_regdata.h"
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


static struct zb_reserved_mem cpu_rev_mem[10] = {
//	{ .base = 0x22a00000, .size = 0x1000000 },
	{ .base = RESERVED_MEM_END_BASE, .size = 0 }
};





/*------------------------------------------------------------------------------
  @brief   레지스터그룹에서 해당하는 레지스터의 저장값을 얻는다.
  @remark  
*///----------------------------------------------------------------------------

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
	//__raw_writel(iomem,temp | 0x1);
	temp = __raw_readl(iomem + 0x0024); // READ Timer Counter
	printk("TIMER Cnt : %8x\n", temp);
	__raw_writel(temp + 0x1000,iomem + 0x0010); // SET TO Output compare Register
	__raw_writel(timer_control | 0x1,iomem); // Now Start Timer

}

struct zb_dma_used_mem {
	unsigned long phys;
	unsigned long len;
};

#define ZB_IO_MEM_SIZE  SZ_128K
#define ZB_ION_COUNT    (ZB_IO_MEM_SIZE / (sizeof(struct zb_dma_used_mem)))
extern unsigned long zb_get_dma_mem(void); 
extern int zb_dma_count;
void zb_add_platform_manage_area(void) 
{ 
	// set dma nxp GL memoryr 
    struct zb_dma_used_mem *imem; 
    unsigned long idx, hit; 

    // search free idx
	imem = (struct zb_dma_used_mem *)zb_get_dma_mem();
	if (!imem)
		return;

	for (idx = 0, hit = 0; idx < ZB_ION_COUNT; idx++, imem++) {
		if (!imem->phys || imem->phys == 0xffffffff)
			continue;

		printk("add dma memory physcal at 0x%lx, len 0x%lx\n", imem->phys, imem->len);
		NAL_CALL(m2n_set_mark_area_of_preloading, imem->phys, imem->len, 0);
		hit++;
		if (hit > (zb_dma_count+1)) {
			printk("unmatched dma count hit 0x%lx, zb_dma_count 0x%x\n", hit, zb_dma_count);
			panic("boggle70 bug\n");
		}	
	}
}

/* imx6 gpu vivante memory reserved area */
struct viv_gpu_platform_data {
    resource_size_t reserved_mem_base;
    resource_size_t reserved_mem_size;
}; 
extern struct viv_gpu_platform_data zb_imx6q_gpu_rsv[2];
static struct zb_reserved_mem *search_unused_rsv(struct zb_reserved_mem *rsv)
{
	int i;

	for (i = 0; i < 10; i++, rsv++) {
		if (!rsv->base && !rsv->size)
			return rsv;
	}
	return NULL;
}

static void zb_add_viv_area(struct zb_reserved_mem *rsv)
{
	struct viv_gpu_platform_data *viv_rsv = zb_imx6q_gpu_rsv;
	struct zb_reserved_mem *zb_mem;
	int i;

	for (i = 0; i < 2; i++, viv_rsv++) {
		if (!viv_rsv->reserved_mem_base || !viv_rsv->reserved_mem_size)
			continue;

		zb_mem = search_unused_rsv(rsv);
		if (!zb_mem)
			continue;

		zb_mem->base = viv_rsv->reserved_mem_base;
		zb_mem->size = viv_rsv->reserved_mem_size;
//		printk("zb_mem base 0x%08X\n", zb_mem->base);
//		printk("zb_mem size 0x%08X\n", zb_mem->size);
	}
}

void zb_io_init( void  )
{
	zb_add_viv_area(cpu_rev_mem);
	zb_registe_reserved_mem( cpu_rev_mem );

	if (0)
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
		zbi->armmp_base = (unsigned int)armmp->iomem;
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

	vfp_enable( NULL );

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



