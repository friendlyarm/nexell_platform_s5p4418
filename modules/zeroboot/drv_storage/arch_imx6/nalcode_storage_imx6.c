/**    
    @file     nalcode_storage_imx6.c
    @date     2014/03/27
    @author   jonghooy@sevo-systems Sevo Systems Co.,Ltd.
    @brief    i.mx6q nalcode nand storage driver
    @todo     
    @bug     
    @remark   
    
    @warning 
*/
//
//  저작권    에프에이리눅스(주) / 세보시스템즈
//            외부공개 금지
//
//----------------------------------------------------------------------------

#ifndef CONFIG_MX6
#define CONFIG_MX6
#endif


#include <typedef.h>
#include <zbi.h>
#include <zb_nand.h>
#include "imx-regs.h"
#include "regs-bch.h"
#include "regs-gpmi.h"
#include "crm_regs.h"
#include "dma.h"

#include <nalcode_storage.h>
#include "nalcode_mxs_nand.h"

#define NAND_DELAY	20000


#define	put_raw_str uart_puts
#define	put_raw_hex32 uart_put_hex


extern struct mxs_nand_info nand_info;

static struct zb_priv_nand *zb_nand;
static unsigned char *zb_bbt;




void get_zb_nand_info() {
		int i;
		memset(&nand_info, 0, sizeof(struct mxs_nand_info));

		nand_info.writesize = zb_nand->writesize; // FIXME
		nand_info.chipsize = zb_nand->size;; //FIXME
		nand_info.oobsize = zb_nand->oobsize; //FIXME
	
#if 1
#ifndef NAL_PRE_LOAD //FIXME

		nand_info.mxs_apbh_base=(uint8_t *)zb_nand->mxs_apbh_base_virt;
		nand_info.mxs_bch_base = (uint8_t *)zb_nand->mxs_bch_base_virt;
		nand_info.mxs_gpmi_base = (uint8_t *)zb_nand->mxs_gpmi_base_virt;
		nand_info.ccm_base = (uint8_t *)zb_nand->mxs_ccm_base_virt;

		nand_info.cmd_buf = (uint8_t *)zb_nand->cmd_buf_addr;; // MEM SIZE SHOULD BE MXS_NAND_COMMAND_BUFFER_SIZE -- shuld be dma memory
		nand_info.data_buf = (uint8_t *)zb_nand->data_buf_addr; ; // NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE  -- shuld be dma memory

		for ( i=0; i< MXS_NAND_DMA_DESCRIPTOR_COUNT; i++ ) {
			nand_info.desc[i] = (uint8_t *)zb_nand->dma_desc[i];
		}
	
		nand_info.cmd_buf_phys = (uint8_t *)zb_nand->cmd_buf_addr_phys;
		nand_info.data_buf_phys = (uint8_t *)zb_nand->data_buf_addr_phys;
		nand_info.oob_buf_phys = nand_info.data_buf_phys + nand_info.writesize;

		for ( i=0; i< MXS_NAND_DMA_DESCRIPTOR_COUNT; i++ ) {
			nand_info.desc_phys[i] = (uint8_t *)zb_nand->dma_desc_phys[i];
		}

#if 0
		put_raw_str( "> mxs_apbh_base           = "); put_raw_hex32( nand_info.mxs_apbh_base);
		put_raw_str( "> mxs_bch_base           = "); put_raw_hex32( nand_info.mxs_bch_base);
		put_raw_str( "> mxs_gpmi_base           = "); put_raw_hex32( nand_info.mxs_gpmi_base);
		put_raw_str( "> ccm_base           = "); put_raw_hex32( nand_info.ccm_base);
		put_raw_str( "> cmd_buf_base           = "); put_raw_hex32( nand_info.cmd_buf);
		put_raw_str( "> data_buf_base           = "); put_raw_hex32( nand_info.data_buf);
		put_raw_str( "> dma_desc[0]           = "); put_raw_hex32( nand_info.desc[0]);
		put_raw_str( "> dma_desc[1]           = "); put_raw_hex32( nand_info.desc[1]);
		put_raw_str( "> dma_desc[2]           = "); put_raw_hex32( nand_info.desc[2]);
		put_raw_str( "> dma_desc[3]           = "); put_raw_hex32( nand_info.desc[3]);
		
		put_raw_str( "> cmd_buf_base_phys           = "); put_raw_hex32( nand_info.cmd_buf_phys);
		put_raw_str( "> data_buf_base_phys           = "); put_raw_hex32( nand_info.data_buf_phys);
		put_raw_str( "> dma_desc_phys[0]           = "); put_raw_hex32( nand_info.desc_phys[0]);
		put_raw_str( "> dma_desc_phys[1]           = "); put_raw_hex32( nand_info.desc_phys[1]);
		put_raw_str( "> dma_desc_phys[2]           = "); put_raw_hex32( nand_info.desc_phys[2]);
		put_raw_str( "> dma_desc_phys[3]           = "); put_raw_hex32( nand_info.desc_phys[3]);
#endif
		
		
#else
		
		nand_info.mxs_apbh_base=(uint8_t *)MXS_APBH_BASE;
		nand_info.mxs_bch_base = (uint8_t *)MXS_BCH_BASE;
		nand_info.mxs_gpmi_base = (uint8_t *)MXS_GPMI_BASE;
		nand_info.ccm_base = (uint8_t *)CCM_BASE_ADDR;

		nand_info.cmd_buf = (uint8_t *)zb_nand->cmd_buf_addr_phys; // MEM SIZE SHOULD BE MXS_NAND_COMMAND_BUFFER_SIZE -- shuld be dma memory
		nand_info.data_buf = (uint8_t *)zb_nand->data_buf_addr_phys; ; // NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE  -- shuld be dma memory

		for ( i=0; i< MXS_NAND_DMA_DESCRIPTOR_COUNT; i++ ) {
			nand_info.desc[i] = (uint8_t *)zb_nand->dma_desc_phys[i];
		}
	
		nand_info.cmd_buf_phys = (uint8_t *)zb_nand->cmd_buf_addr_phys;
		nand_info.data_buf_phys = (uint8_t *)zb_nand->data_buf_addr_phys;
		nand_info.oob_buf_phys = nand_info.data_buf_phys + nand_info.writesize;
		
		for ( i=0; i< MXS_NAND_DMA_DESCRIPTOR_COUNT; i++ ) {
			nand_info.desc_phys[i] = (uint8_t *)zb_nand->dma_desc_phys[i];
		}

#if 0		
		put_raw_str( "> mxs_apbh_base			= "); put_raw_hex32( nand_info.mxs_apbh_base);
		put_raw_str( "> mxs_bch_base		   = "); put_raw_hex32( nand_info.mxs_bch_base);
		put_raw_str( "> mxs_gpmi_base			= "); put_raw_hex32( nand_info.mxs_gpmi_base);
		put_raw_str( "> ccm_base		   = "); put_raw_hex32( nand_info.ccm_base);
		put_raw_str( "> cmd_buf_base		   = "); put_raw_hex32( nand_info.cmd_buf);
		put_raw_str( "> data_buf_base			= "); put_raw_hex32( nand_info.data_buf);
		put_raw_str( "> dma_desc[0] 		  = "); put_raw_hex32( nand_info.desc[0]);
		put_raw_str( "> dma_desc[1] 		  = "); put_raw_hex32( nand_info.desc[1]);
		put_raw_str( "> dma_desc[2] 		  = "); put_raw_hex32( nand_info.desc[2]);
		put_raw_str( "> dma_desc[3] 		  = "); put_raw_hex32( nand_info.desc[3]);
		
		put_raw_str( "> cmd_buf_base_phys			= "); put_raw_hex32( nand_info.cmd_buf_phys);
		put_raw_str( "> data_buf_base_phys			 = "); put_raw_hex32( nand_info.data_buf_phys);
		put_raw_str( "> dma_desc_phys[0]		   = "); put_raw_hex32( nand_info.desc_phys[0]);
		put_raw_str( "> dma_desc_phys[1]		   = "); put_raw_hex32( nand_info.desc_phys[1]);
		put_raw_str( "> dma_desc_phys[2]		   = "); put_raw_hex32( nand_info.desc_phys[2]);
		put_raw_str( "> dma_desc_phys[3]		   = "); put_raw_hex32( nand_info.desc_phys[3]);
#endif	
	

#endif
#endif
}

static void nand_command_lp( unsigned int command,   
			    int column, int page_addr)                                      
{                                                                         
	uint32_t rst_sts_cnt = CONFIG_SYS_NAND_RESET_CNT;                                                                                                 
	                                 
	if (command == NAND_CMD_READOOB) {                                      
		column += nand_info.writesize;                                             
		command = NAND_CMD_READ0;                                             
	}                                                                       
                                                                          

	mxs_nand_cmd_ctrl(command & 0xff,                                     
		       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);                       
                                    
	if (column != -1 || page_addr != -1) {                                  
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;                    
                                                                          
		/* Serially input address */                                          
		if (column != -1) {                                                   
			/* Adjust columns for 16 bit buswidth */                            
			mxs_nand_cmd_ctrl(column, ctrl);                                  
			ctrl &= ~NAND_CTRL_CHANGE;                                          
			mxs_nand_cmd_ctrl(column >> 8, ctrl);                             
		}                                                                     
		if (page_addr != -1) {                                                
			mxs_nand_cmd_ctrl( page_addr, ctrl);                               
			mxs_nand_cmd_ctrl( page_addr >> 8,                                 
				       NAND_NCE | NAND_ALE);                                      
			/* One more address cycle for devices > 128MiB */                   
			if (nand_info.chipsize > (128 << 20))                                   
				mxs_nand_cmd_ctrl( page_addr >> 16,                              
					       NAND_NCE | NAND_ALE);                                    
		}                                                                     
	}                                                                       
	mxs_nand_cmd_ctrl( NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);        
                                                                          
	/*                                                                      
	 * program and erase have their own busy handlers                       
	 * status, sequential in, and deplete1 need no delay                    
	 */                                                                     
	switch (command) {                                                      
                                                                          
	case NAND_CMD_CACHEDPROG:                                               
	case NAND_CMD_PAGEPROG:                                                 
	case NAND_CMD_ERASE1:                                                   
	case NAND_CMD_ERASE2:                                                   
	case NAND_CMD_SEQIN:                                                    
	case NAND_CMD_RNDIN:                                                    
	case NAND_CMD_STATUS:                                                   
	case NAND_CMD_DEPLETE1:                                                 
		return;                                                               
                                                                          
		/*                                                                    
		 * read error status commands require only a short delay              
		 */                                                                   
	case NAND_CMD_STATUS_ERROR:                                             
	case NAND_CMD_STATUS_ERROR0:                                            
	case NAND_CMD_STATUS_ERROR1:                                            
	case NAND_CMD_STATUS_ERROR2:                                            
	case NAND_CMD_STATUS_ERROR3:                                            
		delay(NAND_DELAY);                                             
		return;                                                               
                                                                          
	case NAND_CMD_RESET:
		if (mxs_nand_device_ready())                                                  
			break;     
		delay(NAND_DELAY); 
		mxs_nand_cmd_ctrl( NAND_CMD_STATUS,                                  
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);                     
		mxs_nand_cmd_ctrl( NAND_CMD_NONE,                                    
			       NAND_NCE | NAND_CTRL_CHANGE); 
		
		while (!(mxs_nand_read_byte() & NAND_STATUS_READY) &&                 
			(rst_sts_cnt--));                                                   
		return;                                                               
                                                                          
	case NAND_CMD_RNDOUT:                                                   
		/* No ready / busy check necessary */                                 
		mxs_nand_cmd_ctrl( NAND_CMD_RNDOUTSTART,                             
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);                     
		mxs_nand_cmd_ctrl( NAND_CMD_NONE,                                    
			       NAND_NCE | NAND_CTRL_CHANGE);                                
		return;                                                               
                                                                          
	case NAND_CMD_READ0:                                                    
		mxs_nand_cmd_ctrl( NAND_CMD_READSTART,                               
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE); 
		mxs_nand_cmd_ctrl( NAND_CMD_NONE,                                    
			       NAND_NCE | NAND_CTRL_CHANGE);
		/* This applies to read commands */                                   
	default:                                                                
		/*                                                                    
		 * If we don't have access to the busy pin, we apply the given        
		 * command delay                                                      
		 */                                                                   
		 while (!mxs_nand_device_ready()) {                                               
			delay(NAND_DELAY);                                          
		}
		
		 return; 
	}                                                                       
                                                                          
	/* Apply this short delay always to ensure that we do wait tWB in       
	 * any case on any machine. */                                          
	//delay(NAND_DELAY);                                                            
                                                                                                                          
}                                                                         


/*==============================================================================
  @brief   for NAND bad block start
*///----------------------------------------------------------------------------
#ifndef NAL_PRE_LOAD
#else
static int nand_is_bad_block( u32 block_start_page )
{
	char bad;
	int res;
		
		
	nand_command_lp(NAND_CMD_READOOB,0, block_start_page);		
	delay(NAND_DELAY); 			
	bad = mxs_nand_read_byte(); 		
	if ( bad == 0xFF ) res = 0;
	else 
		res = -1;
			
	return res;
}

static void scan_bad_block( u8 *zb_bbt )
{
	u32   idx, scan_cnt, page, page_end, bad_cnt, i;

	bad_cnt  = 0;
	idx      = 0;
	page     = ZB_NAND_START_PAGE;
	page_end = ZB_NAND_START_PAGE+ZB_NAND_STORAGE_PAGE_COUNT;
    uart_puts( "*scan bbt ....\n" );                        
                                  
	// scan bad
	for ( ; page<page_end; idx++, page+=ZB_NAND_PAGE_PER_BLOCK )
	{
		//uart_putc('.');	
		if ( nand_is_bad_block(page) )
		{
			int i, curblock;

			curblock = (page - ZB_NAND_START_PAGE) >> ZB_NAND_PAGE_INDEX_SHIFT;
			for (i = curblock - bad_cnt; i < curblock; i++) {
				zb_bbt[i] = bad_cnt + 1;
			}

			bad_cnt ++;
    		uart_puts( " bad-page=0x" );           
			uart_put_hex( page ); uart_puts("\n");            

		} 
		
		zb_bbt[idx] = bad_cnt;
	}	

	for ( ; idx<BBT_ARRAY_CNT; idx++)
	{
		zb_bbt[idx] = 0xff;	// not used
		
		
	}
#if 0	
	for ( i=0; i<BBT_ARRAY_CNT; i++) {
		if ( zb_bbt[i] != 0xff ) {
			uart_puts("index:");
			uart_put_hex_raw(i);
			uart_puts(",");
			uart_puts("value:");
			uart_put_hex_raw(zb_bbt[i]);
			uart_puts(" ");
			if(0==(i%8) && i!=0)
				uart_puts("\n");
		}
		
	}
	uart_puts("\n");
#endif	
		uart_puts( "*bad-cnt=" );
		uart_put_hex( bad_cnt );uart_puts("\n");
}
#endif


static int nand_read_page(  u8 *buf , u32 page)
{
	int nand_page_size;
	nand_command_lp( NAND_CMD_READ0, 0x0, page);
	nand_page_size = mxs_nand_ecc_read_page( buf,0, page);
	return nand_page_size;	
}

#ifndef NAL_PRE_LOAD
static int need_open;
inline static void __nalcode_stroage_access_begin( void )
{
#if 1
	if (!mxs_nand_device_ready())
	{
		need_open = 1;
		
		// 커널에서 엑세스중일 경우 곧바로 접근하면 난드플래시가 명령을 받지 못한다.
		// 잠시 대기가 필요하다.
		delay( 8000 ); 
	}
	else
	{
		need_open = 0;		
	}

	if ( NAL_2_NAND == STORAGE_ACCESS_MARK_NALCODE ) return;
	if ( NAL_2_NAND == STORAGE_ACCESS_MARK_DONE ) return;

	nand_command_lp(NAND_CMD_RESET,-1,-1);
	
#endif

}

inline static void __nalcode_stroage_access_end( void )
{
	NAL_2_NAND = STORAGE_ACCESS_MARK_NALCODE;
	if ( need_open )
	{
		// DO NOTHING..
	}
}
#else
inline static void __nalcode_stroage_access_begin( void ) {}
inline static void __nalcode_stroage_access_end( void ) {}
#endif

int get_bbt_skip_count(int index)
{
	int count;

	count = zb_bbt[index >> ZB_NAND_BLOCK_INDEX_SHIFT];
	return count * ZB_NAND_INDEX_PER_BLOCK;
}



static int nand_detect(void) {
	u8 company_id = 0;
	u8 device_id = 0;

	
	nand_command_lp(NAND_CMD_RESET, -1, -1);

	nand_command_lp(NAND_CMD_READID,0x00, -1);

	put_raw_str( "> NAND     = ");


#if 1
	company_id = mxs_nand_read_byte();
	device_id = mxs_nand_read_byte();
	
	put_raw_str( "> company_id           = "); put_raw_hex32( company_id);	
	put_raw_str( "> device_id           = "); put_raw_hex32( device_id);	
#endif
	
	mxs_nand_scan_bbt(); // FIXME
	return 1;
}
/*
void nand_verify()
{
	u32  nand_page;
	u32 mem_page_offset;
	u32 local_checksum;
	int checksum_lp;
	int i;
	u8 test_buf[PAGE_SIZE];


			for ( i=0; i<3000; i++ ) {
				int mem_page_offset_real;
				mem_page_offset = i + 0x1FF6;
				mem_page_offset_real = mem_page_offset + get_bbt_skip_count(mem_page_offset);
				nand_page = ZB_NAND_START_PAGE + mem_page_offset_real*ZB_NAND_PAGE_PER_4K;
				//mem_page_offset_real += get_bbt_skip_count(mem_page_offset);
				memset(test_buf,0,PAGE_SIZE);
				__nalcode_stroage_access_begin();
				{
					nand_read_page( test_buf 				  , nand_page	);
					nand_read_page( test_buf + ZB_NAND_PAGE_SIZE  , nand_page+1 );
				}
				__nalcode_stroage_access_end();
				local_checksum = 0 ; 
				for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ ) {
					local_checksum += test_buf[checksum_lp];
				}
				//printf("Logic Index : %8x, Real index: %8x, checksum: %8x\n", mem_page_offset,mem_page_offset_real, local_checksum);
				//printf("KSUM 0x%08X : 0x%08X\n", mem_page_offset, local_checksum);
				uart_puts("KSUM 0x");uart_put_hex_raw(mem_page_offset);uart_puts(": 0x");uart_put_hex_raw(local_checksum);uart_puts("\n");
			}



}
*/
// nalcode first call setup
void nalcode_storage_first_call(void)
{
 	zb_nand = (struct zb_priv_nand *)get_storage_priv_offset();
 	zb_bbt  = (unsigned char *)BBT_ADDR;
	
	get_zb_nand_info();

#ifndef NAL_PRE_LOAD

#if 0
	int i;
	uart_puts("BBT TABLE RAW DATA===========================\n");	
	for ( i=0; i<BBT_ARRAY_CNT; i++) {
			printf("index:%8x, value:%8x\n",i,zb_bbt[i]);
	}
	

	uart_puts("Not NAL_PRE LOAD\n");
#endif
	board_nand_init();
	nand_detect();
	//nand_verify();

	

#else
	put_raw_str( "> First Called  !!! ");
	board_nand_init();
	put_raw_str( "> First Called 11 !!! ");
	nand_detect();	
	put_raw_str( "> First Called 22 !!! ");

	memset(zb_bbt,0,BBT_ARRAY_CNT);
	
#if 1
	nand_read_page( zb_bbt, ZB_NAND_BBT_PAGE );

	// 첫번째 페이지가 0xff 이면 스캔한다.
	if ( 0xff == zb_bbt[0] )
	{
		zb_nand->need_bbt_page_write = 1;	// 나중에 난드에 저장해야 한다.
		scan_bad_block( zb_bbt );
	}
	else
	{
		int  offset   = ZB_NAND_PAGE_SIZE;
		int  bbt_page = ZB_NAND_BBT_PAGE + 1;

		while( offset < BBT_ARRAY_CNT )
		{
			nand_read_page( zb_bbt+offset, bbt_page );
			
			if ( 0xff == zb_bbt[offset] ) break;
			offset   += ZB_NAND_PAGE_SIZE;
			bbt_page ++;
		}
		
		zb_nand->need_bbt_page_write = 0;
	}	
#endif
	
#endif	
}


#if 0

// 외부 호출함수 ------------------------------------------------------------------------
void nalcode_stroage_read_4k_page( u32 mem_page_offset, u8 *mem_buf, void *vaddr )
{
	u32  nand_page;

#ifndef NAL_PRE_LOAD
	
	
	
#else
	
	
#endif	

	mem_page_offset += get_bbt_skip_count(mem_page_offset);
	nand_page = ZB_NAND_START_PAGE + mem_page_offset*ZB_NAND_PAGE_PER_4K;

	__nalcode_stroage_access_begin();
	{
		nand_read_page( mem_buf                   , nand_page   );
		nand_read_page( mem_buf + ZB_NAND_PAGE_SIZE  , nand_page+1 );
	}
	__nalcode_stroage_access_end();
}
#endif

#if 1
// 외부 호출함수 ------------------------------------------------------------------------
void nalcode_stroage_read_4k_page( u32 mem_page_offset, u8 *mem_buf, void *vaddr )
{
	u32  nand_page;
#if 0	
#ifndef NAL_PRE_LOAD	
	u32 local_checksum;
	int checksum_lp;
	int i;
#endif
#endif
	mem_page_offset += get_bbt_skip_count(mem_page_offset);
	nand_page = ZB_NAND_START_PAGE + mem_page_offset*ZB_NAND_PAGE_PER_4K;


	__nalcode_stroage_access_begin();
	{
		nand_read_page( mem_buf                   , nand_page   );
		nand_read_page( mem_buf + ZB_NAND_PAGE_SIZE  , nand_page+1 );
	}
	__nalcode_stroage_access_end();



#if 0
#ifndef NAL_PRE_LOAD
		for ( i=0; i<3000; i++ ) {
			int mem_page_offset_real;
			mem_page_offset = i + 0x1FF6;
			mem_page_offset_real = mem_page_offset + get_bbt_skip_count(mem_page_offset);
			nand_page = ZB_NAND_START_PAGE + mem_page_offset_real*ZB_NAND_PAGE_PER_4K;
			//mem_page_offset_real += get_bbt_skip_count(mem_page_offset);
			memset(mem_buf,0,PAGE_SIZE);
			__nalcode_stroage_access_begin();
			{
				nand_read_page( mem_buf                   , nand_page   );
				nand_read_page( mem_buf + ZB_NAND_PAGE_SIZE  , nand_page+1 );
			}
			__nalcode_stroage_access_end();
			local_checksum = 0 ; 
			for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ ) {
				local_checksum += mem_buf[checksum_lp];
			}
			//printf("Logic Index : %8x, Real index: %8x, checksum: %8x\n", mem_page_offset,mem_page_offset_real, local_checksum);
			printf("KSUM 0x%08X : 0x%08X\n", mem_page_offset, local_checksum);
		}
#endif
#endif		
}
#endif

void nalcode_stroage_test_function(void)
{
	
	nalcode_storage_first_call();
	//nand_verify();
}
