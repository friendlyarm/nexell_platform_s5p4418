
#ifndef PAGE_SIZE
#define PAGE_SIZE				4096
#endif
#define	PAGE_MASK           	(0xFFFFF000)
#define u32 unsigned long
#define u8  unsigned char

typedef struct
{
	u32 paddr;	// pysical address
	u32 info;	// ATHNNNNN : 
				// A		: Area Type, 
				//  T		: BOOT COPY / SAVE NAND (upper 2bit)
				//   H		: L2 HW POSITION
				//    NNNNN	: NAND PAGE OFFSET
#ifdef DEBUG_4K_CHECKSUM
	u32	sum;
	u32 rsv;
#endif
} m2n_item_t;

#define 		M2N_AREA_UNKONW						0x00000000
#define 		M2N_AREA_PROTECT_BOOTLOADER			0x10000000
#define 		M2N_AREA_MMU_L1						0x20000000
#define 		M2N_AREA_MMU_L2						0x30000000
#define 		M2N_AREA_NALCODE					0x40000000
#define 		M2N_AREA_FAULT_PAGE_INFO			0x50000000
#define 		M2N_AREA_RETURN  					0x60000000
#define 		M2N_AREA_VECTOR   					0x70000000
#define 		M2N_AREA_WORK   					0x80000000
#define 		M2N_AREA_USED   					0x90000000
#define 		M2N_AREA_THREAD_INFO				0xA0000000
#define 		M2N_AREA_VIDEO						0xB0000000
#define			M2N_AREA_MASK						0xF0000000

#define 		M2N_BOOT_COPY_MASK					0x08000000
#define 		M2N_SAVE_NAND_MASK					0x04000000

#define			M2N_L2_HW1							0x00100000
#define			M2N_L2_HW2							0x00200000
#define			M2N_L2_HW3							0x00400000
#define			M2N_L2_HW4							0x00800000
#define			M2N_L2_HW_MASK						0x00F00000
#define			M2N_L2_HW_SHIFT						20

#define			M2N_NAND_OFFSET_MASK				0x000FFFFF

#define			M2N_AREA(x)							((x)&M2N_AREA_MASK)
#define			M2N_BOOT_COPY(x)					((x)==0? 0:M2N_BOOT_COPY_MASK)
#define			M2N_SAVE_NAND(x)					((x)==0? 0:M2N_SAVE_NAND_MASK)
#define			M2N_NAND_OFFSET(x)					((x)&M2N_NAND_OFFSET_MASK)

#define			M2N_INFO(area,boot_copy,save_nand,nand_page)(	\
					 M2N_AREA(area)								\
					|BOOT_COPY(boot_copy)						\
					|SAVE_NAND(save_nand)						\
					|M2N_NAND_OFFSET(nand_page)					\
					)

#define			M2N_SUCCESS							1
#define			M2N_FAIL							0

extern void 		m2n_open			( void 						);
extern void 		m2n_close			( void 						);
                                                                    	
extern void 		disp_m2n_list		( void 						);
                                                                    	
extern void 		m2n_set_count		( int used_count 			);
extern int 			m2n_get_size		( void 						);
extern void 		m2n_set_mem			( void *mem, u32 phys		);
                                                                    	
extern void 		m2n_clean			( void 						);
extern void 		m2n_add				( u32 paddr, u32 info 		);
extern m2n_item_t *	m2n_find			( u32 paddr 				);
                                                                    	
extern int			m2n_set_area		( u32 paddr	, u32 area		);
extern u32			m2n_get_area		( u32 paddr	          		);
                                                                    	
extern int			m2n_set_boot_copy	( u32 paddr					);
extern int			m2n_clear_boot_copy	( u32 paddr					);
extern int			m2n_is_boot_copy	( u32 paddr					);
                                                                    	
extern int			m2n_set_save_nand	( u32 paddr					);
extern int			m2n_clear_save_nand	( u32 paddr					);
extern int			m2n_is_save_nand	( u32 paddr					);

extern u32			m2n_set_nand_page	( u32 paddr	, u32 nand_page	);
extern u32			m2n_get_nand_page	( u32 paddr	          		);


// extern void zba_set_mark_protect_bootloader_manage_area		( u32 paddr, u32 size );  ZBI 관리로 옮겨야 한다.
// extern void zba_set_mark_used_memory             			( u32 paddr, u32 size );  필요 없다.
// extern void zba_set_mark_boot_copy_area_of_mmu_l1			( u32 paddr, u32 size );
// extern void zba_set_mark_boot_copy_area_of_mmu_l2			( u32 paddr, u32 size );
// extern void zba_set_mark_boot_copy_area_of_nalcode			( u32 paddr, u32 size );
// extern void zba_set_mark_boot_copy_area_of_fault_page_info	( u32 paddr, u32 size );
// extern void zba_set_mark_boot_copy_area_of_return			( u32 paddr, u32 size );
// extern void zba_set_mark_boot_copy_area_of_vector			( u32 paddr, u32 size );
// extern void zba_set_mark_area_of_work						( u32 vaddr, u32 size );
// extern void zba_set_mark_boot_copy_area_of_thread_info		( u32 paddr, u32 size );

