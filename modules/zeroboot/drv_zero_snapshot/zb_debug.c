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

#include <zb_nalcode.h>
#include "zbi.h"
#include "zb_debug.h"

#include <zb_mmu.h>
#include <zb_mmu.h>
#include <zb_mmu_debug.h>


/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------

/// @}

void zb_dump_data( s8 *msg, u8 *data , u32 size )
{
    int 				total;
    int 				lp1;
    unsigned char 		*ptr8;
    char    line[80], 	*pline;
    
    printk( "%s\n", msg );

    ptr8 = data;

    total = 0;
    pline = &line[0];

    while( total < size )
    {
    	if( total % 16 == 0 )
    	{
    		memset( line, ' ', sizeof( line ) );
    		pline = &line[0];

        	pline += sprintf( pline, "%04X-%04X : ",  (((unsigned int)ptr8)>>16)&0xFFFF,
                                                       ((unsigned int)ptr8)     &0xFFFF  );
    	}

        pline += sprintf( pline, "%02X ", *ptr8 );

        if      ( *ptr8 < ' ' ) line[62+(total%16)] = '.';
        else if ( *ptr8 > '~' ) line[62+(total%16)] = '.';
        else                    line[62+(total%16)] = *ptr8;

    	total++;
    	ptr8++;

    	if( ( total % 16 == 0 ) || ( total >= size ) )
    	{
    		for( lp1 = 0; lp1 < sizeof( line ); lp1++ )
    			if( line[lp1] == 0 )  line[lp1] = ' ';
    		line[60] = '|';
    		line[79] = 0;
    		printk( "%s\n", line );
    	}
    }
}

/*------------------------------------------------------------------------------
  @brief   0 번지를 접근하여 폴트시험을 해본다.
  @remark  
           
*///----------------------------------------------------------------------------
void zb_nalcode_fault_test_zero_data( void )
{
	u32 *zero_page_fault;
		
	printk( "NALCODE ZERO PAGE FAULT START\n" );
	zero_page_fault = (u32 *) 0;
	*zero_page_fault = 0;
		
	printk( "NALCODE ZERO PAGE FAULT END\n" );
	
}


/*------------------------------------------------------------------------------
  @brief   임의의 페이지를 폴트를 발생 시킨다. 
  @remark  
           
*///----------------------------------------------------------------------------
void zb_nalcode_fault_test_modify_coarse( void )
{
	u32 *app_data;
	u32 l2_entry;
	
	app_data = (u32 *)0x00008000;
//	app_data = 0x00001000;
//	app_data = 0xC0000000;
	printk( "NALCODE APP PAGE FAULT START\n" );
	mmu_set_fault_l2_entry( mmu_get_ttb_base_vaddr(), (u32)app_data );
	l2_entry = mmu_get_l2_entry( mmu_get_ttb_base_vaddr(), (u32)app_data );
	printk( "NALCODE L2 ENTRY %08X\n", l2_entry );
	
//	*app_data = 0;
	
	printk( "NALCODE APP  PAGE TEST %08X\n", *app_data );
	
	printk( "NALCODE APP  PAGE FAULT END\n" );
}

//-----------------------------------------------------------------------------
//  설 명 : 모듈 초기화 
//-----------------------------------------------------------------------------
/* search virtual address in page cache
 * task	: pointer for search
 * addr	: virtual address to search
 * return : physical address in cache
 */
static unsigned long zba_get_paddr_from_page_cache(struct task_struct *task, unsigned long addr)
{
	struct vm_area_struct *vma;
	struct page *page;
	struct file *file;
	pgd_t *pgd;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;
	
	vma = find_vma(task->mm, addr);
	if(!vma) return 0;

	file = vma->vm_file;
	if(!file) return 0;

	pgd = pgd_offset(task->mm,  addr);
	pud = pud_offset(pgd, addr);
	pmd = pmd_offset(pud, addr);
#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,37)
	pte = pte_offset_map_nested(pmd, addr); 
#else
	pte = pte_offset_map(pmd, addr); 
#endif
	
	if( *pte != 0 ) return 0;

	page = find_get_page(file->f_mapping , addr>>PAGE_SHIFT);
	if(!page) return 0;

	return page_to_phys(page);
}

/*------------------------------------------------------------------------------
  @brief   PTE 테이블에 페이지 캐쉬의 물리 메모리를 지정한다.
  @remark  
*///----------------------------------------------------------------------------
static void zba_mark_page_cache_pte_mmu_of_vma( struct task_struct *p, volatile  u32 *tlb, struct vm_area_struct *v )
{
 	u32  	l1_index;
 	u32		l1_entry;

	u32		*pte;
 	u32		l2_start;
 	u32		l2_end;
 	u32  	l2_index;
 	u32		l2_entry;
 	
 	u32		paddr;

	l1_index = (v->vm_start >> MMU_L1_SHIFT) & MMU_L1_MASK ;
	l1_entry = tlb[l1_index];
	
	pte = (u32 *) phys_to_virt(MMU_L1_PADDR_COARSE(l1_entry));

 	l2_start = (v->vm_start >> MMU_L2_SHIFT) & MMU_L2_MASK ;
 	l2_end   = (v->vm_end   >> MMU_L2_SHIFT) & MMU_L2_MASK ;

 	for( l2_index = l2_start; l2_index < l2_end; l2_index++ )
 	{
 		l2_entry = pte[l2_index];
 		
 		if( l2_entry == 0  ) 
 		{
 			paddr 			= zba_get_paddr_from_page_cache( p,v->vm_start+((l2_index-l2_start)*PAGE_SIZE) );
 			if( paddr != 0 )
 			{	
 				l2_entry 		= NALCODE_PAGE_CACHE_MARK(paddr);
 				pte[l2_index] 	= l2_entry;
 			}	
		}
 	}
 }

/*------------------------------------------------------------------------------
  @brief   지정된 프로세서 VMA  에 포함된 페이지를 터치 한다.
  @remark  
*///----------------------------------------------------------------------------
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
void zba_mark_page_cache_for_process( struct task_struct *p )
{
	struct vm_area_struct 	*v; 
	char 					*buf;
	
	struct file 			*vf;
	char 					*ptr;
	char					*sptr;

	printk( ">> process [%s] ", p->comm );
	printk( "          - " );
	printk( "MMU TLB = V:%08X,P:%08X", (int) p->mm->pgd, (int) virt_to_phys( p->mm->pgd ) );
	printk( "\n" );	
	printk( "-----------------------------------------------------------------------\n" );	
	buf = (char *)__get_free_page(GFP_KERNEL);
	
	v = p->mm->mmap;
	
	do
	{
		printk( "   " );
		printk("%8X-"	, (int)( v->vm_start)				);
		printk("%08X"	, (int)( v->vm_end	)				);
		printk("(%7X)"	, (int)( v->vm_end - v->vm_start)	);

		printk( " : " );	

			vf = v->vm_file;
			if( vf == NULL ) { ptr = "?"; }
			else			 { ptr = d_path(&vf->f_path, buf, PAGE_SIZE); if(ptr==NULL)	ptr = "?"; }

			sptr = strrchr(ptr, '/');	if(sptr) ptr = sptr+1;

		printk("%s", ptr );
		printk( "\n" );
		
		zba_mark_page_cache_pte_mmu_of_vma( p, (volatile  u32 *) p->mm->pgd, v );
		
	} while( (v = v->vm_next) );	

		
	free_page((unsigned long)buf);
	printk( "\n" );	
}
#else
void zba_mark_page_cache_for_process( struct task_struct *p )
{
}
#endif


