/*-----------------------------------------------------------------------------
  파 일 : zb_nalcode.h
  설 명 : ZB NALCODE API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_NALCODE_H_
#define _ZB_NALCODE_H_

typedef struct
{
	u32 	magic;
	u32 	org_vector_data_abort;
	u32 	stub_data_abort_vector_address;
	u32 	org_vector_prefetch;
	u32 	stub_prefetch_vector_address;
	u32     nalcode_func;
	u32     nalcode_cpu_save_entry;
	u32     nalcode_cpu_restore_entry;
	u32     nalcode_pte_modify_entry;
	u32		snapshot_wait;
} nalcode_head_t;

extern nalcode_head_t *nalcode;

#define NALCODE_PAGE_PADDR							(PHYS_OFFSET + 0x00088000)
#define NALCODE_PAGE_VADDR							(PAGE_OFFSET + 0x00088000)

#define NALCODE_STACK_PADDR							(PHYS_OFFSET + 0x00098000)
#define NALCODE_STACK_VADDR							(PAGE_OFFSET + 0x00098000)
#define NALCODE_STACK_SIZE							(64*1024)

// NALCODE total size is 64KiB
//#define NALCODE_SIZE								(24*1024)
#define NALCODE_SIZE								((64-8)*1024)
#define NALCODE_SPARE_SIZE							(8*1024)
#define NALCODE_MAGIC                   			0xFA1221AF

#define	NALCODE_VECTOR_TABLE						0xFFFF0000
#define	NALCODE_PREFETCH_VECTOR_VADDRESS			0xFFFF000C
#define	NALCODE_DATA_ABORT_VECTOR_VADDRESS			0xFFFF0010
#define	NALCODE_VECTOR_BRANCH_HOOK					0xEA000003


#define NALCODE_MAGIC_FAULT							0x4
#define NALCODE_FAULT_INDEX_SHIFT					2
#define NALCODE_FAULT_INDEX(x)						((NALCODE_MAGIC_FAULT<<28)|(x<<NALCODE_FAULT_INDEX_SHIFT))
#define NALCODE_GET_MAGIC_FAULT(x)					((x>>28)&0xF)					
#define NALCODE_GET_FAULT_INDEX(x)					((x&0x0FFFFFFF)>>NALCODE_FAULT_INDEX_SHIFT)

#define NALCODE_PAGE_CACHE_MARK(x)					((NALCODE_MAGIC_FAULT<<28)|(((x)&0xFFFFF000)>>4))
#define NALCODE_GET_PAGE_CACHE_PADDR(x)				(((x)<<4)&0xFFFFF000)

void nalcode_storage_first_call(void);

#endif  // _ZB_NALCODE_H_

