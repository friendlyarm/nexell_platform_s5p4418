/*-----------------------------------------------------------------------------
  파 일 : zb_mmu_s3c6410_debug.h
  설 명 : ZB MMU S3C6410 디버깅 관련 처리 API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_MMU_DEBUG_H_
#define _ZB_MMU_DEBUG_H_

extern void zb_disp_mmu_ttb						( void );
extern void zb_disp_mmu_ttbc					( void );

extern void zb_disp_mmu_for_vector_l1_entry		( void );
extern void zb_disp_mmu_for_vaddr_l1_entry_from_ttb( u32 *ttb, u32 vaddr );
extern void zb_disp_mmu_for_vaddr_l1_entry		( u32 vaddr );
extern void zb_disp_mmu_l1_for_current			( void );
extern void zb_disp_mmu_l1						( u32 *ttb );

extern void zb_disp_mmu_l1_l2					( u32 *ttb );

extern void zb_disp_current_mcu_mode			( void );
extern void zb_disp_mmu_l1_l2_for_current		( void );

#endif  // _ZB_MMU_DEBUG_H_

