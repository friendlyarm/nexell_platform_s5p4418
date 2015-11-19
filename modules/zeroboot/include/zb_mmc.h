/*-----------------------------------------------------------------------------
  파 일 : zb_nand.h
  설 명 : ZB NAND API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_MMC_H_
#define _ZB_MMC_H_

#include <zbi.h>
#include <zb_nalcode.h>

/* below is sample for each mmc support
 *
 * in drv_storage/arch-xxx/mmc.h
 *
	struct zb_priv_mmc {
		union {
			struct {
				unsigned int ioaddr;
				unsigned int ioaddr_phys;
				unsigned int data_buf;
				unsigned int data_buf_phys;
				unsigned int idmac_buf;
				unsigned int idmac_buf_phys;	
				unsigned int read_bl_len;
				unsigned int write_bl_len;
				unsigned int erase_grp_size;
				unsigned int high_capacity;
				short	    rca;
				unsigned int card_caps;
			};
		};
	
		u32 storage_priv[128];	// 최대사이즈.
	};
 */


	#ifndef NAL_PRE_LOAD	/* mmu_turn_on */
		#define	get_storage_priv_offset()	(((zbi_t *)ZBI_VADDR)->storage_priv)
	#else
		#define	get_storage_priv_offset()	(((zbi_t *)ZBI_PADDR)->storage_priv)
	#endif // NAL_PRE_LOAD

		

#endif  // _ZB_MMC_H_

