/*-----------------------------------------------------------------------------
  파 일 : zb_app_inf.c
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-02-02
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

#include <nalcode_storage.h>

#define  PROC_ZBSTATUS_NAME		"zb_status"
#define  DEV_NAME				"zb_app_inf"
#define  DEV_VER				"ver 0.2.0"

/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------

/// @}

/// @{
/// @brief  외부함수 정의
///-----------------------------------------------------------------------------
extern u32 zb_get_status( void );
extern wait_queue_head_t *zb_get_status_waitq( void );

/// @}


/*------------------------------------------------------------------------------
  @brief   제로부트 상태를 기다린다.
  @remark  
*///----------------------------------------------------------------------------
static void zbapp_wait_status( u32 wait_status )
{
	wait_queue_head_t *wq;
	u32  cur_status, old_status;

	//printk( "wait_status=%d\n", wait_status );

	if ( wait_status <= zb_get_status() ) return;

	wq = zb_get_status_waitq();
	while( 1 )
	{
		old_status = zb_get_status();
		
		interruptible_sleep_on( wq );
		
		// ctrl-C 
		cur_status = zb_get_status();
		if ( old_status == cur_status ) return;
		
		// goal status
		if ( cur_status >= wait_status ) return;

/*
		switch( status )
		{
		case ZB_STATUS_NORMALBOOT            :	yield()   ; break;
		case ZB_STATUS_SNAPSHOT_READY        :	yield()   ; break;
		case ZB_STATUS_SNAPSHOT_DONE         :	schedule(); break;
		case ZB_STATUS_ZEROBOOT              :  yield()   ; break;
		case ZB_STATUS_ZEROBOOT_RECOVERY_DONE:	return;
		default                              :  return;
		}
*/
	}
}
/*------------------------------------------------------------------------------
  @brief   trigger proc 읽기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zbapp_proc_read(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
	char *p;
	u32  status;

	p = buf;

	p += sprintf(p, "\n" );
	
	switch( status = zb_get_status() )
	{
	case ZB_STATUS_NORMALBOOT            :	p += sprintf(p, " %2d:%s\n", status, "ZB_STATUS_NORMALBOOT            " ); break;
	case ZB_STATUS_SNAPSHOT_READY        :	p += sprintf(p, " %2d:%s\n", status, "ZB_STATUS_SNAPSHOT_READY        " ); break;
	case ZB_STATUS_SNAPSHOT_DONE         :	p += sprintf(p, " %2d:%s\n", status, "ZB_STATUS_SNAPSHOT_DONE         " ); break;
	case ZB_STATUS_ZEROBOOT              :  p += sprintf(p, " %2d:%s\n", status, "ZB_STATUS_ZEROBOOT              " ); break;
	case ZB_STATUS_ZEROBOOT_RECOVERY_DONE:	p += sprintf(p, " %2d:%s\n", status, "ZB_STATUS_ZEROBOOT_RECOVERY_DONE" ); break;
	default                              :  p += sprintf(p, " %2d:%s\n", status, "unknown" ); break;
	}
	
	{
		p += sprintf(p, "\n" );
		p += sprintf(p, "\t*%2d:%s\n", ZB_STATUS_NORMALBOOT            , "ZB_STATUS_NORMALBOOT            " ); 
		p += sprintf(p, "\t*%2d:%s\n", ZB_STATUS_SNAPSHOT_READY        , "ZB_STATUS_SNAPSHOT_READY        " ); 
		p += sprintf(p, "\t*%2d:%s\n", ZB_STATUS_SNAPSHOT_DONE         , "ZB_STATUS_SNAPSHOT_DONE         " ); 
		p += sprintf(p, "\t*%2d:%s\n", ZB_STATUS_ZEROBOOT              , "ZB_STATUS_ZEROBOOT              " ); 
		p += sprintf(p, "\t*%2d:%s\n", ZB_STATUS_ZEROBOOT_RECOVERY_DONE, "ZB_STATUS_ZEROBOOT_RECOVERY_DONE" ); 
	}

	p += sprintf(p, "\n" );

	*eof = 1;

	return p - buf;
}

/*------------------------------------------------------------------------------
  @brief   trigger proc 쓰기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int zbapp_proc_write( struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char	cmd[256];
	int		len, dumy, wait_status;

	memset( cmd, 0, sizeof( cmd ) );
	if ( count > sizeof( cmd ) ) len = sizeof( cmd );
	else						 len = count;	

	dumy = copy_from_user( cmd, buf, len );
	cmd[len-1] = 0;	// CR 제거

	if ( 0 == strncmp( "wait=", cmd, 5 ) ) 
	{
		wait_status = simple_strtoul( &cmd[5], NULL, 0 );
		zbapp_wait_status( wait_status );
	}

	return count;
}


//------------------------------------------------------------------------------
/** @brief   open
	@remark  
*///----------------------------------------------------------------------------
static int zbapp_open( struct inode *inode, struct file *filp )
{	
    return 0;
}
//------------------------------------------------------------------------------
/** @brief   release
	@remark  
*///----------------------------------------------------------------------------
static int zbapp_release( struct inode *inode, struct file *filp)
{
    return 0;
}
//------------------------------------------------------------------------------
/** @brief   ioctl
	@remark  
*///----------------------------------------------------------------------------
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)
static long zbapp_ioctl( struct file *filp, unsigned int cmd, unsigned long arg )
#else
static int zbapp_ioctl( struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg )
#endif
{
	u32 status, cprtn;
		
    switch( cmd )
    {
	case IOCTL_ZB_GET_STATUS   :
		status = zb_get_status();
		cprtn  = copy_to_user( (void *)arg, &status, sizeof(u32) );
		if ( sizeof(u32) == cprtn ) return 0;
		return -EINVAL;

	case IOCTL_ZB_WAIT_STATUS  :
		cprtn = copy_from_user( &status, (void *)arg, sizeof(u32) );
		zbapp_wait_status( status );
		if ( sizeof(u32) == cprtn ) return 0;
		return -EINVAL;
    }
    
    return -EINVAL;
}
/*-----------------------------------------------------------------------------
  드라이버에 사용되는 접근 함수에 대한 함수 포인터 구조체를 정의 한다.
  이 구조체는 fs.h에 정의 되어 있다.
-------------------------------------------------------------------------------*/
static struct file_operations zbapp_fops =
{
        .open    = zbapp_open, 
        .release = zbapp_release, 

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,36)        
		.unlocked_ioctl = zbapp_ioctl,
#else
        .ioctl   = zbapp_ioctl,
#endif        
};

/*------------------------------------------------------------------------------
  @brief   proc  파일을 생성한다.
  @remark  
*///----------------------------------------------------------------------------
void zbapp_create_proc( void )
{
	struct proc_dir_entry *procdir;

	procdir = create_proc_entry( PROC_ZBSTATUS_NAME, S_IFREG | S_IRUGO, 0);
	procdir->read_proc  = zbapp_proc_read;
	procdir->write_proc = zbapp_proc_write;
}

/*------------------------------------------------------------------------------
  @brief   드라이버 초기화 
  @remark  
*///----------------------------------------------------------------------------
static int major;
int zb_app_inf_init( void )
{
	major = register_chrdev( ZB_APP_INF_DRV_MAJOR, DEV_NAME, &zbapp_fops );
	if (major) {
        printk(" register device %s %s OK (major=%d)\n", DEV_NAME, DEV_VER, major);
    }        
    else
    {        
        printk(" unable to get major %d for %s \n", major, DEV_NAME );
        return -EBUSY;
    }

	zbapp_create_proc();
    return 0;
}

/*------------------------------------------------------------------------------
  @brief   드라이버  해제
  @remark  
*///----------------------------------------------------------------------------
void zb_app_inf_exit( void )
{
	remove_proc_entry( PROC_ZBSTATUS_NAME, 0 );
	
	unregister_chrdev( major, DEV_NAME );
}


