/*-----------------------------------------------------------------------------
  파 일 : zb_debug.h
  설 명 : ZB NAND API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_DEBUG_H_
#define _ZB_DEBUG_H_

extern void zb_dump_data						( s8 *msg, u8 *data , u32 size );
extern void zb_nalcode_fault_test_zero_data		( void );
extern void zb_nalcode_fault_test_modify_coarse	( void );

#endif  // _ZB_DEBUG_H_

