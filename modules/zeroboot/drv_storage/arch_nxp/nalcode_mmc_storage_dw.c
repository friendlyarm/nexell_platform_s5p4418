/**    
    @file     nalcode_mmc_storage_imx6.c
    @date     2014/04/10
    @author   jonghooy@sevo-systems Sevo Systems Co.,Ltd.
    @brief    i.mx6q nalcode mmc storage  driver
    @todo     
    @bug     
    @remark   
    
    @warning 
*/
//
//  \C0\FA\C0۱\C7    \BF\A1\C7\C1\BF\A1\C0̸\AE\B4\AA\BD\BA(\C1\D6) / \BC\BC\BA\B8\BDý\BA\C5\DB\C1\EE
//            \BFܺΰ\F8\B0\B3 \B1\DD\C1\F6
//
//----------------------------------------------------------------------------


#include <typedef.h>
#include <zb_blk.h>
#include <zbi.h>
#include "mmc.h"
#include <zb_mmc.h>
#include <stdio.h>
#include <string.h>
#include <uart.h>
#include "nalcode_dwmmc.h"
#include "armv7.h"
#include <nalcode_storage.h>

#define STORAGE_NAME			"mmc driver for nxp4418"
#define WRITE_PROTECT			0			// never write


#define	put_raw_str uart_puts
#define	put_raw_hex32 uart_put_hex

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define DWMCI_CLKSEL			0x09C
#define DWMCI_SHIFT_0			0x0
#define DWMCI_SHIFT_1			0x1
#define DWMCI_SHIFT_2			0x2
#define DWMCI_SHIFT_3			0x3
#define DWMCI_SET_SAMPLE_CLK(x)	(x)
#define DWMCI_SET_DRV_CLK(x)	((x) << 16)
#define DWMCI_SET_DIV_RATIO(x)	((x) << 24)

struct dw_mmc_regs {
	u32 dbAddrVal; // 0x88
	u32 controlVal; //0x00
	u32 rintstsVal; //0x44
	u32 mintstsVal;
	u32 dscAddrVal; //0x94
	u32 bufAddrVal; //0x98
	u32 uhsRegVal; //0x74
	u32 bmModeVal; //0x80
	u32 intMaskVal;
	u32 statusVal;
	u32 idintenVal;
	u32 bytecntVal;
	u32 blkszVal;
	u32 cmdVal; 
};

#define DW_MMC_DMA_COUNT_MAX	2048
u32 dw_mmc_start_block_index = 0;
u32 dw_mmc_dma_addr_count = 0;
u32 dw_mmc_dma_addr_list[DW_MMC_DMA_COUNT_MAX];

struct dw_mmc_regs mmc_backup_regs;
struct dw_mmc_regs mmc_dirtry_check_regs_zero;
struct dw_mmc_regs mmc_dirtry_check_regs_cur;

struct zb_priv_mmc *zb_mmc;
struct zb_priv_mmc zero_mmc;
#define MMC_REGACY_CLOCK		400000

typedef unsigned long lbaint_t;

unsigned int nal_m2n_get_max_page_offset(void);

static u32  mmc_block_size =  512;  
#define MMC_START_BLOCK			(ZB_STORAGE_OFFSET/mmc_block_size)
#define MMC_TOTAL_BLOCK_COUNT	1024 //FIXME

#if 1
#define __be32_to_cpu(x) ((u32)(                         \
        (((u32)(x) & (u32)0x000000ffUL) << 24) |            \
        (((u32)(x) & (u32)0x0000ff00UL) <<  8) |            \
        (((u32)(x) & (u32)0x00ff0000UL) >>  8) |            \
         (((u32)(x) & (u32)0xff000000UL) >> 24)))
#endif


// CACHE RELATED ////////////////// copied from arch/arm/cpu/slsiap/s5p4418/cache_v7.c


#define ARMV7_DCACHE_INVAL_ALL		1
#define ARMV7_DCACHE_CLEAN_INVAL_ALL	2
#define ARMV7_DCACHE_INVAL_RANGE	3
#define ARMV7_DCACHE_CLEAN_INVAL_RANGE	4

static inline s32 log_2_n_round_up(u32 n)
{
        s32 log2n = -1;
        u32 temp = n;

        while (temp) {
                log2n++;
                temp >>= 1;
        }

        if (n & (n - 1))
                return log2n + 1; /* not power of 2 - round up */
        else
                return log2n; /* power of 2 */
}

static u32 get_clidr(void)
{
	u32 clidr;

	/* Read current CP15 Cache Level ID Register */
	asm volatile ("mrc p15,1,%0,c0,c0,1" : "=r" (clidr));
	return clidr;
}

static void set_csselr(u32 level, u32 type)
{	u32 csselr = level << 1 | type;

	/* Write to Cache Size Selection Register(CSSELR) */
	asm volatile ("mcr p15, 2, %0, c0, c0, 0" : : "r" (csselr));
}

static u32 get_ccsidr(void)
{
	u32 ccsidr;

	/* Read current CP15 Cache Size ID Register */
	asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr));
	return ccsidr;
}

static void v7_inval_dcache_level_setway(u32 level, u32 num_sets,
					 u32 num_ways, u32 way_shift,
					 u32 log2_line_len)
{
	int way, set, setway;

	/*
	 * For optimal assembly code:
	 *	a. count down
	 *	b. have bigger loop inside
	 */
	for (way = num_ways - 1; way >= 0 ; way--) {
		for (set = num_sets - 1; set >= 0; set--) {
			setway = (level << 1) | (set << log2_line_len) |
				 (way << way_shift);
			/* Invalidate data/unified cache line by set/way */
			asm volatile ("	mcr p15, 0, %0, c7, c6, 2"
					: : "r" (setway));
		}
	}
	/* DSB to make sure the operation is complete */
	CP15DSB;
}

static void v7_clean_inval_dcache_level_setway(u32 level, u32 num_sets,
					       u32 num_ways, u32 way_shift,
					       u32 log2_line_len)
{
	int way, set, setway;

	/*
	 * For optimal assembly code:
	 *	a. count down
	 *	b. have bigger loop inside
	 */
	for (way = num_ways - 1; way >= 0 ; way--) {
		for (set = num_sets - 1; set >= 0; set--) {
			setway = (level << 1) | (set << log2_line_len) |
				 (way << way_shift);
			/*
			 * Clean & Invalidate data/unified
			 * cache line by set/way
			 */
			asm volatile ("	mcr p15, 0, %0, c7, c14, 2"
					: : "r" (setway));
		}
	}
	/* DSB to make sure the operation is complete */
	CP15DSB;
}

static void v7_maint_dcache_level_setway(u32 level, u32 operation)
{
	u32 ccsidr;
	u32 num_sets, num_ways, log2_line_len, log2_num_ways;
	u32 way_shift;

	set_csselr(level, ARMV7_CSSELR_IND_DATA_UNIFIED);

	ccsidr = get_ccsidr();

	log2_line_len = ((ccsidr & CCSIDR_LINE_SIZE_MASK) >>
				CCSIDR_LINE_SIZE_OFFSET) + 2;
	/* Converting from words to bytes */
	log2_line_len += 2;

	num_ways  = ((ccsidr & CCSIDR_ASSOCIATIVITY_MASK) >>
			CCSIDR_ASSOCIATIVITY_OFFSET) + 1;
	num_sets  = ((ccsidr & CCSIDR_NUM_SETS_MASK) >>
			CCSIDR_NUM_SETS_OFFSET) + 1;
	/*
	 * According to ARMv7 ARM number of sets and number of ways need
	 * not be a power of 2
	 */
	log2_num_ways = log_2_n_round_up(num_ways);

	way_shift = (32 - log2_num_ways);
	if (operation == ARMV7_DCACHE_INVAL_ALL) {
		v7_inval_dcache_level_setway(level, num_sets, num_ways,
				      way_shift, log2_line_len);
	} else if (operation == ARMV7_DCACHE_CLEAN_INVAL_ALL) {
		v7_clean_inval_dcache_level_setway(level, num_sets, num_ways,
						   way_shift, log2_line_len);
	}
}


static void v7_maint_dcache_all(u32 operation)
{
	u32 level, cache_type, level_start_bit = 0;

	u32 clidr = get_clidr();

	for (level = 0; level < 7; level++) {
		cache_type = (clidr >> level_start_bit) & 0x7;
		if ((cache_type == ARMV7_CLIDR_CTYPE_DATA_ONLY) ||
		    (cache_type == ARMV7_CLIDR_CTYPE_INSTRUCTION_DATA) ||
		    (cache_type == ARMV7_CLIDR_CTYPE_UNIFIED))
			v7_maint_dcache_level_setway(level, operation);
		level_start_bit += 3;
	}
}


void v7_outer_cache_flush_all(void) {}
/*
 * Performs a clean & invalidation of the entire data cache
 * at all levels
 */
void flush_dcache_all(void)
{
#ifdef NAL_PRE_LOAD
	v7_maint_dcache_all(ARMV7_DCACHE_CLEAN_INVAL_ALL);

	v7_outer_cache_flush_all();
#endif
}

static void v7_dcache_clean_inval_range(u32 start,
                                        u32 stop, u32 line_len)
{
        u32 mva;

        /* Align start to cache line boundary */
        start &= ~(line_len - 1);
        for (mva = start; mva < stop; mva = mva + line_len) {
                /* DCCIMVAC - Clean & Invalidate data cache by MVA to PoC */
                asm volatile ("mcr p15, 0, %0, c7, c14, 1" : : "r" (mva));
        }
}

static void v7_dcache_inval_range(u32 start, u32 stop, u32 line_len)
{
        u32 mva;

        /*
         * If start address is not aligned to cache-line do not
         * invalidate the first cache-line
         */
        if (start & (line_len - 1)) {
                //printf("ERROR: %s - start address is not aligned - 0x%08x\n",
                //        __func__, start);
                /* move to next cache line */
                start = (start + line_len - 1) & ~(line_len - 1);
        }

        /*
         * If stop address is not aligned to cache-line do not
         * invalidate the last cache-line
         */
        if (stop & (line_len - 1)) {
                //printf("ERROR: %s - stop address is not aligned - 0x%08x\n",
                //        __func__, stop);
                /* align to the beginning of this cache line */
                stop &= ~(line_len - 1);
        }

        for (mva = start; mva < stop; mva = mva + line_len) {
                /* DCIMVAC - Invalidate data cache by MVA to PoC */
                asm volatile ("mcr p15, 0, %0, c7, c6, 1" : : "r" (mva));
        }
}


static void v7_dcache_maint_range(u32 start, u32 stop, u32 range_op)
{
        u32 line_len, ccsidr;

        ccsidr = get_ccsidr();
        line_len = ((ccsidr & CCSIDR_LINE_SIZE_MASK) >>
                        CCSIDR_LINE_SIZE_OFFSET) + 2;
        /* Converting from words to bytes */
        line_len += 2;
        /* converting from log2(linelen) to linelen */
        line_len = 1 << line_len;

        switch (range_op) {
        case ARMV7_DCACHE_CLEAN_INVAL_RANGE:
                v7_dcache_clean_inval_range(start, stop, line_len);
                break;
        case ARMV7_DCACHE_INVAL_RANGE:
                v7_dcache_inval_range(start, stop, line_len);
                break;
        }

        /* DSB to make sure the operation is complete */
        CP15DSB;
}


void v7_outer_cache_inval_range(u32 start, u32 stop) {}
void v7_outer_cache_flush_range(u32 start, u32 stop) {}
void v7_outer_cache_inval_all(void){}

/*
 * Invalidates range in all levels of D-cache/unified cache used:
 * Affects the range [start, stop - 1]
 */
void invalidate_dcache_range(unsigned long start, unsigned long stop)
{

        v7_dcache_maint_range(start, stop, ARMV7_DCACHE_INVAL_RANGE);

        v7_outer_cache_inval_range(start, stop);
}

void flush_dcache_range(unsigned long start, unsigned long stop)
{
        v7_dcache_maint_range(start, stop, ARMV7_DCACHE_CLEAN_INVAL_RANGE);

        v7_outer_cache_flush_range(start, stop);
}

void invalidate_dcache_all(void)
{
        v7_maint_dcache_all(ARMV7_DCACHE_INVAL_ALL);

        v7_outer_cache_inval_all();
}


static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
		"bne 1b" : "=r" (loops) : "0"(loops));
}


////////////////////// LOW LEVEL //////////////////////////////////////////////////////////////////////////
static int dwmci_wait_reset(struct zb_priv_mmc *mmc, u32 value)
{
	unsigned long timeout = 1000;
	u32 ctrl;

	dwmci_writel(mmc, DWMCI_CTRL, value);

	while (timeout--) {
		ctrl = dwmci_readl(mmc, DWMCI_CTRL);
		if (!(ctrl & DWMCI_RESET_ALL))
			return 1;
		delay(100);
	}
	 put_raw_str("Timeout on reset fifo\n");
	return 0;
}

static void dwmci_set_idma_desc(struct dwmci_idmac *idmac,
		u32 desc0, u32 desc1, u32 desc2)
{
	struct dwmci_idmac *desc = idmac;

	desc->flags = desc0;
	desc->cnt = desc1;
	desc->addr = desc2;
	desc->next_addr = (unsigned int)desc + sizeof(struct dwmci_idmac);
}

static void dwmci_prepare_data(struct zb_priv_mmc *mmc,
			       struct mmc_data *data,
			       struct dwmci_idmac *cur_idmac,
			       void *bounce_buffer)
{
	unsigned long ctrl;
	unsigned int i = 0, flags, cnt, blk_cnt;
	ulong data_start, data_end;


	blk_cnt = data->blocks;

	dwmci_wait_reset(mmc, DWMCI_CTRL_FIFO_RESET);

	data_start = (ulong)cur_idmac;
	dwmci_writel(mmc, DWMCI_DBADDR, (unsigned int)mmc->idmac_buf_phys);

	do {
		flags = DWMCI_IDMAC_OWN | DWMCI_IDMAC_CH ;
		flags |= (i == 0) ? DWMCI_IDMAC_FS : 0;
		if (blk_cnt <= 8) {
			flags |= DWMCI_IDMAC_LD;
			cnt = data->blocksize * blk_cnt;
		} else
			cnt = data->blocksize * 8;
#ifndef NAL_PRE_LOAD
		dwmci_set_idma_desc(cur_idmac, flags, cnt,
				    (u32)bounce_buffer + (i * PAGE_SIZE));
#else
		//put_raw_str("dma treanfer3="); put_raw_hex32(dw_mmc_dma_addr_list[i] );put_raw_str("\n");
		dwmci_set_idma_desc(cur_idmac, flags, cnt,
				    (u32)dw_mmc_dma_addr_list[i]);

//		dwmci_set_idma_desc(cur_idmac, flags, cnt,
//				    (u32)bounce_buffer + (i * PAGE_SIZE));
#endif
		if (blk_cnt <= 8)
			break;
		blk_cnt -= 8;
		cur_idmac++;
		i++;
	} while(1);
	//put_raw_str("do\n");

	data_end = (ulong)cur_idmac;

	flush_dcache_all();
	//flush_dcache_range( (unsigned int)mmc->idmac_buf_phys, (unsigned int)mmc->idmac_buf_phys + PAGE_SIZE);

	ctrl = dwmci_readl(mmc, DWMCI_CTRL);
	ctrl |= DWMCI_IDMAC_EN | DWMCI_DMA_EN;
	dwmci_writel(mmc, DWMCI_CTRL, ctrl);

	ctrl = dwmci_readl(mmc, DWMCI_BMOD);
	ctrl |= DWMCI_BMOD_IDMAC_FB | DWMCI_BMOD_IDMAC_EN;
	dwmci_writel(mmc, DWMCI_BMOD, ctrl);

	dwmci_writel(mmc, DWMCI_BLKSIZ, data->blocksize);
	dwmci_writel(mmc, DWMCI_BYTCNT, data->blocksize * data->blocks);
}

static int dwmci_set_transfer_mode(struct zb_priv_mmc *mmc,
		struct mmc_data *data)
{
	unsigned long mode;

	mode = DWMCI_CMD_DATA_EXP;
	if (data->flags & MMC_DATA_WRITE)
		mode |= DWMCI_CMD_RW;

	return mode;
}

static void freeze_interrupt(struct zb_priv_mmc *mmc)
{
	u32 ctrl;
	        
	ctrl = dwmci_readl(mmc, DWMCI_CTRL);
        ctrl &= ~(1<<4); // REMOVE Global Interrupt
        dwmci_writel(mmc, DWMCI_CTRL, ctrl);


}

void dummy_break0()
{
	return;
}

void dummy_break1()
{
        //put_raw_str("dummy1\n");
	return;
}

void dummy_break2()
{
	put_raw_str("dummy2\n");
        return;
}

int collision_flag =0;





static void backup_regs(struct zb_priv_mmc *mmc, struct dw_mmc_regs *reg)
{

        reg->dbAddrVal = dwmci_readl(mmc, DWMCI_DBADDR);
        reg->controlVal = dwmci_readl(mmc, DWMCI_CTRL);
        reg->rintstsVal = dwmci_readl(mmc, DWMCI_RINTSTS);
	reg->mintstsVal = dwmci_readl(mmc, DWMCI_MINTSTS);
        reg->dscAddrVal = dwmci_readl(mmc, DWMCI_DSCADDR);
        reg->bufAddrVal = dwmci_readl(mmc, DWMCI_BUFADDR);
        reg->uhsRegVal  = dwmci_readl(mmc, DWMCI_UHS_REG);
        reg->bmModeVal = dwmci_readl(mmc, DWMCI_BMOD);
        reg->intMaskVal = dwmci_readl(mmc, DWMCI_INTMASK);
        reg->statusVal =  dwmci_readl(mmc, DWMCI_STATUS);
	reg->idintenVal = dwmci_readl(mmc, DWMCI_IDINTEN);
	reg->bytecntVal = dwmci_readl(mmc,DWMCI_BYTCNT);
	reg->blkszVal = dwmci_readl(mmc,DWMCI_BLKSIZ);
	reg->cmdVal = dwmci_readl(mmc,DWMCI_CMD);
}

static void restore_regs(struct zb_priv_mmc *mmc, struct dw_mmc_regs *reg)
{
        dwmci_writel(mmc, DWMCI_DBADDR, (unsigned int) reg->dbAddrVal);
         dwmci_writel(mmc, DWMCI_CTRL, (unsigned int) reg->controlVal);
	//dwmci_writel(mmc, DWMCI_RINTSTS, (unsigned int) reg->rintstsVal);
	//dwmci_writel(mmc, DWMCI_MINTSTS, (unsigned int) reg->mintstsVal );

	//dwmci_writel(mmc, DWMCI_CTRL,0);
	//dwmci_writel(mmc, DWMCI_RINTSTS,0);
	//dwmci_writel(mmc, DWMCI_MINTSTS, 0 );

         dwmci_writel(mmc, DWMCI_DSCADDR, (unsigned int) reg->dscAddrVal);
         dwmci_writel(mmc, DWMCI_BUFADDR, (unsigned int) reg->bufAddrVal);
         dwmci_writel(mmc, DWMCI_UHS_REG, (unsigned int) reg->uhsRegVal);
         dwmci_writel(mmc, DWMCI_BMOD, (unsigned int) reg->bmModeVal);
         dwmci_writel(mmc, DWMCI_INTMASK, (unsigned int) reg->intMaskVal);
        dwmci_writel(mmc, DWMCI_STATUS, (unsigned int) reg->statusVal);
	dwmci_writel(mmc, DWMCI_IDINTEN, (unsigned int) reg->idintenVal);
	 dwmci_writel(mmc, DWMCI_BYTCNT, (unsigned int) reg->bytecntVal);
        dwmci_writel(mmc, DWMCI_BLKSIZ, (unsigned int) reg->blkszVal);
	//dwmci_writel(mmc, DWMCI_CMD, (unsigned int) reg->cmdVal);
	//dwmci_writel(mmc, DWMCI_CMD,0);

}


static int mmc_send_cmd(struct zb_priv_mmc *mmc,struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	//struct mmc_data backup;
	int ret;

	int flags = 0, i;
	unsigned int timeout = 100000;
	u32 retry = 10000;
	u32 mask, ctrl;
	u32 count=0;


//// START
	while (dwmci_readl(mmc, DWMCI_STATUS) & DWMCI_BUSY) {
		if (  count > retry )
		{
		
			//FIXME
			put_raw_str("Timeout on data busy\n");
			return -1;
		}
		count++;
		delay(1000);
	}

	
	dwmci_writel(mmc, DWMCI_RINTSTS, DWMCI_INTMSK_ALL);
	
	//dwmci_writel(mmc, DWMCI_CMD, 0);


	if (data) {
		
		
		dwmci_writel(mmc, DWMCI_DBADDR, (unsigned int)mmc->idmac_buf_phys);
#ifndef NAL_PRE_LOAD
		dwmci_prepare_data(mmc, data, mmc->idmac_buf, //JHYOON.. PHYS or VIRT???
				   mmc->data_buf_phys);
#else
	
		dwmci_prepare_data(mmc, data, mmc->idmac_buf, //JHYOON.. PHYS or VIRT???
				   mmc->data_buf);

#endif
		backup_regs(mmc,&mmc_dirtry_check_regs_zero);

	}

	dwmci_writel(mmc, DWMCI_CMDARG, cmd->cmdarg);

	if (data)
		flags = dwmci_set_transfer_mode(mmc, data);

	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY))
		return -1;

	if (cmd->cmdidx == MMC_CMD_STOP_TRANSMISSION)
		flags |= DWMCI_CMD_ABORT_STOP;
	else
		flags |= DWMCI_CMD_PRV_DAT_WAIT;

	if (cmd->resp_type & MMC_RSP_PRESENT) {
		flags |= DWMCI_CMD_RESP_EXP;
		if (cmd->resp_type & MMC_RSP_136)
			flags |= DWMCI_CMD_RESP_LENGTH;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= DWMCI_CMD_CHECK_CRC;

	flags |= (cmd->cmdidx | DWMCI_CMD_START | DWMCI_CMD_USE_HOLD_REG);

	//put_raw_str("Sending CMD%d\n",cmd->cmdidx);

	if ( data ) {
		if ( collision_flag )
                	dummy_break2();
        	else
                	dummy_break1();
	}

	dwmci_writel(mmc, DWMCI_CMD, flags);

	for (i = 0; i < retry; i++) {
		mask = dwmci_readl(mmc, DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			if (!data)
				dwmci_writel(mmc, DWMCI_RINTSTS, mask);
			break;
		}
		delay(1000);
	}

	if (i == retry) {
		put_raw_str("DWMCI_INTMSK_CDONE TIME OUT...!\n");
		return -1;
	}
	if (mask & DWMCI_INTMSK_RTO) {
		put_raw_str("Response Timeout..\n");
		return -1;
	} else if (mask & DWMCI_INTMSK_RE) {
		put_raw_str("Response Error..\n");
		return -1;
	}


	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			cmd->response[0] = dwmci_readl(mmc, DWMCI_RESP3);
			cmd->response[1] = dwmci_readl(mmc, DWMCI_RESP2);
			cmd->response[2] = dwmci_readl(mmc, DWMCI_RESP1);
			cmd->response[3] = dwmci_readl(mmc, DWMCI_RESP0);
		} else {
			cmd->response[0] = dwmci_readl(mmc, DWMCI_RESP0);
		}
	}

	if (data) {
		do {
			retry = 10000 * 100;
			mask = dwmci_readl(mmc, DWMCI_RINTSTS);
			if (mask & (DWMCI_DATA_ERR | DWMCI_DATA_TOUT)) {
				put_raw_str("DATA ERROR!\n");
				return -1;
			}
			delay(100000);
		} while (!(mask & DWMCI_INTMSK_DTO));
		if (retry ==0) {
			return -1;
		}
		if (data) backup_regs(mmc,&mmc_dirtry_check_regs_cur);
		
		dwmci_writel(mmc, DWMCI_RINTSTS, mask);

		ctrl = dwmci_readl(mmc, DWMCI_CTRL);
		ctrl &= ~(DWMCI_DMA_EN);
		dwmci_writel(mmc, DWMCI_CTRL, ctrl);
	
//			bounce_buffer_stop(&bbstate);	//FIXME
	}

	delay(100);

	return 0;

}



static int mmc_set_blocklen(struct zb_priv_mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;

	return mmc_send_cmd(mmc, &cmd, NULL);
}



static int mmc_send_status(struct zb_priv_mmc *mmc,int timeout)
{
	struct mmc_cmd cmd;
	int err, retries = 5;
#ifdef CONFIG_MMC_TRACE
	int status;
#endif

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;  //FIXME

	do {
		err = mmc_send_cmd(mmc,&cmd, NULL);
		if (!err) {
			if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
			    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
			     MMC_STATE_PRG)
				break;
			else if (cmd.response[0] & MMC_STATUS_MASK) {

						put_raw_str("Status Error: ");
						put_raw_str(cmd.response[0]);
						put_raw_str("\n");
				
				return COMM_ERR;
			}
		} else if (--retries < 0)
			return err;

		delay(1000);

	} while (timeout--);

	if (timeout <= 0) {
		put_raw_str("Timeout waiting card ready\n");
		return TIMEOUT;
	}

	return 0;
}



static int dwmci_setup_bus(struct zb_priv_mmc *mmc, u32 freq)
{
	u32 div, status;
	int timeout = 10000;

	if ( freq ==MMC_REGACY_CLOCK)  {  //FOR STARTUP
		div = 125; 
	} else {

		div = 1; 
	}	
	
	dwmci_writel(mmc, DWMCI_CLKENA, 0);
	dwmci_writel(mmc, DWMCI_CLKSRC, 0);

	dwmci_writel(mmc, DWMCI_CLKDIV, div);
	dwmci_writel(mmc, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
			DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

	do {
		status = dwmci_readl(mmc, DWMCI_CMD);
		if (timeout-- < 0) {
			put_raw_str("TIMEOUT error!!\n");
			return -1;
		}
	} while (status & DWMCI_CMD_START);

	dwmci_writel(mmc, DWMCI_CLKENA, DWMCI_CLKEN_ENABLE |
			DWMCI_CLKEN_LOW_PWR);

	dwmci_writel(mmc, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
			DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

	timeout = 10000;
	do {
		status = dwmci_readl(mmc, DWMCI_CMD);
		if (timeout-- < 0) {
			put_raw_str("TIMEOUT error!!\n");
			return -1;
		}
	} while (status & DWMCI_CMD_START);

	//host->clock = freq;

	return 0;
}

static int mmc_go_idle(struct zb_priv_mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	delay(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	delay(2000);

	return 0;
}

#define	DW_MMC_DRIVE_DELAY(n)		((n & 0xFF) << 0)	// write
#define	DW_MMC_DRIVE_PHASE(n)		((n & 0x03) <<16)	// write
#define	DW_MMC_SAMPLE_DELAY(n)		((n & 0xFF) << 8)	// read
#define	DW_MMC_SAMPLE_PHASE(n)		((n & 0x03) <<24)	// read




static void nxp_dwmci_clksel(struct zb_priv_mmc *mmc)
{
	u32 val;
	val = DWMCI_SET_SAMPLE_CLK(DWMCI_SHIFT_0) |
		DWMCI_SET_DRV_CLK(DWMCI_SHIFT_0) | DWMCI_SET_DIV_RATIO(3);

	dwmci_writel(mmc, DWMCI_CLKSEL, val);
}

static void dwmci_set_ios(struct zb_priv_mmc *mmc)
{
	u32 ctype, regs;

	
	dwmci_setup_bus(mmc, mmc->clock);

	switch (mmc->bus_width) {
	case 8:
		ctype = DWMCI_CTYPE_8BIT;
		break;
	case 4:
		ctype = DWMCI_CTYPE_4BIT;
		break;
	default:
		ctype = DWMCI_CTYPE_1BIT;
		break;
	}

	

	dwmci_writel(mmc, DWMCI_CTYPE, ctype);

#if 1 //CHECK ... 4430 - 4418 different
		regs = dwmci_readl(mmc, DWMCI_UHS_REG);
		//if (mmc->card_caps & MMC_MODE_DDR_52MHz)
		//	regs |= DWMCI_DDR_MODE;
		//else
			regs &= DWMCI_DDR_MODE;
	
		dwmci_writel(mmc, DWMCI_UHS_REG, regs);
#endif


	nxp_dwmci_clksel(mmc);


}

static int dwmci_init(struct zb_priv_mmc *mmc)
{
#if 0
	int fifoth_val = 0x200f0010;

	dwmci_writel(mmc, DWMCI_PWREN, 1);
	
#if 0  // Do Reset will Interface broken
	if (!dwmci_wait_reset(mmc, DWMCI_RESET_ALL)) {
		printk("%s[%d] Fail-reset!!\n",__func__,__LINE__);
		return -1;
	}
#endif
	/* Enumerate at 400KHz */
	dwmci_setup_bus(mmc, 50000000);

	dwmci_writel(mmc, DWMCI_RINTSTS, 0xFFFFFFFF);
	dwmci_writel(mmc, DWMCI_INTMASK, 0);

	dwmci_writel(mmc, DWMCI_TMOUT, 0xFFFFFFFF);

	dwmci_writel(mmc, DWMCI_IDINTEN, 0);
	dwmci_writel(mmc, DWMCI_BMOD, 1);

	dwmci_writel(mmc, DWMCI_FIFOTH,fifoth_val);

	dwmci_writel(mmc, DWMCI_CLKENA, 0);
	dwmci_writel(mmc, DWMCI_CLKSRC, 0);

	dwmci_set_ios(mmc);
#endif
	return 0;
}

#define WriteIODW(Addr,Data) (*(volatile unsigned int*)Addr)=((unsigned int)Data)

static int __dwmci_initialize(void)
{

	WriteIODW(0xf0012004, 0x130438b0);
		delay(10000);
	WriteIODW(0xf0012004, 0x13043ab0);

}

void mmc_set_clock(struct zb_priv_mmc *mmc, uint clock)
{
#if 0
	if (clock > mmc->f_max)
		clock = mmc->f_max;

	if (clock < mmc->f_min)
		clock = mmc->f_min;

#endif	
	mmc->clock = clock;

	dwmci_set_ios(mmc);
}

static void mmc_set_bus_width(struct zb_priv_mmc *mmc,unsigned int width)
{
	mmc->bus_width = width;
	
	dwmci_set_ios(mmc);
}




static int mmc_switch(struct zb_priv_mmc *mmc, u8 set, u8 index, u8 value)
{
	struct mmc_cmd cmd;
	int timeout = 1000;
	int ret;

	cmd.cmdidx = MMC_CMD_SWITCH;
	cmd.resp_type = MMC_RSP_R1b;
	cmd.cmdarg = (MMC_SWITCH_MODE_WRITE_BYTE << 24) |
				 (index << 16) |
				 (value << 8);

	ret = mmc_send_cmd(mmc, &cmd, NULL);

	/* Waiting for the ready status */
	if (!ret)
		ret = mmc_send_status(mmc, timeout);

	return ret;

}

static int mmc_send_ext_csd(struct zb_priv_mmc *mmc, u8 *ext_csd)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err;

	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	data.dest = (char *)ext_csd;
	data.blocks = 1;
	data.blocksize = MMC_MAX_BLOCK_LEN;
	data.flags = MMC_DATA_READ;

	err = mmc_send_cmd(mmc, &cmd, &data);

	return err;
}


static int mmc_change_freq(struct zb_priv_mmc *mmc)
{
	char cardtype;
	int err;
	u8* ext_csd; //FIXME.. 
	mmc->card_caps = 0;
	

	/* Only version 4 supports high-speed */
	if (mmc->version < MMC_VERSION_4)
		return 0;

//	err = mmc_send_ext_csd(mmc, ext_csd);

//	if (err)
//		return err;

//	cardtype = ext_csd[EXT_CSD_CARD_TYPE] & 0xf;

	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);

	if (err)
		return err;

	/* Now check to see that it worked */
//	err = mmc_send_ext_csd(mmc, ext_csd);

//	if (err)
//		return err;

#if 0
	/* No high-speed support */
	if (!mmc->ext_csd[EXT_CSD_HS_TIMING]) {
		 printk("CARD CAPS :  NOT HIGH SPEED\n");
		return 0;
	}
#endif

#if 0  //FIXME
	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & MMC_HS_52MHZ) {
		mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
		 printk("CARD CAPS :  MMC_HS_52MHZ\n");
	}
	else
		 {
			 mmc->card_caps |= MMC_MODE_HS;
			 printk("CARD CAPS : 52MHZ\n"); 
	}
#endif

	return 0;
}

/* We pass in the cmd since otherwise the init seems to fail */
static int mmc_send_op_cond_iter(struct zb_priv_mmc *mmc, struct mmc_cmd *cmd,
		int use_arg)
{
	int err;

	cmd->cmdidx = MMC_CMD_SEND_OP_COND;
	cmd->resp_type = MMC_RSP_R3;
	if ( !use_arg )  cmd->cmdarg = 0;
	else cmd->cmdarg = 0x40300080;
	/*if (use_arg && !mmc_host_is_spi(mmc)) {
		cmd->cmdarg =
			(mmc->cfg->voltages &
			(mmc->op_cond_response & OCR_VOLTAGE_MASK)) |
			(mmc->op_cond_response & OCR_ACCESS_MODE);

		if (mmc->cfg->host_caps & MMC_MODE_HC)
			cmd->cmdarg |= OCR_HCS;
	}*/
	err = mmc_send_cmd(mmc, cmd, NULL);
	if (err) {
		put_raw_str("Error Send op Cond. Iter\n");
		return err;
	}
	//mmc->op_cond_response = cmd->response[0];
	//return 0;
	 return cmd->response[0];
}

int mmc_send_op_cond(struct zb_priv_mmc *mmc)
{
	struct mmc_cmd cmd;
	int op_cond_response;
	int err, i;

	/* Some cards seem to need this */
	err = mmc_go_idle(mmc);
	if ( err ) return err;
 	/* Asking to the card its capabilities */
	//op_cond_pending = 1;
	for (i = 0; i < 2; i++) {
		op_cond_response = mmc_send_op_cond_iter(mmc, &cmd, i != 0);
		
		/* exit if not busy (flag seems to be inverted) */
		if (op_cond_response & OCR_BUSY) {
			put_raw_str("ERROR.. SEnd Op Cond\n");
			return 0;
		}
	}
	return IN_PROGRESS;
}

int mmc_complete_op_cond(struct zb_priv_mmc *mmc)
{
	struct mmc_cmd cmd;
	int timeout = 1000;
	uint start;
	int err;
	int ret;
	
	do {
		ret = mmc_send_op_cond_iter(mmc, &cmd, 1);
		
		delay(100);
		
	} while (!(ret & OCR_BUSY));

	
	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

static int mmc_startup(struct zb_priv_mmc *mmc)
{
	int err, i;
	uint mult, freq;
	u64 cmult, csize, capacity;
	struct mmc_cmd cmd;
	int timeout = 1000;


	/* Put the Card in Identify Mode */
	cmd.cmdidx = MMC_CMD_ALL_SEND_CID; /* cmd not supported in spi */
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err) {
		put_raw_str("startup Error\n");
		return err;
	}
//	memcpy(mmc->cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */

		cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.cmdarg = mmc->rca << 16;
		cmd.resp_type = MMC_RSP_R6;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			put_raw_str("startup mmc_send_cmd error\n");
			return err;
		}



	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	if (err) {
		put_raw_str("startup ready  error\n");
		return err;
		}

	
	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

	
		switch (version) {
			case 0:
				mmc->version = MMC_VERSION_1_2;
				break;
			case 1:
				mmc->version = MMC_VERSION_1_4;
				break;
			case 2:
				mmc->version = MMC_VERSION_2_2;
				break;
			case 3:
				mmc->version = MMC_VERSION_3;
				break;
			case 4:
				mmc->version = MMC_VERSION_4;
				break;
			default:
				mmc->version = MMC_VERSION_1_2;
				break;
		}
	}


	/* Select the card, and put it into Transfer Mode */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = mmc->rca << 16;
		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err) {
			put_raw_str("startup  Transfer Mode  error\n");
			return err;
		}
#if 0
		/* check  ext_csd version and capacity */
		printf("Func: %s, Line : %d\n",__func__, __LINE__);
		err = mmc_send_ext_csd(mmc, ext_csd);
		if (!err && (ext_csd[EXT_CSD_REV] >= 2)) {
			/*
			 * According to the JEDEC Standard, the value of
			 * ext_csd's capacity is valid if the value is more
			 * than 2GB
			 */
			capacity = ext_csd[EXT_CSD_SEC_CNT] << 0
					| ext_csd[EXT_CSD_SEC_CNT + 1] << 8
					| ext_csd[EXT_CSD_SEC_CNT + 2] << 16
					| ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
			capacity *= MMC_MAX_BLOCK_LEN;
			if ((capacity >> 20) > 2 * 1024)
				mmc->capacity_user = capacity;
		

		switch (ext_csd[EXT_CSD_REV]) {
		case 1:
			mmc->version = MMC_VERSION_4_1;
			break;
		case 2:
			mmc->version = MMC_VERSION_4_2;
			break;
		case 3:
			mmc->version = MMC_VERSION_4_3;
			break;
		case 5:
			mmc->version = MMC_VERSION_4_41;
			break;
		case 6:
			mmc->version = MMC_VERSION_4_5;
			break;
		}

		/*
		 * Host needs to enable ERASE_GRP_DEF bit if device is
		 * partitioned. This bit will be lost every time after a reset
		 * or power off. This will affect erase size.
		 */
		if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) &&
		    (ext_csd[EXT_CSD_PARTITIONS_ATTRIBUTE] & PART_ENH_ATTRIB)) {
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
				EXT_CSD_ERASE_GROUP_DEF, 1);

			if (err)
				return err;

			/* Read out group size from ext_csd */
			mmc->erase_grp_size =
				ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] *
					MMC_MAX_BLOCK_LEN * 1024;
		} else {
			/* Calculate the group size from the csd value. */
			int erase_gsz, erase_gmul;
			erase_gsz = (mmc->csd[2] & 0x00007c00) >> 10;
			erase_gmul = (mmc->csd[2] & 0x000003e0) >> 5;
			mmc->erase_grp_size = (erase_gsz + 1)
				* (erase_gmul + 1);
		}

		/* store the partition info of emmc */
		if ((ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) ||
		    ext_csd[EXT_CSD_BOOT_MULT])
			mmc->part_config = ext_csd[EXT_CSD_PART_CONF];

		mmc->capacity_boot = ext_csd[EXT_CSD_BOOT_MULT] << 17;

		mmc->capacity_rpmb = ext_csd[EXT_CSD_RPMB_MULT] << 17;

		for (i = 0; i < 4; i++) {
			int idx = EXT_CSD_GP_SIZE_MULT + i * 3;
			mmc->capacity_gp[i] = (ext_csd[idx + 2] << 16) +
				(ext_csd[idx + 1] << 8) + ext_csd[idx];
			mmc->capacity_gp[i] *=
				ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE];
			mmc->capacity_gp[i] *= ext_csd[EXT_CSD_HC_WP_GRP_SIZE];
		}
	}


	err = mmc_set_capacity(mmc, mmc->part_num);
	if (err)
		return err;

#endif

		err = mmc_change_freq(mmc);

	if (err) {
		
		return err;
		}
		
			unsigned int extw;

			extw = 1;
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, extw);

			if (err) {
				put_raw_str("startup  switch  error\n");
				}

			mmc_set_bus_width(mmc, 4);

#if 0
			err = mmc_send_ext_csd(mmc, test_csd);
			if (!err && ext_csd[EXT_CSD_PARTITIONING_SUPPORT] \
				    == test_csd[EXT_CSD_PARTITIONING_SUPPORT]
				 && ext_csd[EXT_CSD_ERASE_GROUP_DEF] \
				    == test_csd[EXT_CSD_ERASE_GROUP_DEF] \
				 && ext_csd[EXT_CSD_REV] \
				    == test_csd[EXT_CSD_REV]
				 && ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] \
				    == test_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
				 && memcmp(&ext_csd[EXT_CSD_SEC_CNT], \
					&test_csd[EXT_CSD_SEC_CNT], 4) == 0) {

				mmc->card_caps |= ext_to_hostcaps[extw];
				break;
			}
#endif		

		

	mmc_set_clock(mmc, 50000000);


	return 0;
}

static int mmc_complete_init(struct zb_priv_mmc *mmc)
{
	int err = 0;
	//if (mmc->op_cond_pending) {
			
		err = mmc_complete_op_cond(mmc);
	//	}

	if (!err) {
			
		err = mmc_startup(mmc);
		}
	return err;
}


int mmc_start_init(struct zb_priv_mmc *mmc)
{
	int err;

	
	
	
	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);

	/* Reset the Card */
	err = mmc_go_idle(mmc);

	if (err)
		return err;
		err = mmc_send_op_cond(mmc);

	
	return err;
}


int mmc_card_bringup(struct zb_priv_mmc *mmc)
{
	
	if ( mmc_start_init(mmc) == -1 ) return -1;
	if ( mmc_complete_init(mmc) == -1 ) return -1;
	
}


static int dwmci_init_by_retry(struct zb_priv_mmc *mmc)
{


	
	//zb_mmc_retry_pre();

	__dwmci_initialize();

	dwmci_wait_reset(mmc,DWMCI_RESET_ALL);

	dwmci_writel(mmc, DWMCI_BMOD, DWMCI_BMOD_IDMAC_RESET);
	dwmci_writel(mmc,  DWMCI_IDINTEN, SDMMC_IDMAC_INT_NI | SDMMC_IDMAC_INT_RI |
		   SDMMC_IDMAC_INT_TI);
	dwmci_writel(mmc, DWMCI_DBADDR,mmc->host_sg_dma_addr);
	
	dwmci_writel(mmc, DWMCI_FIFOTH, 0x200f0010);
	dwmci_writel(mmc, DWMCI_CLKCTRL,DW_MMC_DRIVE_DELAY(0) | DW_MMC_SAMPLE_DELAY(0) | DW_MMC_DRIVE_PHASE(1) | DW_MMC_SAMPLE_PHASE(0));
	dwmci_writel(mmc, DWMCI_RINTSTS, 0xFFFFFFFF);
	dwmci_writel(mmc, DWMCI_INTMASK, DWMCI_INTMSK_CDONE | DWMCI_INTMSK_DTO |
			   DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR |
			   DWMCI_INTMSK_ERROR_FLAGS );


	mmc->bus_width=4;
	mmc->clock = MMC_REGACY_CLOCK;;

	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, MMC_REGACY_CLOCK);

	if ( mmc_card_bringup(mmc) == -1 ) return -1;
	dwmci_writel(mmc, DWMCI_CTRL, DWMCI_INT_ENABLE);


	return 0;
}




static int wait_busy(struct zb_priv_mmc *mmc)
{
        int count=0;
	int timeout = 300000;
	u32 status;

	collision_flag = 0;
#if 0  //Check CMD Busy
	do {
                status = dwmci_readl(mmc, DWMCI_CMD);
	      	if( status & DWMCI_CMD_START )	{
				collision_flag =1;
				 // printk("TDWMCI_CMD_START!!\n");
	      		}
               if (timeout-- < 0) {
                        printk("TIMEOUT erro!!\n");
                       return -1;
                }
		udelay(1);
        } while (status & DWMCI_CMD_START);
#endif

#if 1  //Check CMD Busy
	

		while ( dwmci_readl(mmc, DWMCI_CMD) & DWMCI_CMD_START) {
				
			  if (  count > timeout ) {
                     			   //FIXME
		                        put_raw_str("Command busy..\n");
		                        return -1;
		               }
			  count++;
				delay(100);
		}
#endif
	


	
#if 1 // CHECK DATA Busy
	count= 0;

	while (dwmci_readl(mmc, DWMCI_STATUS) & DWMCI_BUSY) {
                if (  count > timeout )
                {
                        //FIXME
                        put_raw_str("Timeout on data busy\n");
                        return -1;
                }
		collision_flag =1;		
                count++;
                delay(100);
        }
	
#endif
	
#if 0 // DMA BUSY
	while(1) {
		status = (dwmci_readl(mmc, DWMCI_IDSTS) & 0x1E000) >> 13;	
		if ( status == 0 ) break; //IDLE
		else {
			//printk("DMA Status: %8x\n", status);
			collision_flag =1;
		}
		udelay(1);;
	}
#endif

#if 1 // DMA BUSY
			count = 0;
			while((dwmci_readl(mmc, DWMCI_IDSTS) & 0x1E000) >> 13) {
				  if (  count > timeout ) {
                     			   //FIXME
		                        put_raw_str("DMA busy..\n");
		                        return -1;
		               	 }
				  count++;
				delay(1000);;
			}
#endif


		/*f ( collision_flag == 1 )	{
			printk("CMD Collision\n");
			udelay(10);
			
		}*/

		

        return 0;

}


static void unfreeze_interrupt(struct zb_priv_mmc *mmc)
{
	u32 ctrl;
	        
	ctrl = dwmci_readl(mmc, DWMCI_CTRL);
        ctrl |= (1<<4); // SET Global Interrupt
        dwmci_writel(mmc, DWMCI_CTRL, ctrl);


}





static int mmc_read_blocks( struct zb_priv_mmc *mmc,void *dst, ulong start,
			   lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_READ_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_READ_SINGLE_BLOCK;

	if ( mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start *  mmc->read_bl_len;

	cmd.resp_type = MMC_RSP_R1;

	data.dest = dst;
	data.blocks = blkcnt;
	data.blocksize =  mmc->read_bl_len;
	data.flags = MMC_DATA_READ;
	//data.phys_addr = mmc->data_buf_phys; //FIXME
	

	if (mmc_send_cmd(mmc,&cmd, &data))
		return 0;
	

	if (blkcnt > 1) {
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (mmc_send_cmd(mmc,&cmd, NULL)) {
			put_raw_str("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	return blkcnt;
}

static ulong mmc_bread(struct zb_priv_mmc *mmc, ulong start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;
	//unsigned long irq_flag;

	

	if ( wait_busy(mmc) == -1 ) return 0;
		

	//local_irq_save(irq_flag);
	//local_irq_disable();
	//BUF_STORAGE_ACCESS = STORAGE_ACCESS_MARK_NALCODE;
	//zb_buf_status = 1;
	//spin_lock_irqsave(&mLock, irq_flag);
	
	//if ( !skip_reg_backup ) backup_regs(mmc);
	backup_regs(mmc,&mmc_backup_regs);
	freeze_interrupt(mmc);

	//if (mmc_set_blocklen(mmc, mmc->read_bl_len))
	//	return 0;

	do {
		cur = blocks_todo;
		//printk("CUR, BLOCKS_TO_DO : %d\n",cur);
		if(mmc_read_blocks(mmc,dst, start, cur) != cur) {
			//restore_regs(mmc);
	//		local_irq_restore(irq_flag);
			//spin_unlock_irqrestore(&mLock, irq_flag);
			return 0;
		}
		blocks_todo -= cur;
		start += cur;
		dst += cur * mmc->read_bl_len;
	} while (blocks_todo > 0);
	

	 restore_regs(mmc,&mmc_backup_regs);
	
	//BUF_STORAGE_ACCESS = STORAGE_ACCESS_MARK_DONE;
	//zb_buf_status = 0;
//	local_irq_restore(irq_flag);
	//spin_unlock_irqrestore(&mLock, irq_flag);
	return blkcnt;
}




#if 0
/*------------------------------------------------------------------------------
  @brief   \ud398\uc774\uc9c0 \uc8fc\uc18c\ub97c 4K \ub2e8\uc704\uc758 \ubc88\ud638\ub85c \ubcc0\ud658\ud574 \uc900\ub2e4
  @remark  
*///----------------------------------------------------------------------------
static unsigned int mmc_phys_to_idx(unsigned int phys)
{
	unsigned int unit;

	unit = ( PAGE_SIZE/mmc_block_size );
	return ( (phys - MMC_START_BLOCK) / unit );
}
#endif

/*-----------------------------	-------------------------------------------------
  @brief   4K 단위의 번호를 페이지 주소로 변환해 준다
  @remark  
*///----------------------------------------------------------------------------
static unsigned int mem_index_to_mmc_phys(unsigned int mem_page)
{
	unsigned int offset_mmc_block;

	offset_mmc_block = mem_page * (PAGE_SIZE/mmc_block_size);
//printk( "-----%s(%d) offset_nand_page=%d  NAND_START_PAGE=%d\n", __func__,__LINE__, offset_nand_page, NAND_START_PAGE );	
	return MMC_START_BLOCK + offset_mmc_block;
}

int dma_read_dirty(struct zb_priv_mmc *mmc)
{
	//put_raw_hex32(zero_mmc.high_capacity);
	if ( mmc_dirtry_check_regs_zero.bufAddrVal != 0 && (mmc_dirtry_check_regs_zero.bufAddrVal != mmc_dirtry_check_regs_cur.bufAddrVal)) {
			put_raw_str("DIRTY:"); put_raw_hex32(mmc_dirtry_check_regs_zero.bufAddrVal);
			put_raw_str("AFTER="); put_raw_hex32(mmc_dirtry_check_regs_cur.bufAddrVal);put_raw_str("\n");
			return -1;
		}
	
	return 0;
	
}

/*------------------------------------------------------------------------------
  @brief   \uba54\ubaa8\ub9ac 4K \ud398\uc774\uc9c0 \ub2e8\uc704\ub85c \uc77d\ub294\ub2e4.
  @remark  
*///----------------------------------------------------------------------------
#ifdef NAL_PRE_LOAD
static int mmc_read_block_4k_preload( u32 mem_page,  u8 *page_buf, unsigned int paddr)
{
	u32 count;
	u32 block_index;
	u8	*data;
	u32 start;
	u32 retry_count=0;
	u32 reset_retry_count=0;
	block_index 	= mem_index_to_mmc_phys(mem_page);;
	data		= page_buf;
	count		= PAGE_SIZE/mmc_block_size;

READ_RETRY:

	while (mmc_bread(&zero_mmc,block_index,count,(void *)zero_mmc.data_buf) == 0) {
		retry_count++;

		if ( retry_count >=10 ) {
			put_raw_str("RETRY ERROR Count Limit Over\n");
			return -1;
		}

		while ( dwmci_init_by_retry(&zero_mmc) ==  -1 ) {
			reset_retry_count++;
			put_raw_str ("reset_retry\n");
			if ( reset_retry_count >=10 ) {
				put_raw_str ("reset_retry counter over \n");
				return -1;
			}
			delay(100);
		}
	}

	if (dma_read_dirty(&zero_mmc) == -1) { 
		goto READ_RETRY;
	}

	memcpy(data,zero_mmc.data_buf, count*mmc_block_size);
	flush_dcache_all();
	return count;	
}
#else
#include <dma_api.h>
static int mmc_read_block_4k_nalload( u32 mem_page,  u8 *page_buf, unsigned int paddr)
{
	u32 count;
	u32 block_index;
	u8	*data;
	u32 start;
	u32 retry_count=0;
	u32 reset_retry_count=0;
	block_index 	= mem_index_to_mmc_phys(mem_page);;
	data		= page_buf;
	count		= PAGE_SIZE/mmc_block_size;

READ_RETRY:

#if 1
	while (mmc_bread(&zero_mmc,block_index,count,(void *)zero_mmc.data_buf) == 0) {
#else
	while (mmc_bread(&zero_mmc,block_index,count,(void *)data) == 0) {
#endif
		retry_count++;

		if ( retry_count >=10 ) {
			put_raw_str("RETRY ERROR Count Limit Over\n");
			return -1;
		}

		while ( dwmci_init_by_retry(&zero_mmc) ==  -1 ) {
			reset_retry_count++;
			put_raw_str ("reset_retry\n");
			if ( reset_retry_count >=10 ) {
				put_raw_str ("reset_retry counter over \n");
				return -1;
			}
			delay(100);
		}
	}

	if (dma_read_dirty(&zero_mmc) == -1) { 
		goto READ_RETRY;
	}

	memcpy(data,zero_mmc.data_buf, count*mmc_block_size);
	start = (unsigned int)data;
//	//v7_dma_inv_range(start, start + PAGE_SIZE, 0, 0);
//	v7_inv_range(start, start + PAGE_SIZE, 0, 0);

	v7_dma_flush_range(start, start + PAGE_SIZE, 0, 0);

	return count;	
}
#endif
static int mmc_read_block_4k( u32 mem_page,  u8 *page_buf, unsigned int paddr)
{
#ifdef NAL_PRE_LOAD
	mmc_read_block_4k_preload(mem_page, page_buf, paddr);
#else
	mmc_read_block_4k_nalload(mem_page, page_buf, paddr);
#endif
}


#if 0
/*------------------------------------------------------------------------------
  @brief   페이지 주소를 4K 단위의 번호로 변환해 준다
  @remark  
*///----------------------------------------------------------------------------
static unsigned int mmc_phys_to_idx(unsigned int phys)
{
	unsigned int unit;

	unit = ( PAGE_SIZE/mmc_block_size );
	return ( (phys - MMC_START_BLOCK) / unit );
}
#endif


/*------------------------------------------------------------------------------
  @brief   저장소의 offset 값을 알려준다
  @remark  
*///----------------------------------------------------------------------------
static u32 mmc_get_storage_offset( void )
{
	return ZB_STORAGE_OFFSET;
}
/*------------------------------------------------------------------------------
  @brief   디바이스 제어를 위한 가상주소
  @remark  
*///----------------------------------------------------------------------------
static u32 mmc_get_io_vaddr(void)
{
	return (u32)0;
}

static int mmc_post_zeroboot( void )
{
	
	return 0;
}


void get_zb_mmc_info() {
	
#ifndef NAL_PRE_LOAD
	memcpy(&zero_mmc, zb_mmc, sizeof(struct zb_priv_mmc));
#endif

#ifndef NAL_PRE_LOAD
		


#else
	zero_mmc.ioaddr_phys = (unsigned int) zb_mmc->ioaddr_phys;
	zero_mmc.data_buf_phys = (unsigned int) zb_mmc->data_buf_phys;
	zero_mmc.idmac_buf_phys = (unsigned int) zb_mmc->idmac_buf_phys;
	zero_mmc.read_bl_len = (unsigned int) zb_mmc->read_bl_len;
	zero_mmc.write_bl_len = (unsigned int) zb_mmc->write_bl_len;
	zero_mmc.high_capacity = (unsigned int) zb_mmc->high_capacity;
	zero_mmc.rca = (unsigned int) zb_mmc->rca;
	zero_mmc.card_caps = (unsigned int) zb_mmc->card_caps;
	zero_mmc.host_sg_dma_addr = (unsigned int) zb_mmc->host_sg_dma_addr;
	


	zero_mmc.ioaddr = (unsigned int) zb_mmc->ioaddr_phys;
	zero_mmc.data_buf =  (unsigned int) zb_mmc->data_buf_phys;
	zero_mmc.idmac_buf=  (unsigned int) zb_mmc->idmac_buf_phys;
#endif

}



// nalcode first call setup
void nalcode_storage_first_call(void)
{
 	zb_mmc = (struct zb_priv_mmc *)get_storage_priv_offset();
	put_raw_str( " zb_MMC_ADDRESS : "); put_raw_hex32( zb_mmc);
	get_zb_mmc_info();

	put_raw_str( "> mmc_io_addr			= "); put_raw_hex32( zero_mmc.ioaddr);
	put_raw_str( "> mmc_io_addr_phys			= "); put_raw_hex32( zero_mmc.ioaddr_phys);
	put_raw_str( "> mmc_data_buf_phys			= "); put_raw_hex32( zero_mmc.data_buf_phys);
	put_raw_str( "> mmc_idmac_phys			= "); put_raw_hex32( zero_mmc.idmac_buf_phys);
	put_raw_str( "> mmc_read_bl_len			= "); put_raw_hex32( zero_mmc.read_bl_len);
	put_raw_str( "> mmc_write_bl_len			= "); put_raw_hex32( zero_mmc.write_bl_len);
	put_raw_str( "> mmc_erase_grp_size			= "); put_raw_hex32( zero_mmc.write_bl_len);
	put_raw_str( "> mmc_high_capacity			= "); put_raw_hex32(zero_mmc.high_capacity);
	put_raw_str( "> mmc_rca			= "); put_raw_hex32( zero_mmc.rca);
	put_raw_str( "> mmc_card_caps			= "); put_raw_hex32( zero_mmc.card_caps);
	put_raw_str( "> mmc_host_sg_dma_addr			= "); put_raw_hex32( zero_mmc.host_sg_dma_addr);
	
#ifdef NAL_PRE_LOAD
	if (dwmci_init(&zero_mmc) == 0) {

		uart_puts("MMC INIT  SUCCESS_1\n");
	} else {
		put_raw_str( "MMC detect Failed!! ");
	}
#endif

}


#ifndef NAL_PRE_LOAD
inline static void __nalcode_stroage_access_begin( void )
{
#if 1
	// FX
	
	//zb_storage_access_enter_mark( STORAGE_ACCESS_MARK_NALCODE); 
	BUF_STORAGE_ACCESS = STORAGE_ACCESS_MARK_NALCODE; 
	
#endif

}

inline static void __nalcode_stroage_access_end( void )
{
	
		// DO NOTHING..
}
#else
inline static void __nalcode_stroage_access_begin( void ) {}
inline static void __nalcode_stroage_access_end( void ) {}
#endif

#ifndef NAL_PRE_LOAD
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

//#include "nalcode_mem_storage.c"
//#define	ZB_MEM_BLK_SIZE		(SZ_256M + SZ_128M + SZ_32M)
#define	ZB_MEM_BLK_SIZE		(SZ_512M)
#define	MEM_STORAGE_BASE	(SZ_1G - ZB_MEM_BLK_SIZE)

void mem_storage_block_4k(u32 mem_page_offset, u8 *mem_buf)
{
	unsigned long base = PAGE_OFFSET + MEM_STORAGE_BASE;
	u32 start;

	base += 0x1000 * mem_page_offset;

	if (base >= 0xc0000000) {
		uart_puts("HANG ADDRESS!!!!!!!!!!!!!!!!\n");
		while (1);
	}
		
	memcpy(mem_buf, (void *)base, PAGE_SIZE);

#ifndef NAL_PRE_LOAD
	start = (unsigned int)mem_buf;
	v7_dma_flush_range(start, start + PAGE_SIZE, 0, 0);
#endif
 }
#endif
void nalcode_stroage_read_4k_page( u32 mem_page_offset, u8 *mem_buf, void *vaddr , unsigned int paddr)
{
#if defined(DEBUG_MEM_STORAGE) && !defined(NAL_PRE_LOAD)
//#ifndef NAL_PRE_LOAD
		mem_storage_block_4k(mem_page_offset, mem_buf);
#else
		__nalcode_stroage_access_begin();
		{
			mmc_read_block_4k(mem_page_offset,mem_buf, paddr);
			//put_raw_str( "*\n");
			
		}
		__nalcode_stroage_access_end();
#endif
}
void nalcode_stroage_test_function(void)
{
	
	nalcode_storage_first_call();
	//nand_verify();
}

#ifdef NAL_PRE_LOAD
/* function : zb_init_dma_list
 * desc		: init dma count
 * return	: none
 */
void zb_init_dma_list(void)
{
	put_raw_str("zb_init_dma_list Called="); 

	dw_mmc_start_block_index = 0;
	dw_mmc_dma_addr_count = 0;

}
/* function : zb_add_dma_list
 * desc		: add one dma list
 * blkinx	: block index
 * phys		: physical memory address
 * return	: dma count reach max 1
 * 			  ohterwise 0
 */
void zb_add_dma_list(unsigned int blkidx, unsigned int phys)
{
	if ( dw_mmc_dma_addr_count == 0 ) {
		int block_index;
		block_index 	= mem_index_to_mmc_phys(blkidx);
		dw_mmc_start_block_index = block_index;
	}
#if 1 // block continues check..
	if ( dw_mmc_dma_addr_count >=  DW_MMC_DMA_COUNT_MAX ) put_raw_str("HANG ADDRESS!--- OVER MAX!!!!!!!!!!!!!!!\n");
#endif
	dw_mmc_dma_addr_list[dw_mmc_dma_addr_count] = phys;
	dw_mmc_dma_addr_count++;
	
}
/* function : zb_request_dma_transfer
 * desc		: request transfer all dma list
 * return	: none
 */
void zb_request_dma_transfer(void)
{
	if ( dw_mmc_dma_addr_count == 0  )  {
		put_raw_str("dma count is 0 ================================> Error");
	}  else {
	  	put_raw_str("dma count ="); put_raw_hex32(dw_mmc_dma_addr_count);put_raw_str("\n");
	  }
	if ( mmc_read_blocks(&zero_mmc,(void *)NULL, dw_mmc_start_block_index, dw_mmc_dma_addr_count * 8) != dw_mmc_dma_addr_count* 8) {
		put_raw_str("DMA READ ERROR !!!! \n");
	}
	put_raw_str("zb_request_dma_transfer end.\n");
}
#endif


