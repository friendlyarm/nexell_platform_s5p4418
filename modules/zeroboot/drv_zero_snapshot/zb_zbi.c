/*-----------------------------------------------------------------------------
  파 일 : zb_zbi.c
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
#include <linux/highmem.h>

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

#include <linux/delay.h>
#include <asm/unistd.h>
#include <asm/io.h>

#include <asm/pgalloc.h>
#include <asm/page.h>
#include <asm/tlbflush.h>         
#include <linux/gfp.h>

#include "zbi.h"
#include <zb_mmu.h>
#include <zb_blk.h>

#define STRUCT
#include <zb_nalfuncs.h>
#undef STRUCT

#define DEV_NAME    "zeroboot_zbi"
#define PROC_NAME	"zeroboot_zbi"

/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------
static zbi_t	*zbi_work = NULL; 	
static u32		zbi_next_page_offset;
static u8 zbi_save_data_buff[PAGE_SIZE];

// 인덱스 방식
// zblk 에서는 4K 크기의 인덱스를 사용한다.
// 헤더 정보는 인덱스 번호의 시작을 항상 0으로 사용한다.
// 저장매체의 단위와 관계없이 항상 4K 단위의 인덱스 형태로 읽기 쓰기를 요청하게 된다.
// 저장매체 디바이스에서는 항상 4K 단위의 인덱스를 기반으로 쓰기를 하게 된다.
// 저장 정보는 변환정보 없이 실제 물리주소 access 에서 사용할 주소체계를 사용하므로
// 디바이스에게 인덱스 번호의 물리주소번호를 구하여 사용하게 되며, 이 번호체계의 
// 단위나 정보를 알수 없으므로 어떤 참조도 하지 않아야 한다.

/// @}

/*------------------------------------------------------------------------------
  @brief   ZBI 초기화를 처리 한다. 
  @remark  
*///----------------------------------------------------------------------------
extern unsigned long zb_get_idmap_pgd_phys(void);
extern unsigned long zb_get_resume_ptr(void);
extern unsigned long zb_get_cpu_resume_phys(void);

#ifdef CONFIG_CACHE_L2X0
extern unsigned long get_zb_l2x0_base(void);
extern unsigned long get_zb_l2x0_way_mask(void);
extern unsigned long get_zb_l2x0_size(void);
#else
unsigned long get_zb_l2x0_base(void) { return 0;};
unsigned long get_zb_l2x0_way_mask(void) { return 0;};
unsigned long get_zb_l2x0_size(void) { return 0;};
#endif

extern int (*get_zb_access_status)(void);
extern void (*set_zb_access_status)(int);
extern void (*zb_storage_wait_lock)(void);
extern void (*zb_storage_wait_unlock)(void);

int get_nal_access_status(void)
{
	return NAL_CALL(m2n_get_dev_status, 0, 0, 0);
}
 
void set_nal_access_status(int val)
{
	NAL_CALL(m2n_set_dev_status, val, 0, 0);
}
EXPORT_SYMBOL(set_nal_access_status);

void wait_nal_access_lock(int val)
{
	NAL_CALL(m2n_wait_lock, 0, 0, 0);
}
EXPORT_SYMBOL(wait_nal_access_lock);

static void zbi_clear( void )
{
	if( zbi_work == NULL ) return ;
	printk( "clear ZBI\n" );
	memset( zbi_work,0, sizeof( zbi_t ) );	
	zbi_work->magic 		= ZBI_MAGIC;
	zbi_work->version 		= ZBI_VERSION;

	zbi_work->storage_dev_offset = zblk_get_storage_offset();
	
	zbi_work->copy_count 	= 0;
	zbi_work->copy_checksum	= 0;

	zbi_work->bootup_marker = ZBI_BOOTUP_MARKER;
	zbi_work->cpu_idmap 	= zb_get_idmap_pgd_phys();
	zbi_work->cpu_resume 	= zb_get_cpu_resume_phys();
	zbi_work->zb_l2x0_base		= get_zb_l2x0_base();
	zbi_work->zb_l2x0_way_mask	= get_zb_l2x0_way_mask();
	zbi_work->zb_l2x0_size		= get_zb_l2x0_size();

	zbi_next_page_offset = ZBI_UNIT_SIZE/PAGE_SIZE;
}

void zb_save_cpu_resume(void)
{
	zbi_work->phys_cpu_ptr		= zb_get_resume_ptr();
}

void zbi_change_pgd(unsigned long pgd)
{
	unsigned long *ptr;

	ptr = (unsigned long *)phys_to_virt(zb_get_resume_ptr());
	*ptr = virt_to_phys((void *)pgd);
}

/*------------------------------------------------------------------------------
  @brief   ZBI 생성 
  @remark  
*///----------------------------------------------------------------------------
// zbi_work 의 크기는 부팅시에 복사되는 entry * 8 ( sizeof(zbi_copy_info_t) ) 이다.
// 1개의 page 로 만들수 있는 zbi_copy_info_t 자료형은 4096/8=512 개의 entry 이다.
// 1개의 entry 는 4Kbyte 이므로 1page 당 2M의 사전 복사 영역을 확보할수 있다.
// 하지만 zbi 의 구조에서 첫 2048 은 별도의 영역으로 지정되므로
// 아래와 같은 공식을 사용하도록 한다.
// 1page = 1M + 0*2M = 1M
// 2page = 1M + 1*2M = 3M
// 4page = 1M + 3*2M = 7M
// 8page = 1M + 7*2M = 15M
// npage = 1M + (n-1)*2M
//
// 부팅시 복사되는 영역은 nal + L1 + L2 + video + return 등이다.
// 현재 zeroboot hall 은 512K=128page 이다.
// 즉 최대 255M 의 부팅 복사 영역을 저장할수 있다.
//
// 현재 안드로이드에서 실제 복사되는 크기는 4M 이내이다.
// 따라서 현재는 4개의 page 를 최대 크기로 해서 사용하도록 한다
// 4개의 page를 사용하더라도 저장과 복구시는 copy_count 에 의해서
// 실제 사용된 양만을 저장하고 불러들인다.
#ifdef CONFIG_SMP
#include <linux/dma-mapping.h>
static unsigned long get_nocache_memory(void)
{
	dma_addr_t nocache_phy;
	unsigned long cpu_nocache;

	cpu_nocache = (unsigned long)dma_alloc_coherent(NULL, PAGE_SIZE, &nocache_phy, GFP_DMA);

	if (!cpu_nocache)
		panic("Zeroboot SMP needs coherent memroy space");

	memset((void *)cpu_nocache, 0, PAGE_SIZE);
	return cpu_nocache;
}
#else
static unsigned int get_nocache_memory(void) { return 0; }
#endif
extern int zb_buf_status;
void zbi_init( void )
{
	u32	align_check;
	struct thread_info *ti;

//	zbi_work 		= (zbi_t *) __get_free_pages(GFP_ATOMIC | GFP_DMA, get_order(ZBI_UNIT_SIZE) );
	zbi_work = (zbi_t *)ZBI_VADDR;
	align_check = (u32)zbi_work->copy_data;
	if( (align_check & 0x07ff) ) {
		printk("Oops!!! YOU Must be align copy_data \n");
		printk("You added zbi_t variable too much\n");
		printk(" zbi_work    0x%p\n", zbi_work);
		printk(" copy_data   0x%p\n", zbi_work->copy_data);
		printk(" align_check 0x%08X\n", align_check);
		while(1);
	}
	zbi_clear();

	ti = (struct thread_info *)(0);
	zbi_work->cpu_offset = (unsigned int)&(ti->cpu);
	zbi_work->cpu_nocache = (unsigned int)get_nocache_memory();
	zbi_work->cpu_nr = CONFIG_NR_CPUS;
}

void zbi_storage_ready_done(void)
{
	get_zb_access_status = get_nal_access_status;
	set_zb_access_status = set_nal_access_status;
	zb_storage_wait_lock = wait_nal_access_lock;
}

void zbi_info_dump(void)
{
	unsigned long *ptr;
//	printk(" magic	0x%08X\n", zbi_work->magic);
	printk(" cpu_idmap	0x%08X\n", zbi_work->cpu_idmap);

	ptr = (unsigned long *)zbi_work->phys_cpu_ptr;
	ptr = (unsigned long *)phys_to_virt(zbi_work->phys_cpu_ptr);
	printk(" phys_cpu_ptr 	0x%lX\n", *ptr++);
	printk(" sp             0x%lX\n", *ptr++);
	printk(" fn             0x%lX\n", *ptr++);
	
}

/*------------------------------------------------------------------------------
  @brief   ZBI 소멸
  @remark  
*///----------------------------------------------------------------------------
void zbi_free( void )
{
//	if( zbi_work 		!= NULL ) free_pages((unsigned long) zbi_work, get_order(ZBI_UNIT_SIZE) );
}

/*------------------------------------------------------------------------------
  @brief   부트로더에 의해서 복사할 정보를 추가 한다. 
  @remark  
*///----------------------------------------------------------------------------
int zbi_append_bootcopy_data( u32 dest_paddr , u32 attr )
{
	u32 			index;
	zbi_copy_info_t	*copy_data;
	
	if( zbi_work == NULL ) return -1;
		
	index = zbi_work->copy_count;
	if( index >= ZBI_COPY_MAX )
	{
		printk( "Oops!!! Program Bug... over bootcopy data MAX 0x%08lX request 0x%08X\n", ZBI_COPY_MAX, index );
		return -2; 
	}	
		
	copy_data = &(zbi_work->copy_data[index]);
	copy_data->dest_paddr  = dest_paddr;
	copy_data->attr = (u16) attr;
	zbi_work->copy_count++;
		
	return 0;
}

/*------------------------------------------------------------------------------
  @brief   부트로더에 의해서 복사되어야 하는 정보를 zbi에 쓴다. 
  @remark  
*///----------------------------------------------------------------------------
void zbi_write_info_of_bootcopy_to_zbi( void )
{
	u32     index;
	u32		count;

	printk( ">> Transfer from info of bootcopy info to zbi\n" );
	
	count = NAL_CALL(m2n_get_count, 0, 0, 0);
	for( index = 0; index < count; index++ )
	{
		int rval;
		u32	paddr;

		paddr = NAL_CALL(m2n_get_block_phys, index, 0, 0);
		rval = 	NAL_CALL(m2n_get_item_boot_copy, index, 0, 0);
		if( rval==1 )	zbi_append_bootcopy_data( paddr, ZBI_ATTR_MMU_PTE );
		if( rval==2 )	zbi_append_bootcopy_data( paddr, ZBI_ATTR_NONE    ); 
	}
}

/*------------------------------------------------------------------------------
  @brief   ZBI 의 데이터가 저장될 Storage PAGE 를 계산한다.
  @remark  
*///----------------------------------------------------------------------------
void zbi_build_block_page ( void )
{
	zbi_copy_info_t	*copy_data;
	u32 			index;
	
	if( zbi_work == NULL ) return ;

	printk( ">> build storage page for data of zbi (cnt=%d)\n", zbi_work->copy_count );
		
    copy_data = zbi_work->copy_data;
    for( index = 0; index < zbi_work->copy_count; index++ )
    {
    	copy_data->page_offset = zbi_next_page_offset;
		// find bootup
		//if( copy_data->dest_paddr == (PHYS_OFFSET + 0x0008D000) ) {
		if( copy_data->dest_paddr == (PHYS_OFFSET + 0x00090000) ) {
			zbi_work->bootup_address = copy_data->page_offset;
			zbi_work->bootup_paddress = (PHYS_OFFSET + 0x00090000);
			zbi_work->bootup_size     = (24 * 1024);
			//printk(" copy_data->dest_paddr    0x%08X\n", copy_data->dest_paddr);
			//printk(" zbi_work->bootup_marker  0x%08X\n", zbi_work->bootup_marker);
			//printk(" zbi_work->bootup_address 0x%08X\n", zbi_work->bootup_address);
		}
    	copy_data++;
    	zbi_next_page_offset++;
    }
}

/*------------------------------------------------------------------------------
  @brief   m2n_data 가 관리하는 사용된 메모리 낸드위치 계산
  @remark  
*///----------------------------------------------------------------------------
void zbi_build_block_page_for_m2n_data( void )
{
	zbi_next_page_offset = NAL_CALL(m2n_build_block_page, zbi_next_page_offset, 0, 0);
}

/*------------------------------------------------------------------------------
  @brief   부팅시 점프할 주소를 저장한다.
  @remark  
*///----------------------------------------------------------------------------
int zbi_jump_vaddr( u32 vaddr )
{
	if( zbi_work == NULL ) return 0;
	zbi_work->jump_vaddr = vaddr;
	
	return 0;
}

/*------------------------------------------------------------------------------
  @brief   Storage에 적용할 가상 IO  베이스 주소를 저장한다.
  @remark  
*///----------------------------------------------------------------------------
void zbi_set_zblk_io( u32 vaddr )
{
	if( zbi_work == NULL ) return;
	zbi_work->fault_storage_io_vaddr = vaddr;
}

/*------------------------------------------------------------------------------
  @brief   Section option data 를 저장한다.
  @remark
*///----------------------------------------------------------------------------
void zbi_set_section_option(u32 opt)
{
	if( zbi_work == NULL ) return;
	zbi_work->section_opt = opt;
}
u32 zbi_get_section_option(void)
{
	if( zbi_work == NULL ) return 0;
	return zbi_work->section_opt;
}

/*------------------------------------------------------------------------------
  @brief   ZBI 의 데이터를 Stroage에 저장한다.
  @remark  
*///----------------------------------------------------------------------------
void zbi_make_fault(u32 pte_paddr, u32 *pte_buf)
{
	NAL_CALL(m2n_mark_fault, pte_paddr, (u32)pte_buf, 0);
}

extern void *high_memory;
static inline void zbi_copy_to_page(void *dst, unsigned int paddr, int size)
{
	void *src;
#ifdef CONFIG_HIGHMEM
	struct page *from;
	void *kfrom;
#endif

	src = phys_to_virt( paddr );
	BUG_ON (src < (void *)PAGE_OFFSET);

	if (src < high_memory)
		memcpy(dst, src, size);
#ifdef CONFIG_HIGHMEM
	else {
		// high memory hit...
		from = pfn_to_page(__phys_to_pfn(paddr));
		kfrom = kmap_atomic(from, KM_USER0);
		memcpy(dst, kfrom, size);
		kunmap_atomic(kfrom, KM_USER0);
	}
#endif
}


#ifdef DEBUG_4K_CHECKSUM
static unsigned int zbi_make_sum(unsigned char *src)
{
	unsigned int count;
	unsigned int sum;
	unsigned char *p;

	p = src;

	for(sum = 0, count = 0; count < PAGE_SIZE; count++) {
		sum += (unsigned int)*p++;
	}

	return sum;
}

void zbi_calculate_m2n_data_sum( void )
{
	u32 					index, paddr;
	u32						i;
	u32						used_count;

	u32				checksum_lp;
	u32				local_checksum;
	u32 ret;

	used_count = NAL_CALL(m2n_get_count, 0, 0, 0);

	for( i=0; i< used_count; i++)	{

    	index = NAL_CALL(m2n_get_block_offset, i, 0, 0);
		if( index == 0 ) 
			continue;
    	
    	paddr 	 = NAL_CALL(m2n_get_block_phys, i, 0, 0);
		zbi_copy_to_page( (void *)zbi_save_data_buff, paddr, PAGE_SIZE);

		local_checksum = 0 ; 
		for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ ) {
			local_checksum += zbi_save_data_buff[checksum_lp];
		}
		
    	ret = NAL_CALL(m2n_set_debug_checksum, paddr, local_checksum, 0);
		if (ret != local_checksum) {
			printk("local checksum save fail\n");
			printk("index 0x%08X paddr 0x%08X checksum 0x%08X ret 0x%08X\n",index, paddr, local_checksum, ret);
		}

//		if ( index>=0x1ff6 && index<(0x1ff6 + 3000) ) 
//			printk("KSUM 0x%08X : 0x%08X : 0x%08X\n", index, paddr, local_checksum);
	}
}
#endif

void zbi_save_data_for_zbi_data( void )
{
	zbi_copy_info_t	*copy_data;
	u32 			index, lp;
	u32				checksum_lp;
	
	if( zbi_work == NULL ) return ;

#ifdef DEBUG_4K_CHECKSUM
	zbi_calculate_m2n_data_sum();
#endif

	zbi_work->copy_checksum	= 0;
    copy_data = zbi_work->copy_data;
    
    for( lp = 0; lp < zbi_work->copy_count; lp++ )
    {
    	index = copy_data->page_offset;
		if( index == 0 ) continue;

		zbi_copy_to_page( (void *)zbi_save_data_buff, copy_data->dest_paddr, PAGE_SIZE);    	
    	if( copy_data->attr & ZBI_ATTR_MMU_PTE ) 
    	{
			zbi_make_fault(copy_data->dest_paddr, (u32 *)zbi_save_data_buff);
    	}	

//		printk(">>> phys add copy_data->dest_paddr : 0x%08x\n", copy_data->dest_paddr);
		zblk_page_write( index, zbi_save_data_buff );

		//if( copy_data->dest_paddr != (PHYS_OFFSET + 0x0008D000) ) {
		if( (copy_data->dest_paddr < (u32)zbi_work->bootup_paddress) ||
			(copy_data->dest_paddr >= ((u32)zbi_work->bootup_paddress + (u32)zbi_work->bootup_size + (8 * 1024)))) {
			for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ )
			{
				zbi_work->copy_checksum	+= zbi_save_data_buff[checksum_lp];
			}
		} 
		copy_data++;
		if( ( lp % 256 ) == 0 )
		{	
//			printk( "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" );
//			printk( ">> save data for zbi data  %5d/%5d", lp, zbi_work->copy_count);
		}	
    }
//			printk( "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b" );
//			printk( ">> save data for zbi data  %5d/%5d", lp, zbi_work->copy_count);
	printk("\n");
}

// m2n 에서는 base 주소를 뺀 page 번호를 사용하고
// blk 에서는 4K 단위 인덱스를 사용한다
// storage 에서의 크기와 베이스 주소는 고려하지 않는다.
void zbi_save_for_m2n_data( void )
{
	u32 					index, paddr;
	u32						i;
	u32						used_count;

	used_count = NAL_CALL(m2n_get_count, 0, 0, 0);

//	printk( ">> Write data of m2n data to storage used_count 0x%08X:%d\n", used_count, used_count);
	
	// 1st step save normal block
	for( i=0; i< used_count; i++)	
	{

    	index = NAL_CALL(m2n_get_block_offset, i, 0, 0);
		if( index == 0 ) continue;
    	
    	paddr 	 = NAL_CALL(m2n_get_block_phys, i, 0, 0);
		zbi_copy_to_page( (void *)zbi_save_data_buff, paddr, PAGE_SIZE);

		//printk( ">> fault page paddr = %08X, info = %08X\n", paddr, index);
		zblk_page_write( index, zbi_save_data_buff );
	}
	printk( ">> save data for m2n data %05d/%05d\n", i, used_count);
}

/*------------------------------------------------------------------------------
  @brief   ZBI 의 헤더를 storage에 저장한다.
  @remark  
*///----------------------------------------------------------------------------
void zbi_save_header( void )
{
	u8 		*write_data;
	int 	lp;
	u32		zbi_used_size;
	
	if( zbi_work == NULL ) return ;

//	printk( ">> Write header of zbi to storage\n" );
//	printk("===>>>zbi header copy_count 0x%x:%d\n", zbi_work->copy_count, zbi_work->copy_count);  
//	printk("===>>>zbi offset is 0x%p\n", zbi_work);
//	printk("===>>>zbi last offset 0x%p\n", &zbi_work->copy_data[zbi_work->copy_count+1]);

	zbi_used_size = (u32)(&zbi_work->copy_data[zbi_work->copy_count+1]) - (u32)zbi_work;
	zbi_used_size = (zbi_used_size+(PAGE_SIZE-1))/PAGE_SIZE;

	printk("===>>>zbi header zbi_used_size 0x%x:%d\n", zbi_used_size, zbi_used_size);  

	write_data = (u8 *) zbi_work;
	
	for( lp = 0; lp < zbi_used_size; lp++ )
	{
		zblk_page_write( lp, write_data );

		write_data += PAGE_SIZE;
	}
	//for ( i=0; i<128; i++)	
	//	printk("zbi_work->storage_priv[%d] : %8x\n",  i,zbi_work->storage_priv[i]);
}

/*------------------------------------------------------------------------------
  @brief   정보를 표출한다.
  @remark  
*///----------------------------------------------------------------------------
void 	zbi_display			( void )
{
	zbi_copy_info_t	*copy_data;
	u32 			index;
	u32            *data;
	unsigned int *ptr;
	
	if( zbi_work == NULL ) return ;
		
	printk( "ZBI Information\n" );

    printk( "> zbi size = %d/%d\n", sizeof(zbi_t), (int) ZBI_UNIT_SIZE    );
	
    printk( "> magic         = %08X\n"	, zbi_work->magic        );
    printk( "> version       = %d\n"	, zbi_work->version      );
    
    printk( "> storage_dev_offset = %08X\n"	, (int) zbi_work->storage_dev_offset);

    printk( "> coprocessor count = %d, size = %d\n"	, (int) zbi_work->co_save_size/4, (int) zbi_work->co_save_size );
    printk( "> cpu_idmap         = 0x%x\n"	, zbi_work->cpu_idmap );

	ptr = (unsigned int *)phys_to_virt(zbi_work->phys_cpu_ptr);
    printk( "> saved idmap		 = 0x%x\n"	, *ptr++ );
    printk( "> saved sp			 = 0x%x\n"	, *ptr++ );
    printk( "> resume fn 		 = 0x%x\n"	, *ptr++ );

    data = (u32 *) zbi_work->co_save;
    for( index = 0; index < zbi_work->co_save_size/4; index++ )
    {
    	printk( "%08X ", *data++ );
    	if( (index%8) == 7 ) printk( "\n" );
    }
    printk( "\n" );

    printk( "> cpu count = %d, size = %d\n"	, (int) zbi_work->cpu_save_size/4, (int) zbi_work->cpu_save_size );
    data = (u32 *) zbi_work->cpu_save;
    for( index = 0; index < zbi_work->cpu_save_size/4; index++ )
    {
    	printk( "%08X ", *data++ );
    	if( (index%8) == 7 ) printk( "\n" );
    }
    printk( "\n" );

    printk( "> copy_count    = (%d/%ld) max=%d\n"	, 
                                                        (int) zbi_work->copy_count,
                                                        ZBI_COPY_MAX,
                                                        (int) (ZBI_UNIT_SIZE-(sizeof(zbi_t) - (sizeof(zbi_copy_info_t)*ZBI_COPY_MAX)))/sizeof(zbi_copy_info_t)
                                                        );
    printk( "> copy_checksum = %08X\n"	, (int) zbi_work->copy_checksum);

    copy_data = zbi_work->copy_data;
    for( index = 0; index < zbi_work->copy_count; index++ )
    {
//    	printk( "> Bootcopy " );
//    	printk( "PADDR = %08X, "			, copy_data->dest_paddr );
//    	printk( "MEM PAGE OFFSET = %d, "	, copy_data->page_offset );
//    	printk( "\n" );
		if( copy_data->attr & ZBI_ATTR_MMU_PTE ) 
		{
//			printk( "> Bootcopy MMU PTE " );
//    		printk( "PADDR = %08X, "			, copy_data->dest_paddr );
//    		printk( "MEM PAGE OFFSET = %d, "	, copy_data->page_offset );
//    		printk( "\n" );
		}	
  	
    	copy_data++;
    }

    printk( "> fault_storage_io_vaddr	= %08X\n"	, (int) zbi_work->fault_storage_io_vaddr);
    printk( "> jump vaddress       = %08X\n"	, (int) zbi_work->jump_vaddr       );
}

void zb_add_page_value( void )
{
	NAL_CALL(m2n_set_page_value, (u32)zbi_work, 0, 0);
}

zbi_t *get_zbi_base(void)
{
	return zbi_work;
}
