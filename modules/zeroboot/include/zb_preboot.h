/*-----------------------------------------------------------------------------
  파 일 : zb_preboot.h
  설 명 : ZB PREBOOT API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_PREBOOT_H_
#define _ZB_PREBOOT_H_


#define PREBOOT_PAGE_PADDR							0x20090000
#define PREBOOT_PAGE_VADDR							0x80090000
#define PREBOOT_SIZE								(0x8000)			// FIXME
#define PREBOOT_MAGIC                   			0xFA3443AF

extern void zb_preboot_setup		( void );
extern void zb_preboot_deactive		( void );
extern u32  zb_preboot_start_paddr  ( void );
extern u32  zb_preboot_size         ( void );

#endif  // _ZB_PREBOOT_H_

