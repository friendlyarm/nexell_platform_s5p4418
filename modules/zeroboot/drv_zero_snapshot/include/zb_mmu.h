/*-----------------------------------------------------------------------------
  파 일 : zb_mmu.h
  설 명 : ZB MMU 관련 처리 API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_MMU_H_
#define _ZB_MMU_H_

#include <zb_mmu_asm.h>
#include <zb_core_reg.h>


extern u32 		mmu_get_ttb_base							( void );
extern u32 		mmu_get_ttb2_base							( void );

#define			mmu_get_ttb_base_vaddr()					(phys_to_virt(mmu_get_ttb_base()&MMU_TTB_BASE_ADDR_MASK))
#define			mmu_get_vaddr_of_section_from_l1_entry(x)	(phys_to_virt(MMU_L1_PADDR_SECTION(x)))
#define			mmu_get_vaddr_of_coarse_from_l1_entry(x)	(phys_to_virt(MMU_L1_PADDR_COARSE(x)))


extern void 	mmu_set_ttb_base							( u32 value );
extern void 	mmu_set_ttb2_base							( u32 value );
extern u32 		mmu_get_ttbc								( void );

extern u32 		mcu_get_cpsr								( void );
                                    						
extern u32 		mmu_get_l1_entry							( u32 *ttb, u32 vaddr );
extern s32		mmu_is_coarse								( u32 l1_entry );
extern u32 		mmu_get_l2_entry							( u32 *ttb, u32 vaddr );
extern u32 		mmu_set_fault_l2_entry						( u32 *ttb, u32 vaddr );

extern u32 		mmu_virt_to_phys							( u32 vaddr );

extern u8 *		mmu_snapshot_coprocessor					( u8 *save_buffer );
extern unsigned char *	mmu_snapshot_coprocessor_new(u8 *save_buffer);
extern void dump_coprocessor_value(unsigned char *save_buffer, int opt);
extern void active_dcache(unsigned char *save_buffer);

extern void 	zb_toutch_all_memory_by_mmu					( u32 *ttb, u32 dram_size );

extern void		zb_rebuild_mmu_by_coarse_dram_area			( u32 dram_phys_base, u32 dram_size );

#endif  // _ZB_MMU_H_

