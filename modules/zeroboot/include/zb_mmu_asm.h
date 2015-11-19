/*-----------------------------------------------------------------------------
  파 일 : zb_mmu_asm.h
  설 명 : ZB MMU 관련 처리 low macro 헤더 파일 
  작 성 : boggle70@falinux.com
  날 짜 : 2012-05-03
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_MMU_ASM_H_
#define _ZB_MMU_ASM_H_

#define MMU_TTB_BASE_ADDR_MASK    	0xFFFFC000

#define MMU_L1_FAULT      			0
#define MMU_L1_COARSE     			1
#define MMU_L1_SECTION    			2
#define MMU_L1_RESERVE     			3

#define MMU_L1_MASK_TYPE			0x3
#define MMU_L1_DOMAIN(x)			((x>>5)&0xF)
#define MMU_L1_P(x)					((x>>9)&0x1)
#define MMU_L1_TEX(x)				((x>>12)&0x7)
#define MMU_L1_AP(x)				((x>>10)&0x3)
#define MMU_L1_PADDR_SECTION(x) 	((x)&0xFFF00000)
#define MMU_L1_PADDR_COARSE(x) 		((x)&0xFFFFFC00)

#define MMU_L1_MAX_ENTRY    		4096

#define MMU_L2_FAULT      			0
#define MMU_L2_LARGE     			1
#define MMU_L2_SMALL    			2
#define MMU_L2_SMALL_X    			3

#define MMU_L2_MASK_TYPE			0x3
#define MMU_L2_MASK_FAULT(x)		((x)&0xFFFFFFFC)


#define MMU_L2_B(x)					((x>>2)&0x1)
#define MMU_L2_C(x)					((x>>3)&0x1)
#define MMU_L2_AP(x)				((x>>4)&0x3)
#define MMU_L2_TEX(x)				((x>>6)&0x7)
#define MMU_L2_APX(x)				((x>>9)&0x1)
#define MMU_L2_S(x)					((x>>10)&0x1)
#define MMU_L2_nG(x)				((x>>11)&0x1)

#define MMU_L2_PADDR_SMALL(x)		((x)&0xFFFFF000)


#define MMU_L2_MAX_ENTRY    		256

#define MMU_L1_SHIFT        		20
#define MMU_L2_SHIFT        		12 

#define MMU_L1_MASK        			0xFFF
#define MMU_L2_MASK        			0xFF

#endif  // _ZB_MMU_ASM_H_

