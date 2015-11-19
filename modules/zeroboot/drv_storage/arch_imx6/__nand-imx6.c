/*-----------------------------------------------------------------------------
  파 일 : nand-s5pv210.c
  설 명 : 
  작 성 : frog@falinux.com
          freefrug@falinux.com
  날 짜 : 2011-07-29
  주 의 :
          512M SLC 난드 전용이다.
-------------------------------------------------------------------------------*/
#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
#endif

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h> 
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/ioport.h>
#include <linux/slab.h>     // kmalloc() 
#include <linux/poll.h>     
#include <linux/proc_fs.h>
#include <linux/workqueue.h>
#include <linux/irq.h>		
#include <linux/time.h>			
#include <linux/timer.h>		
#include <linux/clk.h>
#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/irq.h>

#include <zb_blk.h>
#include "nand-s5pv210-nand-offset.h"

#define STORAGE_NAME			"s5pv210"
#define NAND_PHYS_BASE			(0xB0E00000)
#define NAND_PHYS_SIZE			(0x2000)

#define OFS_NFCONF  			(0x00)	// R/W Configuration register								0xX000_100X 		
#define OFS_NFCONT 				(0x04)	// R/W Control register                          			0x0001_00C6 
#define OFS_NFCMD            	(0x08)	// R/W Command register                          			0x0000_0000 
#define OFS_NFADDR            	(0x0c)	// R/W Address register                          			0x0000_0000 
#define OFS_NFDATA8            	(0x10)	// R/W Data register                             			0xXXXX_XXXX 
#define OFS_NFSTAT            	(0x28)	// R/W status registet                      				0x0080_001D 

#define REG_NFCONF  			(*(volatile u32 *)(nand_base + OFS_NFCONF		 ))
#define REG_NFCONT 				(*(volatile u32 *)(nand_base + OFS_NFCONT		 ))
#define REG_NFCMD            	(*(volatile u8  *)(nand_base + OFS_NFCMD         ))
#define REG_NFADDR            	(*(volatile u8  *)(nand_base + OFS_NFADDR        ))
#define REG_NFDATA8            	(*(volatile u8  *)(nand_base + OFS_NFDATA8       ))
#define REG_NFSTAT            	(*(volatile u32 *)(nand_base + OFS_NFSTAT        ))

#define NAND_DISABLE_CE()		(REG_NFCONT |= (1 << 1))
#define NAND_ENABLE_CE()		(REG_NFCONT &= ~(1 << 1))

#define NF_TRANSRnB()			do { while(!(REG_NFSTAT & (1 << 0))); } while(0)
                            	
#define NAND_CMD_READ0			0x00
#define NAND_CMD_READSTART		0x30
#define NAND_CMD_READID			0x90
#define NAND_CMD_ERASE1     	0x60
#define NAND_CMD_ERASE2     	0xd0
#define NAND_CMD_SEQIN      	0x80
#define NAND_CMD_PAGEPROG   	0x10

/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------
static void __iomem *nand_base = NULL; 	
static u32  nand_page_size  =  (      2048); 	
static u32  nand_block_size =  (128 * 1024); 	
static u32  page_in_block   =  (128 * 1024)/(2048);

/// @}

#define NAND_START_PAGE			(ZB_STORAGE_OFFSET/2048)
#define NAND_BLOCK_ALIGN(x)		( ((x)/page_in_block)*page_in_block )

/*------------------------------------------------------------------------------
  @brief   4K 단위의 번호를 페이지 주소로 변환해 준다
  @remark  
*///----------------------------------------------------------------------------
static unsigned int nand_idx_to_phys(unsigned int idx)
{
	unsigned int unit;

	unit = ( PAGE_SIZE/nand_page_size );
	return ( NAND_START_PAGE + idx*unit );
	//return ( nand_base_addr/nand_page_size + idx*unit );
}

/*------------------------------------------------------------------------------
  @brief   페이지 주소를 4K 단위의 번호로 변환해 준다
  @remark  
*///----------------------------------------------------------------------------
static unsigned int nand_phys_to_idx(unsigned int phys)
{
	unsigned int unit;

	unit = ( PAGE_SIZE/nand_page_size );
	return ( (phys - NAND_START_PAGE) / unit );
	//return ( (phys - nand_base_addr/nand_page_size) / unit );
}

/*------------------------------------------------------------------------------
  @brief   NAND 디텍션을 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int nand_detect( void )
{
	u8 	company_id;
	u8 	device_id;
	u32	lp;

	printk( "nand detect\n" );

    NAND_ENABLE_CE();

    	REG_NFCMD = NAND_CMD_READID;
    	REG_NFADDR =  0x00;

       	for (lp=0; lp<2000; lp++);	// wait for a while

		company_id = REG_NFDATA8;
		device_id  = REG_NFDATA8;

	NAND_DISABLE_CE();

	printk( "company_id = %02X\n", company_id );
	printk( "device_id  = %02X\n", device_id );

	//if( (company_id == 0xEC) && (device_id == 0xDC)) 
	{
		nand_page_size  =  (      2048); 	
		nand_block_size =  (128 * 1024); 	
		return 0;
	}	

	//return 0;
}
/*------------------------------------------------------------------------------
  @brief   NAND 배드 블록을 체크한다.
  @return  배드블럭 이면 -1
           정상이면  0  
*///----------------------------------------------------------------------------
static int nand_is_bad_block( u32 block_start_page )
{
	s32 	lp;
	char    oob[16];

	NAND_ENABLE_CE();

    REG_NFCMD = NAND_CMD_READ0;

    REG_NFADDR = nand_page_size & 0xff;
	REG_NFADDR =(nand_page_size >> 8) & 0xff;

	REG_NFADDR = (block_start_page) & 0xff;
	REG_NFADDR = (block_start_page >> 8) & 0xff;
	REG_NFADDR = (block_start_page >> 16) & 0xff;

	for (lp=0; lp<2000; lp++);	// wait for a while

	REG_NFCMD = NAND_CMD_READSTART;

    NF_TRANSRnB();

	for( lp=0; lp < 2; lp++)
	{
    	oob[lp] = REG_NFDATA8;
    }

    NAND_DISABLE_CE();
	
	if ( oob[0] == 0xff && oob[1] == 0xff ) return 0;
	
	return -1;	
}
/*------------------------------------------------------------------------------
  @brief   페이지가 배드블럭인지 확인한다.
  @return  배드페이지 이면 -1
           정상이면  0  
*///----------------------------------------------------------------------------
static int nand_is_vaild( u32 page )
{
	// 블럭단위로 페이지 주소를 정렬
	page = (page / page_in_block) * page_in_block;

	if ( 0 == nand_is_bad_block( page ) ) return ZB_PAGE_VALID; 
	return ZB_PAGE_INVALID;
}

/*------------------------------------------------------------------------------
  @brief   NAND 지우기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int nand_erase( u32 block_start_page )
{
	u32 lp;

	NAND_ENABLE_CE();

    REG_NFCMD = NAND_CMD_ERASE1;

	REG_NFADDR = (block_start_page) & 0xff;
	REG_NFADDR = (block_start_page >> 8) & 0xff;
	REG_NFADDR = (block_start_page >> 16) & 0xff;

	for (lp=0; lp<2000; lp++);	// wait for a while

	REG_NFCMD = NAND_CMD_ERASE2;

	NF_TRANSRnB();

    NAND_DISABLE_CE();

	return nand_block_size;
}

/*------------------------------------------------------------------------------
  @brief   NAND 쓰기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int nand_write_page( u32 page , u8 *buf )
{
	s32 	lp;

	NAND_ENABLE_CE();

    REG_NFCMD = NAND_CMD_SEQIN;

    REG_NFADDR = 0;
	REG_NFADDR = 0;

	REG_NFADDR = (page) & 0xff;
	REG_NFADDR = (page >> 8) & 0xff;
	REG_NFADDR = (page >> 16) & 0xff;

	for (lp=0; lp<2000; lp++);	// wait for a while

	for( lp=0; lp < nand_page_size; lp++)
	{
		REG_NFDATA8 = *buf++;
    }

	REG_NFCMD = NAND_CMD_PAGEPROG;

    NF_TRANSRnB();

    NAND_DISABLE_CE();
	
	return nand_page_size;	
}
/*------------------------------------------------------------------------------
  @brief   NAND 읽기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int nand_read_page( u32 page , u8 *buf )
{
	s32 	lp;

	NAND_ENABLE_CE();

    REG_NFCMD = NAND_CMD_READ0;

    REG_NFADDR = 0;
	REG_NFADDR = 0;

	REG_NFADDR = (page) & 0xff;
	REG_NFADDR = (page >> 8) & 0xff;
	REG_NFADDR = (page >> 16) & 0xff;

	for (lp=0; lp<2000; lp++);	// wait for a while

	REG_NFCMD = NAND_CMD_READSTART;

    NF_TRANSRnB();

	for( lp=0; lp < nand_page_size; lp++)
	{
    	*buf++ = REG_NFDATA8;
    }

    NAND_DISABLE_CE();
	
	return nand_page_size;	
}

/*------------------------------------------------------------------------------
  @brief   ZERO 부트를 위하여 할당된 파티션 영역을 모두 지운다.
  @remark  
*///----------------------------------------------------------------------------
static int nand_all_erase( void )
{
	int page_lp;
	int page_count;
	int page_step;

	printk( "NAND FLASH all erase....\n" );
	page_count	= ZB_STORAGE_SIZE/nand_page_size;
	page_step	= nand_block_size/nand_page_size;
	
	printk( "> erase count = %d [", page_count );
	
	for( page_lp=0; page_lp < page_count; page_lp += page_step )
	{
		nand_erase( page_lp+NAND_START_PAGE );
		if( ( page_lp % 10240 ) == 0 ) printk( "." );		
	}	
	printk( "]\n" );
	return 0;
}


/*------------------------------------------------------------------------------
  @brief   시작하기 전에 필요한 일을 한다.
  @remark  사용할 플래시를 모두 지운다.
*///----------------------------------------------------------------------------
static int nand_setup_first( void )
{
	nand_all_erase();
	return 0;
}
/*------------------------------------------------------------------------------
  @brief   메모리 4K 페이지 단위로 쓴다.
  @remark  
*///----------------------------------------------------------------------------
static int nand_write_page_4k( u32 mem_page,  u8 *page_buf )
{
	u32 lp;
	u32 count;
	u32 page_index;
	u8	*data;
	
	page_index 	= nand_idx_to_phys(mem_page);
	data		= page_buf;
	count		= PAGE_SIZE/nand_page_size;
	
	for( lp = 0; lp < count; lp++ )
	{
		nand_write_page( page_index, data );
		page_index++;
		data += nand_page_size;
	}	
	
	return 0;
}

/*------------------------------------------------------------------------------
  @brief   메모리 4K 페이지 단위로 읽는다.
  @remark  
*///----------------------------------------------------------------------------
static int nand_read_page_4k( u32 mem_page,  u8 *page_buf )
{
	u32 lp;
	u32 count;
	u32 page_index;
	u8	*data;
	
	page_index 	= nand_page;
	data		= page_buf;
	count		= PAGE_SIZE/nand_page_size;
	
	for( lp = 0; lp < count; lp++ )
	{
		nand_read_page( page_index, data );
		page_index++;
		data += nand_page_size;
	}	
	return 0;
}

/*------------------------------------------------------------------------------
  @brief   저장소의 offset 값을 알려준다
  @remark  
*///----------------------------------------------------------------------------
static u32 nand_get_storage_offset( void )
{
	return ZB_STORAGE_OFFSET;
}
/*------------------------------------------------------------------------------
  @brief   디바이스 제어를 위한 가상주소
  @remark  
*///----------------------------------------------------------------------------
static u32 nand_get_io_vaddr(void)
{
	return (u32)nand_base;
}

/*------------------------------------------------------------------------------
  @brief   초기화
  @remark   
*///----------------------------------------------------------------------------
int zb_storage_init( struct zblk_ops *zblk_ops )
{
	printk( "STORAGE %s NAND-READY\n", STORAGE_NAME );

	nand_base = ioremap( NAND_PHYS_BASE, NAND_PHYS_SIZE );
	
	printk( "io mapping  nand_base=%p   phys=%08x\n", nand_base, NAND_PHYS_BASE );
	
	if ( 0 != nand_detect() ) 
	{
		iounmap( nand_base );
		nand_base = NULL;
		return -1;
	}
	
	zblk_ops->setup_first        = nand_setup_first       ;	// 준비 작업(처음 한번 호출된다.)
	zblk_ops->get_storage_offset = nand_get_storage_offset;	// 저장소(난드플래시)의 시작 주소(바이트 단위)
	zblk_ops->page_is_valid      = nand_is_vaild          ;	// 현재의 4K 단위의 페이지가 정상인지 묻는 함수
	zblk_ops->page_write         = nand_write_page_4k     ;	// 4KiB 단위의 버퍼 쓰기
	zblk_ops->page_read          = nand_read_page_4k      ;	// 4KiB 단위의 버퍼 읽기
	zblk_ops->get_io_vaddr		 = nand_get_io_vaddr      ;	// nand_base 주소를 얻는 함수
 
    return 0;
}

/*------------------------------------------------------------------------------
  @brief   해제
  @remark  
*///----------------------------------------------------------------------------
void zb_storage_exit( void )
{
	if ( nand_base ) iounmap( nand_base );
	
	printk( "STORAGE NAND-EXIT\n" );
}
