/*-----------------------------------------------------------------------------
  파 일 : zb_storage.c
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-01-11
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
#include <linux/irq.h>		
#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <linux/time.h>			
#include <linux/timer.h>		
#include <linux/clk.h>

#include <zb_blk.h>

extern int   zb_storage_init( struct zblk_ops *zblk_ops );	// 외부 drv_storage/dev-nnnn/nnnn-xxx.c  파일에 정의되어 있어야 한다.
extern void  zb_storage_exit( void ); 						// 외부 drv_storage/dev-nnnn/nnnn-xxx.c  파일에 정의되어 있어야 한다.
extern struct zblk_ops *get_zb_blk_info(void);
 
/*------------------------------------------------------------------------------
  @brief   모듈 초기화 
  @remark  
*///----------------------------------------------------------------------------
//extern int zero_pvr_resume(int a);
static __init int zb_module_stroage_init( void )
{
	struct zblk_ops *zblk_ops = get_zb_blk_info();
	
//	printk( "resume\n" );
//	zero_pvr_resume(0);
//	return -1;

	printk( "zero-storage init...\n" );

	zb_storage_init( zblk_ops );
	
	printk("ZBLK OPS PAGE WRITE Function Address : %p\n", 	zblk_ops->page_write);
	
    return 0;
}

/*------------------------------------------------------------------------------
  @brief   모듈 해제
  @remark  
*///----------------------------------------------------------------------------
static __exit void zb_module_stroage_exit( void )
{
	zb_storage_exit();
	printk( "...zero-storage exit\n" );
}

module_init(zb_module_stroage_init);
module_exit(zb_module_stroage_exit);

MODULE_AUTHOR("freefrug@falinux.com");
MODULE_LICENSE("GPL");

/* end */

