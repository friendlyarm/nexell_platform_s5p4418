//------------------------------------------------------------------------------
// 화일명 : zeroboot.c
// 설  명 : ezBoot의 제로부트 부팅을 시작한다.
// 
// 작성자 : 유영창 에프에이리눅스(주) frog@falinux.com
// 작성일 : 2011년 ...
// 저작권 : 에프에이리눅스(주)
// 주  의 : 
//------------------------------------------------------------------------------

//******************************************************************************
//
// 헤더 정의
//
//******************************************************************************
#include <typedef.h>

#include <zb_core_reg.h>
#include <uart.h>
#include <zbi.h>
#include <nal_m2n.h>

//******************************************************************************
// 참조 하는 것들
// ZBI_CPU_SAVE_SIZE	- zbi_t
// ZBI_CO_SAVE_SIZE		- zbi_t
// zbi_copy_info_t 		- zbi_t
// zbi_t				- zbi_t
// ZB_STORAGE_OFFSET	- zbi 헤더의 첫 페이지를 어디에서 읽을 것인가
// ZBI_PADDR			- zbi 헤더를 물리번지 어디에 위치시킬 것인가
// ZBI_VADDR			- 복귀시 인자
// PAGE_SIZE			- page size
//******************************************************************************

#define printf(args...)	do { } while(0)

extern void zeroboot_read_4k_page( int nand_page, unsigned char *mem_page );
extern void nalcode_storage_first_call(void);

extern void zb_init_dma_list(void);
extern int zb_add_dma_list(unsigned int blkidx, unsigned int phys);
extern void zb_request_dma_transfer(void);
#define	zeroboot_read_4k_page( n, m)	nalcode_stroage_read_4k_page(n, m, (void *)0x0, m)

zbi_t	*zbi = (zbi_t *) ZBI_PADDR;

struct nal_priv {
	u32 m2n_datas;
	u32 m2n_count;
};

//******************************************************************************
//
// 함수 정의
//
//******************************************************************************
//#define	USE_SELF_UART_CODE
#ifdef USE_SELF_UART_CODE
static void put_raw_char( char ch )
{
	while(!(UTRSTAT0 & UTRSTAT_TX_EMPTY));
	UTXH0 = ch;									
}
void put_raw_hex( u32 data )
{
	data = data & 0xF;
	if     ( data < 0xA ) data = data + '0';
	else				  data = data - 0xA + 'A';	
	
	put_raw_char( data );	
}
void put_raw_hex32( u32 data )
{
	int lp;
	
	for( lp = 7; lp >= 0; lp-- )
		put_raw_hex( data >> (4*lp) );
}
void put_raw_str( char *str )
{
	while(*str) 
	{
		if( *str == '\n' ) put_raw_char( '\r' );	
		put_raw_char( *str++ );	
	}	
}
#else
#define	put_raw_char	uart_putc
#define put_raw_hex		uart_put_hex_nibble
#define put_raw_hex32	uart_put_hex
#define	put_raw_str		uart_puts
#endif

void zb_read_zbi_header( void )
{
	int	lp;
	u32   nand;
	unsigned char  *data;
	u32	zbi_used;
	
	nand = 0x0;
	data = (unsigned char *) zbi;
	
	// Read zbi first page
//	put_raw_str( "> storage start page = "); put_raw_hex32( ZB_STORAGE_OFFSET/2048 );	//put_raw_str( "\n");
	put_raw_str( "> storage start page = "); put_raw_hex32( zbi->storage_priv[0]>>11 );	//put_raw_str( "\n");
	zb_init_dma_list();
	//zeroboot_read_4k_page( nand, data );
	zb_add_dma_list(nand, (unsigned int)data);
	zb_request_dma_transfer();

	nand++;
	data += PAGE_SIZE;

	// calculate how page used
	zbi_used = (u32)(&zbi->copy_data[zbi->copy_count+1]) - (u32)zbi;
	zbi_used = (zbi_used+(PAGE_SIZE-1))/PAGE_SIZE;
	if(zbi_used>0) zbi_used--;

	put_raw_str( "> zbi_used           = "); put_raw_hex32( zbi_used 			   );	//put_raw_str( "\n");
	zb_init_dma_list();
	for( lp = 0; lp < zbi_used; lp++ )
	{
		//zeroboot_read_4k_page( nand, data );
		zb_add_dma_list(nand, (unsigned int)data);
		nand++;
		data += PAGE_SIZE;
	}	
	zb_request_dma_transfer();
	
	put_raw_str( "> magic              = "); put_raw_hex32( zbi->magic           );		//put_raw_str( "\n");
	put_raw_str( "> version            = "); put_raw_hex32( zbi->version         );		//put_raw_str( "\n");
	put_raw_str( "> copy_count         = "); put_raw_hex32( zbi->copy_count      );		//put_raw_str( "\n");
	put_raw_str( "> copy_checksum      = "); put_raw_hex32( zbi->copy_checksum   );		//put_raw_str( "\n");
	put_raw_str( "> jump               = "); put_raw_hex32( zbi->jump_vaddr      );		//put_raw_str( "\n");
	put_raw_str( "> bootup_address     = "); put_raw_hex32( zbi->bootup_address  );		//put_raw_str( "\n");
	put_raw_str( "> phys_cpu_ptr   	   = "); put_raw_hex32( zbi->phys_cpu_ptr  );		//put_raw_str( "\n");
	put_raw_str( "> idmap_pgd     	   = "); put_raw_hex32( zbi->cpu_idmap  );		//put_raw_str( "\n");

}

//#define	MAKE_SUM
void zb_copy_data1( void )
{
	zbi_copy_info_t	*copy_data;
	u32 			index;
	u32 			nand_page;
	volatile u8 	*page_buf;
	u32				last_page;
	u32				inx_count;

#ifdef MAKE_SUM
	u32				checksum_lp;
	u32 			check_sum;
	
	check_sum = 0;
#endif

    copy_data = zbi->copy_data;
    
    for( index = 0; index < zbi->copy_count; index++ )
    {
//		put_raw_str("> Bootcopy " );
//		put_raw_str( "I = "); put_raw_hex32( index );
//		put_raw_str( "P = "); put_raw_hex32( copy_data->dest_paddr );
//		put_raw_str( ", N = "); put_raw_hex32( copy_data->page_offset );
//		put_raw_str( "\n" );

		// FIXME
		// 4K 단위 사이즈로 인덱스에 해당하는 페이지 번호를 구한다.
    	nand_page = copy_data->page_offset;	
		page_buf  = (u8 *) copy_data->dest_paddr;

		// skip self area
		//if( (u32)page_buf != (u32)(PHYS_OFFSET+0x0008D000) ) {
		if( ((u32)page_buf < (u32)zbi->bootup_paddress) || ((u32)page_buf >= ((u32)zbi->bootup_paddress + (u32)zbi->bootup_size + (8 * 1024)))) {
			zeroboot_read_4k_page( nand_page, (unsigned char * ) page_buf );

#ifdef MAKE_SUM
			for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ )
			{
				check_sum += page_buf[checksum_lp];
			}
#endif
		} 
		copy_data++;
    }
//	if (inx_count)
//		zb_request_dma_transfer();
    
#ifdef MAKE_SUM
    put_raw_str( "> CHECKSUM : "); put_raw_hex32( check_sum ); put_raw_str( "\n");
#endif
    
}

void zb_copy_data( void )
{
	zbi_copy_info_t	*copy_data;
	u32 			index;
	u32 			nand_page;
	volatile u8 	*page_buf;
	u32				last_page;
	u32				inx_count;

#ifdef MAKE_SUM
	u32				checksum_lp;
	u32 			check_sum;
	
	check_sum = 0;
#endif

    copy_data = zbi->copy_data;
    
	zb_init_dma_list();
	last_page = 0;
	inx_count = 0;
    for( index = 0; index < zbi->copy_count; index++ )
    {
//		put_raw_str("> Bootcopy " );
//		put_raw_str( "I = "); put_raw_hex32( index );
//		put_raw_str( "P = "); put_raw_hex32( copy_data->dest_paddr );
//		put_raw_str( ", N = "); put_raw_hex32( copy_data->page_offset );
//		put_raw_str( "\n" );

		// FIXME
		// 4K 단위 사이즈로 인덱스에 해당하는 페이지 번호를 구한다.
    	nand_page = copy_data->page_offset;	
		page_buf  = (u8 *) copy_data->dest_paddr;

		// skip self area
		//if( (u32)page_buf != (u32)(PHYS_OFFSET+0x0008D000) ) {
		if( ((u32)page_buf < (u32)zbi->bootup_paddress) || ((u32)page_buf >= ((u32)zbi->bootup_paddress + (u32)zbi->bootup_size + (8 * 1024)))) {
//			zeroboot_read_4k_page( nand_page, (unsigned char * ) page_buf );

#define	MAX_DMA_COUNT	256
		// continue check
			if (last_page && (last_page != nand_page) ) {
//				put_raw_str( "last_page = "); put_raw_hex32( last_page );
//				put_raw_str( "nand_page = "); put_raw_hex32( nand_page );
//				put_raw_str( "\n" );
				zb_request_dma_transfer();
				zb_init_dma_list();
				inx_count = 0;
			}

			zb_add_dma_list(nand_page, (unsigned int)page_buf);

		// check max count
			if (inx_count < MAX_DMA_COUNT)
				inx_count++;

			if (inx_count >= MAX_DMA_COUNT) {
				zb_request_dma_transfer();
				zb_init_dma_list();
				inx_count = 0;
				last_page = 0;
			} else
				last_page = nand_page + 1;

#ifdef MAKE_SUM
			for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ )
			{
				check_sum += page_buf[checksum_lp];
			}
#endif
		} 
		copy_data++;
    }
	if (inx_count)
		zb_request_dma_transfer();
    
#ifdef MAKE_SUM
    put_raw_str( "> CHECKSUM : "); put_raw_hex32( check_sum ); put_raw_str( "\n");
#endif
    
}


//struct nal_priv {
//	m2n_item_t	*m2n_datas;
//	int			m2n_count;
//};
u32 __m2n_set_nand_page( m2n_item_t *item, u32 blk_index)
{
	if( item == NULL )
	{
		return 0;
	}
	
	item->info 	= ( item->info & ~M2N_NAND_OFFSET_MASK ) | M2N_NAND_OFFSET(blk_index);
	
	return 0;
}

#define SZ_1M                           0x00100000
#define SZ_2M                           0x00200000
#define SZ_4M                           0x00400000
#define SZ_8M                           0x00800000
#define SZ_16M                          0x01000000
#define SZ_32M                          0x02000000
#define SZ_64M                          0x04000000
#define SZ_128M                         0x08000000
#define SZ_256M                         0x10000000
#define SZ_512M                         0x20000000
#define SZ_1G                           0x40000000
void zb_preload_all(void)
{
	zbi_t *zbi = (zbi_t *)ZBI_PADDR;
	struct nal_priv *np = (struct nal_priv *)zbi->nal_reserv;
	m2n_item_t *item;
	int count, used, blk_index;
#ifdef DEBUG_MEM_STORAGE
//#define	ZB_MEM_BLK_SIZE		(SZ_256M + SZ_128M + SZ_32M)
#define	ZB_MEM_BLK_SIZE		(SZ_512M)
#define	MEM_STORAGE_BASE	(SZ_1G - ZB_MEM_BLK_SIZE)
	unsigned long base = (PHYS_OFFSET + MEM_STORAGE_BASE);
#endif

#ifdef DEBUG_4K_CHECKSUM
	u8 *buf;
	u32 lp;
	u32 checksum_lp;
	u32 local_checksum;
#endif

//    put_raw_str( "> m2n_datas : "); put_raw_hex32( np->m2n_datas ); put_raw_str( "\n");
//    put_raw_str( "> m2n_count : "); put_raw_hex32( np->m2n_count ); put_raw_str( "\n");

	used = 0;
	item = (m2n_item_t *)np->m2n_datas;
	for(count = 0; count < np->m2n_count; count++, item++) {
		
		if ((count%50)==0) {
//			put_raw_hex32(count); 
//			put_raw_hex32(np->m2n_count); 
//			put_raw_str( "\n");
		}
		if (!item->paddr)
			continue;

		used++;

		blk_index = M2N_NAND_OFFSET(item->info);
		if (!blk_index)
			continue;

//    	put_raw_str( "> blk_index : "); put_raw_hex32( blk_index );
//    	put_raw_str( "> paddr     : "); put_raw_hex32( item->paddr );
//		put_raw_str( "\n");
	
#ifdef DEBUG_MEM_STORAGE
		zeroboot_read_4k_page(blk_index, (unsigned char * )(base + 0x1000 * blk_index));
#else
		zeroboot_read_4k_page(blk_index, (unsigned char * )item->paddr);
		__m2n_set_nand_page(item, M2N_AREA_UNKONW);
#endif

#ifdef DEBUG_4K_CHECKSUM
		local_checksum = 0 ; 
#ifdef DEBUG_MEM_STORAGE
		buf = (unsigned char * )(base + 0x1000 * blk_index);
#else
		buf = (u8 *)item->paddr;
#endif
		for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ ) {
			local_checksum += buf[checksum_lp];
		}

		if (item->sum != local_checksum) {
#ifndef DEBUG_MEM_STORAGE
			put_raw_str("================================\n");
			put_raw_str("Checksum Fail\n");
			put_raw_str("calculated checksum is 0x"); put_raw_hex32(local_checksum);
			put_raw_str("paddr 0x"); put_raw_hex32(item->paddr);
			put_raw_str("sum   0x"); put_raw_hex32(item->sum);

			put_raw_str("index at 0x"); put_raw_hex32(blk_index);
			put_raw_str("nalcode halt!\n");
			put_raw_str("================================");
#endif
			//while(1);
		} else {
	//		put_raw_str("================================\n");
	//		put_raw_str("calculated checksum is 0x"); put_raw_hex32(local_checksum);
	//		put_raw_str("paddr 0x"); put_raw_hex32(item->paddr);
	//		put_raw_str("sum   0x"); put_raw_hex32(item->sum);
	//		put_raw_str("================================\n");
		}
#endif

	}

    put_raw_str( "> used : "); put_raw_hex32( used ); put_raw_str( "\n");
}

/*------------------------------------------------------------------------------
  @brief   코프로세서 상태를 복구한다.
  @remark  
*///----------------------------------------------------------------------------
u32 coprocessor_restore( u8 *co_load_buffer, u32 jump_vaddr, u32 dummy2, u32 *cpu_load_buffer );
//                           r0                  r1              r2          r3
#if	defined(ARCH_V4)
asm("   	       					                \n\
.align  5						                    \n\
.text                                               \n\
.global coprocessor_restore                         \n\
coprocessor_restore:                                \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
    str r2, save_cr                                 \n\
 @	mcr p15, 0, r2, c1,  c0,  0		@ CR            \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
    mcr p15, 0, r2, c3 , c0 , 0     @ DACR			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c5 , c0 , 0     @ DFSR			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c5 , c0 , 1     @ IFSR			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c6 , c0 , 0     @ FAR			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c9 , c0 , 0     @ D_CLR			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c9 , c0 , 1     @ I_CLR			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c9 , c1 , 0     @ D_TCMRR		\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c9 , c1 , 1     @ I_TCMRR		\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c10, c0 , 0     @ TLBLR			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c13, c0 , 0     @ FCSE PID Reg	\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c13, c0 , 1     @ CID			\n\
    ldr r2, [r0], #4                               	\n\
    mcr p15, 0, r2, c2 , c0 , 0     @ TTBR			\n\
                                                    \n\
.align	5                                           \n\
	mov	r0, r3                                      \n\
    ldr r2, save_cr                                 \n\
	\n\
	mov	r0, r2 \n\
	mov pc, lr \n\
	\n\
    mcr p15, 0, r2, c1, c0 ,0       @  CR           \n\
	mrc	p15, 0, r2, c0, c0, 0		@ read id reg   \n\
	mov	r2, r2                                      \n\
	mov	pc, r1                                      \n\
                                                    \n\
save_cr: .word	0x0		@ CR                        \n\
                                                    \n\
");
#endif

#if	defined(ARCH_V6) || defined (ARCH_V7)
asm("   	       					                \n\
.align  5						                    \n\
.text                                               \n\
.global coprocessor_restore                         \n\
coprocessor_restore:                                \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
    str r2, save_cr                                 \n\
 @	mcr p15, 0, r2, c1,  c0,  0		@ CR            \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c1,  c0,  1		@ Auxiliary     \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c1,  c0,  2		@ CACR          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c1,  c1,  1 	@ SDER          \n\
	ldr	r2, [r0], #4                                \n\
	mcr p15, 0, r2, c1,  c1,  3 	@ VCR           \n\
	ldr	r2, [r0], #4                                \n\
	mcr p15, 0, r2, c2,  c0,  0		@ TTB_0R        \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c2,  c0,  1		@ TTB_1R        \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c2,  c0,  2		@ TTBCR         \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c3,  c0,  0		@ DACR          \n\
	ldr	r2, [r0], #4                             \n\
	mcr p15, 0, r2, c7,  c4,  0		@ PAR           \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c10, c0,  0		@ D_TLBLR       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c10, c2,  0		@ PRRR          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c10, c2,  1		@ NRRR          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c12, c0,  0		@ SNSVBAR       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c12, c0,  1 	@ MVBAR        	\n\
	ldr	r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  0		@ FCSE          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  1		@ CID           \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  2		@ URWTPID       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  3		@ UROTPID       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  4		@ POTPID        \n\
   	ldr r2, [r0], #4                                \n\
@	mcr p15, 0, r2, c15, c7,  2 	@ MTLBAR        \n\
	ldr	r2, [r0], #4                                \n\
                                                    \n\
.align	5                                           \n\
                                                    \n\
	mov	r0, r3                                      \n\
    ldr r2, save_cr                                 \n\
	\n\
	mov	r0, r2 \n\
	mov pc, lr \n\
	\n\
    mcr p15, 0, r2, c1 , c0 , 0     @  CR           \n\
	mrc	p15, 0, r2, c0, c0, 0		@ read id reg   \n\
	mov	r2, r2                                      \n\
	mov	pc, r1                                      \n\
                                                    \n\
                                                    \n\
save_cr: .word	0x0		@ CR                        \n\
                                                    \n\
                                                    \n\
");


#if 0
asm("   	       					                \n\
.align  5						                    \n\
.text                                               \n\
.global coprocessor_restore                         \n\
coprocessor_restore:                                \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
    str r2, save_cr                                 \n\
 @	mcr p15, 0, r2, c1,  c0,  0		@ CR            \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c1,  c0,  1		@ Auxiliary     \n\
                                                    \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c1,  c0,  2		@ CACR          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c2,  c0,  0		@ TTB_0R        \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c2,  c0,  1		@ TTB_1R        \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c2,  c0,  2		@ TTBCR         \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c3,  c0,  0		@ DACR          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c5,  c0,  0		@ D_FSR         \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c5,  c0,  1		@ I_FSR         \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c6,  c0,  0		@ D_FAR         \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c6,  c0,  2		@ I_FAR         \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c7,  c4,  0		@ PAR           \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c10, c0,  0		@ D_TLBLR       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c10, c2,  0		@ PRRR          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c10, c2,  1		@ NRRR          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c11, c1,  0		@ PLEUAR        \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c11, c2,  0		@ PLECNR        \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c11, c4,  0		@ PLECR         \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c11, c5,  0		@ PLEISAR       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c11, c7,  0		@ PLEIEAR       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c11, c15, 0		@ PLECIDR       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c12, c0,  0		@ SNSVBAR       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  0		@ FCSE          \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  1		@ CID           \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  2		@ URWTPID       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  3		@ UROTPID       \n\
    ldr r2, [r0], #4                                \n\
	mcr p15, 0, r2, c13, c0,  4		@ POTPID        \n\
   	ldr r2, [r0], #4                                \n\
                                                    \n\
.align	5                                           \n\
                                                    \n\
	mov	r0, r3                                      \n\
    ldr r2, save_cr                                 \n\
	\n\
	mov	r0, r2 \n\
	mov pc, lr \n\
	\n\
    mcr p15, 0, r2, c1 , c0 , 0     @  CR           \n\
	mrc	p15, 0, r2, c0, c0, 0		@ read id reg   \n\
	mov	r2, r2                                      \n\
	mov	pc, r1                                      \n\
                                                    \n\
                                                    \n\
save_cr: .word	0x0		@ CR                        \n\
                                                    \n\
                                                    \n\
");
#endif
#endif

extern u32 mmu_restore( u8 *co_load_buffer, u32 jump_vaddr, u32 dummy2, u32 *cpu_load_buffer );

u32 coprocessor_restore_new(struct core_reg *coreg, u32 jump_vaddr, u32 dummy2, u32 *cpu_load_buffer );
u32 coprocessor_restore_new(struct core_reg *coreg, u32 jump_vaddr, u32 dummy2, u32 *cpu_load_buffer )
{
	unsigned int value;

	/* CR0 */
	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r"(coreg->c0.c0_0_2));	// cssr

	/* CR1 */
//	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(coreg->c1.c0_0));
	asm volatile ("mcr p15, 0, %0, c1, c0, 1" : : "r"(coreg->c1.c0_1));
	asm volatile ("mcr p15, 0, %0, c1, c0, 2" : : "r"(coreg->c1.c0_2));
	asm volatile ("mcr p15, 0, %0, c1, c1, 0" : : "r"(coreg->c1.c1_0));
	asm volatile ("mcr p15, 0, %0, c1, c1, 1" : : "r"(coreg->c1.c1_1));
	asm volatile ("mrc p15, 0, %0, c1 ,c1, 2" : : "r"(coreg->c1.c1_2));		// nsacr
	asm volatile ("mcr p15, 0, %0, c1, c1, 3" : : "r"(coreg->c1.c1_3));

	/* CR2 */
	asm volatile ("mcr p15, 0, %0, c2, c0, 0" : : "r"(coreg->c2.c0_0));
	asm volatile ("mcr p15, 0, %0, c2, c0, 1" : : "r"(coreg->c2.c0_1));
	asm volatile ("mcr p15, 0, %0, c2, c0, 2" : : "r"(coreg->c2.c0_2));

	/* CR3 */
	asm volatile ("mcr p15, 0, %0, c3, c0, 0" : : "r"(coreg->c3.c0_0));

	/* CR5 */
	asm volatile ("mcr p15, 0, %0, c5, c0, 0" : : "r"(coreg->c5.c0_0));
	asm volatile ("mcr p15, 0, %0, c5, c0, 1" : : "r"(coreg->c5.c0_1));
	asm volatile ("mcr p15, 0, %0, c5, c1, 0" : : "r"(coreg->c5.c1_0));
	asm volatile ("mcr p15, 0, %0, c5, c1, 1" : : "r"(coreg->c5.c1_1));

	/* CR6 */
	asm volatile ("mcr p15, 0, %0, c6, c0, 0" : : "r"(coreg->c6.c0_0));
	asm volatile ("mcr p15, 0, %0, c6, c0, 2" : : "r"(coreg->c6.c0_2));

	/* CR7 */
	asm volatile ("mcr p15, 0, %0, c7, c4, 0" : : "r"(coreg->c7.c4_0));

#ifdef CONFIG_HW_PERF_EVENTS
	/* CR9 */
	asm volatile ("mcr p15, 0, %0, c9, c12, 0" : : "r"(coreg->c9.c12_0));
	asm volatile ("mcr p15, 0, %0, c9, c12, 1" : : "r"(coreg->c9.c12_1));
	asm volatile ("mcr p15, 0, %0, c9, c12, 2" : : "r"(coreg->c9.c12_2));
	asm volatile ("mcr p15, 0, %0, c9, c12, 3" : : "r"(coreg->c9.c12_3));
	asm volatile ("mcr p15, 0, %0, c9, c12, 5" : : "r"(coreg->c9.c12_5));
	asm volatile ("mcr p15, 0, %0, c9, c13, 0" : : "r"(coreg->c9.c13_0));
	asm volatile ("mcr p15, 0, %0, c9, c13, 1" : : "r"(coreg->c9.c13_1));
	asm volatile ("mcr p15, 0, %0, c9, c13, 2" : : "r"(coreg->c9.c13_2));
	asm volatile ("mcr p15, 0, %0, c9, c14, 0" : : "r"(coreg->c9.c14_0));
	asm volatile ("mcr p15, 0, %0, c9, c14, 1" : : "r"(coreg->c9.c14_1));
	asm volatile ("mcr p15, 0, %0, c9, c14, 2" : : "r"(coreg->c9.c14_2));
#endif

	/* CR10 */
	asm volatile ("mcr p15, 0, %0, c10, c0, 0" : : "r"(coreg->c10.c0_0));
	asm volatile ("mcr p15, 0, %0, c10, c2, 0" : : "r"(coreg->c10.c2_0));
	asm volatile ("mcr p15, 0, %0, c10, c2, 1" : : "r"(coreg->c10.c2_1));
	/* CR11 */

	/* CR12 */
	asm volatile ("mcr p15, 0, %0, c12, c0, 0" : : "r"(coreg->c12.c0_0));
	asm volatile ("mcr p15, 0, %0, c12, c0, 1" : : "r"(coreg->c12.c0_1));
	asm volatile ("mcr p15, 0, %0, c12, c1, 1" : : "r"(coreg->c12.c1_1));

	/* CR13 */
	asm volatile ("mcr p15, 0, %0, c13, c0, 0" : : "r"(coreg->c13.c0_0));
	asm volatile ("mcr p15, 0, %0, c13, c0, 1" : : "r"(coreg->c13.c0_1));
	asm volatile ("mcr p15, 0, %0, c13, c0, 2" : : "r"(coreg->c13.c0_2));
	asm volatile ("mcr p15, 0, %0, c13, c0, 3" : : "r"(coreg->c13.c0_3));
	asm volatile ("mcr p15, 0, %0, c13, c0, 4" : : "r"(coreg->c13.c0_4));

	/* CR15 */
	asm volatile ("mcr p15, 5, %0, c15, c7, 2" : : "r"(coreg->c15.c7_2_5));

	return coreg->c1.c0_0;
}

/*------------------------------------------------------------------------------
  @brief   모든 TLB 캐쉬를 비운다. 
  @remark  
*///----------------------------------------------------------------------------
#if	defined(ARCH_V6) || defined (ARCH_V4)
void all_tbl_clean( void );
asm("   	            					                            			\n\
											                            			\n\
.align  5									                            			\n\
.text                                                                   			\n\
.global all_tbl_clean                                                   			\n\
all_tbl_clean:                                                          			\n\
				mov    r0, #0                                                       \n\
			    mcr    p15, 0, r0, c7, c14, 0        @ clean+invalidate D cache     \n\
			    mcr    p15, 0, r0, c7, c5, 0         @ invalidate I cache           \n\
			    mcr    p15, 0, r0, c7, c15, 0        @ clean+invalidate cache       \n\
			    mcr    p15, 0, r0, c7, c10, 4        @ drain write buffer           \n\
			    mcr    p15, 0, r0, c8, c7, 0         @ invalidate I + D TLBs        \n\
																					\n\
				mov	pc,lr                   										\n\
");
#endif

// additional irq, fiq off
u32 force_mmu_off(u32 dummy);
asm("   	            					\n\
											\n\
.align  5									\n\
.text                                       \n\
.global force_mmu_off                       \n\
force_mmu_off:                              \n\
				mrs r0, cpsr				\n\
				orr r0, r0, #1<<6			\n\
				orr r0, r0, #1<<7			\n\
				msr cpsr, r0				\n\
				mrs r0, cpsr				\n\
				mrc p15, 0, r0, c1, c0, 0	\n\
				bic r0, r0, #1				\n\
				mcr p15, 0, r0, c1, c0, 0	\n\
				isb							\n\
				dsb							\n\
				mov	pc,lr                   \n\
");

extern unsigned long _bss_start;
extern unsigned long _bss_end;
void zeroboot(int opt)
{
//	u32 (*mmu_restore)( u8 *co_load_buffer, u32 jump_vaddr, u32 dummy2, u32 *cpu_load_buffer );
	u32 cr=0;
//	printf( "Zeroboot start\n" );
	u32 *ptr;
	u32 idmap_pgd, sp;
	u32 *start, *end, i;
	u32 *clr;
	
	// clear all memory area
	// from memory start to end
	// we will clear all data to 0.
	// except nalcode area, (PAGE_OFFSET + 0x8000) - (PAGE_OFFSET + 0xr98000)

	put_raw_hex32(_bss_start);
	put_raw_hex32(_bss_end);
	// bss clear
//	start = (u32 *)_bss_start;
//	end = (u32 *)_bss_end;
//	while (start != end) {
//		*start++ = 0;
//	};

	put_raw_str( "> zero bootup start\n"); 
#if 0
	start = 0x40000000;
	end   = 0x80000000 - 4;

	force_mmu_off(0);

	while (start != end) {
		if (!(end & 0xffff))
			put_raw_hex32(end);
		if ((end < 0x40008000) || (end >= 0x40098000)) {
			clr = (u32 *)end;
			*clr = 0;
		}

		end = end -4;
	};
#endif

	put_raw_str( "> mem clear end\n"); 
	put_raw_str(__DATE__);
	put_raw_str( " - ");
	put_raw_str(__TIME__);
	zbi = (zbi_t *) ZBI_PADDR;

	nalcode_storage_first_call();
	zb_read_zbi_header();
	zb_copy_data();

	if (opt)
		put_raw_str( "> preload all at boot mode\n"); 
	else
		put_raw_str( "> preload normal mode\n"); 

	if (opt)
		zb_preload_all();

//	put_raw_str( "ZeroBoot Copy End\n" );
	
#if 0
	cr = coprocessor_restore( zbi->co_save, 
	                     zbi->jump_vaddr, 
	                     0, 
	                     (zbi->cpu_save - ZBI_PADDR)+ZBI_VADDR );
#else	
// 5p4118 do nothing
//	cr = coprocessor_restore_new( zbi->co_save, 
//	                     zbi->jump_vaddr, 
//	                     0, 
//	                     (zbi->cpu_save - ZBI_PADDR)+ZBI_VADDR );
#endif
	
	put_raw_str( "\n" );
	put_raw_hex32(zbi->co_save);
	put_raw_hex32(zbi->jump_vaddr);
	put_raw_hex32((zbi->cpu_save - ZBI_PADDR)+ZBI_VADDR );
	put_raw_hex32(mmu_restore);
	put_raw_hex32(cr);
	put_raw_str( "\n" );

//	put_raw_str( "> mmu_restore \n");
	/* FIXME  disable cache & buffer forced... cr & 0xfffffff3 */
	                     //cr & (~(1<<2)) & (~(1<<12)), 
	                     //cr,

// 5p4118 do nothing
//	mmu_restore( zbi->co_save, 
//	                     zbi->jump_vaddr, 
//	                     cr,
//	                     (zbi->cpu_save - ZBI_PADDR)+ZBI_VADDR );

#if 0
 21     *save_ptr = virt_to_phys(ptr);
 22 
 23     /* This must correspond to the LDM in cpu_resume() assembly */
 24     *ptr++ = virt_to_phys(idmap_pgd);
 25     *ptr++ = sp;
 26     *ptr++ = virt_to_phys(cpu_do_resume);
 27 
 28 #ifdef CONFIG_FALINUX_ZEROBOOT_NAL
 29     zb_ptr = *save_ptr;
 30 #endif
#endif
	void (*jumpkernel)(u32, u32) = 0;

	force_mmu_off(0);

	ptr = (u32*)zbi->phys_cpu_ptr;
	
	idmap_pgd = *ptr++;
	sp		  = *ptr++;
	jumpkernel = (void (*)(void))(zbi->cpu_resume);

	put_raw_str( "ptr " );
	put_raw_hex32(ptr);
	put_raw_str( "\n" );

	put_raw_str( "idmap_pgd " );
	put_raw_hex32(idmap_pgd);
	put_raw_str( "\n" );

	put_raw_str( "jumpto kernel " );
	put_raw_hex32(jumpkernel);
	put_raw_str( "\n" );

	jumpkernel(ptr, idmap_pgd);

	put_raw_str( "Bug\n" );
	while(1);
	
}

