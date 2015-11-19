/*-----------------------------------------------------------------------------
  파 일 : drv_zero_snapshot.c
  설 명 : 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
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
#include <linux/workqueue.h>
#include <linux/ioport.h>
#include <linux/slab.h>     // kmalloc() 
#include <linux/poll.h>     // poll
#include <linux/proc_fs.h>
#include <linux/irq.h>
#include <linux/gfp.h>

#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>

#include <linux/cdev.h>          
#include <linux/platform_device.h>
#include <linux/err.h>

#include <asm/gpio.h>
#include <linux/irqflags.h>

#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/bootmem.h>

#include <asm/setup.h>

#include <linux/mmzone.h> // search_all_pg_data()
#include <linux/pagemap.h>

#include <linux/delay.h>
#include <asm/unistd.h>
#include <asm/io.h>

#include <asm/pgalloc.h>
#include <asm/page.h>
#include <asm/tlbflush.h>         
#include <linux/gfp.h>

#include <linux/time.h>			
#include <linux/timer.h>		
#include <linux/clk.h>

#include <asm/mach/map.h>

#include "zbi.h"
#include "zb_debug.h"

#include "zb_nalcode.h"
#include "zb_proc.h"
#include "zb_mmu.h"
#include <zb_mmu.h>
#include <zb_cpu.h>
#include <zb_mmu_debug.h>
#include <zb_blk.h>
#include "zb_used_memory.h"

#include "zb_snapshot.h"
#include "zb_trigger.h"

#define DEV_NAME    "zeroboot_snapshot"
                                       
#define BOGGLE70_DEBUG	0
#define FREEFRUG_DEBUG	0
                                       
#define STRUCT
#include <zb_nalfuncs.h>
#undef STRUCT

u32 (*nalcode_func)( u32 function_offset, u32 arg1, u32 arg2, u32 arg3 );
nalcode_head_t *nalcode = NULL;

/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------
#include <nalcode_storage.h>
static u32 zb_status = ZB_STATUS_NORMALBOOT;
static wait_queue_head_t zb_status_waitq;
static struct delayed_work  zb_workq;  
void zb_set_status( u32 status );
/// @}

static int debug_mode = 0;
static int zb_dma_area_max = 0;


#define MAX_DMA_AREA_COUNT	6

struct zb_dma_area_info {
	unsigned long start_phys;
	int size;
	unsigned long virt;
};
struct zb_dma_area_info zb_dma_area[MAX_DMA_AREA_COUNT];


module_param( debug_mode, int, 0 );

/*------------------------------------------------------------------------------
  @brief  부트로더가 관리하는 영역 보호 지정
  @remark add full zbi hole size, not used size
*///----------------------------------------------------------------------------
void zb_add_protected_bootloader_manage_area(void)
{
	NAL_CALL(m2n_set_mark_protect_bootloader_manage_area, ZBI_PADDR, ZBI_MAX_SIZE, 0);
}

unsigned long zb_get_text(void);
unsigned long zb_get_etext(void);
unsigned long zb_get_sdata(void);
unsigned long zb_get_edata(void);
unsigned long zb_get___bss_start(void);
unsigned long zb_get___bss_stop(void);

extern unsigned long zb_get_idmap_pgd_phys(void);
unsigned long zb_get_idmap_text_start(void);
unsigned long zb_get_idmap_text_end(void);
extern int (*zb_snapshot_fn)(unsigned long);	// platform_suspend_ops.enter function at arch/arm/mach-xxx/pm.c 
extern int  zb_app_inf_init( void );	// zb_app_inf.c
extern void zb_app_inf_exit( void );

#include <asm/cpu.h>
void zb_add_protected_kernel_manage_area(void)
{
	phys_addr_t text, etext, sdata, edata, bss_start, bss_stop;
	phys_addr_t idmap_text_start, idmap_text_end;
	unsigned long nalstack_start, nalstack_size;
	unsigned long zb_sp;
	struct task_struct 	*p;
	unsigned int cpu;

#if 0
	text = zb_get_text();
	etext = zb_get_etext();
	etext += PAGE_SIZE-1;
	etext &= PAGE_MASK;
	NAL_CALL(m2n_set_mark_area_of_preloading, virt_to_phys((void *)text), etext - text, 0);
	printk("add preloading for phys text 0x%x - 0x%x\n", text, etext );

	sdata= zb_get_sdata();
	edata= zb_get_edata();
	edata += PAGE_SIZE-1;
	edata &= PAGE_MASK;
	NAL_CALL(m2n_set_mark_area_of_preloading, virt_to_phys((void *)sdata), edata - sdata, 0);
	printk("add preloading for data 0x%x - 0x%x\n", sdata, edata);

	bss_start = zb_get___bss_start();
	bss_stop = zb_get___bss_stop();
	bss_stop += PAGE_SIZE-1;
	bss_stop &= PAGE_MASK;
	NAL_CALL(m2n_set_mark_area_of_preloading, virt_to_phys((void *)bss_start), bss_stop - bss_start, 0);
	printk("add preloading for bss  0x%x - 0x%x\n", bss_start, bss_stop);
#else
	text = zb_get_text();
	etext = zb_get___bss_stop();
	etext += PAGE_SIZE-1;
	etext &= PAGE_MASK;
	NAL_CALL(m2n_set_mark_area_of_preloading, virt_to_phys((void *)text), etext - text, 0);
	printk("add preloading for phys text 0x%x - 0x%x\n", text, etext );
#endif

	for_each_process(p)	{
		zb_sp = (unsigned long)p->stack;
		zb_sp &= PAGE_MASK;
		NAL_CALL(m2n_set_mark_area_of_preloading, virt_to_phys((void *)zb_sp), SZ_8K, 0);
		printk("add preloading for sp   0x%x\n", virt_to_phys((void *)zb_sp));
	}

#ifdef CONFIG_SMP
	for_each_present_cpu(cpu) {
 		struct cpuinfo_arm *ci = &per_cpu(cpu_data, cpu);
 		struct task_struct *idle = ci->idle;

		if (!idle || !idle->stack)
			continue;

		zb_sp = (unsigned long)idle->stack;
		zb_sp &= PAGE_MASK;
		NAL_CALL(m2n_set_mark_area_of_preloading, virt_to_phys((void *)zb_sp), SZ_8K, 0);
		printk("add preloading for idle sp   0x%x\n", virt_to_phys((void *)zb_sp));
	}
#endif
	
	idmap_text_start = zb_get_idmap_text_start();
	idmap_text_end = zb_get_idmap_text_end();
	NAL_CALL(m2n_set_mark_area_of_preloading, idmap_text_start, idmap_text_end - idmap_text_start, 0);
	printk("add preloading for idmap 0x%x\n", idmap_text_start);

	// at return from suspend
	// use mm->context.id
	// so, i want save current mm struct
	{
		struct mm_struct *mm = current->active_mm;
		unsigned long mm_start, mm_end, mm_size;

		mm_start = virt_to_phys(mm);
		mm_end = mm_start + sizeof(struct mm_struct);

		mm_end += PAGE_SIZE-1;
		mm_end &= PAGE_MASK;

		mm_size = mm_end - (mm_start & PAGE_MASK);

		NAL_CALL(m2n_set_mark_area_of_preloading, mm_start & PAGE_MASK, mm_size, 0);
		printk("add active_mm for switch 0x%lx, size 0x%lx\n", mm_start, mm_size);
	}

	nalstack_start = NALCODE_STACK_PADDR;
	nalstack_size = NALCODE_STACK_SIZE;
	NAL_CALL(m2n_set_mark_area_of_preloading, nalstack_start, nalstack_size, 0);

}

void zb_add_arch_manage_area(void)
{
	zb_add_platform_manage_area();
}

void zb_add_driver_work(void)
{
	unsigned long *start;
	unsigned long size;

	start = zb_get_mem_array();
	size = zb_get_mem_array_size();

	NAL_CALL(m2n_set_mark_area_of_work, virt_to_phys((void *)start), size, 0);
}

void zb_pre_setup_dma_area(unsigned long start_phys, unsigned long virt, int size )
{
	static int zb_dma_area_index = 0;

	zb_dma_area[zb_dma_area_index].start_phys = start_phys;
	zb_dma_area[zb_dma_area_index].size= size;
	zb_dma_area[zb_dma_area_index].virt= virt;
	printk("Pre Setup Memory Snapshot subtraction : addr : %lx, size : %8x\n",zb_dma_area[zb_dma_area_index].start_phys, zb_dma_area[zb_dma_area_index].size);
	zb_dma_area_index++;
	zb_dma_area_max = zb_dma_area_index;
}
EXPORT_SYMBOL(zb_pre_setup_dma_area);


void zb_add_dma_area(void )
{
	int i;

	for ( i=0; i<zb_dma_area_max; i++ ) {
		printk("ADD Memory Snapshot subtraction : addr : %lx, size : %8x\n",zb_dma_area[i].start_phys, zb_dma_area[i].size);
		NAL_CALL(m2n_set_mark_area_of_work, zb_dma_area[i].start_phys, zb_dma_area[i].size, 0);
	}
}


/*------------------------------------------------------------------------------
  @brief   	MMU 데이터를 부트로더가 복사하는 대상으로 지정 실제 구현 루틴
  @remark  	
*///----------------------------------------------------------------------------
void zb_add_boot_copy_area_of_mmu_data_detail( u32 ttb_paddr )
{
	volatile u32 	*ttb_vaddr;
 	u32  			l1_index;
 	u32				l1_entry;
 	u32				l1_entry_type;
	unsigned long l1_entry_min, l1_entry_max;
	
	
	NAL_CALL(m2n_set_mark_boot_copy_area_of_mmu_l1, ttb_paddr, 4096*4 ,0);

	ttb_vaddr = (volatile u32 *) phys_to_virt( ttb_paddr ) ;
	l1_entry_min = PHYS_OFFSET;
	l1_entry_max = (unsigned long)high_memory - (PAGE_OFFSET - PHYS_OFFSET) -1;
	
 	for( l1_index = 0; l1_index < MMU_L1_MAX_ENTRY; l1_index++ )
 	{
 		l1_entry 		= ttb_vaddr[l1_index];
 		l1_entry_type 	= l1_entry & MMU_L1_MASK_TYPE;
 		if( l1_entry_type == MMU_L1_COARSE )
 		{	
			if ((l1_entry < l1_entry_min) || (l1_entry > l1_entry_max)) {
				printk(" invalid Coarse area\n");
 				printk( "VADDR = %03X0-0000 L1_ENTRY = %08X\n", l1_index, l1_entry );
				BUG();
			}
#ifndef FULL_RESTORE_AT_BOOT
			NAL_CALL(m2n_set_mark_boot_copy_area_of_mmu_l2, MMU_L1_PADDR_COARSE(l1_entry), 1024, 0);
#endif
 		}
 	}
}

void zb_add_boot_copy_area_of_mmu_data_detail1( u32 ttb_paddr )
{
	volatile u32 	*ttb_vaddr;
 	u32  			l1_index;
 	u32				l1_entry;
 	u32				l1_entry_type;
	unsigned long l1_entry_min, l1_entry_max;
	
	
	NAL_CALL(m2n_set_mark_boot_copy_area_of_mmu_l1, ttb_paddr, 4096*4 ,0);

	ttb_vaddr = (volatile u32 *) phys_to_virt( ttb_paddr ) ;
	l1_entry_min = PHYS_OFFSET;
	l1_entry_max = (unsigned long)high_memory - (PAGE_OFFSET - PHYS_OFFSET) -1;
	
 	for( l1_index = 0; l1_index < MMU_L1_MAX_ENTRY; l1_index++ )
 	{
 		l1_entry 		= ttb_vaddr[l1_index];
 		l1_entry_type 	= l1_entry & MMU_L1_MASK_TYPE;
 		if( l1_entry_type == MMU_L1_COARSE )
 		{	
			if ((l1_entry < l1_entry_min) || (l1_entry > l1_entry_max)) {
				printk(" invalid Coarse area\n");
 				printk( "VADDR = %03X0-0000 L1_ENTRY = %08X\n", l1_index, l1_entry );
				BUG();
			}
#ifndef FULL_RESTORE_AT_BOOT
			NAL_CALL(m2n_set_mark_boot_copy_area_of_mmu_l2, MMU_L1_PADDR_COARSE(l1_entry), 1024, 0);
#endif
 		}
 	}
}

// exchange two address space mapping table to fake 
int zb_exchange_address_space_with_two_pgd(pgd_t *src, pgd_t *dst, unsigned long addr)
{
	pgd_t *spgd, *dpgd;
	pud_t *spud, *dpud;
	pmd_t *spmd, *dpmd;
	unsigned long sval, dval;

	spgd = src + pgd_index(addr);
	dpgd = dst + pgd_index(addr);

	spud = pud_offset(spgd, addr);
	dpud = pud_offset(dpgd, addr);

 	spmd = pmd_offset(spud, addr);
 	dpmd = pmd_offset(dpud, addr);

	sval = pmd_val(*spmd);
	dval = pmd_val(*dpmd);

	*spmd = dval;
	*dpmd = sval;

	return 0;
}

#include <asm/cputype.h>
#include <asm/pgalloc.h>
#include <asm/pgtable.h>
#include <asm/sections.h>
#ifndef CONFIG_ARM_LPAE
// set dma mapping table for idmap, copy swapper to idmap for vaddr
static int zb_set_idmap_for_dma_mapping(pgd_t *idmap, pgd_t *swap_ptr, unsigned long vaddr)
{
	pgd_t *spgd, *dpgd;
	pud_t *spud, *dpud;
	pmd_t *spmd, *dpmd;

	spgd = swap_ptr + pgd_index(vaddr);
	dpgd = idmap + pgd_index(vaddr);

	spud = pud_offset(spgd, vaddr);
	dpud = pud_offset(dpgd, vaddr);

 	spmd = pmd_offset(spud, vaddr);
 	dpmd = pmd_offset(dpud, vaddr);

	if (vaddr & SECTION_SIZE)
		dpmd[1] = spmd[1];
	else
		dpmd[0] = spmd[0];
	
	return 0;
}
#else
// still not work
#endif

/*------------------------------------------------------------------------------
  @brief   	MMU 데이터를 부트로더가 복사하는 대상으로 지정
  @remark  	
*///----------------------------------------------------------------------------
void zb_add_boot_copy_area_of_mmu_data( void )
{
	u32  				ttb_paddr;
	struct task_struct 	*p;
	pgd_t *idmap, *swap_ptr;
	unsigned long idmap_start;
	int i;
	
	// we needs dma mapping for idmap
	for ( i=0; i<zb_dma_area_max; i++ ) {
		idmap = (pgd_t *)phys_to_virt(zb_get_idmap_pgd_phys());
		swap_ptr = (pgd_t *)KERNEL_SWAPPER_PG_DIR_VADDR;
		idmap_start = zb_dma_area[i].virt;
		zb_set_idmap_for_dma_mapping(idmap, swap_ptr, idmap_start);
	}

	// swapper save
	ttb_paddr = ( u32 )(virt_to_phys((void *)KERNEL_SWAPPER_PG_DIR_VADDR));
	zb_add_boot_copy_area_of_mmu_data_detail( ttb_paddr );
	printk( ">> Swapper MMU TTB PADDR = %08X\n", ttb_paddr );

	// idmap pgd save
	ttb_paddr = zb_get_idmap_pgd_phys();
	zb_add_boot_copy_area_of_mmu_data_detail1( ttb_paddr );
	printk( ">> idmap pgd paddr TTB PADDR = %08X\n", ttb_paddr );

	for_each_process(p)	{
		if( p->mm == NULL ) 
			continue;

		ttb_paddr = virt_to_phys(p->mm->pgd) ;	
		zb_add_boot_copy_area_of_mmu_data_detail( ttb_paddr );
		printk(">> Process[%s] TTB PADDR = 0x%p:%08X\n", p->comm, p->mm->pgd, ttb_paddr);
	}
}

/*------------------------------------------------------------------------------
  @brief   NALCODE 영역을  부트로더가 복사하는 대상으로 지정
  @remark  	
*///----------------------------------------------------------------------------
void zb_add_boot_copy_area_of_nalcode( void )
{
	u32 nalcode_start_paddr;
	u32 nalcode_size;
	
	nalcode_start_paddr = zb_nalcode_start_paddr();
	nalcode_size        = zb_nalcode_size();
	NAL_CALL(m2n_set_mark_boot_copy_area_of_nalcode, nalcode_start_paddr, nalcode_size, 0);

	// reserved area
	nalcode_start_paddr += nalcode_size;
	nalcode_size = zb_nalcode_spare_size();
	NAL_CALL(m2n_set_mark_area_of_work, nalcode_start_paddr, nalcode_size, 0);
}

void zb_add_boot_copy_area_of_m2n( void )
{
	unsigned int size;
	void *m2n_data;

	m2n_data = (void *)NAL_CALL(m2n_get_mem, 0, 0, 0);
	size = NAL_CALL(m2n_get_count, 0, 0, 0);
	size *= NAL_CALL(m2n_get_size, 0, 0, 0);
	NAL_CALL(m2n_set_mark_boot_copy_area_of_nalcode, virt_to_phys((void *)m2n_data), size, 0);
}

/*------------------------------------------------------------------------------
  @brief   VECTOR 영역을  부트로더가 복사하는 대상으로 지정
  @remark  	
*///----------------------------------------------------------------------------
void zb_add_boot_copy_area_of_vector( void )
{
	u32 paddr;
	
	paddr = mmu_virt_to_phys( KERNEL_VECTOR_TABLE_VADDR );

	NAL_CALL(m2n_set_mark_boot_copy_area_of_vector, paddr, PAGE_SIZE ,0);
}

/*------------------------------------------------------------------------------
  @brief   비디오 영역을  부트로더가 복사하는 대상으로 지정
  @remark  	
*///----------------------------------------------------------------------------
void zb_add_boot_copy_area_of_video( void )
{
	u32 paddr = 0;
	u32 size;

//	zb_io_get_info_video( &paddr, &size );

//	size = 800*480*4;	// RGB 24bit mode
//	size = 800*480*2;	// 656 16bit mode

	if( !paddr ) return;

	NAL_CALL(m2n_set_mark_boot_copy_area_of_video, paddr, size, 0);
}

/*------------------------------------------------------------------------------
  @brief   복귀 위치를 지정한다. 
  @remark  	
*///----------------------------------------------------------------------------
u32 zb_add_return_position( void )
{
	u32 			cpu_restore;

	cpu_restore  	= (u32 ) (&(nalcode->nalcode_cpu_restore_entry));
	zbi_jump_vaddr( cpu_restore );					
	
	return 0;
}

void zb_add(void)
{
	zb_add_protected_bootloader_manage_area();			// 부트로더 관리 영역
	zb_add_protected_kernel_manage_area();				// 커널에서 저장하야할 poreloading data
	zb_add_arch_manage_area();
	zb_add_boot_copy_area_of_nalcode();					// NALCODE 영역을 부트로더가 복사하는 대상으로 지정
	zb_add_boot_copy_area_of_m2n();
	zb_add_boot_copy_area_of_vector();					// VECTOR 영역을 부트로더가 복사하는 대상으로 지정
	zb_add_boot_copy_area_of_video();					// 비디오 영역을 복사 영역으로 지정한다.
	zb_add_return_position();							// 복귀되는 위치를 지정한다. 
	zb_add_page_value();								// Zbi 의 정보를 저장한다.
	zb_add_driver_work();
	zb_add_dma_area();

	// always place at last
	zb_add_boot_copy_area_of_mmu_data();				// 현재 MMU 데이터를 부트로더가 복사하는 대상으로 지정
}

extern u32 zero_trace;
extern u32 (* zero_set_pte_ext_hook)( u32 *pte, u32 pte_val, u32 pte_ext );
u32 zero_set_pte_ext_hook_func( u32 *pte, u32 pte_val, u32 pte_ext )
{
	unsigned long	irq_flag;
	volatile u32    paddr;
	volatile u32    *pte_phys;
	volatile u32    dummy;
	
#ifdef FULL_RESTORE_AT_BOOT
	return 0;
#endif

	local_irq_save(irq_flag);
	local_irq_disable();

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37)
	pte_phys = pte + 2048/4;
#else
	pte_phys = pte - 2048/4;
#endif

	dummy = *pte_phys;
	if( dummy != 0 ) 
	{
		paddr = MMU_L2_PADDR_SMALL(dummy);
		do
		{
//			printk( "zero_set_pte_ext_hook_func(S) pte_phys = %p dummy= %08X paddr= %08X\n", pte_phys, dummy, paddr);

			// dummy is org pte value
			// after nal call modified
			// access pte_phys
			// recover dymy to modified
			// pte entry reset

			NAL_CALL(read_mem_from_paddr, paddr, (u32)pte_phys, 0);
			//*pte_phys = dummy;

			//asm("mcr p15, 0, %0, c7, c10, 1" : : "r" (pte_phys) : "cc");

			//NAL_CALL(read_mem_from_paddr, paddr, (u32)pte_phys, 0);
			//pte_phys = phys_to_virt( MMU_L2_PADDR_SMALL(pte_val) );
		} while(0);
	}	

	local_irq_restore(irq_flag);

	return 0;
}

static void zb_display_memory_info( u32 dram_phys_base, u32 dram_size )
{
	printk( "Analyzer start....\n");
	
	printk( ">> RAM PADDR = %08X\n", dram_phys_base );
	printk( ">> RAM VADDR = %08X\n", (int) phys_to_virt(dram_phys_base) );
	printk( ">> RAM SIZE  = %08X:%d MB\n", dram_size , dram_size / (1024*1024) );

	printk( ">> total page = %ld\n", dram_size/PAGE_SIZE);
}
extern void * high_memory;

extern void (*zeroboot_cpu_idle_hook)( void );	// arch/arm/kernel/process.c
static void zba_zeroboot_cpu_idle_hook( void )
{
	static volatile unsigned long pfn = 0;
	volatile void *zb_idle_mem_offset;
	volatile unsigned long dummy;
	unsigned long irq_flag, cur;
	int high;
#ifdef CONFIG_HIGHMEM
	struct page *from;
	void *kfrom;
#endif

	local_irq_save(irq_flag);
	local_irq_disable();

	cur = zb_get_next_page(pfn, &high);

	if (cur == pfn) {
		printk("\nzb idle process end\n");
		zeroboot_cpu_idle_hook = NULL;
		zb_set_status( ZB_STATUS_ZEROBOOT_RECOVERY_DONE );
		local_irq_restore(irq_flag);
		return;
	}

	if (high) {
#ifdef CONFIG_HIGHMEM
		from = pfn_to_page(cur);
		kfrom = kmap_atomic(from, KM_USER0);
		kunmap_atomic(kfrom, KM_USER0);
#else
		printk("Unexpeted pfn 0x%08lX\n", pfn);
		while(1);
#endif

	} else {
		zb_idle_mem_offset = (void *)((cur<<PAGE_SHIFT) - PHYS_OFFSET + PAGE_OFFSET);
//		printk("\nzb_idle_mem_offset 0x%08X\n", (unsigned int)zb_idle_mem_offset);
		dummy = *((volatile unsigned long *)zb_idle_mem_offset);
	}

	pfn = cur;
	local_irq_restore(irq_flag);
}
		 
// 할당해야할 추가 메모리 크기를 알려준다.
static unsigned int get_used_max_by_order(unsigned int used_count, unsigned int array_size)
{
	unsigned int size, pre_order, order, align_size;

	size = used_count * array_size;
	pre_order = get_order(size);

	align_size = PAGE_SIZE << pre_order;
	align_size = align_size / PAGE_SIZE * array_size;

	order = get_order( size + align_size );

	return order;
}
static int m2n_order;
static unsigned int m2n_setup(unsigned int dram_size)
{
	u32	used_map_count, order, size;
	u32	*m2n_data;

	// 날코드의 m2n 오픈
	// 커널에서 사용된 메모리의 크기를 구한후  
	// 추가로 필요한 메모리의 크기를 더하여 실제 사용될 크기를 구한다.
	// m2n_clean 을 호출하여 초기화한다.
	NAL_CALL(m2n_open, 0, 0, 0);

	used_map_count = zb_get_used_mem_count();
	size = NAL_CALL(m2n_get_size, 0, 0, 0);
	order = get_used_max_by_order( used_map_count, size);
	used_map_count += (PAGE_SIZE<<order)/PAGE_SIZE;

	m2n_data = (u32 *) __get_free_pages(GFP_ATOMIC | GFP_DMA, order );
	NAL_CALL(m2n_set_count, used_map_count, 0, 0);
	NAL_CALL(m2n_set_mem, (u32)m2n_data, (u32)virt_to_phys((void *)m2n_data), 0);
	NAL_CALL(m2n_clean, 0, 0, 0);
	NAL_CALL(m2n_set_dram_size, dram_size, 0, 0);
	printk(">>used_map_count = 	0x%08X\n", used_map_count);
	printk(">>m2n_size		 =	0x%08X\n", size);
	printk(">>m2n_data		 =	0x%p\n", m2n_data);
	printk(">>m2n_order		 =	0x%08X\n", order);

	m2n_order = order;

	return used_map_count;
}
static void m2n_remove(void)
{
	u32	m2n_data;

	if (m2n_order == 0) return;

	m2n_data = NAL_CALL(m2n_get_mem, 0, 0, 0);
	if (m2n_data)
		free_pages((unsigned long)m2n_data, get_order(m2n_order) );
}

int zb_snapshot(unsigned long opt)
{
	static int shapshot_done=0;
	u32		dram_size;
	
	if (shapshot_done)
		return 1;
	shapshot_done = 1;

	dram_size = zb_get_exist_npage() * PAGE_SIZE;
	printk( "\nNow, zero boot start to snapshot. zb_snapshot() = %p high_memory = %08X\n", zb_snapshot, (u32)high_memory );
	printk( ">>> PAGE_OFFSET = %08lX, PAGE_SIZE = %08lX\n", PAGE_OFFSET, PAGE_SIZE );
printk(" raw_smp_processor_id %d\n", raw_smp_processor_id());

	zblk_probe();
	zblk_setup_first();	
	zb_nalcode_init();
	zbi_set_zblk_io( zblk_get_io_vaddr() );

	// m2n init
	zb_display_memory_info( PHYS_OFFSET, dram_size);
	printk("Func: %s, Line: %d...\n", __func__, __LINE__ );
	m2n_setup(dram_size);
	printk("Func: %s, Line: %d...\n", __func__, __LINE__ );


	// walk around used memory & add to m2n
	zb_check_used_memory();								// 현재 시스템에서 사용되고 있는 메모리를 조사 하여 추가 한다. 
	printk("Func: %s, Line: %d...\n", __func__, __LINE__ );

	zb_add();											// 필수 항목에 대한 정보를 추가한다.
	printk("Func: %s, Line: %d...\n", __func__, __LINE__ );
	zb_save_cpu_resume();

	// NALCODE Valid info
	printk(">> dependant core ARMv4, ARMv6, ARMv7 ...\n" );
	NAL_CALL(disp_zba_info, 0, 0, 0);					// Do not Remove!
	if( NAL_CALL(m2n_get_release_info, 0, 0, 0) ) printk(" NALCODE is Release Version\n");
	else										  printk(" NALCODE is Under Limited\n");
	
	// analyze data to zbi
	zbi_write_info_of_bootcopy_to_zbi();				// 분석된 정보를 이용하여 zbi 정보를 만든다.

	// Virtual index 를 block 저장위치를 저장한다.
	zbi_build_block_page();								// ZBI 의 데이터가 저장될 block PAGE 를 계산한다.

	zbi_build_block_page_for_m2n_data();
														// 블록 디바이스를 쓸수 있는 상태로 준비한다.
														// NAND 의 경우 모든 파티션을 모두 지운다.
 	if (1) {
 		printk( "<*** SNAPSHOT SAVE FLOW 1 ***>\n" );

		/* set function & value for operation */
		zero_trace = 1;										// use kernel
		zb_snapshot_fn = NULL;								// disable at suspend
		nalcode->snapshot_wait = 1; 						// use user space
		zero_set_pte_ext_hook = zero_set_pte_ext_hook_func;	// use idle function
		zbi_storage_ready_done();

		// temp force idle restor
		if( NAL_CALL(m2n_get_release_info, 01, 0, 0) )	// Release Version only
			zeroboot_cpu_idle_hook = zba_zeroboot_cpu_idle_hook;
		else
			zeroboot_cpu_idle_hook = NULL;

		/* save step 1 */
		zbi_save_data_for_zbi_data();						// ZBI 의 데이터를 block에 저장한다.
		printk( "snapshot save data end\n" );

		/* save step 2 */
		zbi_save_for_m2n_data();	// 폴트 복구 데이터를 block에 저장한다.
		printk( "snapshot save fault page end\n" );

		/* save step 3 */
		zbi_save_header		();							// ZBI 의 헤더를 block에 저장한다.
		printk( "snapshot save header end\n" );

		zbi_display();									// ZBI 의 정보를 보여준다.
 
		printk( "OK!, it is end of snapshot.\n" );

		zb_set_status( ZB_STATUS_SNAPSHOT_DONE );
		
		zero_trace = 0;
		zero_set_pte_ext_hook = NULL;
		zeroboot_cpu_idle_hook = NULL;
		nalcode->snapshot_wait = 0; 

		zb_nalcode_deactive();						// last operation for suspend. do not work below line
	}
	else
	{
		printk( "snapshot kernel restore start\n" );
		printk( "OK!, First system restore.\n" );

		zero_trace = 1;
		zero_set_pte_ext_hook		= zero_set_pte_ext_hook_func;
		if( NAL_CALL(m2n_get_release_info, 0, 0, 0) )	// Release Version only
			zeroboot_cpu_idle_hook = zba_zeroboot_cpu_idle_hook;
		else
			zeroboot_cpu_idle_hook = NULL;

		// 제로부팅 후 저장장치 처리
		zblk_setup_post_zeroboot();

		//NAL_CALL(m2n_blk_test_function, 0, 0, 0);
		zb_set_status( ZB_STATUS_ZEROBOOT );
		
		// 인터럽트 활성화 이후 할일
		//schedule_delayed_work( &zb_workq, HZ/100 );
	}

//	NAL_CALL(m2n_blk_test_function, 0, 0, 0);

	return 0;
}

EXPORT_SYMBOL(zb_snapshot);


/*------------------------------------------------------------------------------
  @brief   제로부트 상태 변경
  @remark  
*///----------------------------------------------------------------------------
void zb_set_status( u32 status )
{
	zb_status = status;
	wake_up_interruptible( &zb_status_waitq );
}
EXPORT_SYMBOL( zb_set_status );
/*------------------------------------------------------------------------------
  @brief   제로부트 상태 획득
  @remark  
*///----------------------------------------------------------------------------
u32 zb_get_status( void )
{
	return zb_status;	
}
EXPORT_SYMBOL( zb_get_status );
/*------------------------------------------------------------------------------
  @brief   제로부트 이벤트에 웨이트큐 포인터 획득
  @remark  
*///----------------------------------------------------------------------------
wait_queue_head_t *zb_get_status_waitq( void )
{
	return  &zb_status_waitq;	
}
EXPORT_SYMBOL( zb_get_status_waitq );
/*------------------------------------------------------------------------------
  @brief   irq 를 살린이후 io 복구를 위한 워크핸들러
  @remark  
*///----------------------------------------------------------------------------
static void  zb_workq_handle( struct work_struct *_work ) 
{
	printk( "zb restore with irq\n" );
}

/*------------------------------------------------------------------------------
  @brief   모듈 초기화 
  @remark  
*///----------------------------------------------------------------------------
extern int mem_storage_init_module(void);
extern void mem_storage_cleanup_module(void);
static __init int zb_snapshot_init( void )
{
	printk( "ZERO SNAPSHOT prepare\n" );

	zb_proc_init();

	nalcode 	 = (nalcode_head_t *)NALCODE_PAGE_VADDR;
	nalcode_func = (u32 (*)( u32, u32 , u32 , u32 )) (&(nalcode->nalcode_func));
	m2n_order = 0;

	init_waitqueue_head( &zb_status_waitq );
	INIT_DELAYED_WORK( &zb_workq, zb_workq_handle );

	zb_io_init();
	zb_trigger_init();
	zb_blk_init();
	zb_app_inf_init();
	zbi_init();
	zb_used_memory_init();

	zb_snapshot_fn = zb_snapshot;

	printk( "ZERO SNAPSHOT ready\n" );
    return 0;
}

/*------------------------------------------------------------------------------
  @brief   모듈 해제
  @remark  
*///----------------------------------------------------------------------------
static __exit void zb_snapshot_exit( void )
{
	zb_io_free();
	zb_trigger_exit();
	nalcode 		= NULL;
	zb_snapshot_fn = NULL;

	zb_proc_exit();
	zb_blk_exit();
	zb_app_inf_exit();
	m2n_remove();
	zb_used_memory_exit();

	printk( "ZERO SNAPSHOT EXIT\n" );

}

module_init(zb_snapshot_init);
module_exit(zb_snapshot_exit);

MODULE_AUTHOR("frog@falinux.com");
MODULE_AUTHOR("boggle70@falinux.com");
MODULE_LICENSE("GPL");

