
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
#include <asm/delay.h>
#include <zb_blk.h>
#include <zbi.h>
#include <zb_nand.h>
#include <zb_snapshot.h>
#include <linux/jiffies.h>
// ADDED

#include <linux/mtd/nand.h>
#include <asm/delay.h>


#define STORAGE_NAME			"zero-imx-gpmi-nand"
#define  CONFIG_SYS_NAND_RESET_CNT  100; 
#define	MXS_NAND_COMMAND_BUFFER_SIZE		32


static struct nand_chip nand_chip[1];
static struct mtd_info mtd[1];
static u32  nand_page_size  =  (      2048); 	
static u32  nand_block_size =  (128 * 1024); 	
static u32  page_in_block   =  (128 * 1024)/(2048);
static u32  page_min  = 0; 
static u32  page_max  = 0;




/// @}

struct zb_priv_nand *zb_nand;

extern int board_nand_init(struct nand_chip *nand);


static struct nand_flash_dev *nand_get_flash_type( int busw, int *maf_id);


static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
		"bne 1b" : "=r" (loops) : "0"(loops));
}

/*
 * zb_bbt  : bbt array 8bit size, how many bad block have before
 *         : PAGE_OFFSET + 0x0008E000
 *         : PAGE_OFFSET + 0x0008F000
 *         : 8192 MAX
 * caution : bug - if bad block count is greater than 256, will go 0
 *         : bug - if nand storage size is over block_size * 8192
 *                 128KiB block is 1GiB Max
 */
unsigned char *zb_bbt;
#define	BBT_MAX		255

/*
 * same source is in drv_storage/arch_xxx/nand-xxxx.c
 * save source is in drv_storage/arch_xxx/nalcode_storage_xxx.c
 * bbt table is per block
 * index is 4K unit
 * get block number by index
 * multiply block per index
 */
int get_bbt_skip_count(int index)
{
	int count;

	count = zb_bbt[index >> ZB_NAND_BLOCK_INDEX_SHIFT];
	return count * ZB_NAND_INDEX_PER_BLOCK;
}
/*-----------------------------	-------------------------------------------------
  @brief   4K 단위의 번호를 페이지 주소로 변환해 준다
  @remark  
*///----------------------------------------------------------------------------
static unsigned int mem_index_to_nand_phys(unsigned int mem_page)
{
	unsigned int offset_nand_page;

	offset_nand_page = mem_page * (PAGE_SIZE/nand_page_size);
	return ZB_NAND_START_PAGE + offset_nand_page;
}

/*------------------------------------------------------------------------------
  @brief   페이지 주소를 4K 단위의 번호로 변환해 준다
  @remark  
*///----------------------------------------------------------------------------
static unsigned int nand_phys_to_idx(unsigned int phys)
{
	unsigned int unit;

	unit = ( PAGE_SIZE/nand_page_size );
	return ( (phys - ZB_NAND_START_PAGE) / unit );
	//return ( (phys - nand_base_addr/nand_page_size) / unit );
}

/*------------------------------------------------------------------------------
  @brief   NAND 디텍션을 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int nand_detect( void )
{
	int busw, nand_maf_id;
	int i;
	struct nand_flash_dev *type;		
	

	/* Get buswidth to select the correct functions */
	busw = nand_chip->options & NAND_BUSWIDTH_16;
	
	/* Read the flash type */  
	type = nand_get_flash_type( busw, &nand_maf_id);

	nand_chip->numchips = 1;
	mtd->size = 1 * nand_chip->chipsize;
	
	
	
	if (!(nand_chip->options & NAND_OWN_BUFFERS))
		nand_chip->buffers = kmalloc(sizeof(*nand_chip->buffers), GFP_KERNEL);
	if (!nand_chip->buffers)
		return -ENOMEM;

	/* Set the internal oob buffer location, just after the page data */
	nand_chip->oob_poi = nand_chip->buffers->databuf + mtd->writesize;


	nand_chip->ecc.layout->oobavail = 0;
	for (i = 0; nand_chip->ecc.layout->oobfree[i].length; i++)
		nand_chip->ecc.layout->oobavail +=
			nand_chip->ecc.layout->oobfree[i].length;
	mtd->oobavail = nand_chip->ecc.layout->oobavail;

	/*
	 * Set the number of read / write steps for one page depending on ECC
	 * mode
	 */
	nand_chip->ecc.steps = mtd->writesize / nand_chip->ecc.size;
	if(nand_chip->ecc.steps * nand_chip->ecc.size != mtd->writesize) {
		printk(KERN_WARNING "Invalid ecc parameters\n");
		BUG();
	}
	nand_chip->ecc.total = nand_chip->ecc.steps * nand_chip->ecc.bytes;

	/*
	 * Allow subpage writes up to ecc.steps. Not possible for MLC
	 * FLASH.
	 */
	if (!(nand_chip->options & NAND_NO_SUBPAGE_WRITE) &&
	    !(nand_chip->cellinfo & NAND_CI_CELLTYPE_MSK)) {
		switch(nand_chip->ecc.steps) {
		case 2:
			mtd->subpage_sft = 1;
			break;
		case 4:
		case 8:
			mtd->subpage_sft = 2;
			break;
		}
	}
	nand_chip->subpagesize = mtd->writesize >> mtd->subpage_sft;

	/* Initialize state */
	nand_chip->state = FL_READY;

	nand_chip->select_chip(mtd, 0); // DONOT DESELECT JHYOON

	/* Invalidate the pagebuffer reference */
	nand_chip->pagebuf = -1;

#if 0
	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->erase = nand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = nand_read;
	mtd->write = nand_write;
	mtd->read_oob = nand_read_oob;
	mtd->write_oob = nand_write_oob;
	mtd->sync = nand_sync;
	mtd->lock = NULL;
	mtd->unlock = NULL;
	mtd->suspend = nand_suspend;
	mtd->resume = nand_resume;
	mtd->block_isbad = nand_block_isbad;
	mtd->block_markbad = nand_block_markbad;

	/* propagate ecc.layout to mtd_info */
	mtd->ecclayout = chip->ecc.layout;
#endif

	/* Check, if we should skip the bad block table scan */
	//chip->options |= NAND_BBT_SCANNED;//check

	nand_chip->scan_bbt(mtd);
	return 0;

	
}


/**
 * nand_wait - [DEFAULT]  wait until the command is done
 * @mtd:	MTD device structure
 * @chip:	NAND chip structure
 *
 * Wait for command done. This applies to erase and program only
 * Erase can take up to 400ms and program up to 20ms according to
 * general NAND and SmartMedia specs
 */
static int nand_wait(void)
{
	int count=0;
	
	nand_chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);
	
	while (1) {
		if ( count>10000) {
			printk("Timeout!\n");
			return 0x01;	
		}	
		if (nand_chip->dev_ready) {
			if (nand_chip->dev_ready(mtd))
				break;
		} else {
			if (nand_chip->read_byte(mtd) & NAND_STATUS_READY)
				break;
		}
		udelay(1);
		count++;
	}
	return (int)nand_chip->read_byte(mtd);
}

																	
static void nand_command_lp(struct mtd_info *mtd, unsigned int command,   
			    int column, int page_addr)                                      
{                                                                         
	uint32_t rst_sts_cnt = CONFIG_SYS_NAND_RESET_CNT;                                                                                                 
	                                 
	if (command == NAND_CMD_READOOB) {                                      
		column += mtd->writesize;                                             
		command = NAND_CMD_READ0;                                             
	}                                                                       
                                                                          

	nand_chip->cmd_ctrl(mtd, command & 0xff,                                     
		       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);                       
                                    
	if (column != -1 || page_addr != -1) {                                  
		int ctrl = NAND_CTRL_CHANGE | NAND_NCE | NAND_ALE;                    
                                                                          
		/* Serially input address */                                          
		if (column != -1) {                                                   
			/* Adjust columns for 16 bit buswidth */                            
			if (nand_chip->options & NAND_BUSWIDTH_16)                               
				column >>= 1;                                                     
			nand_chip->cmd_ctrl(mtd, column, ctrl);                                  
			ctrl &= ~NAND_CTRL_CHANGE;                                          
			nand_chip->cmd_ctrl(mtd, column >> 8, ctrl);                             
		}                                                                     
		if (page_addr != -1) {                                                
			nand_chip->cmd_ctrl(mtd, page_addr, ctrl);                               
			nand_chip->cmd_ctrl(mtd, page_addr >> 8,                                 
				       NAND_NCE | NAND_ALE);                                      
			/* One more address cycle for devices > 128MiB */                   
			if (nand_chip->chipsize > (128 << 20))                                   
				nand_chip->cmd_ctrl(mtd, page_addr >> 16,                              
					       NAND_NCE | NAND_ALE);                                    
		}                                                                     
	}                                                                       
	nand_chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);        
                                                                          
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
		udelay(nand_chip->chip_delay);                                             
		return;                                                               
                                                                          
	case NAND_CMD_RESET:
		if (nand_chip->dev_ready(mtd))                                                  
			break;     
		udelay(nand_chip->chip_delay);  
		nand_chip->cmd_ctrl(mtd, NAND_CMD_STATUS,                                  
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);                     
		nand_chip->cmd_ctrl(mtd, NAND_CMD_NONE,                                    
			       NAND_NCE | NAND_CTRL_CHANGE); 
		
		while (!(nand_chip->read_byte(mtd) & NAND_STATUS_READY) &&                 
			(rst_sts_cnt--));                                                   
		return;                                                               
                                                                          
	case NAND_CMD_RNDOUT:                                                   
		/* No ready / busy check necessary */                                 
		nand_chip->cmd_ctrl(mtd, NAND_CMD_RNDOUTSTART,                             
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);                     
		nand_chip->cmd_ctrl(mtd, NAND_CMD_NONE,                                    
			       NAND_NCE | NAND_CTRL_CHANGE);                                
		return;                                                               
                                                                          
	case NAND_CMD_READ0:                                                    
		nand_chip->cmd_ctrl(mtd, NAND_CMD_READSTART,                               
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);                     
		nand_chip->cmd_ctrl(mtd, NAND_CMD_NONE,                                    
			       NAND_NCE | NAND_CTRL_CHANGE);                                
                                                                          
		/* This applies to read commands */                                   
	default:                                                                
		/*                                                                    
		 * If we don't have access to the busy pin, we apply the given        
		 * command delay                                                      
		 */                                                                   
		while (!nand_chip->dev_ready(mtd)) {                                               
			udelay(1);                                           
		}  
		return;   
	}                                                                       
                                                                          
	/* Apply this short delay always to ensure that we do wait tWB in       
	 * any case on any machine. */                                          
	//delay(100);                                                            
     udelay(1);                                                                     
	nand_wait_ready(mtd);       
	//nand_wait();
}                                                                         

/*------------------------------------------------------------------------------
  @brief   NAND 배드 블록을 체크한다.
  @return  배드블럭 이면 -1
           정상이면  0  
*///----------------------------------------------------------------------------
	//nand_block_bad() at nand_base.c line 369
static int nand_is_bad_block(struct nand_chip * chip,u32 block_start_page) {
		char bad;
		int res;
		
		
		nand_command_lp(mtd, NAND_CMD_READOOB,0, block_start_page);		
		udelay(100); 			
		bad = chip->read_byte(mtd); 		
		if ( bad == 0xFF ) res = 0;
		else 
			res = -1;
			
		return res;
}


/**
 * single_erease_cmd - [GENERIC] NAND standard block erase command function
 * @mtd:	MTD device structure
 * @page:	the page address of the block which will be erased
 *
 * Standard erase command for NAND chips
 */
static void single_erase_cmd(int page)
{
	
	nand_chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
	nand_chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1);
	
}



/*------------------------------------------------------------------------------
  @brief   NAND 지우기를 지원한다. 
  @remark  
*///----------------------------------------------------------------------------
static int nand_erase( u32 block_start_page )
{
	single_erase_cmd(block_start_page);
	return 0;

}






/*------------------------------------------------------------------------------
  @brief   NAND 쓰기를 지원한다.  with ECC
  @remark  
*///----------------------------------------------------------------------------

static int nand_write_page( u32 page , u8 *buf )
{
	

	int status;

	memset(nand_chip->oob_poi, 0xff, mtd->oobsize);
	
	nand_chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);
	
	nand_chip->ecc.write_page(mtd, nand_chip, buf,1);
	
	nand_chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	
	status = nand_wait();
	/*
	 * See if operation failed and additional status checks are
	 * available
	 */
	if ((status & NAND_STATUS_FAIL) && (nand_chip->errstat))
		status = nand_chip->errstat(mtd, nand_chip, FL_WRITING, status,
				       page);
	if (status & NAND_STATUS_FAIL)
		return -EIO;
	

	return 0;


}




static int nand_read_page(  u32 page, u8 *buf )
{
	
	nand_chip->cmdfunc(mtd, NAND_CMD_READ0, 0x0, page);
	nand_chip->ecc.read_page(mtd, nand_chip, buf,0, page);
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
		nand_erase( page_lp+ZB_NAND_START_PAGE );
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
	u32  page;
	
	printk("ZBI SETUP First Called....\n");
	// 영역 지우기
	for( page=page_min; page<page_max; page+=page_in_block )
	{
		nand_erase( page );
		udelay(2000);
	}
	return 0;
	//return nand_all_erase();
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

	
	mem_page	+= get_bbt_skip_count(mem_page);
	page_index 	= mem_index_to_nand_phys(mem_page);;
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
  @brief   메모리 4K 페이지 단위로 쓴다.
  @remark  
*///----------------------------------------------------------------------------
static int nand_write_page_4k( u32 mem_page,  u8 *page_buf )
{
	u32 lp;
	u32 count;
	u32 page_index;
	u32 test_mem_page;
	u8	*data;
	u8 tmp_manf;
	u8 tmp_id;
	u8 test_buf[4096];

	test_mem_page = mem_page;
	mem_page	+= get_bbt_skip_count(mem_page);
	page_index 	= mem_index_to_nand_phys(mem_page);
	data		= page_buf;

	count		= PAGE_SIZE/nand_page_size;
	for( lp = 0; lp < count; lp++ )
	{
		nand_write_page( page_index, data );
		page_index++;
		data += nand_page_size;
	}

#if 0
	memset(test_buf,0,4096);
	nand_read_page_4k( test_mem_page, test_buf );
	if ( memcmp(page_buf,test_buf,4096)!= 0 ) {
		printk("mem Page : %d, Verification Error\n",page_index);

	}	
#endif		
	
	return 0;
}


/*------------------------------------------------------------------------------
  @brief   저장소의 offset 값을 알려준다
  @remark  
*///----------------------------------------------------------------------------
static u32 nand_get_storage_offset( void )
{
	return zb_nand->offset; // ZB_STORAGE_OFFSET;
}
/*------------------------------------------------------------------------------
  @brief   Not USED
  @remark  
*///----------------------------------------------------------------------------
static u32 nand_get_io_vaddr(void)
{
	
	return (u32)0;
}

/*==============================================================================
  @brief   for NAND private ptr  (ver 1.4.0)
*///----------------------------------------------------------------------------
static int nand_get_shift(int base)
{
	int shift = 0;

	while (base) {
		base = base >> 1;
		if (!base) break;
		shift++;
	}
	return shift;
}

#include <linux/mtd/mtd.h>
extern struct mtd_info *get_mtd_device_nm(const char *name);
extern uint64_t zero_get_mtd_part(struct mtd_info *mtd);
static int zb_setup_private_info(void)
{
	int i;
	
	//zb_nand->vaddr_nand_ctrl = (u32)nand_base; //FIXME
	//zb_nand->vaddr_dma_ctrl  = (u32)dma_base;

#if 1
	{
	
		zb_nand->offset          = ZB_STORAGE_OFFSET;
		zb_nand->size            = ZB_STORAGE_SIZE;
		zb_nand->erasesize       = mtd->erasesize;
		zb_nand->pagesize        = nand_page_size;
		zb_nand->zb_start_page   = ZB_NAND_START_PAGE;
		zb_nand->page_per_4k     = PAGE_SIZE/zb_nand->pagesize;
		zb_nand->page_per_block  = zb_nand->erasesize/zb_nand->pagesize;
		zb_nand->index_per_block = zb_nand->erasesize/PAGE_SIZE;
		zb_nand->block_index_shtft = nand_get_shift(zb_nand->index_per_block);
		zb_nand->block_page_shtft  = nand_get_shift(zb_nand->page_per_block);
		zb_nand->zb_page_count   = zb_nand->size/zb_nand->pagesize;
		

		zb_nand->oobsize = mtd->oobsize;
		zb_nand->writesize = mtd->writesize;

	}
	
#else							// 커널의 mtd 에서 직접 읽어온다.
	struct mtd_info *zmtd;

	zmtd = get_mtd_device_nm("snapshot");
	if ( zmtd )
	{
		printk("\t*find snapshot partition\n" );		
		memset(zb_nand, 0, sizeof(struct zb_priv_nand));
		zb_nand->offset            = 0;//zero_get_mtd_part(zmtd);
		zb_nand->size              = zmtd->size;
		zb_nand->erasesize         = zmtd->erasesize;
		zb_nand->pagesize          = zmtd->writesize;
		zb_nand->zb_start_page     = zb_nand->offset/zb_nand->pagesize;
		zb_nand->page_per_4k       = PAGE_SIZE/zb_nand->pagesize;
		zb_nand->page_per_block    = zb_nand->erasesize/zb_nand->pagesize;
		zb_nand->index_per_block   = zb_nand->erasesize/PAGE_SIZE;
		zb_nand->block_index_shtft = nand_get_shift(zb_nand->index_per_block);
		zb_nand->block_page_shtft  = nand_get_shift(zb_nand->page_per_block);
		zb_nand->zb_page_count     = zb_nand->size/zb_nand->pagesize;
	}
	else
	{
		printk( "============fatal error can't find 'snapshot' patition ==============\n" );
	}
#endif
	
	printk("\toffset            0x%08lX\n", zb_nand->offset);
	printk("\ttotal size        0x%08X\n",  zb_nand->size);
	printk("\terasesize         0x%08X\n",  zb_nand->erasesize);
	printk("\tpagesize          0x%08X\n",  zb_nand->pagesize);
	printk("\tstart page        0x%08X\n",  zb_nand->zb_start_page);
	printk("\tcount page        0x%08X\n",  zb_nand->zb_page_count);
	printk("\tpage_per_4k       0x%08X\n",  zb_nand->page_per_4k);
	printk("\tpage_per_block    0x%08X\n",  zb_nand->page_per_block);
	printk("\tindex_per_block   0x%08X\n",  zb_nand->index_per_block);
	printk("\tblock index shtft 0x%08X\n",  zb_nand->block_index_shtft);
	printk("\tblock page shtft  0x%08X\n",  zb_nand->block_page_shtft);


	printk("\------------------]n");
	printk("\twritesize	0x%08X\n",	zb_nand->writesize);
	printk("\toobsize	0x%08X\n",	zb_nand->oobsize);
	printk("\tcmd_buf_addr 0x%08X\n",	zb_nand->cmd_buf_addr);
	printk("\tdata_buf_addr	0x%08X\n",	zb_nand->data_buf_addr);
	
	printk("\tcmd_buf_addr_phys	0x%08X\n",	zb_nand->cmd_buf_addr_phys);
	printk("\tdata_buf_addr_phys	0x%08X\n",	zb_nand->data_buf_addr_phys);
	printk("\tcmd_buf_addr 0x%08X\n",	zb_nand->cmd_buf_addr);
	printk("\tmxs_bch_base_virt	0x%08X\n",	zb_nand->mxs_bch_base_virt);
	printk("\tmxs_gpmi_base_virt	0x%08X\n",	zb_nand->mxs_gpmi_base_virt);
	printk("\tmxs_ccm_base_virt	0x%08X\n",	zb_nand->mxs_ccm_base_virt);
	printk("\tmxs_apbh_base_virt 0x%08X\n",	zb_nand->mxs_apbh_base_virt);

	for ( i=0; i<4; i++ ) {
		printk("\tdma_desc_phys_addr[%d]	0x%08X\n",i,	zb_nand->dma_desc_phys[i]);
		printk("\tdma_desc_addr[%d]	0x%08X\n",	i,zb_nand->dma_desc[i]);
	}
	zb_pre_setup_dma_area(zb_nand->cmd_buf_addr_phys,PAGE_SIZE);
	//zb_pre_setup_dma_area(zb_nand->data_buf_addr_phys, NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE);
	zb_pre_setup_dma_area(zb_nand->data_buf_addr_phys, NAND_MAX_PAGESIZE);
	zb_pre_setup_dma_area(zb_nand->dma_desc_phys[0],PAGE_SIZE );
	zb_pre_setup_dma_area(zb_nand->dma_desc_phys[1],PAGE_SIZE );
	zb_pre_setup_dma_area(zb_nand->dma_desc_phys[2],PAGE_SIZE );
	zb_pre_setup_dma_area(zb_nand->dma_desc_phys[3],PAGE_SIZE );

	return 0;
}

/* bad check function is page unit.
 * we need only block unit check.
 * block_base : base page number
 * block_max  : max page number
 */
static int zb_bbt_init(void)
{
	int index, page, page_end;
	int bad_cnt;
	int i;

	zb_bbt = (unsigned char *)BBT_ADDR;
	memset(zb_bbt, 0x00, BBT_ARRAY_CNT);

	bad_cnt  = 0;
	index    = 0;  
	page     = zb_nand->zb_start_page;
	page_end = zb_nand->zb_start_page + zb_nand->zb_page_count;
	
	for ( ; page<page_end; page+=zb_nand->page_per_block ) 
	{
		
		if ( nand_is_bad_block(nand_chip, page ) < 0 )
		{
			int i, curblock;
			curblock = (page - zb_nand->zb_start_page) / zb_nand->page_per_block;
			for (i = curblock - bad_cnt; i < curblock; i++) {
				zb_bbt[i] = bad_cnt + 1;
			}
			bad_cnt++;
		}
		
		zb_bbt[index++] = bad_cnt;
	}
#if 0
	for ( i=0; i<BBT_ARRAY_CNT; i++ ) {
		printk("index:%8x,",i);
		printk("value:%8x ",zb_bbt[i]);
		if ( (i%8) == 0 && i!=0 ) printk("\n");
	}	
#endif
	if (bad_cnt > BBT_MAX) {
		printk("BUG bad block count is too high\n");
		printk("bad block algorithm can not working\n");
		printk("bad block is  %d\n", bad_cnt);
		printk("bad block max %d\n", BBT_MAX);
		panic("Snapshot BBT count is Too Big");
		while(1);
	}

	printk("\tnand bad total       %d\n", bad_cnt);
	printk("\tbbt table            0x%p\n", zb_bbt);
	return 0;
}
/*------------------------------------------------------------------------------
  @brief   제로부팅 이후 처리할일
  @remark   
*///----------------------------------------------------------------------------
/* //JHYOON REMOVE
#ifdef CONFIG_FALINUX_NAND_BBT_ROOTFS_SURPORT
extern void zero_repack_rootfs(void);
#else
static void zero_repack_rootfs(void) { }
#endif
*/

//static void zero_repack_rootfs(void) { }

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
		{
			nand_read_page( nand_page, test_buf			);
			nand_read_page( nand_page+1, test_buf + ZB_NAND_PAGE_SIZE   );
		}
		local_checksum = 0 ; 
		for( checksum_lp = 0; checksum_lp < PAGE_SIZE; checksum_lp++ ) {
			local_checksum += test_buf[checksum_lp];
		}
		//printf("Logic Index : %8x, Real index: %8x, checksum: %8x\n", mem_page_offset,mem_page_offset_real, local_checksum);
		printk("KSUM 0x%08X : 0x%08X\n", mem_page_offset, local_checksum);
		//uart_puts("KSUM 0x");uart_put_hex_raw(mem_page_offset);uart_puts(": 0x");uart_put_hex_raw(local_checksum);uart_puts("\n");
	}



}




static int nand_post_zeroboot( void )
{
	int  wr_bbt, page;
		                        
	// BBT 를 저장해야 한다면

	/*
	if ( zb_nand->need_bbt_page_write )
	{
		wr_bbt = 0;
		page   = ZB_NAND_BBT_PAGE;

		while( wr_bbt < BBT_ARRAY_CNT )
		{
			nand_write_page( page, zb_bbt + wr_bbt ); 
			page ++;
			wr_bbt += zb_nand->pagesize;
		}

		zb_nand->need_bbt_page_write = 0;	 
		printk(" write bbt table\n" );
	}*/
	
	//nand_verify();

	
	
	return 0;
}


void nand_speed_test(void)
{
	int error_count= 0; 
	int page_no;
	char test_data[2048];
	char data[2048];
	int i;
	
	int test_size = 40000;
	long start_time_jiffies, end_time_jiffies; 
	
	for ( i=0; i< test_size; i++ ) single_erase_cmd(i);
	
	memset(test_data,0xff,sizeof(test_data));
	
	for ( i=0; i<256; i=i+16) {
			test_data[i+0]=0xde;
			test_data[i+1]=0xad;
			test_data[i+2]=0xbe;
			test_data[i+3]=0xef;
			test_data[i+4]=0xde;
			test_data[i+5]=0xad;
			test_data[i+6]=0xbe;
			test_data[i+7]=0xef;
			test_data[i+8]=0xde;
			test_data[i+9]=0xad;
			test_data[i+10]=0xbe;
			test_data[i+11]=0xef;
			test_data[i+12]=0xde;
			test_data[i+13]=0xad;
			test_data[i+14]=0xbe;
			test_data[i+15]=0xef;
	}
		
	start_time_jiffies = jiffies;
	for ( page_no = 0; page_no<test_size; page_no++) {
		nand_write_page(page_no,test_data);	
	}
	end_time_jiffies = jiffies;
	
	printk("Nand operation Count %d Pages,	Write time_consume: %d\n",test_size, (end_time_jiffies - start_time_jiffies) / HZ );

	start_time_jiffies = jiffies;
	for ( page_no = 0; page_no<test_size; page_no++) {
		nand_read_page(page_no,data);	
	}
	end_time_jiffies = jiffies;
	printk("Nand operation Count %d Pages,	Read time_consume: %d\n",test_size, (end_time_jiffies - start_time_jiffies) / HZ );

	if ( memcmp(test_data,data,sizeof(test_data)) != 0 ) printk("VERIFICATIOn FAILED!\n");
		else printk("VERIFICATIOn Success!\n");
		

}

/*------------------------------------------------------------------------------
     for NAND private ptr 
*///============================================================================

void nand_test(void)
{
	int error_count= 0; 
	long long page_no;
	int i;
	char test_data[2048];
	char data[2048];
	
	for ( page_no = page_min; page_no<32768*2; page_no++) {
		
		memset(test_data,0xff,sizeof(test_data));
		memset(data,0,sizeof(data));

		for ( i=0; i<256; i=i+16) {
			test_data[i+0]=0xde;
			test_data[i+1]=0xad;
			test_data[i+2]=0xbe;
			test_data[i+3]=0xef;
			test_data[i+4]=0xde;
			test_data[i+5]=0xad;
			test_data[i+6]=0xbe;
			test_data[i+7]=0xef;
			test_data[i+8]=0xde;
			test_data[i+9]=0xad;
			test_data[i+10]=0xbe;
			test_data[i+11]=0xef;
			test_data[i+12]=0xde;
			test_data[i+13]=0xad;
			test_data[i+14]=0xbe;
			test_data[i+15]=0xef;
		}
		printk(" Page %8x test started\n", page_no);
		single_erase_cmd(page_no);
		udelay(1000);
		nand_write_page(page_no,test_data);	
		udelay(1000);
		nand_read_page(page_no,data);
		udelay(1000);
		if ( memcmp(test_data,data,sizeof(test_data)) != 0 ) {
			
			error_count++;
			printk("Failed...........\n");
			for ( i=0; i<256; i=i+16)
					printk("data=%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7]
					,data[i+8],data[i+9],data[i+10],data[i+11],data[i+12],data[i+13],data[i+14],data[i+15]);

		}

	}
	
	printk("Nand operation Count %ll Pages,	Error Count: %d\n", page_max - page_min, error_count);



}

/*------------------------------------------------------------------------------
  @brief   초기화
  @remark   
*///----------------------------------------------------------------------------
int zb_storage_init( struct zblk_ops *zblk_ops )
{


	mtd->priv = nand_chip;
	
	printk( "STORAGE %s NAND-READY\n", STORAGE_NAME );	

	zb_nand = (struct zb_priv_nand *)get_storage_priv_offset();
	
	if ( board_nand_init(nand_chip) == 0 ) {
 		  
		if (nand_detect() == 0) {
			printk("NAND DETECT SUCCESS\n");
			
			zblk_ops->setup_first		  = nand_setup_first	   ;	// 준비 작업(처음 한번 호출된다.)
			zblk_ops->get_storage_offset  = nand_get_storage_offset;	// 저장소(난드플래시)의 시작 주소(바이트 단위)
			zblk_ops->page_write		  = nand_write_page_4k	   ;	// 4KiB 단위의 버퍼 쓰기
			zblk_ops->page_read 		  = nand_read_page_4k	   ;	// 4KiB 단위의 버퍼 읽기
			zblk_ops->get_io_vaddr		  = nand_get_io_vaddr	   ;	// nand_base 주소를 얻는 함수
			zblk_ops->setup_post_zeroboot = nand_post_zeroboot	   ;	// 제로부트 이후에 할일
				
			zb_setup_private_info();		  
			
			page_min = zb_nand->zb_start_page; 
			page_max = zb_nand->zb_start_page + zb_nand->zb_page_count;
			zb_bbt_init();
		
			printk( "NAND-STORAGE-OFFSET  0x%08lx (%ld-MB)\n", zb_nand->offset , zb_nand->offset>>20 );
			printk( "NAND-STORAGE-SIZE	  0x%08x (%d-MB)\n"  , zb_nand->size   , zb_nand->size	>>20 );
			printk( "NAND-PAGE-OFFSET	  %d\n" 			 , zb_nand->zb_start_page );
			printk( "NAND-PAGE-AREA 	  %d <= page <= %d\n", page_min, page_max );
		} else {
			printk( " NAND DETECT FAILED !! \n" );
			return 0; // FIXME RETURN CODE
			
		}

		//nand_test();
		//nand_speed_test();
	}
	
    return 0;

}
/*------------------------------------------------------------------------------
  @brief   해제
  @remark  
*///----------------------------------------------------------------------------
void zb_storage_exit( void )
{
	
	printk( "STORAGE NAND-EXIT\n" );
}
 

/*
 * Get the flash and manufacturer id and lookup if the type is supported
 */
static struct nand_flash_dev *nand_get_flash_type( int busw, int *maf_id)
{
	struct nand_flash_dev *type = NULL;
	int i, dev_id, maf_idx;
	int tmp_id, tmp_manf;
 	u8 id_data[8];
   
  	nand_chip->cmdfunc = nand_command_lp;   // FIXME.. 
            
	/* Select the device */
	nand_chip->select_chip(mtd, 0);
    /*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up
	 */
	nand_chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
                
	/* Send the command for reading device ID */
	nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
    /* Read manufacturer and device IDs */
	*maf_id = nand_chip->read_byte(mtd);
	dev_id = nand_chip->read_byte(mtd);
    
		    
	/* Try again to make sure, as some systems the bus-hold or other
	 * interface concerns can cause random data which looks like a
	 * possibly credible NAND flash to appear. If the two results do
	 * not match, ignore the device completely.
	 */

	nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */

	tmp_manf = nand_chip->read_byte(mtd);
	tmp_id = nand_chip->read_byte(mtd);

	if (tmp_manf != *maf_id || tmp_id != dev_id) {
		printk(KERN_INFO "%s: second ID read did not match "
		       "%02x,%02x against %02x,%02x\n", __func__,
		       *maf_id, dev_id, tmp_manf, tmp_id);
		return ERR_PTR(-ENODEV);
	}

	/* Lookup the flash id */
	for (i = 0; nand_flash_ids[i].name != NULL; i++) {
		if (dev_id == nand_flash_ids[i].id) {
			type =  &nand_flash_ids[i];
			break;
		}
	}

	nand_chip->onfi_version = 0;
	
	/* Read entire ID string */
	nand_chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
	for (i = 0; i < 8; i++)
		id_data[i] = nand_chip->read_byte(mtd);

	if (!type)
		return ERR_PTR(-ENODEV);

	if (!mtd->name)
		mtd->name = type->name;

	nand_chip->chipsize = (uint64_t)type->chipsize << 20;

	/* Newer devices have all the information in additional id bytes */
	if (!type->pagesize) {
		int extid;
		/* The 3rd id byte holds MLC / multichip data */
		nand_chip->cellinfo = id_data[2];
		/* The 4th id byte is the important one */
		extid = id_data[3];

		/*
		 * Field definitions are in the following datasheets:
		 * Old style (4,5 byte ID): Samsung K9GAG08U0M (p.32)
		 * New style   (6 byte ID): Samsung K9GBG08U0M (p.40)
		 *
		 * Check for wraparound + Samsung ID + nonzero 6th byte
		 * to decide what to do.
		 */
		if (id_data[0] == id_data[6] && id_data[1] == id_data[7] &&
				id_data[0] == NAND_MFR_SAMSUNG &&
				(nand_chip->cellinfo & NAND_CI_CELLTYPE_MSK) &&
				id_data[5] != 0x00) {
			/* Calc pagesize */
			mtd->writesize = 2048 << (extid & 0x03);
			extid >>= 2;
			/* Calc oobsize */
			switch (extid & 0x03) {
			case 1:
				mtd->oobsize = 128;
				break;
			case 2:
				mtd->oobsize = 218;
				break;
			case 3:
				mtd->oobsize = 400;
				break;
			default:
				mtd->oobsize = 436;
				break;
			}
			extid >>= 2;
			/* Calc blocksize */
			mtd->erasesize = (128 * 1024) <<
				(((extid >> 1) & 0x04) | (extid & 0x03));
			busw = 0;
		} else {
			/* Calc pagesize */
			mtd->writesize = 1024 << (extid & 0x03);
			extid >>= 2;
			/* Calc oobsize */
			mtd->oobsize = (8 << (extid & 0x01)) *
				(mtd->writesize >> 9);
			extid >>= 2;
			/* Calc blocksize. Blocksize is multiples of 64KiB */
			mtd->erasesize = (64 * 1024) << (extid & 0x03);
			extid >>= 2;
			/* Get buswidth information */
			busw = (extid & 0x01) ? NAND_BUSWIDTH_16 : 0;
		}
	} else {
		/*
		 * Old devices have chip data hardcoded in the device id table.
		 */
		mtd->erasesize = type->erasesize;
		mtd->writesize = type->pagesize;
		mtd->oobsize = mtd->writesize / 32;
		busw = type->options & NAND_BUSWIDTH_16;

		/*
		 * Check for Spansion/AMD ID + repeating 5th, 6th byte since
		 * some Spansion chips have erasesize that conflicts with size
		 * listed in nand_ids table.
		 * Data sheet (5 byte ID): Spansion S30ML-P ORNAND (p.39)
		 */
		if (*maf_id == NAND_MFR_AMD && id_data[4] != 0x00 &&
				id_data[5] == 0x00 && id_data[6] == 0x00 &&
				id_data[7] == 0x00 && mtd->writesize == 512) {
			mtd->erasesize = 128 * 1024;
			mtd->erasesize <<= ((id_data[3] & 0x03) << 1);
		}
	}


	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	printk(KERN_INFO "NAND device: Manufacturer ID:"
		       " 0x%02x, Chip ID: 0x%02x (%s %s)\n", *maf_id,
		       dev_id, nand_manuf_ids[maf_idx].name, mtd->name);

	
 
	return type;
}


