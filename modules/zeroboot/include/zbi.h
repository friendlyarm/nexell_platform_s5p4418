/*-----------------------------------------------------------------------------
  파 일 : zbi.h
  설 명 : 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :
		  중요한 내용이므로 함부로 수정하지 말것 !!!!
-------------------------------------------------------------------------------*/

#ifndef _ZBI_H_
#define _ZBI_H_

#ifndef PHYS_OFFSET
#define	PHYS_OFFSET						ARG_NAL_PHYS_OFFSET
#endif
#ifndef PAGE_OFFSET
#define	PAGE_OFFSET						ARG_NAL_PAGE_OFFSET
#endif

// common use driver & nalcode
#define ZBI_PADDR						(PHYS_OFFSET + 0x00008000)				// FIXME PHYS_OFFSET + 0x8000
#define ZBI_VADDR						(PAGE_OFFSET + 0x00008000)				// FIXME 

#ifdef ARG_ZB_STORAGE_OFFSET
#define ZB_STORAGE_OFFSET				ARG_ZB_STORAGE_OFFSET
#define ZB_STORAGE_SIZE					ARG_ZB_STORAGE_SIZE
#endif

#define ZBI_MAGIC						0xFA0000B2
#define ZBI_VERSION						130

#define ZBI_CPU_SAVE_SIZE				512
#define ZBI_CO_SAVE_SIZE				512

//define for nalcode    define for module
#if defined(ARCH_V4) || defined(CONFIG_CPU_ARM920T)
#define ZBI_L2_FAULT_MARK(paddr)		(paddr|0xAAE); 			// FIXME
																// AP3 AP2 AP1 AP0 c b sp
																// 01  01  01  01  1 1 10 = 55E
#elif defined(ARCH_OMAP4) || defined(CONFIG_ARCH_OMAP4)
#define ZBI_L2_FAULT_MARK(paddr)		(paddr|0x45E); 			// FIXME
																// nG  S AP[2]  TEX[2:0] AP[1:0] C B 1 XN
																//  0  1     0       001      01 1 1 1  0 = 0 10 001 01 111 0 = 0x45E
#elif defined(CONFIG_MACH_SMDKV210)
#define ZBI_L2_FAULT_MARK(paddr)		(paddr|0x1E); 			// FIXME
																// nG  S AP[2]  TEX[2:0] AP[1:0] C B 1 XN
																//  0  1     0       001      01 1 1 1  0 = 0 10 001 01 111 0 = 0x45E
#elif defined(CONFIG_ARCH_MX6Q)
#define ZBI_L2_FAULT_MARK(paddr)		(paddr|0x1E); 			// FIXME
																// nG  S APX  TEX[2:0] AP[1:0] P Domain XN C B 1 XN
																//  0  0  0     000      01    0  0000   0 1 1 1  0 = 0x40E

#else
#define ZBI_L2_FAULT_MARK(paddr)		(paddr|0x2E); 			// FIXME
																// nG  S APX  TEX AP C B 1 XN
		                                						//  0  0   0   00 10 1 1 1  0 = 0 000 0010 1110 = 0x2E
#endif

#ifndef PAGE_SIZE
#define PAGE_SIZE						4096
#endif

//#define ZBI_DRAM_MMU_L1_INDEX			(PAGE_OFFSET >> PAGE_SHIFT)				// FIXME 0xC00
#define ZBI_DRAM_MMU_L1_INDEX			(PAGE_OFFSET >> 20 )					// FIXME 0xC00
	
#define	KERNEL_VECTOR_TABLE_VADDR		0xFFFF0000
#define KERNEL_SWAPPER_PG_DIR_VADDR		(PAGE_OFFSET + 0x00004000)

#define ZBI_ATTR_NONE                   0 
#define ZBI_ATTR_MMU_PTE                (1<<0)

/// @{
/// @brief  zbi 구조체
///-----------------------------------------------------------------------------

typedef struct
{
	u32 		dest_paddr;
	u16 		page_offset;
	u16 		attr;
#ifdef DEBUG_4K_CHECKSUM
	u32			checksum;
	u32			rsv;
#endif
} zbi_copy_info_t;

#define ZBI_MAX_SIZE					(128*PAGE_SIZE)			// FIXME
#ifdef CONFIG_ARCH_OMAP4
	#define ZBI_UNIT_SIZE					(15*PAGE_SIZE)			// FIXME
#else
	#ifdef DEBUG_4K_CHECKSUM
	#define ZBI_UNIT_SIZE					(ZBI_MAX_SIZE - PAGE_SIZE)
	#else
	//#define ZBI_UNIT_SIZE					(10*PAGE_SIZE)			// FIXME
	#define ZBI_UNIT_SIZE					(127*PAGE_SIZE)			// FIXME
	#endif
#endif
#define ZBI_HEADER_MAX					(ZBI_CPU_SAVE_SIZE+ZBI_CO_SAVE_SIZE+1024)
#define ZBI_COPY_MAX					((ZBI_UNIT_SIZE-ZBI_HEADER_MAX)/sizeof(zbi_copy_info_t))	
#define	ZBI_BOOTUP_MARKER				0x07987998

typedef struct
{
	union	{
		struct {
			u32 		magic;		// ZBI_MAGIC
			u32  		version;    // ZBI_VERSION
			u8  		cpu_save	[ZBI_CPU_SAVE_SIZE];
			u8   		co_save		[ZBI_CO_SAVE_SIZE];
			
			u32			storage_dev_offset;		// old-name : nand_base_page;
			
			u32			co_save_size;
			u32			cpu_save_size;
			
			u32			copy_count;
			u32       	copy_checksum;
			
			u32			jump_vaddr;
			u32			fault_storage_io_vaddr;	// old-name : fault_nand_io_vaddr

			// 이전의 항목들은 변경하지 말것
			// 부트로더와 연동이 되어 있어 변경시 부트로더를 함께 변경해야 함
			u32			dram_size;
			u32			page_size;
			u32			page_blt;
			u32			bootup_marker;
			u32			bootup_address;

			u32			storage_priv[128];	// for storage private use

			u32			priv[32];			// free use for each manufacture

			u32			section_opt;
			// Added for 64K Extend
			u32         bootup_paddress;    // adderss
			u32         bootup_size;

			// A9 MPCore base address
			u32			armmp_base;

			// cpu resume address
			u32			cpu_idmap;
			u32			phys_cpu_ptr;
			u32			cpu_resume;
			u32			cpu_offset;
			u32			cpu_nocache;
			u32			cpu_nr;

			// l2x0 support
			u32			zb_l2x0_base;
			u32			zb_l2x0_way_mask;
			u32			zb_l2x0_size;

			// nalcode used
			u32			nal_reserv[16];

		};
		
		u8	align_2048[2048];
	};
	
//	u8	arm_smp[4096+2048];

	zbi_copy_info_t		copy_data	[ZBI_COPY_MAX];
} zbi_t;
/// @}

/// @{
/// @brief  zbi_ 함수정의
///-----------------------------------------------------------------------------
extern void 		zbi_init							( void );
extern void 		zbi_storage_ready_done				( void );
extern void 		zbi_free							( void );
                                                		
extern int 			zbi_append_bootcopy_data			( u32 dest_paddr, u32 attr );

extern void     	zbi_build_block_page				( void );
extern void     	zbi_write_info_of_bootcopy_to_zbi	( void );
extern void			zbi_build_block_page_for_m2n_data	( void );
                                                		
extern int 			zbi_save_cpu						( u8 *cpu_data , u32 size );
extern int 			zbi_jump_vaddr						( u32 vaddr );
extern void 		zbi_set_zblk_io						( u32 vaddr );
extern void			zb_add_page_value					( void );

extern void 		zbi_save_data_for_zbi_data			( void );
extern void 		zbi_save_for_m2n_data				( void );
                                                		
extern void 		zbi_save_header						( void );
extern void 		zbi_display							( void );

extern void zbi_set_section_option(u32 opt);
extern u32 zbi_get_section_option(void);
extern zbi_t *get_zbi_base(void);

extern u32 (*nalcode_func)( u32 function_offset, u32 arg1, u32 arg2, u32 arg3 );

void zbi_info_dump(void);
void zb_save_cpu_resume(void);
void zbi_change_pgd(unsigned long);
/// @}

#endif  // _ZBI_H_

