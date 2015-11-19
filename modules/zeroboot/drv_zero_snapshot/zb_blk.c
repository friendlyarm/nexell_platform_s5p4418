/*-----------------------------------------------------------------------------
  파 일 : drv_blk.c
  설 명 : 
  작 성 : boggle70@falinux.com
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
#include <linux/poll.h>     // poll
#include <linux/proc_fs.h>
#include <linux/irq.h>
#include <linux/gfp.h>


#include <linux/err.h>
#include <linux/irqflags.h>

#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/delay.h>


#include "zbi.h"

#include <zb_blk.h>
#include <zb_nand.h>


/// @brief  변수정의
///-----------------------------------------------------------------------------
struct zblk_ops zops;
struct zbi_t *zbi_blk=(struct zbi_t *)ZBI_VADDR;
///-----------------------------------------------------------------------------


/// @ 드라이버에서 사용하는 공통 API
///-----------------------------------------------------------------------------


/*-----------------------------------------------------------------------------
 *
 * 공통 API
 *
 *----------------------------------------------------------------------------*/
// zblk_probe
// 장치를 확인한다
int zblk_probe(void)
{
	if( !zops.probe )	return -1;

	return zops.probe();
}

// 블록 장치를 사용할 준비를시킨다.
// 대상 블록을 지우거나 초기화시킨다.
int zblk_setup_first(void)
{
	if( !zops.setup_first)	return -1;

	return zops.setup_first();
}

// 저장할 디바이스의 offset 주소를 구하는 함수
u32 zblk_get_storage_offset(void)
{
	if( !zops.get_storage_offset)	return 0;

	return zops.get_storage_offset();
}

u32 zblk_get_io_vaddr(void)
{
	if( !zops.get_io_vaddr)	{
		printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
		printk("\t\tNeeds zops.get_io_vaddr\n");
		printk("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	}
	if( !zops.get_io_vaddr)	return ZB_PAGE_INVALID;

	return zops.get_io_vaddr();
}

u32 zblk_get_index(u32 page)
{
	if( !zops.get_index)	return ZB_PAGE_INVALID;

	return zops.get_index(page);
}

int zblk_page_write(u32 page, u8 *buf)
{
	if( !zops.page_write)	return -1;

	return zops.page_write(page, buf);
}

int zblk_page_read(u32 page, u8 *buf)
{
	if( !zops.page_read)	return -1;

	return zops.page_read(page, buf);
}

int zblk_burst_read(u32 page, u8 *buf, u32 size)
{
	if( !zops.burst_write)	return -1;

	return zops.burst_read(page, buf, size);
}

int zblk_burst_write(u32 page, u8 *buf, u32 size)
{
	if( !zops.burst_read)	return -1;

	return zops.burst_read(page, buf, size);
}

int zblk_dev_reset(u32 ndev)
{
	if( !zops.reset)	return -1;

	return zops.reset(ndev);
}

struct zblk_ops *get_zb_blk_info(void)
{
	return &zops;
}
EXPORT_SYMBOL(get_zb_blk_info);

//u32 zb_nand_get_page_size(void)
//{
//	return 2048;
//}

int zblk_setup_post_zeroboot(void)
{
	if( !zops.setup_post_zeroboot)	return -1; 	
	
	return zops.setup_post_zeroboot();
}

/*-----------------------------------------------------------------------------
 *
 * 초기화및 exit 함수
 *
 *----------------------------------------------------------------------------*/
int	zb_blk_init(void)
{
	memset(&zops, 0x00, sizeof(zops));
	return 0;
}
int zb_blk_exit(void)
{
	memset(&zops, 0x00, sizeof(zops));
	return 0;
}



MODULE_AUTHOR("boggle70@falinux.com");
MODULE_LICENSE("GPL");

