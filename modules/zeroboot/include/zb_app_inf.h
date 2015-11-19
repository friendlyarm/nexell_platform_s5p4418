/*-----------------------------------------------------------------------------
  파 일 : zb_app_inf.h
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-02-03
  주 의 :

	linux/mtd/nand/nand_base.c 에서 사용될 때는 이 헤더파일을 해당 디렉토리에 
	복사하여 사용한다.
-------------------------------------------------------------------------------*/
#ifndef __ZB_APP_INF_H__
#define __ZB_APP_INF_H__


// <제로부트 상태>
enum {
	ZB_STATUS_NORMALBOOT                = 0,
	ZB_STATUS_SNAPSHOT_READY            = 10,
	ZB_STATUS_SNAPSHOT_DONE             = 20,
	ZB_STATUS_ZEROBOOT                  = 30,
	ZB_STATUS_ZEROBOOT_RECOVERY_DONE    = 40,
};

// <저장소 상태>
enum {
	STORAGE_ACCESS_MARK_DONE			= 0,
	STORAGE_ACCESS_MARK_KERNEL			= 1,
	STORAGE_ACCESS_MARK_KERNEL_ERASE	= 2,
	STORAGE_ACCESS_MARK_KERNEL_WRITE	= 3,
	STORAGE_ACCESS_MARK_NALCODE			= 0xfa,
};

// for appication, ioctl()
#ifdef CONFIG_ARCH_OMAP4
#define ZB_APP_INF_DRV_MAJOR			219
#else
#define ZB_APP_INF_DRV_MAJOR			0
#endif
#define	IOCTL_ZB_GET_STATUS				_IOR( 'z', 0, int )	
#define	IOCTL_ZB_WAIT_STATUS			_IOR( 'z', 1, int )	

#endif  // __ZB_APP_INF_H__

