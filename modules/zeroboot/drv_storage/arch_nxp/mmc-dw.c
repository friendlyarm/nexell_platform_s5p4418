/*-----------------------------------------------------------------------------
  \ud30c \uc77c : mmc-s5pv210.c
  \uc124 \uba85 : 
  \uc791 \uc131 : frog@falinux.com
          freefrug@falinux.com
  \ub0a0 \uc9dc : 2012-06-29
  \uc8fc \uc758 :
		  MMC
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
#include <asm/delay.h>
#include <linux/jiffies.h>
#include<linux/spinlock.h>
#include "armv7.h"

#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/kthread.h>


#include <zb_blk.h>
#include <zbi.h>
#include <zb_mmc.h>
#include "mmc.h"
#include "dwmmc.h"
#include "nalcode_storage.h"

#include <linux/dma-mapping.h>
#include <zb_snapshot.h>

#define DWMCI_CLKSEL			0x09C
#define DWMCI_SHIFT_0			0x0
#define DWMCI_SHIFT_1			0x1
#define DWMCI_SHIFT_2			0x2
#define DWMCI_SHIFT_3			0x3
#define DWMCI_SET_SAMPLE_CLK(x)	(x)
#define DWMCI_SET_DRV_CLK(x)	((x) << 16)
#define DWMCI_SET_DIV_RATIO(x)	((x) << 24)

//#define READ_WRITE_TEST	1

#define MMC_REGACY_CLOCK		400000

#define STORAGE_NAME			"mmc driver for nxp5430"
#define WRITE_PROTECT			0			// never write

//#define USE_BUFFER_MEMORY		1

#define MEMORY_BUFFER_PAGE_COUNT	1

struct zb_priv_mmc *zb_mmc;

typedef ulong lbaint_t;

extern unsigned long volatile jiffies;
BUF_STORAGE_ACCESS;

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

struct dw_mmc_regs mmc_backup_regs;
struct dw_mmc_regs mmc_dirtry_check_regs_zero;
struct dw_mmc_regs mmc_dirtry_check_regs_cur;

int gNeed_init_sd_clock	= 0;


/// @{
/// @brief  \ubcc0\uc218\uc815\uc758
///-----------------------------------------------------------------------------

	
struct task_struct *g_th_id=NULL;

/// @}
//#define TIME_DEBUG	1
#define TEST_COUNT		10

#ifdef TIME_DEBUG
	#define JIFFY_PRINT	printk("Func: %s, Line: %d,  jiffies : %d\n",__func__, __LINE__, jiffies);
#else
	#define JIFFY_PRINT {};
#endif
static struct zb_priv_mmc zero_mmc;

static u32  mmc_block_size =  512;  
#define MMC_START_BLOCK			(ZB_STORAGE_OFFSET/mmc_block_size)
#define MMC_TOTAL_BLOCK_COUNT	1024 //FIXME


#define PROC_DW_MMC_NAME	"mmc_test"

static DEFINE_MUTEX(zb_storage_mutex);

int gContinue =0 ;


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


static int dw_mmc_proc_read(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
	char *p;

	p = buf;

	p += sprintf(p, "\n" );
	p += sprintf(p, "it is command of dw mmc storage test control\n" );
	p += sprintf(p, "usage) echo 'start'   > /proc/mmc_test\n" );
	p += sprintf(p, "       echo 'stop' > /proc/mmc_test\n" );
	p += sprintf(p, "\n" );

	*eof = 1;

	return p - buf;
}

extern int zb_dw_mmc_clean(void);
//extern void zb_mmc_retry_after();
//extern void zb_mmc_retry_pre();

static int dw_mmc_proc_write( struct file *file, const char __user *buf, unsigned long count, void *data)
{
	char	cmd[256];
	int		wait_sec = 1;
	int		len;
	unsigned long subaddress = 0;

	memset( cmd, 0, sizeof( cmd ) );
	if ( count > sizeof( cmd ) ) len = sizeof( cmd );
	else						 len = count;	

	if (copy_from_user( cmd, buf, len ))
		return -EFAULT;

	cmd[len-1] = 0;	// CR 제거
	printk( "write command = [%s]\n", cmd );

	if      ( 0 == strncmp( "start", cmd, 5 ) ) 
	{
		gContinue = 1;
	}
	else if( 0 == strncmp( "stop", cmd, 4 ) ) 
	{
		gContinue = 0;
	}else if( 0 == strncmp( "clean", cmd, 5 ) ) 
	{
		zb_dw_mmc_clean();
	}
	
	return count;
}


void mmc_create_test_command_proc( void )
{
	struct proc_dir_entry *procdir;

	procdir = create_proc_entry( PROC_DW_MMC_NAME, S_IFREG | S_IRUGO, 0);
	procdir->read_proc  = dw_mmc_proc_read;
	procdir->write_proc = dw_mmc_proc_write;
}




// CACHE RELATED ////////////////// copied from arch/arm/cpu/slsiap/s5p4418/cache_v7.c


#define ARMV7_DCACHE_INVAL_ALL          1
#define ARMV7_DCACHE_CLEAN_INVAL_ALL    2
#define ARMV7_DCACHE_INVAL_RANGE        3
#define ARMV7_DCACHE_CLEAN_INVAL_RANGE  4

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
{       u32 csselr = level << 1 | type;

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
         *      a. count down
         *      b. have bigger loop inside
         */
        for (way = num_ways - 1; way >= 0 ; way--) {
                for (set = num_sets - 1; set >= 0; set--) {
                        setway = (level << 1) | (set << log2_line_len) |
                                 (way << way_shift);
                        /* Invalidate data/unified cache line by set/way */
                        asm volatile (" mcr p15, 0, %0, c7, c6, 2"
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
         *      a. count down
         *      b. have bigger loop inside
         */
        for (way = num_ways - 1; way >= 0 ; way--) {
                for (set = num_sets - 1; set >= 0; set--) {
                        setway = (level << 1) | (set << log2_line_len) |
                                 (way << way_shift);
                        /*
                         * Clean & Invalidate data/unified
                         * cache line by set/way
                         */
                        asm volatile (" mcr p15, 0, %0, c7, c14, 2"
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
//#ifdef NAL_PRE_LOAD
        v7_maint_dcache_all(ARMV7_DCACHE_CLEAN_INVAL_ALL);

        v7_outer_cache_flush_all();
//#endif
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
		udelay(1);
	}
	printk("RESET TIME OUT\n");
	return 0;
}

static void dwmci_set_idma_desc(struct dwmci_idmac *idmac,
		u32 desc0, u32 desc1, u32 desc2, u32 desc3)
{
	struct dwmci_idmac *desc = idmac;

	desc->flags = desc0;
	desc->cnt = desc1;
	desc->addr = desc2;
	desc->next_addr = (unsigned int)desc3 + sizeof(struct dwmci_idmac);
	//desc->next_addr = desc2 + sizeof(struct dwmci_idmac);
	//printk("flags: %8x, cnt: %d, addr: %8x, next_addr: %8x\n",desc->flags,desc->cnt,desc->addr,desc->next_addr);
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

		dwmci_set_idma_desc(cur_idmac, flags, cnt,
				    (u32)bounce_buffer + (i * PAGE_SIZE), mmc->idmac_buf_phys + i*sizeof(struct dwmci_idmac));
		//printk("cur_idmac: %8x, cnt: %d, buffer : %8x\n",cur_idmac, cnt, (u32)bounce_buffer + (i * PAGE_SIZE));
		if (blk_cnt <= 8)
			break;
		blk_cnt -= 8;
		cur_idmac++;
		i++;
	} while(1);

	data_end = (ulong)cur_idmac;
#if 0
	flush_dcache_range(data_start, data_end + ARCH_DMA_MINALIGN);
#else
	/*bok */
	//flush_dcache_all();	//FIXME
#endif


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
		{
		//printk("WRITE MODE\n");
		mode |= DWMCI_CMD_RW;
		}
	else {
		//printk("READ MODE\n");
		}
	return mode;
}

static int dwmci_send_cmd(struct zb_priv_mmc *mmc, struct mmc_cmd *cmd,
		struct mmc_data *data)
{
	int flags = 0, i;
	u32 retry = 1000000;
	u32 mask, ctrl;
	u32 count=0;

	JIFFY_PRINT;

	while (dwmci_readl(mmc, DWMCI_STATUS) & DWMCI_BUSY) {
		if (  count > retry )
		{
		
			//FIXME
			printk("Timeout on data busy\n");
			return TIMEOUT;
		}
		count++;
		udelay(10);
	}
	JIFFY_PRINT;

	dwmci_writel(mmc, DWMCI_RINTSTS, DWMCI_INTMSK_ALL);

#if 0
	if (data) {
		if (data->flags == MMC_DATA_READ) {
			bounce_buffer_start(&bbstate, (void*)data->dest,
					    data->blocksize *
					    data->blocks, GEN_BB_WRITE);
		} else {
			bounce_buffer_start(&bbstate, (void*)data->src,
					    data->blocksize *
					    data->blocks, GEN_BB_READ);
		}
		dwmci_prepare_data(host, data, cur_idmac,
				   bbstate.bounce_buffer);
	}
#endif
	JIFFY_PRINT;

	if (data) {

		dwmci_prepare_data(mmc, data,(struct dwmci_idmac *) mmc->idmac_buf,
				   (void *)mmc->data_buf_phys);
	}
	JIFFY_PRINT;

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

	//printk("Sending CMD%d\n",cmd->cmdidx);
	if ( gNeed_init_sd_clock ) {
			flags  |= DWMCI_CMD_INIT;
			gNeed_init_sd_clock = 0;
	}
	dwmci_writel(mmc, DWMCI_CMD, flags);
	JIFFY_PRINT;


	for (i = 0; i < retry; i++) {
		mask = dwmci_readl(mmc, DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			if (!data)
				dwmci_writel(mmc, DWMCI_RINTSTS, mask);
			break;
		}
		udelay(1);
	}


	JIFFY_PRINT;

	if (i == retry) {
		printk("Func: %s, Line: %d Timeout  ..: RINTS : %08x, MINTS: %08x, STATUS: %08x\n", __func__,__LINE__, dwmci_readl(mmc, DWMCI_RINTSTS),
																					dwmci_readl(mmc, DWMCI_MINTSTS),
																					dwmci_readl(mmc, DWMCI_STATUS));
		return TIMEOUT;
	}
	if (mask & DWMCI_INTMSK_RTO) {
		printk("Response Timeout..\n");
		printk("Func: %s, Line: %d Timeout  ..: RINTS : %08x, MINTS: %08x, STATUS: %08x\n", __func__,__LINE__, dwmci_readl(mmc, DWMCI_RINTSTS),
																					dwmci_readl(mmc, DWMCI_MINTSTS),
																					dwmci_readl(mmc, DWMCI_STATUS));
#if 1
					//TRY CLEAR DATA ERROR
			//	printk("Clear Response Timeout ERROR!\n");
			//	dwmci_writel(mmc, DWMCI_RINTSTS, mask);
#endif
		return TIMEOUT;
	} else if (mask & DWMCI_INTMSK_RE) {
		printk("Response Error..\n");
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
	JIFFY_PRINT;


#if 1
	if (data) {
		retry = 1000 * 100;
		do {
			mask = dwmci_readl(mmc, DWMCI_RINTSTS);
			if (mask & (DWMCI_DATA_ERR | DWMCI_DATA_TOUT)) {
				printk("DATA ERROR!\n");
				printk("Func: %s, Line: %d Timeout  ..: RINTS : %08x, MINTS: %08x, STATUS: %08x\n", __func__,__LINE__, dwmci_readl(mmc, DWMCI_RINTSTS),
																					dwmci_readl(mmc, DWMCI_MINTSTS),
																					dwmci_readl(mmc, DWMCI_STATUS));
#if 1
			//TRY CLEAR DATA ERROR
				printk("Clear  DATA ERROR!\n");
				dwmci_writel(mmc, DWMCI_RINTSTS, mask);

#endif
				return -1;
			}
			udelay(1);
		} while (!(mask & DWMCI_INTMSK_DTO) && retry--);
		if (retry ==0) return -1;

		dwmci_writel(mmc, DWMCI_RINTSTS, mask);

		ctrl = dwmci_readl(mmc, DWMCI_CTRL);
		ctrl &= ~(DWMCI_DMA_EN);
		dwmci_writel(mmc, DWMCI_CTRL, ctrl);

	}
#endif
	JIFFY_PRINT;

	//udelay(1);

	return 0;
}

int dma_read_dirty(struct zb_priv_mmc *mmc)
{
	
	if ( mmc_dirtry_check_regs_zero.bufAddrVal != 0 && (mmc_dirtry_check_regs_zero.bufAddrVal != mmc_dirtry_check_regs_cur.bufAddrVal)) {
			printk("REG BEFORE===========================\n");
			printk("bufAddrVal : %08x\n", mmc_dirtry_check_regs_zero.bufAddrVal);
			/*printk("dbAddrVal : %08x\n", mmc_dirtry_check_regs_zero.dbAddrVal);
			printk("controlVal : %08x\n", mmc_dirtry_check_regs_zero.controlVal);
			printk("rintstsVal : %08x\n", mmc_dirtry_check_regs_zero.rintstsVal);
			printk("mintstsVal : %08x\n", mmc_dirtry_check_regs_zero.mintstsVal);
			printk("dscAddrVal : %08x\n", mmc_dirtry_check_regs_zero.dscAddrVal);
			printk("bufAddrVal : %08x\n", mmc_dirtry_check_regs_zero.bufAddrVal);
			printk("uhsRegVal : %08x\n", mmc_dirtry_check_regs_zero.uhsRegVal);
			printk("bmModeVal : %08x\n", mmc_dirtry_check_regs_zero.bmModeVal);
			printk("intMaskVal : %08x\n", mmc_dirtry_check_regs_zero.intMaskVal);
			printk("idintenVal : %08x\n", mmc_dirtry_check_regs_zero.idintenVal);
			printk("bytecntVal : %08x\n", mmc_dirtry_check_regs_zero.bytecntVal);
			printk("blkszVal : %08x\n", mmc_dirtry_check_regs_zero.blkszVal);
			printk("cmdVal : %08x\n", mmc_dirtry_check_regs_zero.cmdVal);*/
			
			printk("REG AFTER===========================\n");
			printk("bufAddrVal : %08x\n", mmc_dirtry_check_regs_cur.bufAddrVal);
			return -1;
		}
	
	return 0;
	/*printk("dbAddrVal : %08x\n", mmc_dirtry_check_regs_cur.dbAddrVal);
	printk("controlVal : %08x\n", mmc_dirtry_check_regs_cur.controlVal);
	printk("rintstsVal : %08x\n", mmc_dirtry_check_regs_cur.rintstsVal);
	printk("mintstsVal : %08x\n", mmc_dirtry_check_regs_cur.mintstsVal);
	printk("dscAddrVal : %08x\n", mmc_dirtry_check_regs_cur.dscAddrVal);
	printk("bufAddrVal : %08x\n", mmc_dirtry_check_regs_cur.bufAddrVal);
	printk("uhsRegVal : %08x\n", mmc_dirtry_check_regs_cur.uhsRegVal);
	printk("bmModeVal : %08x\n", mmc_dirtry_check_regs_cur.bmModeVal);
	printk("intMaskVal : %08x\n", mmc_dirtry_check_regs_cur.intMaskVal);
	printk("idintenVal : %08x\n", mmc_dirtry_check_regs_cur.idintenVal);
	printk("bytecntVal : %08x\n", mmc_dirtry_check_regs_cur.bytecntVal);
		printk("blkszVal : %08x\n", mmc_dirtry_check_regs_cur.blkszVal);
	printk("cmdVal : %08x\n", mmc_dirtry_check_regs_cur.cmdVal);*/



	
	
}

static int mmc_send_cmd(struct zb_priv_mmc *mmc,struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	int flags = 0, i;
	u32 retry = 1000000;
	u32 mask, ctrl;
	u32 count=0;

	JIFFY_PRINT;

	while (dwmci_readl(mmc, DWMCI_STATUS) & DWMCI_BUSY) {
		if (  count > retry )
		{
		
			//FIXME
			printk("Timeout on data busy\n");
			return -1;
		}
		count++;
		udelay(10);
	}
	JIFFY_PRINT;

	dwmci_writel(mmc, DWMCI_RINTSTS, DWMCI_INTMSK_ALL);

	JIFFY_PRINT;

	if (data) {

		dwmci_prepare_data(mmc, data,(struct dwmci_idmac *) mmc->idmac_buf,
				   (void *)mmc->data_buf_phys);
		backup_regs(mmc,&mmc_dirtry_check_regs_zero);
	}
	JIFFY_PRINT;

	

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

	//printk("Sending CMD%d\n",cmd->cmdidx);
	if ( gNeed_init_sd_clock ) {
			flags  |= DWMCI_CMD_INIT;
			gNeed_init_sd_clock = 0;
	}
	dwmci_writel(mmc, DWMCI_CMD, flags);
	JIFFY_PRINT;


	for (i = 0; i < retry; i++) {
		mask = dwmci_readl(mmc, DWMCI_RINTSTS);
		if (mask & DWMCI_INTMSK_CDONE) {
			if (!data)
				dwmci_writel(mmc, DWMCI_RINTSTS, mask);
			break;
		}
		udelay(1);
	}


	JIFFY_PRINT;

	if (i == retry) {
		printk("Func: %s, Line: %d Timeout  ..: RINTS : %08x, MINTS: %08x, STATUS: %08x\n", __func__,__LINE__, dwmci_readl(mmc, DWMCI_RINTSTS),
																					dwmci_readl(mmc, DWMCI_MINTSTS),
																					dwmci_readl(mmc, DWMCI_STATUS));
		return -1;
	}
	if (mask & DWMCI_INTMSK_RTO) {
		printk("Response Timeout..\n");
		printk("Func: %s, Line: %d Timeout  ..: RINTS : %08x, MINTS: %08x, STATUS: %08x\n", __func__,__LINE__, dwmci_readl(mmc, DWMCI_RINTSTS),
																					dwmci_readl(mmc, DWMCI_MINTSTS),
																					dwmci_readl(mmc, DWMCI_STATUS));
#if 1
					//TRY CLEAR DATA ERROR
			//	printk("Clear Response Timeout ERROR!\n");
			//	dwmci_writel(mmc, DWMCI_RINTSTS, mask);
#endif
		return -1;
	} else if (mask & DWMCI_INTMSK_RE) {
		printk("Response Error..\n");
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
	JIFFY_PRINT;


#if 1
	if (data) {
		retry = 1000 * 100;
		do {
			mask = dwmci_readl(mmc, DWMCI_RINTSTS);
			if (mask & (DWMCI_DATA_ERR | DWMCI_DATA_TOUT)) {
				printk("DATA ERROR!\n");
				printk("Func: %s, Line: %d Timeout  ..: RINTS : %08x, MINTS: %08x, STATUS: %08x\n", __func__,__LINE__, dwmci_readl(mmc, DWMCI_RINTSTS),
																					dwmci_readl(mmc, DWMCI_MINTSTS),
																					dwmci_readl(mmc, DWMCI_STATUS));
#if 1
			//TRY CLEAR DATA ERROR
				printk("Clear  DATA ERROR!\n");
				dwmci_writel(mmc, DWMCI_RINTSTS, mask);

#endif
				return -1;
			}
			udelay(1);
		} while (!(mask & DWMCI_INTMSK_DTO) && retry--);
		if (retry ==0) return -1;

		if (data) backup_regs(mmc,&mmc_dirtry_check_regs_cur);

		/*if ( data) {
			if (dma_read_dirty(mmc)  ==  -1 ) {
				restore_regs(mmc,&mmc_backup_regs);
				return-1;
			}

		}*/

		dwmci_writel(mmc, DWMCI_RINTSTS, mask);

		ctrl = dwmci_readl(mmc, DWMCI_CTRL);
		ctrl &= ~(DWMCI_DMA_EN);
		dwmci_writel(mmc, DWMCI_CTRL, ctrl);

		

	}
#endif
	JIFFY_PRINT;

	//udelay(1);

	return 0;
}


static int dwmci_setup_bus(struct zb_priv_mmc *mmc, u32 freq)
{
	u32 div, status;
	int timeout = 10000;

#if 0
	unsigned long sclk;

	if ((freq == host->clock) || (freq == 0))
		return 0;
	/*
	 * If host->get_mmc_clk didn't define,
	 * then assume that host->bus_hz is source clock value.
	 * host->bus_hz should be set from user.
	 */
	if (host->get_mmc_clk)
		sclk = host->get_mmc_clk(host);
	else if (host->bus_hz)
		sclk = host->bus_hz;
	else {
		printf("Didn't get source clock value..\n");
		return -EINVAL;
	}

	div = DIV_ROUND_UP(sclk, 2 * freq);
#endif

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
			printk("TIMEOUT error!! in Setup Bus\n");
			return -ETIMEDOUT;
		}
	} while (status & DWMCI_CMD_START);

	dwmci_writel(mmc, DWMCI_CLKENA, DWMCI_CLKEN_ENABLE |
			DWMCI_CLKEN_LOW_PWR);

	//dwmci_writel(mmc, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
	//		DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

		dwmci_writel(mmc, DWMCI_CMD, DWMCI_CMD_PRV_DAT_WAIT |
			DWMCI_CMD_UPD_CLK | DWMCI_CMD_START);

	timeout = 10000;
	do {
		status = dwmci_readl(mmc, DWMCI_CMD);
		if (timeout-- < 0) {
			printk("TIMEOUT error!!\n");
			return -ETIMEDOUT;
		}
	} while (status & DWMCI_CMD_START);

	//host->clock = freq;

	return 0;
}

static int mmc_go_idle(struct zb_priv_mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	udelay(1000);

	cmd.cmdidx = MMC_CMD_GO_IDLE_STATE;
	cmd.cmdarg = 0;
	cmd.resp_type = MMC_RSP_NONE;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	udelay(2000);

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
	unsigned long irq_flag;
	
	local_irq_save(irq_flag);
	local_irq_disable();
	
	dwmci_set_ios(mmc);
	
	local_irq_restore(irq_flag);
#endif

	return 0;
}




#define WriteIODW(Addr,Data) (*(volatile unsigned int*)Addr)=((unsigned int)Data)

static int __dwmci_initialize(void)
{

	WriteIODW(0xf0012004, 0x130438b0);
		mdelay(1);
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



static int mmc_send_status(struct zb_priv_mmc *mmc,int timeout)
{
	struct mmc_cmd cmd;
	int err, retries = 5;
#ifdef CONFIG_MMC_TRACE
	int status;
#endif

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	//if (!mmc_host_is_spi(mmc))
	if (1) cmd.cmdarg = mmc->rca << 16;  //FIXME

	do {
		err = mmc_send_cmd(mmc,&cmd, NULL);
		if (!err) {
			if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
			    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
			     MMC_STATE_PRG)
				break;
			else if (cmd.response[0] & MMC_STATUS_MASK) {

				printk("Status Error: %8x\n",cmd.response[0]);
				return COMM_ERR;
			}
		} else if (--retries < 0)
			return err;

		udelay(1);

	} while (timeout--);

	if (timeout <= 0) {
		printk("Timeout waiting card ready\n");
		return TIMEOUT;
	}

	return 0;
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
	printk("Func: %s, Line : %d, cmdarg: %08x\n",__func__, __LINE__,cmd->cmdarg );
	err = mmc_send_cmd(mmc, cmd, NULL);
	if (err) {
		printk("Func: %s, Line : %d\n",__func__, __LINE__);
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
	err= mmc_go_idle(mmc);
	if ( err ) return err;
 	/* Asking to the card its capabilities */
	//op_cond_pending = 1;
	for (i = 0; i < 2; i++) {
		op_cond_response = mmc_send_op_cond_iter(mmc, &cmd, i != 0);
		
		/* exit if not busy (flag seems to be inverted) */
		if (op_cond_response & OCR_BUSY) {
			printk("Func: %s, Line : %d\n",__func__, __LINE__);
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
		
		udelay(100);
		printk("Func: %s, Line : %d\n",__func__, __LINE__);
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
		printk("Func: %s, Line : %d\n",__func__, __LINE__);
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
			printk("Func: %s, Line : %d\n",__func__, __LINE__);
			return err;
		}



	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = mmc->rca << 16;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err) {
		printk("Func: %s, Line : %d\n",__func__, __LINE__);
		return err;
	}
	

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	if (err) {
		printk("Func: %s, Line : %d\n",__func__, __LINE__);
		return err;
		}
//	mmc->csd[0] = cmd.response[0];
//	mmc->csd[1] = cmd.response[1];
//	mmc->csd[2] = cmd.response[2];
//	mmc->csd[3] = cmd.response[3];
	printk("Func: %s, Line : %d\n",__func__, __LINE__);

	if (mmc->version == MMC_VERSION_UNKNOWN) {
		int version = (cmd.response[0] >> 26) & 0xf;

			printk("Func: %s, Line : %d, version = %08x\n",__func__, __LINE__,version);

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
			printk("Func: %s, Line : %d\n",__func__, __LINE__);
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
		printk("Func: %s, Line : %d\n",__func__, __LINE__);
		return err;
		}
		
			unsigned int extw;

			extw = 1;
			err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
					EXT_CSD_BUS_WIDTH, extw);

			if (err) {printk("ERROR!!!!!!!!!!!!\n");}

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
	printk("Func: %s, Line : %d\n",__func__, __LINE__);

	//if (mmc->op_cond_pending) {
			printk("Func: %s, Line : %d\n",__func__, __LINE__);
		err = mmc_complete_op_cond(mmc);
	//	}

	if (!err) {
			printk("Func: %s, Line : %d\n",__func__, __LINE__);
		err = mmc_startup(mmc);
		}
	return err;
}


int mmc_start_init(struct zb_priv_mmc *mmc)
{
	int err;

	
	printk("Func: %s, Line : %d\n",__func__, __LINE__);
	
	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);

	/* Reset the Card */
	err = mmc_go_idle(mmc);

	if (err)
		return err;
	printk("Func: %s, Line : %d\n",__func__, __LINE__);
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


	__dwmci_initialize();

	dwmci_wait_reset(&zero_mmc,DWMCI_RESET_ALL);

	dwmci_writel(mmc, DWMCI_RINTSTS, 0xFFFFFFFF);
	dwmci_writel(mmc, DWMCI_BMOD, DWMCI_BMOD_IDMAC_RESET);
	dwmci_writel(mmc,  DWMCI_IDINTEN, SDMMC_IDMAC_INT_NI | SDMMC_IDMAC_INT_RI |
		   SDMMC_IDMAC_INT_TI);
	dwmci_writel(mmc, DWMCI_DBADDR,mmc_backup_regs.dbAddrVal);
	
	dwmci_writel(mmc, DWMCI_FIFOTH, 0x200f0010);
	dwmci_writel(mmc, DWMCI_CLKCTRL,DW_MMC_DRIVE_DELAY(0) | DW_MMC_SAMPLE_DELAY(0) | DW_MMC_DRIVE_PHASE(1) | DW_MMC_SAMPLE_PHASE(0));
	dwmci_writel(mmc, DWMCI_RINTSTS, 0xFFFFFFFF);
	dwmci_writel(mmc, DWMCI_INTMASK, DWMCI_INTMSK_CDONE | DWMCI_INTMSK_DTO |
			   DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR |
			   DWMCI_INTMSK_ERROR_FLAGS );


	mmc->clock = MMC_REGACY_CLOCK;
	mmc_set_bus_width(mmc,1);	
	mmc_set_clock(mmc, MMC_REGACY_CLOCK);

				
	if ( mmc_card_bringup(mmc) == -1 ) return -1;

	dwmci_writel(mmc, DWMCI_CTRL, DWMCI_INT_ENABLE);

	return 0;
}

extern void (*zb_storage_reset)(void);
extern void (*zb_storage_wait_lock)(void);
extern void (*zb_storage_wait_unlock)(void);
void zb_dwmci_init_by_retry(void)
{
	struct zb_priv_mmc *mmc= &zero_mmc;
	unsigned long irq_flag;

//	local_irq_save(irq_flag);
//	local_irq_disable();
	mutex_lock(&zb_storage_mutex);

	__dwmci_initialize();

	dwmci_wait_reset(&zero_mmc,DWMCI_RESET_ALL);

	dwmci_writel(mmc, DWMCI_BMOD, DWMCI_BMOD_IDMAC_RESET);
	dwmci_writel(mmc,  DWMCI_IDINTEN, SDMMC_IDMAC_INT_NI | SDMMC_IDMAC_INT_RI |
		   SDMMC_IDMAC_INT_TI);
//	dwmci_writel(mmc, DWMCI_DBADDR,mmc_backup_regs.dbAddrVal);
	dwmci_writel(mmc, DWMCI_DBADDR, mmc->host_sg_dma_addr);
	
	dwmci_writel(mmc, DWMCI_FIFOTH, 0x200f0010);
	dwmci_writel(mmc, DWMCI_CLKCTRL,DW_MMC_DRIVE_DELAY(0) | DW_MMC_SAMPLE_DELAY(0) | DW_MMC_DRIVE_PHASE(1) | DW_MMC_SAMPLE_PHASE(0));
	dwmci_writel(mmc, DWMCI_RINTSTS, 0xFFFFFFFF);
	dwmci_writel(mmc, DWMCI_INTMASK, DWMCI_INTMSK_CDONE | DWMCI_INTMSK_DTO |
			   DWMCI_INTMSK_TXDR | DWMCI_INTMSK_RXDR |
			   DWMCI_INTMSK_ERROR_FLAGS );
	dwmci_writel(mmc, DWMCI_CTRL, DWMCI_INT_ENABLE);

	mmc->bus_width=4;
	mmc->clock = 4000000;
	//dwmci_set_ios(mmc,4,1);
	//dwmci_set_ios(mmc);
	//mmc->bus_width=1;
	
	//dwmci_set_ios(mmc,1,0);
	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 4000000);
//	mdelay(10);

				

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

		

	

	mmc_card_bringup(mmc);
	//mmc_go_idle(mmc);
	//zb_mmc_retry_after();
	
	//gNeed_init_sd_clock = 1;

	dwmci_writel(mmc, DWMCI_CTRL, DWMCI_INT_ENABLE);
//	local_irq_restore(irq_flag);
	mutex_unlock(&zb_storage_mutex);
}

void zb_dwmci_wait_lock(void)
{
	mutex_lock(&zb_storage_mutex);
}

void zb_dwmci_wait_unlock(void)
{
	mutex_unlock(&zb_storage_mutex);
}

static int mmc_set_blocklen(struct zb_priv_mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;

	return mmc_send_cmd(mmc, &cmd, NULL);
}




static ulong mmc_erase_t( struct zb_priv_mmc *mmc,ulong start, lbaint_t blkcnt)
{
	struct mmc_cmd cmd;
	ulong end;
	int err, start_cmd, end_cmd;

	if ( mmc->high_capacity)
		end = start + blkcnt - 1;
	else {
		end = (start + blkcnt - 1) * mmc->write_bl_len;
		start *=  mmc->write_bl_len;
	}

	/*if ( mmc->version & SD_VERSION_SD ) {
		start_cmd = SD_CMD_ERASE_WR_BLK_START;
		end_cmd = SD_CMD_ERASE_WR_BLK_END;
	} else {
		start_cmd = MMC_CMD_ERASE_GROUP_START;
		end_cmd = MMC_CMD_ERASE_GROUP_END;
	}*/

	//CHECK ABOVE //JHYOON

	start_cmd = MMC_CMD_ERASE_GROUP_START;
	end_cmd = MMC_CMD_ERASE_GROUP_END;
	

	cmd.cmdidx = start_cmd;
	cmd.cmdarg = start;
	cmd.resp_type = MMC_RSP_R1;

	err = mmc_send_cmd(mmc,&cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = end_cmd;
	cmd.cmdarg = end;

	err = mmc_send_cmd(mmc,&cmd, NULL);
	if (err)
		goto err_out;

	cmd.cmdidx = MMC_CMD_ERASE;
	cmd.cmdarg = SECURE_ERASE;
	cmd.resp_type = MMC_RSP_R1b;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		goto err_out;

	return 0;

err_out:
	printk("mmc erase failed\n");
	return err;
}

static unsigned long
mmc_berase(struct zb_priv_mmc *mmc, unsigned long start, lbaint_t blkcnt)
{
	int err = 0;
	lbaint_t blk = 0, blk_r = 0;

	if ((start % mmc->erase_grp_size) || (blkcnt % mmc->erase_grp_size))
		printk("\n\nCaution! Your devices Erase group is 0x%x\n"
			"The erase range would be change to 0x%lx~0x%lx\n\n",
		       mmc->erase_grp_size, start & ~(mmc->erase_grp_size - 1),
		       ((start + blkcnt +mmc->erase_grp_size)
		       & ~(mmc->erase_grp_size - 1)) - 1);

	while (blk < blkcnt) {
		blk_r = ((blkcnt - blk) >mmc->erase_grp_size) ?
			mmc->erase_grp_size : (blkcnt - blk);
		err = mmc_erase_t(mmc,start + blk, blk_r);
		if (err)
			break;

		blk += blk_r;

		/* Waiting for the ready status */
		/*if ((mmc,timeout)) //FIXME --JHYOON
			return 0;*/
	}

	return blk;
}



static ulong
mmc_write_blocks( struct zb_priv_mmc *mmc,ulong start, lbaint_t blkcnt, const void*src)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout = 1000;

#if 0
	if ((start + blkcnt) > mmc->block_dev.lba) {
		printk("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			start + blkcnt, mmc->block_dev.lba);
		return 0;
	}
#endif // FIXME

	if (blkcnt > 1)
		cmd.cmdidx = MMC_CMD_WRITE_MULTIPLE_BLOCK;
	else
		cmd.cmdidx = MMC_CMD_WRITE_SINGLE_BLOCK;

	if (mmc->high_capacity)
		cmd.cmdarg = start;
	else
		cmd.cmdarg = start *  mmc->write_bl_len;

	cmd.resp_type = MMC_RSP_R1;

	data.src = src;
	data.blocks = blkcnt;
	data.blocksize =  mmc->write_bl_len;
	data.flags = MMC_DATA_WRITE;

	JIFFY_PRINT;
	if (mmc_send_cmd(mmc,&cmd, &data)) {
		printk("mmc write failed\n");
		return 0;
	}
	JIFFY_PRINT;

	/* SPI multiblock writes terminate using a special
	 * token, not a STOP_TRANSMISSION request.
	 */
//	if (!mmc_host_is_spi(mmc) && blkcnt > 1) { //FIXME JHYOON

	JIFFY_PRINT;

	if ( blkcnt > 1 ) {	
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (mmc_send_cmd(mmc,&cmd, NULL)) {
			printk("mmc fail to send stop cmd\n");
			return 0;
		}
	}
	JIFFY_PRINT;
//	}

	/* Waiting for the ready status */
	if (mmc_send_status(mmc, timeout))
		return 0;

	return blkcnt;
}




int collision_flag = 0;
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
		                        printk("Command busy..\n");
		                        return -1;
		               }
			  count++;
				udelay(1);
		}
#endif

	
#if 1 // CHECK DATA Busy
	count= 0;

	while (dwmci_readl(mmc, DWMCI_STATUS) & DWMCI_BUSY) {
                if (  count > timeout )
                {
                        //FIXME
                        printk("Timeout on data busy\n");
                        return -1;
                }
		collision_flag =1;		
                count++;
                udelay(1);
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
		                        printk("DMA busy..\n");
		                        return -1;
		               	 }
				  count++;
				udelay(1);;
			}
#endif


		/*f ( collision_flag == 1 )	{
			printk("CMD Collision\n");
			udelay(10);
			
		}*/


        return 0;

}

static void freeze_interrupt(struct zb_priv_mmc *mmc)
{
	u32 ctrl;
	        
	ctrl = dwmci_readl(mmc, DWMCI_CTRL);
        ctrl &= ~(1<<4); // REMOVE Global Interrupt
        dwmci_writel(mmc, DWMCI_CTRL, ctrl);


}

static void unfreeze_interrupt(struct zb_priv_mmc *mmc)
{
	u32 ctrl;
	        
	ctrl = dwmci_readl(mmc, DWMCI_CTRL);
        ctrl |= (1<<4); // SET Global Interrupt
        dwmci_writel(mmc, DWMCI_CTRL, ctrl);


}



static ulong
mmc_bwrite( struct zb_priv_mmc *mmc,ulong start, lbaint_t blkcnt, const void*src)
{
	lbaint_t cur, blocks_todo = blkcnt;

	unsigned long irq_flag;
	wait_busy(mmc);	

	local_irq_save(irq_flag);
	local_irq_disable();
	
	backup_regs(mmc,&mmc_backup_regs);
	freeze_interrupt(mmc);

	do {
		cur =  blocks_todo; //CHECK JHYOON

		if(mmc_write_blocks(mmc,start, cur, src) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		src += cur *  mmc->write_bl_len;
	} while (blocks_todo > 0);
	
	restore_regs(mmc,&mmc_backup_regs);
	local_irq_restore(irq_flag);

	return blkcnt;
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
			printk("mmc fail to send stop cmd\n");
			return 0;
		}
	}

	return blkcnt;
}



static DEFINE_SPINLOCK(mLock) ;

//extern int zb_buf_status;
extern void set_nal_access_status(int val);



static ulong mmc_bread(struct zb_priv_mmc *mmc, ulong start, lbaint_t blkcnt, void *dst, int skip_reg_backup)
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





/*-----------------------------	-------------------------------------------------
  @brief   4K \ub2e8\uc704\uc758 \ubc88\ud638\ub97c \ud398\uc774\uc9c0 \uc8fc\uc18c\ub85c \ubcc0\ud658\ud574 \uc900\ub2e4
  @remark  
*///----------------------------------------------------------------------------
static unsigned int mem_index_to_mmc_phys(unsigned int mem_page)
{
	unsigned int offset_mmc_block;

	offset_mmc_block = mem_page * (PAGE_SIZE/mmc_block_size);
//printk( "-----%s(%d) offset_nand_page=%d  NAND_START_PAGE=%d\n", __func__,__LINE__, offset_nand_page, NAND_START_PAGE );	
	return MMC_START_BLOCK + offset_mmc_block;
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

/*------------------------------------------------------------------------------
  @brief   ZERO \ubd80\ud2b8\ub97c \uc704\ud558\uc5ec \ud560\ub2f9\ub41c \ud30c\ud2f0\uc158 \uc601\uc5ed\uc744 \ubaa8\ub450 \uc9c0\uc6b4\ub2e4.
  @remark  
*///----------------------------------------------------------------------------
static int mmc_all_erase(struct zb_priv_mmc *mmc )
{
	//mmc_berase(mmc,MMC_START_BLOCK,MMC_TOTAL_BLOCK_COUNT);
	//printk("DEBUG.. ERASE OFFSET: %8x, BLOCK :  %8x\n",ZB_STORAGE_OFFSET,MMC_START_BLOCK);
	mmc_berase(mmc,MMC_START_BLOCK,1);
	return 0;
}


/*------------------------------------------------------------------------------
  @brief   \uc2dc\uc791\ud558\uae30 \uc804\uc5d0 \ud544\uc694\ud55c \uc77c\uc744 \ud55c\ub2e4.
  @remark  \uc0ac\uc6a9\ud560 \ud50c\ub798\uc2dc\ub97c \ubaa8\ub450 \uc9c0\uc6b4\ub2e4.
*///----------------------------------------------------------------------------
static int mmc_setup_first( void )
{ 
	if( dwmci_init(&zero_mmc)  == 0 ) {
		printk("INIT RETRY .\n");
	}	
	
	//return mmc_all_erase(&zero_mmc);
	return 0;// NEW
}
/*------------------------------------------------------------------------------
  @brief   \uba54\ubaa8\ub9ac 4K \ud398\uc774\uc9c0 \ub2e8\uc704\ub85c \uc4f4\ub2e4.
  @remark  
*///----------------------------------------------------------------------------
static int mmc_write_block_4k( u32 mem_page,  u8 *page_buf )
{
	u32 count;
	u32 block_index;
	u8	*data;
	static u32 done = 0;


	block_index 	= mem_index_to_mmc_phys(mem_page);;
	data		= page_buf;
	count		= PAGE_SIZE/mmc_block_size;
//	printk("DEBUG.. WRITE BLOCK :  %8x\n",block_index);

	memcpy((void *) zero_mmc.data_buf,(const void *)page_buf, count*mmc_block_size);

	//flush_dcache_all();
	mmc_bwrite(&zero_mmc,block_index,count,(const void *)zero_mmc.data_buf);

#ifndef READ_WRITE_TEST	
	done++;
//	if ( (done % 100 ) == 0 ) printk("*");
//	if ( (done % 1000 ) == 0 ) printk("\n");
#endif
	return count;
}


int holded_page_index_table[MEMORY_BUFFER_PAGE_COUNT];


int find_page_in_buffer_memory(int page_index)
{
	int i=0;
	for ( i=0;i<MEMORY_BUFFER_PAGE_COUNT; i++) {
		if ( page_index == holded_page_index_table[i] ) 
			return i;
	}
	return -1;

}

int reload_buffer_memory(int mem_page,int block_index)
{
	int i;

	mmc_bread(&zero_mmc,block_index,8*MEMORY_BUFFER_PAGE_COUNT,(void *)zero_mmc.data_buf,0);

	for (i=0; i<MEMORY_BUFFER_PAGE_COUNT; i++) {
		
		holded_page_index_table[i] = mem_page+i;
	}


}

/*------------------------------------------------------------------------------
  @brief   \uba54\ubaa8\ub9ac 4K \ud398\uc774\uc9c0 \ub2e8\uc704\ub85c \uc77d\ub294\ub2e4.
  @remark  
*///----------------------------------------------------------------------------
static int mmc_read_block_4k( u32 mem_page,  u8 *page_buf )
{
	u32 count;
	u32 block_index;
	u8	*data;
	int retry_count=0;
	int fail;
	unsigned long irq_flag;
	int skip_reg_backup = 0;
	int reset_retry_count=0;

	block_index 	= mem_index_to_mmc_phys(mem_page);;
	data		= page_buf;
	count		= PAGE_SIZE/mmc_block_size;
	//printk("DEBUG.. READ BLOCK :  %8x\n",block_index);
	memset((void *)zero_mmc.data_buf,0,2048);
#ifdef USE_BUFFER_MEMORY
	page_index = find_page_in_buffer_memory(mem_page);
	if (page_index!=-1) {
		//printk("FOUND IN BUFFER INDEX\n");
		memcpy(data,zero_mmc.data_buf + page_index * PAGE_SIZE, PAGE_SIZE);
	} else {
		//printk("NOT FOUND IN BUFFER INDEX\n");
		reload_buffer_memory(mem_page,block_index);
		memcpy(data,zero_mmc.data_buf, PAGE_SIZE);
	}
#else

//	local_irq_save(irq_flag);
//	local_irq_disable();
	mutex_lock(&zb_storage_mutex);

	set_nal_access_status(1);
	//dwmci_wait_reset(&zero_mmc,DWMCI_RESET_ALL);
	//dwmci_init_by_retry(&zero_mmc); // FOR TEST

READ_RETRY:
	while(mmc_bread(&zero_mmc,block_index,count,(void *)zero_mmc.data_buf,skip_reg_backup)==0 ) {
		retry_count++;
		printk("retry_count : %d\n",retry_count);
		if ( retry_count >=10 )  {
			//local_irq_restore(irq_flag);
			mutex_unlock(&zb_storage_mutex);
			return -1;

		}
		//local_irq_restore(irq_flag);
		//dwmci_wait_reset(&zero_mmc,DWMCI_CTRL_FIFO_RESET);
		//dwmci_wait_reset(&zero_mmc,DWMCI_RESET_ALL);
		//dwmci_wait_reset(&zero_mmc,DWMCI_CTRL_FIFO_RESET|DWMCI_CTRL_RESET);
		//local_irq_restore(irq_flag);
		while ( dwmci_init_by_retry(&zero_mmc) ==  -1 ) {
			reset_retry_count++;
			printk("reset_retry_count : %d\n",reset_retry_count);
			if ( reset_retry_count >=10 )  {
				//local_irq_restore(irq_flag);
				mutex_unlock(&zb_storage_mutex);
				return -1;

			}
			udelay(10);
		}
		skip_reg_backup = 1;
		
		//local_irq_save(irq_flag);
		//local_irq_disable();
		

#if 0
						/*
				 * After an error, there may be data lingering
				 * in the FIFO, so reset it - doing so
				 * generates a block interrupt, hence setting
				 * the scatter-gather pointer to NULL.
				 */
				sg_miter_stop(&host->sg_miter);
				host->sg = NULL;
				ctrl = mci_readl(host, CTRL);
				ctrl |= SDMMC_CTRL_FIFO_RESET;
				mci_writel(host, CTRL, ctrl);
#endif

		udelay(1000);

		
	}

	if ( dma_read_dirty(&zero_mmc)   ==  -1 ) { goto READ_RETRY; }
	
	//local_irq_restore(irq_flag);
	mutex_unlock(&zb_storage_mutex);
	//zb_buf_status = 0;
	// mmc_bread/(&zero_mmc,block_index,count,(void *)zero_mmc.data_buf)  == 0 )  
	memcpy(data,(void *)zero_mmc.data_buf, count*mmc_block_size);
#endif	


	return count;	
}


/*------------------------------------------------------------------------------
  @brief   \uc800\uc7a5\uc18c\uc758 offset \uac12\uc744 \uc54c\ub824\uc900\ub2e4
  @remark  
*///----------------------------------------------------------------------------
static u32 mmc_get_storage_offset( void )
{
	return ZB_STORAGE_OFFSET;
}
/*------------------------------------------------------------------------------
  @brief   \ub514\ubc14\uc774\uc2a4 \uc81c\uc5b4\ub97c \uc704\ud55c \uac00\uc0c1\uc8fc\uc18c
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


void mmc_test_write(void)
{
		int page_no;
		int i;
		int test_count =TEST_COUNT;
		char *test_data0;
		//mmc_all_erase(&zero_mmc);
		test_data0 = kmalloc(4096,GFP_KERNEL);
		for (i=0; i<4096;i++ ) test_data0[i]=i;
		
		printk("START TEST WRITE\n");
		for ( page_no = 0; page_no<test_count; page_no++) {
			mmc_write_block_4k(page_no,test_data0);

		}
		printk("\n");
		printk("END TEST WRITE : Block Count %d\n",test_count);
	

		kfree(test_data0);
}

void mmc_read_test(void)
{
		int page_no;
		int i;
		int test_count =TEST_COUNT;
		char *test_data0;
		int err_count = 0;
		
		//mmc_all_erase(&zero_mmc);
		test_data0 = kmalloc(4096,GFP_KERNEL);
		
		for ( page_no = 0; page_no<test_count; page_no++) 	{
			memset(test_data0,0,4096);
			mmc_read_block_4k(page_no,test_data0);
			
			for (i=0; i<256;i++ ) {
						if ( test_data0[i] != i ) {
							printk("data:%8x,i=%8x\n",test_data0[i],i);
							err_count++;
						}
					}
			/*for ( i=0; i<256; i=i+16)
							printk("data=%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",test_data0[i],test_data0[i+1],test_data0[i+2],test_data0[i+3],test_data0[i+4],test_data0[i+5],test_data0[i+6],test_data0[i+7]
							,test_data0[i+8],test_data0[i+9],test_data0[i+10],test_data0[i+11],test_data0[i+12],test_data0[i+13],test_data0[i+14],test_data0[i+15]);	
							*/
			if ( err_count>0 ) printk("ERROR..MMC READ\n");
		}

		
		
		kfree(test_data0);
		
}	

int mmc_test(void)
{
	int error_count= 0; 
	int page_no;
	int i;
	char *test_data0;
	char *test_data1;
	char *data;
	int test_count =TEST_COUNT;

	//mmc_all_erase(&zero_mmc);
	test_data0 = kmalloc(4096,GFP_KERNEL);
	memset(test_data0,0,4096);

	for (i=0; i<4096;i++ ) test_data0[i]=i;
	
	data = kmalloc(4096,GFP_KERNEL); 
	//printk("Read Test In startup\n");
	for ( page_no = 0; page_no<test_count; page_no++) {

		memset(data,0,4096);

		if ( mmc_read_block_4k(page_no,data) == -1 ) {
			printk("ERROR  -- RETRY Count limit over .... Test Stopped...\n");
				kfree(data);
				kfree(test_data0);
				return -1;
		}

#if 1
		if ( memcmp(test_data0,data,sizeof(test_data0)) != 0 ) {			
					error_count++;
					printk("Failed...........\n");		
					for ( i=0; i<256; i=i+16)
									printk("data=%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7]
									,data[i+8],data[i+9],data[i+10],data[i+11],data[i+12],data[i+13],data[i+14],data[i+15]);	

					
			}
			

#endif	
		
	}

	//printk("mmc operation Count %d Pages,	Error Count: %d\n",test_count, error_count);
	
	kfree(data);
	kfree(test_data0);
	
	if ( error_count ) return -1;

	return 0;
}



#if 0
int mmc_test(void)
{
	int error_count= 0; 
	int page_no;
	int i;
	char *test_data0;
	char *test_data1;
	char *data;
	int test_count =TEST_COUNT;

	//mmc_all_erase(&zero_mmc);
	test_data0 = kmalloc(4096,GFP_KERNEL);
	test_data1 = kmalloc(4096,GFP_KERNEL);
	memset(test_data0,0,4096);
	memset(test_data1,0,4096);

	for (i=0; i<4096;i++ ) test_data0[i]=i;
	for ( i=0; i<256; i=i+16) {
							test_data1[i+0]=0xde;
							test_data1[i+1]=0xad;
							test_data1[i+2]=0xbe;
							test_data1[i+3]=0xef;
							test_data1[i+4]=0xde;
							test_data1[i+5]=0xad;
							test_data1[i+6]=0xbe;
							test_data1[i+7]=0xef;
							test_data1[i+8]=0xde;
							test_data1[i+9]=0xad;
							test_data1[i+10]=0xbe;
							test_data1[i+11]=0xef;
							test_data1[i+12]=0xde;
							test_data1[i+13]=0xad;
							test_data1[i+14]=0xbe;
							test_data1[i+15]=0xef;
						}
	
	data = kmalloc(4096,GFP_KERNEL); 

#if	0

	for ( page_no = 0; page_no<test_count; page_no++) {
		
		if ( page_no %2 == 0 )
			mmc_write_block_4k(page_no,test_data0);
		else
			mmc_write_block_4k(page_no,test_data1);
	}

	memset(data,0,4096);
#endif

	//printk("START TEST READ\n");


	for ( page_no = 0; page_no<test_count; page_no++) {

		memset(data,0,4096);

		mmc_read_block_4k(page_no,data);

#if 1
		if ( page_no %2== 0 ) {
			if ( memcmp(test_data0,data,sizeof(test_data0)) != 0 ) {
			
			error_count++;
			printk("Failed...........\n");
		
			for ( i=0; i<256; i=i+16)
							printk("data=%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7]
							,data[i+8],data[i+9],data[i+10],data[i+11],data[i+12],data[i+13],data[i+14],data[i+15]);	
			}

			
			} else {

			if ( memcmp(test_data1,data,sizeof(test_data1)) != 0 ) {
			
			error_count++;
			printk("Failed...........\n");
		
			for ( i=0; i<256; i=i+16)
							printk("data=%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7]
							,data[i+8],data[i+9],data[i+10],data[i+11],data[i+12],data[i+13],data[i+14],data[i+15]);	
			}



		}
#endif	
		
		



	}


	//printk("mmc operation Count %d Pages,	Error Count: %d\n",test_count, error_count);
	
	kfree(data);
	kfree(test_data0);
	kfree(test_data1);
	
	if ( error_count ) return -1;

	return 0;
}
#endif

static int zb_setup_private_info(void)
{
	
	zb_mmc->ioaddr = (unsigned int) zero_mmc.ioaddr;
	zb_mmc->ioaddr_phys = (unsigned int) zero_mmc.ioaddr_phys;
	zb_mmc->data_buf =  (unsigned int)zero_mmc.data_buf;
	zb_mmc->data_buf_phys =  (unsigned int)zero_mmc.data_buf_phys;
	zb_mmc->idmac_buf = (unsigned int) zero_mmc.idmac_buf;
	zb_mmc->idmac_buf_phys = (unsigned int) zero_mmc.idmac_buf_phys;
	zb_mmc->erase_grp_size = (unsigned int) zero_mmc.erase_grp_size;
	zb_mmc->high_capacity = (unsigned int) zero_mmc.high_capacity;
	zb_mmc->read_bl_len = (unsigned int) zero_mmc.read_bl_len;
	zb_mmc->write_bl_len = (unsigned int) zero_mmc.write_bl_len;
	zb_mmc->rca= (unsigned int) zero_mmc.rca;
	zb_mmc->card_caps= (unsigned int) zero_mmc.card_caps;
	zb_mmc->bus_width =(unsigned int) zero_mmc.bus_width;
	zb_mmc->host_sg_dma_addr =  (unsigned int) zero_mmc.host_sg_dma_addr;
	zb_mmc->clock =(unsigned int) zero_mmc.clock;
	zb_mmc->version =(unsigned int) zero_mmc.version;
	zb_mmc->ocr =(unsigned int) zero_mmc.ocr;
	
	printk("ioaddr : %8x\n", zb_mmc->ioaddr);
	printk("ioaddr_phys : %8x\n", zb_mmc->ioaddr_phys);
	printk("zb_mmc->data_buf : %8x\n", 	zb_mmc->data_buf);
	printk("zb_mmc->data_buf_phys: %8x\n",zb_mmc->data_buf_phys);
	printk("zb_mmc->idmac_buf : %8x\n", zb_mmc->idmac_buf);
	printk("zb_mmc->idmac_buf_phys : %8x\n", zb_mmc->idmac_buf_phys);
	printk("zb_mmc->erase_grp_size : %8x\n", zb_mmc->erase_grp_size);
	printk("zb_mmc->high_capacity : %8x\n", zb_mmc->high_capacity);
	printk("zb_mmc->read_bl_len : %8x\n", zb_mmc->read_bl_len);
	printk("zb_mmc->write_bl_len : %8x\n", zb_mmc->write_bl_len);
	printk("zb_mmc->rca : %8x\n", zb_mmc->rca);
	printk("zb_mmc->card_caps : %8x\n", zb_mmc->card_caps);
	printk("zb_mmc->bus_width : %8x\n", zb_mmc->bus_width);
	printk("zb_mmc->host_sg_dma_addr : %8x\n", zb_mmc->host_sg_dma_addr);
	printk("zb_mmc->clock : %8x\n", zb_mmc->clock);
	printk("zb_mmc->version : %8x\n", zb_mmc->version);
	printk("zb_mmc->ocr : %8x\n", zb_mmc->ocr);
	
	
	zb_pre_setup_dma_area((unsigned long)zero_mmc.data_buf_phys, zero_mmc.data_buf, PAGE_SIZE);
	zb_pre_setup_dma_area((unsigned long)zero_mmc.idmac_buf_phys, zero_mmc.idmac_buf, PAGE_SIZE);
	

	return 0;
}

extern int zb_suspend_type;


static int kthread_mmc_test_thr_fun(void *arg)
{
	int test_count=0;
  printk(KERN_ALERT "@ %s() : called\n", __FUNCTION__);
  
  while(!kthread_should_stop())
  {

   	if ( gContinue  ) {
  		//printk("READ TEST\n");
   		// mmc_read_test();
			   if ( mmc_test() == -1 ) {
			   	
			   	 printk("TEST FAIL............................................................................\n");
			  
			   	//break;
			   	zb_dw_mmc_clean();
			   	//zb_mmc_retry_after();
			   }
			 
				  test_count++;
			    if (test_count >= 10) {
					printk("TEST DONE_ %d Count\n", test_count);
					//msleep_interruptible(1000);
					mdelay(100); 
					test_count=0;
			    	} else {
			    		//msleep_interruptible(1);
			    		mdelay(1);
						
			    	}
	} else {
		msleep_interruptible(1000);
	}
  }
  printk(KERN_ALERT "@ %s() : kthread_should_stop() called. Bye.\n", __FUNCTION__);
  return 0;
} 


static int kthread_init(void)
{
  printk(KERN_ALERT "@ %s() : called\n", __FUNCTION__);

  mmc_test_write();
 
  if(g_th_id == NULL){ 
     g_th_id = (struct task_struct *)kthread_run(kthread_mmc_test_thr_fun, NULL, "kthread_mmc_test");
  }
  return 0;
} 

static void kthread_release(void)
{
  if(g_th_id){
     kthread_stop(g_th_id);
     g_th_id = NULL;
  }
  printk(KERN_ALERT "@ %s() : Bye.\n", __FUNCTION__);
} 


/*------------------------------------------------------------------------------
  @brief   \ucd08\uae30\ud654
  @remark   
*///----------------------------------------------------------------------------
extern dma_addr_t zb_mmc_sg_dma;
int zb_storage_init( struct zblk_ops *zblk_ops )
{
	dma_addr_t phys;

	printk( "STORAGE %s READY\n", STORAGE_NAME );

	memset(&zero_mmc,0,sizeof(struct zb_priv_mmc ));
	
	zero_mmc.data_buf = dma_alloc_coherent(NULL,PAGE_SIZE*MEMORY_BUFFER_PAGE_COUNT,&phys,GFP_DMA);
	zero_mmc.data_buf_phys = phys;

	zero_mmc.idmac_buf = dma_alloc_coherent(NULL,PAGE_SIZE*8,&phys,GFP_DMA);
	zero_mmc.idmac_buf_phys = phys;
	
	if ( zero_mmc.data_buf == NULL ) {
		printk("DMA ALLOC MEMORY FAILED!!!\n");
		return 0;
	}
		zero_mmc.ioaddr_phys = 0xc0069000;
		zero_mmc.ioaddr = ioremap(0xc0069000,0x400);
		zero_mmc.read_bl_len = 0x200;
		zero_mmc.write_bl_len = 0x200;
		zero_mmc.erase_grp_size = 0x400;
		zero_mmc.card_caps = 0x7;
		zero_mmc.rca = 1;
		zero_mmc.high_capacity = 1;
		zero_mmc.host_sg_dma_addr = zb_mmc_sg_dma;


		if ( dwmci_init(&zero_mmc)   == 0 ) {
			//if (mmc_set_blocklen(&zero_mmc,  zero_mmc.write_bl_len))
			//	return 0;

			memset( holded_page_index_table, -1, MEMORY_BUFFER_PAGE_COUNT*sizeof(int));

			printk("mmc_io_addr: %8x\n",zero_mmc.ioaddr);
			printk("mmc_io_addr_phys: %8x\n",zero_mmc.ioaddr_phys);
			printk("mmc_read_bl_len: %8x\n",zero_mmc.read_bl_len);
			printk("mmc_write_bl_len: %8x\n",zero_mmc.write_bl_len);
			printk("mmc_erase_grp_size: %8x\n",zero_mmc.erase_grp_size);
			printk("mmc_high_capacity: %8x\n",zero_mmc.high_capacity);
			printk("mmc_rca: %8x\n",zero_mmc.rca);
			printk("mmc_card_caps: %8x\n",zero_mmc.card_caps);
			printk("mmc_data_buf: %8x\n",zero_mmc.data_buf);
			printk("mmc_data_buf_phys: %8x\n",zero_mmc.data_buf_phys);
			printk("mmc_idmac_buf: %8x\n",zero_mmc.idmac_buf);
			printk("mmc_idmac_buf_phys: %8x\n",zero_mmc.idmac_buf_phys);
			mmc_create_test_command_proc();

#ifdef	READ_WRITE_TEST
			mmc_test_write();
			mmc_test();
#endif				
			zblk_ops->setup_first		  = mmc_setup_first	   ;	// \uc900\ube44 \uc791\uc5c5(\ucc98\uc74c \ud55c\ubc88 \ud638\ucd9c\ub41c\ub2e4.)
			zblk_ops->get_storage_offset  = mmc_get_storage_offset;	// \uc800\uc7a5\uc18c(\ub09c\ub4dc\ud50c\ub798\uc2dc)\uc758 \uc2dc\uc791 \uc8fc\uc18c(\ubc14\uc774\ud2b8 \ub2e8\uc704)
			zblk_ops->page_write		  = mmc_write_block_4k	   ;	// 4KiB \ub2e8\uc704\uc758 \ubc84\ud37c \uc4f0\uae30
			zblk_ops->page_read 		  = mmc_read_block_4k	   ;	// 4KiB \ub2e8\uc704\uc758 \ubc84\ud37c \uc77d\uae30
			zblk_ops->get_io_vaddr		  = mmc_get_io_vaddr	   ;	// nand_base \uc8fc\uc18c\ub97c \uc5bb\ub294 \ud568\uc218
			zblk_ops->setup_post_zeroboot = mmc_post_zeroboot	   ;	// \uc81c\ub85c\ubd80\ud2b8 \uc774\ud6c4\uc5d0 \ud560\uc77c
				
			zb_mmc = (struct zb_priv_mmc *)get_storage_priv_offset();
			printk(" zb_MMC_ADDRESS : %8x\n",zb_mmc);
			zb_setup_private_info();
			zb_suspend_type = 1;

			//printk("mmc reset test\n");
			//mmc_test();
			/*
			int irq_flag;
			local_irq_save(irq_flag);
			local_irq_disable();
			//dwmci_init_by_retry(&zero_mmc);
			local_irq_restore(irq_flag);
			printk("mmc reset tested\n");*/
#ifdef	READ_WRITE_TEST
			kthread_init();
			zb_storage_reset = zb_dwmci_init_by_retry;
			zb_storage_wait_lock = zb_dwmci_wait_lock;
			zb_storage_wait_unlock = zb_dwmci_wait_unlock;
#endif		
		} else {
			printk( " MMC INIT FAILED !! \n" );
			return 0; // FIXME RETURN CODE
			
		}
	return 0;


}

/*------------------------------------------------------------------------------
  @brief   \ud574\uc81c
  @remark  
*///----------------------------------------------------------------------------
void zb_storage_exit( void )
{
	//if ( nand_base ) iounmap( nand_base );
	kthread_release();
	printk( "STORAGE MMC-EXIT\n" );
}
