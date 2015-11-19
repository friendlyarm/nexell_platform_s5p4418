/*-----------------------------------------------------------------------------
  파 일 : zb_used_memory.h
  설 명 : 사용된 메모리 분석
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_USED_MEMORY_H_
#define _ZB_USED_MEMORY_H_

struct zb_reserved_mem {
    unsigned int base;
    unsigned int size;
};
#define RESERVED_MEM_END_BASE	0

extern void zb_registe_reserved_mem( struct zb_reserved_mem *rmlst );

extern u32  zb_get_used_mem_count( void );
extern void zb_check_used_memory( void );
extern void zb_used_memory_init(void);
extern void zb_used_memory_exit(void);
extern unsigned long *zb_get_mem_array(void);
extern unsigned long zb_get_mem_array_size(void);
extern unsigned long zb_get_next_page(unsigned long pfn, int *highzone);
extern unsigned long zb_get_exist_npage(void);

#endif  // _ZB_USED_MEMORY_H_
