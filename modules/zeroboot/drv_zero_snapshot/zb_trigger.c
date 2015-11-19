/*-----------------------------------------------------------------------------
  파 일 : drv_zero_trigger.c
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
#include <linux/kthread.h> 
#include <linux/cdev.h>          
#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/irqflags.h>
#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/bootmem.h>
#include <linux/workqueue.h>
#include <linux/time.h>			
#include <linux/timer.h>		

#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <asm/gpio.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <asm/delay.h>
#include <asm/mach/map.h>

#include <linux/mmzone.h> // search_all_pg_data()
#include <zb_snapshot.h>

#if defined(CONFIG_ARCH_OMAP4) || defined(CONFIG_ARCH_TI814X)
#define	ZB_TRIGGER_THREAD		0
#elif defined(CONFIG_ARCH_MXC)
#define	ZB_TRIGGER_THREAD		1
#else
#define	ZB_TRIGGER_THREAD		1				// 스냅샷을 쓰레드에서 시작한다.  S5PV210
#endif

static int	  wait_sec = 1;
static struct timer_list	kernel_timer;		// 커널 타이머
#if ZB_TRIGGER_THREAD
static struct task_struct 	*zerotask;
#endif
DECLARE_WAIT_QUEUE_HEAD( zeroboot_waitq );

extern int falinux_sdhci_suspend_host( void );	// driver/mmc/host/sdhci.c
extern int falinux_sdhci_resume_host ( void );	// driver/mmc/host/sdhci.c


#if ZB_TRIGGER_THREAD
//------------------------------------------------------------------------------
/** @brief   스냅샷 시작 지원 쓰레드
	@remark  
*///----------------------------------------------------------------------------
static int zeroboot_thread(void *arg)
{
	daemonize("zero_thread");
	allow_signal(SIGKILL);
	set_current_state(TASK_INTERRUPTIBLE);

	for(;;) {
		interruptible_sleep_on(&zeroboot_waitq);

#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18))
//		if (kthread_should_stop())
//			break;
#endif
		while (signal_pending(current)) {
			siginfo_t info;
			unsigned long signr;

			signr = dequeue_signal_lock(current, &current->blocked, &info);

			switch(signr) {
				case SIGKILL:
					printk(KERN_INFO "%s SIGKILL received.\n", __FUNCTION__);
					goto die;
				default:
					printk(KERN_INFO "%s signal %ld received.\n", __FUNCTION__, signr);
					break;
			}
		}
		
		printk( "..............snapshot start with the thread\n" );
		zb_snapshot(0);
		
		yield();
	}
#if (LINUX_VERSION_CODE > KERNEL_VERSION(2,6,18))
die:
	;
#else
	complete_and_exit(NULL, 0);
#endif
	return 0;
}
#else // end ZB_TRIGGER_THREAD
//------------------------------------------------------------------------------
/** @brief   스냅샷 시작 지원 타이머 핸들러 함수
	@remark  
*///----------------------------------------------------------------------------
static void zb_timer_trigger( unsigned long arg )
{
	printk( "..............snapshot start with the timer\n" );
	zb_snapshot(0);
}
#endif // not ZB_TRIGGER_THREAD

//------------------------------------------------------------------------------
/** @brief   스냅샷 트리거
	@remark  
*///----------------------------------------------------------------------------
void zb_event_trigger( void )
{
#if ZB_TRIGGER_THREAD
	wake_up_interruptible(&zeroboot_waitq);
#else
	kernel_timer.expires  = get_jiffies_64() + (wait_sec*HZ);
	kernel_timer.function = zb_timer_trigger;
	kernel_timer.data     = 0;
	add_timer( &kernel_timer );
#endif
}
/*------------------------------------------------------------------------------
  @brief   타이머일 경우 사용
  @remark  
*///----------------------------------------------------------------------------
void zb_event_trigger_set_time(int sec)
{
	if( sec < 1 ) sec = 1;
	wait_sec = sec;
}
/*------------------------------------------------------------------------------
  @brief   타이머일 경우 사용
  @remark  
*///----------------------------------------------------------------------------
void zb_timer_remove( void )
{
	del_timer( &kernel_timer );
}

/*------------------------------------------------------------------------------
  @brief   모듈 초기화 
  @remark  
*///----------------------------------------------------------------------------
int zb_trigger_init( void )
{
	printk( "ZERO TRIGGER READY\n" );

#if ZB_TRIGGER_THREAD
	zerotask = kthread_run(zeroboot_thread, NULL, "zero_thread");
#endif	

	init_timer( &kernel_timer );

    return 0;
}

/*------------------------------------------------------------------------------
  @brief   모듈 해제
  @remark  
*///----------------------------------------------------------------------------
void zb_trigger_exit( void )
{
#if ZB_TRIGGER_THREAD
	kthread_stop(zerotask);
	wake_up_interruptible(&zeroboot_waitq);
#endif

	del_timer( &kernel_timer );

	printk( "ZERO TRIGGER EXIT\n" );
}

MODULE_AUTHOR("frog@falinux.com");
MODULE_LICENSE("GPL");

