/*-----------------------------------------------------------------------------
  파 일 : zb_cpu.h
  설 명 : ZB CPU 관련 처리 API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_CPU_H_
#define _ZB_CPU_H_

// extern u32 cpu_snapshot( u8 *save_buffer, u32 dummy1, u32 dummy2, u32 dummy3 );
// extern u32 cpu_restore( u8 *save_buffer, u32 dummy1, u32 dummy2, u32 dummy3 );

extern void zb_io_init( void  );
extern void zb_io_free( void  );

extern void zb_io_save( void  );
extern void zb_io_save_close( void  );
extern void zb_io_restore( void  );				// 인터럽트가 비활성화 된 상태에서 호출된다.
extern void zb_io_restore_with_irq( void  );	// 인터럽트가 활성화 된 후 호출된다.

extern void zb_io_get_info_video( u32 *paddr, u32 *size );

#endif  // _ZB_CPU_H_

