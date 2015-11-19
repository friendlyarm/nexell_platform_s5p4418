/*-----------------------------------------------------------------------------
  파 일 : zb_nand.h
  설 명 : ZB NAND API 헤더 파일 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef _ZB_NAND_H_
#define _ZB_NAND_H_

#include <zbi.h>
#include <zb_nalcode.h>
struct zb_priv_nand {
	union {
		struct {
			unsigned long		offset;
			unsigned int		size;
			unsigned int		erasesize;
			unsigned int		pagesize;

			unsigned int		zb_start_page;
			unsigned int		page_per_4k;
			unsigned int		page_per_block;
			unsigned int		index_per_block;
			unsigned int		block_index_shtft;    
			unsigned int		block_page_shtft;
			unsigned int		zb_page_count;
			
			unsigned int		vaddr_nand_ctrl;
			unsigned int		vaddr_dma_ctrl;
			unsigned int		vaddr_ecc_ctrl;

			unsigned int		bbt_page;
			unsigned int		need_bbt_page_write;


// ADDED BY SEVO
                       unsigned int            writesize;    
                       unsigned int            oobsize;
                       unsigned int            cmd_buf_addr;
                       unsigned int            data_buf_addr;
                       unsigned int            dma_desc[4];

                       unsigned int            cmd_buf_addr_phys;
                       unsigned int            data_buf_addr_phys;
                       unsigned int            dma_desc_phys[4];

                       unsigned int            mxs_bch_base_virt;
                       unsigned int            mxs_gpmi_base_virt;
                       unsigned int            mxs_ccm_base_virt;
                       unsigned int            mxs_apbh_base_virt;
		};
	};

	u32	storage_priv[128];	// 최대사이즈.
};



//#ifdef ARG_ZB_STORAGE_OFFSET
//	// alreay defined both ZB_STORAGE_OFFSET, ZB_STORAGE_SIZE
//	#define	ZB_NAND_PAGE_SIZE				(2048)
//	#define ZB_NAND_START_PAGE				(ZB_STORAGE_OFFSET/ZB_NAND_PAGE_SIZE)
//	#define ZB_NAND_STORAGE_PAGE_COUNT		(ZB_STORAGE_SIZE/ZB_NAND_PAGE_SIZE)
//	#define ZB_NAND_PAGE_PER_4K				(PAGE_SIZE/ZB_NAND_PAGE_SIZE)	
//	#define ZB_NAND_PAGE_PER_BLOCK			(64)	
//	#define	ZB_NAND_BLOCK_SIZE				(ZB_NAND_PAGE_SIZE*ZB_NAND_PAGE_PER_BLOCK)			// 128k
//	#define ZB_NAND_INDEX_PER_BLOCK  		(ZB_NAND_PAGE_PER_BLOCK/ZB_NAND_PAGE_PER_4K)
//	#define ZB_NAND_BLOCK_INDEX_SHIFT		(5)	//  128K erase block = 5   256K erase block = 6
//											
//	#ifndef NAL_PRE_LOAD	/* mmu_turn_on */
//		#define ZB_VADDR_NAND_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->vaddr_nand_ctrl)
//		#define ZB_VADDR_DMA_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->vaddr_dma_ctrl )
//		#define	get_storage_priv_offset()	(((zbi_t *)ZBI_VADDR)->storage_priv)
//		#define BBT_ADDR					(PAGE_OFFSET + 0x00088000 + 0x00006000)
//	#else
//		#define ZB_VADDR_NAND_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->vaddr_nand_ctrl)
//		#define ZB_VADDR_DMA_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->vaddr_dma_ctrl )
//		#define	get_storage_priv_offset()	(((zbi_t *)ZBI_PADDR)->storage_priv)
//		#define BBT_ADDR					(PHYS_OFFSET + 0x00088000 + 0x00006000)
//	#endif // NAL_PRE_LOAD
//
//	#define BBT_ARRAY_CNT					8192		// 고정값
//	#define ZB_NAND_BBT_PAGE				(ZB_NAND_START_PAGE + ZB_NAND_STORAGE_PAGE_COUNT - ZB_NAND_PAGE_PER_BLOCK )
//
//#else

	#ifndef NAL_PRE_LOAD	/* mmu_turn_on */
//		#define ZB_STORAGE_OFFSET			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->offset         )
//		#define ZB_STORAGE_SIZE				((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->size           )
		#define ZB_NAND_START_PAGE			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->zb_start_page  )
		#define ZB_NAND_STORAGE_PAGE_COUNT	((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->zb_page_count  )
		#define ZB_NAND_PAGE_PER_4K			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->page_per_4k    )
		#define ZB_NAND_PAGE_PER_BLOCK		((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->page_per_block )
		#define ZB_NAND_INDEX_PER_BLOCK  	((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->index_per_block )
		#define ZB_NAND_BLOCK_INDEX_SHIFT	((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->block_index_shtft )
		#define ZB_NAND_PAGE_INDEX_SHIFT	((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->block_page_shtft )
		#define ZB_NAND_BLOCK_SIZE			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->erasesize      )
		#define ZB_NAND_PAGE_SIZE			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->pagesize       )
		#define ZB_VADDR_NAND_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->vaddr_nand_ctrl)
		#define ZB_VADDR_DMA_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->vaddr_dma_ctrl )
		#define ZB_VADDR_ECC_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->vaddr_ecc_ctrl )
		#define	get_storage_priv_offset()	(((zbi_t *)ZBI_VADDR)->storage_priv)
		#define BBT_ADDR					(PAGE_OFFSET + 0x00098000 - 0x00002000)
	#else
//		#define ZB_STORAGE_OFFSET			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->offset         )
//		#define ZB_STORAGE_SIZE				((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->size           )
		#define ZB_NAND_START_PAGE			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->zb_start_page  )
		#define ZB_NAND_STORAGE_PAGE_COUNT	((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->zb_page_count  )
		#define ZB_NAND_PAGE_PER_4K			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->page_per_4k    )
		#define ZB_NAND_PAGE_PER_BLOCK		((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->page_per_block )
		#define ZB_NAND_INDEX_PER_BLOCK		((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->index_per_block )
		#define ZB_NAND_BLOCK_INDEX_SHIFT	((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->block_index_shtft )
		#define ZB_NAND_PAGE_INDEX_SHIFT	((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->block_page_shtft )
		#define ZB_NAND_BLOCK_SIZE			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->erasesize      )
		#define ZB_NAND_PAGE_SIZE			((u32   )((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->pagesize       )
		#define ZB_VADDR_NAND_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->vaddr_nand_ctrl)
		#define ZB_VADDR_DMA_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_PADDR)->storage_priv)->vaddr_dma_ctrl )
		#define ZB_VADDR_ECC_CTRL			((void *)((struct zb_priv_nand *)((zbi_t *)ZBI_VADDR)->storage_priv)->vaddr_ecc_ctrl )
		#define	get_storage_priv_offset()	(((zbi_t *)ZBI_PADDR)->storage_priv)
		#define BBT_ADDR					(PHYS_OFFSET + 0x00098000 - 0x00002000)
	#endif // NAL_PRE_LOAD

		#define BBT_ARRAY_CNT				8192		// 고정값 for 1GB storage
		#define ZB_NAND_BBT_PAGE			(ZB_NAND_START_PAGE + ZB_NAND_STORAGE_PAGE_COUNT - ZB_NAND_PAGE_PER_BLOCK )

//#endif // ARG_ZB_STORAGE_OFFSET

//struct zbi_t *zbi_blk;
//struct zb_priv_nand *zb_storage_nand;

#endif  // _ZB_NAND_H_

