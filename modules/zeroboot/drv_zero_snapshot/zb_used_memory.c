/*-----------------------------------------------------------------------------
  파 일 : zb_used_memory.c
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
#include <linux/cdev.h>          
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/irqflags.h>
#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/swap.h>
#include <linux/bootmem.h>
#include <linux/gfp.h>
#include <linux/mmzone.h> // search_all_pg_data()
#include <linux/delay.h>
#include <asm/gpio.h>
#include <asm/setup.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/pgalloc.h>
#include <asm/page.h>
#include <asm/tlbflush.h>         
#include <asm/mach/map.h>
#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>

#include <zbi.h>
#include <zb_debug.h>
#include <zb_used_memory.h>

#define STRUCT
#include <zb_nalfuncs.h>
#undef STRUCT

/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------
static unsigned long *used_info;

static inline void set_used_info(int n)
{
	int nr, qt;
	qt = n / sizeof(unsigned long);
	nr = n % sizeof(unsigned long);
	set_bit(nr, &used_info[qt]);
}
static inline int get_used_info(int n)
{
	int nr, qt;
	qt = n / sizeof(unsigned long);
	nr = n % sizeof(unsigned long);
	return test_bit(nr, &used_info[qt]);
}

// FIXME
// 비트패턴을 확인할때 바이트 오더가 바뀔수 있다.
// 문제가 생길경우 확인할것!!!
int check_boot_reserved(struct zone *zone, int order)
{   
    struct bootmem_data *bdata;
    struct pglist_data  *pgdat; 
    int align;
    long *map;
    
    pgdat = zone->zone_pgdat;
    if( !pgdat || !pgdat->bdata ) printk(" pgdat 0x%p\n", pgdat);
    if( !pgdat || !pgdat->bdata ) return 0;
    
    bdata = pgdat->bdata;
    if( !bdata || !bdata->node_bootmem_map ) printk(" bdata 0x%p\n", bdata);
    if( !bdata || !bdata->node_bootmem_map ) return 0;

	printk(" pgdat 0x%p\n", pgdat);
	printk(" bdata 0x%p\n", bdata);
	printk(" node_bootmem_map 0x%p\n", bdata->node_bootmem_map);

    align = ALIGN(order+1, 32) / 32 - 1;
    map = (long *)bdata->node_bootmem_map;
    if( test_bit(order%32, &map[align]) ) return 1;
    return 0;       
}   
struct min_max {
    unsigned int min;
    unsigned int size;
};

#if 0
static struct min_max s5p_min_max[] = {
    {0x21000000 + 0x00200000, 0x01800000 - 0x00200000},
    {0, 0},

    {0x48000000, 0x04000000},
    {0x21500000, 0x02400000},
    {0x4c000000, 0x02400000},
    {0x4e400000, 0x00600000},
    {0x4ea00000, 0x009ab000},
    {0x4f3ab000, 0x00600000},
    {0x23900000, 0x00800000},
    {0x4f9ab000, 0x005dc000},
    {0x24100000, 0x00800000},

    {0, 0},
};
int check_boot_reserved_s5p(unsigned int pos)
{
    struct min_max *mm;

    for(mm = s5p_min_max; ; mm++) {
        unsigned int min, max;

        if(mm->min==0 || mm->size==0 ) return 0;

        min = mm->min;
        max = min + mm->size;
        if( (pos>=min) && (pos<max) ) return 1;
    }

    return 0;
}
#else

// by freefrug
static struct zb_reserved_mem *zb_reserved_mem_list = NULL;
void zb_registe_reserved_mem( struct zb_reserved_mem *rmlst )
{
	zb_reserved_mem_list = rmlst;
}

// 예약된 영역일 경우 return 1
int check_zbi_reserved_area(unsigned int pos)
{
	// remove by fg  절대 참이 될수 없다.
	// if( ((PHYS_OFFSET+0x8000+0x80000) <= pos) &&  ((PHYS_OFFSET+0x8000)	>  pos) ) return 1;
	
	if ( zb_reserved_mem_list )
	{
		struct zb_reserved_mem *rmlst = zb_reserved_mem_list;
		
		while( 1 )
		{
			if ( rmlst->base == RESERVED_MEM_END_BASE ) break;
			if ( rmlst->size == 0 ) break;
			
			if ( ( pos >= rmlst->base ) && ( pos < (rmlst->base + rmlst->size) ) ) 
			{
				//printk(" reserved mem base=0x%08x\n", pos );
				return 1;
			}
			rmlst ++;
		} 
	}
		
	return 0;
}

static struct min_max s5p_min_max[] = {
    {0, 0},
    {0x31023000, 0x02400000},
    {0x40203000, 0x02400000},
    {0x42603000, 0x00600000},
    {0x42c03000, 0x009ab000},
    {0x435ae000, 0x00600000},
    {0x33423000, 0x00800000},

    {0, 0},
    {0x43bae000, 0x005dc000},

};
int check_boot_reserved_s5p(unsigned int pos)
{
    struct min_max *mm;

    for(mm = s5p_min_max; ; mm++) {
        unsigned int min, max;

        if(mm->min==0 || mm->size==0 ) return 0;

        min = mm->min;
        max = min + mm->size;
        if( (pos>=min) && (pos<max) ) return 1;
    }

    return 0;
}
#endif

u32 zb_get_used_mem_count( void )
{   
	u8 			*search_base;
	u32			size_use = 0;
	u32			i;

    struct zone         *zone;
    struct list_head    *curr;
    unsigned long       pfn;
    int                 order;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
	int					t;
#endif

	printk( "Check used memory\n" );
	
	size_use	= 0;
    for_each_zone(zone) {   
        if (!populated_zone(zone)) continue;
   
		memset(used_info, 0x00, zone->spanned_pages);
		search_base = (u8 *)(zone->zone_start_pfn << PAGE_SHIFT);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        for_each_migratetype_order(order, t) {
            list_for_each(curr, &zone->free_area[order].free_list[t]) {
#else
		for (order = 0; order < MAX_ORDER; order++) {
			list_for_each(curr, &zone->free_area[order].free_list) {
#endif
                unsigned long i;
                pfn = page_to_pfn(list_entry(curr, struct page, lru));
                for (i = 0; i < (1UL << order); i++) {
					set_used_info(pfn + i - zone->zone_start_pfn);
                }   
            }       
        }       

		for(i=0; i<zone->spanned_pages; i++, search_base+=PAGE_SIZE) {
			if( !get_used_info(i) ) {

				if( check_zbi_reserved_area((unsigned int)search_base) ) continue;

/* rm by freefrug
//				if( check_zbi_reserved_area((unsigned int)search_base) ) continue;
#ifndef CONFIG_MACH_EZS3C6410
				//if( check_boot_reserved(zone, i) ) continue;
				if( check_boot_reserved_s5p((unsigned int)search_base) ) continue;
#endif
 end by freefrug */

				//printk("USE PAGE : %d PADDR = %08X, PADDR = %08lX \n", i, (unsigned int)search_base, virt_to_phys(search_base) );
				//printk("USE PAGE : %d PADDR = %08X, PADDR = %08lX \n", i, (unsigned int)search_base, phys_to_virt(search_base) );
				//NAL_CALL(m2n_add, virt_to_phys(search_base), 0, 0);
				//NAL_CALL(m2n_add, (u32)search_base, 0, 0);
				size_use++;
			}
		}
    }       
	
	printk( "Total used memory = %d MB\n", size_use * 4096 / 1024 /1024 );
  
	return size_use;
}

void zb_check_used_memory( void )
{
	u8 			*search_base;
	u32			size_use = 0;
	u32			i;

    struct zone         *zone;
    struct list_head    *curr;
    unsigned long       pfn;
    int                 order;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
	int					t;
#endif

	printk( "Check used memory\n" );
	
	size_use	= 0;
    for_each_zone(zone) {   
        if (!populated_zone(zone)) continue;
   
		memset(used_info, 0x00, zone->spanned_pages);
		search_base = (u8 *)(zone->zone_start_pfn << PAGE_SHIFT);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,29)
        for_each_migratetype_order(order, t) {
            list_for_each(curr, &zone->free_area[order].free_list[t]) {
 #else
		for (order = 0; order < MAX_ORDER; order++) {
			list_for_each(curr, &zone->free_area[order].free_list) {
#endif
       
                unsigned long i;
                pfn = page_to_pfn(list_entry(curr, struct page, lru));
                for (i = 0; i < (1UL << order); i++) {
					set_used_info(pfn + i - zone->zone_start_pfn);
                }   
            }       
        }       

		for(i=0; i<zone->spanned_pages; i++, search_base+=PAGE_SIZE) {
			if( !get_used_info(i) ) {
				if( check_zbi_reserved_area((unsigned int)search_base) ) continue;

/* rm by freefrug
#ifndef CONFIG_MACH_EZS3C6410
				//if( check_boot_reserved(zone, i) ) continue;
				if( check_boot_reserved_s5p((unsigned int)search_base) ) continue;
#endif
 end by freefrug */
 
				//printk("USE PAGE : %d PADDR = %08X, PADDR = %08lX \n", i, (unsigned int)search_base, virt_to_phys(search_base) );
				//printk("USE PAGE : %d PADDR = %08X, PADDR = %08lX \n", i, (unsigned int)search_base, phys_to_virt(search_base) );
				//NAL_CALL(m2n_add, virt_to_phys(search_base), 0, 0);
				NAL_CALL(m2n_add, (u32)search_base, 0, 0);
				size_use += PAGE_SIZE;
			}
		}
    }       
	
	printk( "Total used memory = %d MB\n", size_use /(1024*1024) );
}

/* allocate memory for 4G memory used pattern
 * 2G / PAGE_SIZE = 524288
 * 524288 / 8 = 65536
 * required buffer size 64KB
 *
 * Attention : Continous physical memory must not exceeded 2G
 */
#define	USED_INFO_MAX 524288
void zb_used_memory_init(void)
{
	used_info = (unsigned long *)__get_free_pages(GFP_ATOMIC | 
												  GFP_DMA, 
												  get_order(USED_INFO_MAX) 
												  );
	if (!used_info) {
		printk("Oops : no memory allocate for used_info, Check it!!!\n");
		while(1) ;
	}
}

void zb_used_memory_exit(void)
{
	if (used_info)
		free_pages((unsigned long)used_info, get_order(USED_INFO_MAX) );
}

unsigned long *zb_get_mem_array(void)
{
	return used_info;
}

unsigned long zb_get_mem_array_size(void)
{
	return USED_INFO_MAX;
}

struct zinfo {
	unsigned long s;
	unsigned long e;
	int high;
};

#define	MAX_ZINFO 6
static struct zinfo zi[MAX_ZINFO+1];

extern void * high_memory;
static unsigned long zb_setup_zinfo(int *highzone)
{
	struct zone *zone;
	unsigned long high_pfn;
	int order = 0;

	order = 0;
	high_pfn = ((unsigned long)high_memory - PAGE_OFFSET + PHYS_OFFSET);
	high_pfn = high_pfn >> PAGE_SHIFT;
	memset(zi, 0, sizeof(struct zinfo) * (MAX_ZINFO+1));

	for_each_zone(zone) {
		if (!populated_zone(zone)) continue;

		zi[order].s = zone->zone_start_pfn;
		zi[order].e = zone->zone_start_pfn + zone->spanned_pages -1;
		if (high_pfn <= zi[order].s)
			zi[order].high = 1;
		else
			zi[order].high = 0;

		printk(" zone %d start 0x%08lX end 0x%08lX high %d\n", order, zi[order].s, zi[order].e, zi[order].high);
		order++;
	}
	*highzone = 0;
	return zi[0].s;
}

unsigned long zb_get_next_page(unsigned long pfn, int *highzone)
{
	int z;

	if (pfn == 0)
		return zb_setup_zinfo(highzone);

	for (z = 0; z < MAX_ZINFO; z++) {
		unsigned long start;
		unsigned long end;

		if (zi[z].s == 0x0)
			break;

		start = zi[z].s;
		end = zi[z].e;

		if((start <= pfn) && (pfn < end)) {
			pfn++;
			*highzone = zi[z].high;
			break;
		}
		if (pfn == end) {
			if (zi[z+1].s == 0)
				break;

			pfn = zi[z+1].s;
			*highzone = zi[z+1].high;
			break;
		}
	}
	return pfn;
}

unsigned long zb_get_exist_npage(void)
{
	struct zone *zone;
	unsigned long total=0;

	for_each_zone(zone) {
		if (!populated_zone(zone)) continue;

		total += zone->spanned_pages;
	}
	return total;
}

