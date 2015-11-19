/*-----------------------------------------------------------------------------
  파 일 : zb_snapshot.h
  설 명 : ZB SNAPSHOT API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_SNAPSHOT_H_
#define _ZB_SNAPSHOT_H_


extern int zb_snapshot(unsigned long);
extern void zb_pre_setup_dma_area(unsigned long start_phys, unsigned long virt, int size );

// all platform must be define zb_add_platform_manage_area
extern void zb_add_platform_manage_area(void);


#endif  // _ZB_SNAPSHOT_H_

