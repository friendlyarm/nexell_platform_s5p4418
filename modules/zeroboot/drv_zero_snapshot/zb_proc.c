/*-----------------------------------------------------------------------------
  파 일 : drv_zero_nalcode_main.c
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

#include <zbi.h>
#include <zb_blk.h>
#include "zb_nalcode.h"
#include "zb_trigger.h"
#include <zb_snapshot.h>

#define PROC_SNAPSHOT_NAME	"zeroboot_snapshot"
#define PROC_NAL_NAME 		"nalcode"
#define PROC_TRIGGER_NAME	"zeroboot_trigger"
// #define NALCODE_START   0xFFFF8000

/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------
#include <nalcode_storage.h>

static u32 nalcode_offset = 0;
/// @}
extern void zb_set_status( u32 status );  // drv_zero_snapshot_main.c


/*------------------------------------------------------------------------------
  @brief   NALCODE DATA ABORT 를 동작하게 한다. 
  @remark  
*///----------------------------------------------------------------------------
static void fa_nalcode_active_data_abort( void )
{
	nalcode_head_t	*nalcode;
	u32 			*vector_hook;
	u32				nal_offset;
	u32				*nal_entry;
	
	printk( "NALCODE DATA ABORT ACTIVE\n" );
	
	nalcode 	= (nalcode_head_t *)(NALCODE_PAGE_VADDR);
	
	if( nalcode->magic != NALCODE_MAGIC ) 
	{
		printk( "Oops Program Bug ... Nalcode Magic is bad\n" );
		return;
	}	
	
	vector_hook 					= (u32 *)(NALCODE_DATA_ABORT_VECTOR_VADDRESS);
	nalcode->org_vector_data_abort 	= *vector_hook;
	nal_entry						= (u32 *)(NALCODE_DATA_ABORT_VECTOR_VADDRESS + 4*7);
	
	printk("DATA ABORT ORG VECTOR VADDRESS 0x%08X\n", (int) nalcode->org_vector_data_abort );
	
	nal_offset = nalcode->org_vector_data_abort;
	nal_offset &= 0x0000FFFFF;
	nal_offset *= 4;
	nal_offset += 0xFFFF0014;
	nalcode->stub_data_abort_vector_address = nal_offset;

	*nal_entry = NALCODE_PAGE_VADDR + sizeof(nalcode_head_t) + sizeof(long);
	*vector_hook = NALCODE_VECTOR_BRANCH_HOOK;
//	printk("*vector_hook = %08X:%08X:%08X\n",  vector_hook , *vector_hook,  NALCODE_VECTOR_BRANCH_INSTRUCTION );
	
	// FIXME:
//	clean_dcache_area( (void *) NALCODE_VECTOR_TABLE, PAGE_SIZE );
	
}

/*------------------------------------------------------------------------------
  @brief   NALCODE DATA ABORT 를 동작하게 한다. 
  @remark  
*///----------------------------------------------------------------------------
static void fa_nalcode_active_prefetch( void )
{
	nalcode_head_t	*nalcode;
	u32 		*vector_hook;
	u32			nal_offset;
	u32				*nal_entry;
	
	printk( "PREFETCH ACTIVE\n" );
	
	nalcode 	= (nalcode_head_t *)(NALCODE_PAGE_VADDR);
	
	
	if( nalcode->magic != NALCODE_MAGIC ) 
	{
		printk( "Oops Program Bug ... Nalcode Magic is bad\n" );
		return;
	}	
	
	vector_hook 				= (u32 *)(NALCODE_PREFETCH_VECTOR_VADDRESS);
	nalcode->org_vector_prefetch = *vector_hook;
	nal_entry					= (u32 *)(NALCODE_PREFETCH_VECTOR_VADDRESS + 4*7);
	
	printk("PREFETCH ORG VECTOR VADDRESS 0x%08X\n", (int) nalcode->org_vector_prefetch );
	
	nal_offset = nalcode->org_vector_prefetch;
	nal_offset &= 0x0000FFFFF;
	nal_offset *= 4;
	nal_offset += 0xFFFF0014;
	nalcode->stub_prefetch_vector_address 	= nal_offset;
	
	*vector_hook = NALCODE_VECTOR_BRANCH_HOOK;
	*nal_entry = NALCODE_PAGE_VADDR + sizeof(nalcode_head_t);
//	printk("*vector_hook = %08X:%08X:%08X\n",  vector_hook , *vector_hook , NALCODE_VECTOR_BRANCH_INSTRUCTION_PREFETCH );

	// FIXME:
//	clean_dcache_area( (void *) NALCODE_VECTOR_TABLE, PAGE_SIZE );
}

/*------------------------------------------------------------------------------
  @brief   NALCODE 를 비 활성화 시킨다. 
  @remark  
*///----------------------------------------------------------------------------
void zb_nalcode_deactive( void )
{
	nalcode_head_t	*nalcode;
	u32 			*vector_hook;
	
	printk( "NALCODE DEACTIVE\n" );
	
	nalcode 	= (nalcode_head_t *)(NALCODE_PAGE_VADDR);
	if( nalcode->magic != NALCODE_MAGIC ) 
	{
		printk( "Oops Program Bug ... Nalcode Magic is bad\n" );
		return;
	}	
	
	vector_hook  = (u32 *)(NALCODE_DATA_ABORT_VECTOR_VADDRESS);
	*vector_hook = nalcode->org_vector_data_abort;

	vector_hook = (u32 *)(NALCODE_PREFETCH_VECTOR_VADDRESS);
	*vector_hook = nalcode->org_vector_prefetch;

//	clean_dcache_area( (void *) NALCODE_VECTOR_TABLE, PAGE_SIZE );
	flush_cache_all();
	outer_flush_all();
}


/*------------------------------------------------------------------------------
  @brief   NALCODE 펑션 콜을 테스트 한다. 
  @remark  
*///----------------------------------------------------------------------------
void fa_nalcode_func_test( void )
{
	void (*nalcode_func)( u32 function, u32 param0, u32 param1, u32 param2 );
//	void (*nalcode_func)( void );
	
	
	nalcode_head_t	*nalcode;
	nalcode 	= (nalcode_head_t *)(NALCODE_PAGE_VADDR);
	
	nalcode_func = (void (*)( u32 function, u32 param0, u32 param1, u32 param2 )) &(nalcode->nalcode_func);
//	nalcode_func = (void (*)( void )) &(nalcode->nalcode_func);
	
	printk( "NALCODE FUNCTION = %08X\n", (int) nalcode_func );
	
	nalcode_func(0,0,0,0);
	
	printk( "RETURN 4\n" );
}

/*------------------------------------------------------------------------------
  @brief   NALCODE 를 초기화 한다.
  @remark  
*///----------------------------------------------------------------------------
void zb_nalcode_init( void )
{
//	u32 *data;
	
	// NALCODE 복구 코드가 호출되도록 페이지폴트 벡터를 바꾼다.
	fa_nalcode_active_data_abort();									
	fa_nalcode_active_prefetch();
	
//	data = 0;
//	printk( "%d\n", *data );
	
//	fa_nalcode_func_test();

}

/*------------------------------------------------------------------------------
  @brief   NALCODE 의 시작 주소를 반환한다. 
  @remark  
*///----------------------------------------------------------------------------
u32  zb_nalcode_start_paddr  ( void )
{
	return NALCODE_PAGE_PADDR;
}

/*------------------------------------------------------------------------------
  @brief   NALCODE 의 크기를 반환한다. 
  @remark  
*///----------------------------------------------------------------------------
u32  zb_nalcode_size         ( void )
{
	return NALCODE_SIZE;
}

u32 zb_nalcode_spare_size(void)
{
	return 2*PAGE_SIZE;
}

/*------------------------------------------------------------------------------
  @brief   trigger proc 읽기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zb_trigger_proc_read(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
	char *p;

	p = buf;

	p += sprintf(p, "\n" );
	p += sprintf(p, "it is command of zeroboot snapshot trigger\n" );
	p += sprintf(p, "usage) echo 'start'   > /proc/zeroboot_trigger\n" );
	p += sprintf(p, "       echo 'start 5' > /proc/zeroboot_trigger\n" );
	p += sprintf(p, "       echo 'cancel'  > /proc/zeroboot_trigger\n" );
	p += sprintf(p, "       echo 'boot=0xXXXXXXXX'  > /proc/zeroboot_trigger\n" );
	p += sprintf(p, "       echo 'fault'  > /proc/zeroboot_trigger\n" );
	p += sprintf(p, "\n" );

	*eof = 1;

	return p - buf;
}

static int zb_multi_boot_start(unsigned long subaddress)
{
    u32 (*bootup)( void );
    zbi_t *zbi;
    char *buf;
    u32 count;
    
    printk("Zeroboot multi boot start at 0x%08lX\n", subaddress);

    zbi = kmalloc(SZ_8K, GFP_KERNEL);
    if (!zbi) return 0;
    
    buf = (unsigned char *)zbi;
    zblk_page_read(subaddress/PAGE_SIZE, buf);
    
    if( 0xFA0000B2 != zbi->magic ) 
    {
        printk("different magic read value : 0x%08X\n", zbi->magic);
        goto err_out;
    }
    printk(" magic 0x%08X\n", zbi->magic);
    
    bootup = (u32 (*)( void ))(PAGE_OFFSET + 0x00090000);
    for(count = 0; (count*4*1024) < (zbi->bootup_size + (8*1024)); count++)
    {
        zblk_page_read((zbi->bootup_address + count), ((unsigned char *)bootup + (count*4*1024)));
    }

    printk("======= jump\n");
//  msleep(1000);
    printk("S\n");

    // run multi boot laod
    bootup = (u32 (*)( void ))(PAGE_OFFSET + 0x00090000);
    bootup();
    
err_out:
    if (zbi) kfree(zbi);
    return 0;
}

static pmd_t *get_phys_pmd_address(unsigned long addr)
{
	struct mm_struct *mm;
	pgd_t *pgd;
	pgd_t *pud;
	pmd_t *pmd;

	if (!addr) return 0;

	if (!current->mm) {
		printk("No, mm.... sorry\n");
		return 0;
	}

	mm = current->mm;
	pgd = pgd_offset(mm, addr);
	pud = pud_offset(pgd, addr);
	pmd = pmd_offset(pud, addr);

	printk("mm       0x%p\n", mm);
	printk("mm->pgd  0x%p\n", mm->pgd);
	printk("pmd      0x%p\n", pmd);

	return pmd;
}

struct zb_l1_mapping_info {
	/* do not change order */
	int				used;
	unsigned long	fake_l1_value;
	pmd_t *fake_l1_ptr;
	unsigned long	old_l1_value;
};
static int zb_mmu_onoff(unsigned long subaddress)
{
    u32 (*bootup)(void *, void *, unsigned long, unsigned long);
	u32	lr;
	struct zb_l1_mapping_info map_info;

	pmd_t *pmd;

    printk("Zeroboot mmu on off start at 0x%08lX\n", subaddress);

    // run mmu on off code
    bootup = (u32 (*)(void *, void *, unsigned long, unsigned long))(PAGE_OFFSET + 0x00090000);
	printk("======= jump\n");

	// get tlb pointer	ttb_base
	// get physical memory base PHYS_OFFSET
	// calculate where is phycal base offset
	// get offset pointer

	pmd = get_phys_pmd_address(PHYS_OFFSET + 0x00090000);

	map_info.used = 1;
	map_info.fake_l1_ptr = pmd;
	map_info.fake_l1_value = PHYS_OFFSET + 0x40E;
	map_info.old_l1_value = pmd_val(*pmd);

	printk("used %p\n", &map_info);
	printk("used %d\n", map_info.used);
	printk("used %p\n", map_info.fake_l1_ptr);
	printk("used %lx\n", map_info.fake_l1_value);
	printk("used %lx\n", map_info.old_l1_value);
	
	flush_cache_all();
    lr = bootup(0, (void *)&map_info, PAGE_OFFSET, PHYS_OFFSET);
	printk("======= return pc 0x%08X\n", lr);
    
    return 0;
}


/*------------------------------------------------------------------------------
  @brief   trigger proc 쓰기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zb_trigger_proc_write( struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char	cmd[256];
	int		wait_sec = 1;
	int		len;
	unsigned long subaddress = 0;

	memset( cmd, 0, sizeof( cmd ) );
	if ( count > sizeof( cmd ) ) len = sizeof( cmd );
	else						 len = count;	

	if (copy_from_user( cmd, buf, len ))
		return -EFAULT;

	cmd[len-1] = 0;	// CR 제거
	printk( "write command = [%s]\n", cmd );

	if      ( 0 == strncmp( "start", cmd, 5 ) ) 
	{
		zb_set_status( ZB_STATUS_SNAPSHOT_READY );
		
		if( strlen( cmd ) > 5 ) 
		{
			wait_sec = simple_strtoul(&cmd[6],NULL,0);
			zb_event_trigger_set_time(wait_sec);
		}
		else
		{
			wait_sec = 1;
			zb_event_trigger_set_time(wait_sec);
		}	
		
		printk("you may see to starting of zero boot snapshot , after %d second...\n", wait_sec );
		
		//zb_event_trigger();
		zb_snapshot(0);
		yield();
	}
	else if( 0 == strncmp( "cancel", cmd, 5 ) ) 
	{
		zb_timer_remove();
	}
	else if( 0 == strncmp( "boot=", cmd, 5 ) )
	{
		subaddress = simple_strtoul(&cmd[5],NULL,0);
		zb_multi_boot_start(subaddress);
	}
	else if( 0 == strncmp( "mmu", cmd, 3 ) )
	{
		subaddress = simple_strtoul(&cmd[3],NULL,0);
		printk("mmu turn on subaddr 0x%lX\n", subaddress);
		zb_mmu_onoff(subaddress);
	}
	else if( 0 == strncmp( "fault", cmd, 5 ) )
	{
		unsigned int *on_fault, tmp;

		on_fault = NULL;

		tmp = *on_fault;
	}


	return count;
}

/*------------------------------------------------------------------------------
  @brief   proc 읽기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zb_nal_proc_read(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
	char 			*p;
	unsigned char  *read_data;
	int				lp;
	
	read_data = (unsigned char  *)NALCODE_PAGE_VADDR;

	p = buf;

	p += sprintf(p, "NALCODE DATA ADDRESS = %08X\n", (int) read_data );
	p += sprintf(p, "=============================================\n");
	
	for( lp = 0; lp < 128; lp++ )
	{
		if( lp % 16 == 0 ) p += sprintf(p, "%08X : ", (int) read_data );
		p += sprintf(p, "%02X ", (int) *read_data++ );	
		if( lp % 16 == 15 ) p += sprintf(p, "\n");
	}
	
	p += sprintf(p, "\n");

	*eof = 1;

	return p - buf;
}

/*------------------------------------------------------------------------------
  @brief   proc 쓰기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zb_nal_proc_write( struct file *file, const char __user *buf, unsigned long count, void *data)
{
	u32				magic_code;
	unsigned char 	*write_address;
	
	get_user(magic_code, (u32 *)buf);
	if( magic_code == 0xFA1221AF )
	{
		printk( "NALCODE MAGIC FOUND\n" );
		nalcode_offset = 0;
		memset( (void *)NALCODE_PAGE_VADDR, 0, NALCODE_SIZE );
	}	 
		
	write_address = (unsigned char *)(NALCODE_PAGE_VADDR + nalcode_offset);
	printk( "NALCODE WRITE DEST ADDRESS = %08X, SIZE = %d\n", (int) write_address, (int) count  );
	
	if (copy_from_user(write_address, buf, count))
		return -EFAULT;

	nalcode_offset += count;
	
	return count;
}

/*------------------------------------------------------------------------------
  @brief   proc 읽기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zb_snaphot_proc_read(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
	char *p;

	p = buf;

	p += sprintf(p, "\n" );
	p += sprintf(p, "it is command of zeroboot snapshot\n" );
	p += sprintf(p, "usage) echo 'info'   > /proc/zeroboot_snapshot\n" );
	p += sprintf(p, "\n" );

	*eof = 1;

	return p - buf;
}

/*------------------------------------------------------------------------------
  @brief   proc 쓰기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zb_snapshot_proc_write( struct file *file, const char __user *buf, unsigned long count, void *data)
{

	char	cmd[256];

	int		len;

	memset( cmd, 0, sizeof( cmd ) );
	if ( count > sizeof( cmd ) ) len = sizeof( cmd );
	else						 len = count;	

	if (copy_from_user( cmd, buf, len ))
		return -EFAULT;

	cmd[len-1] = 0;	// CR 제거
	printk( "write command = [%s]\n", cmd );

	if      ( 0 == strncmp( "start", cmd, 5 ) ) 
	{
		
	}

	return count;
}

/*------------------------------------------------------------------------------
  @brief   트리거  proc  파일을 생성한다.
  @remark  
*///----------------------------------------------------------------------------
void zb_create_trigger_proc( void )
{
	struct proc_dir_entry *procdir;

	procdir = create_proc_entry( PROC_TRIGGER_NAME, S_IFREG | S_IRUGO, 0);
	procdir->read_proc  = zb_trigger_proc_read;
	procdir->write_proc = zb_trigger_proc_write;
}

/*------------------------------------------------------------------------------
  @brief   snapshot proc  파일을 생성한다.
  @remark  
*///----------------------------------------------------------------------------
static void zb_create_snapshot_proc( void )
{
	struct proc_dir_entry *procdir;

	procdir = create_proc_entry( PROC_SNAPSHOT_NAME, S_IFREG | S_IRUGO, 0);
	procdir->read_proc  = zb_snaphot_proc_read;
	procdir->write_proc = zb_snapshot_proc_write;
}

/*------------------------------------------------------------------------------
  @brief   NALCODE proc  파일을 생성한다.
  @remark  
*///----------------------------------------------------------------------------
void zb_create_nalcode_proc( void )
{
	struct proc_dir_entry *procdir;

	procdir = create_proc_entry( PROC_NAL_NAME, S_IFREG | S_IRUGO | S_IRWXUGO, 0);
	procdir->data 			= (void *)NALCODE_PAGE_VADDR;
	procdir->read_proc  	= zb_nal_proc_read;
	procdir->write_proc 	= zb_nal_proc_write;
	
}

/*------------------------------------------------------------------------------
  @brief   proc 초기화 
  @remark  
*///----------------------------------------------------------------------------
int zb_proc_init( void )
{
	zb_create_nalcode_proc();
	zb_create_trigger_proc();
	zb_create_snapshot_proc();

	printk( "ZEROBOOT PROC READY\n" );
    return 0;
}

/*------------------------------------------------------------------------------
  @brief   proc 해제
  @remark  
*///----------------------------------------------------------------------------
void zb_proc_exit( void )
{
	remove_proc_entry( PROC_NAL_NAME,0 );
	remove_proc_entry( PROC_TRIGGER_NAME,0 );
	remove_proc_entry( PROC_SNAPSHOT_NAME,0 );

	printk( "ZEROBOOT PROC REMOVE\n" );
}

MODULE_AUTHOR("frog@falinux.com");
MODULE_LICENSE("GPL");

