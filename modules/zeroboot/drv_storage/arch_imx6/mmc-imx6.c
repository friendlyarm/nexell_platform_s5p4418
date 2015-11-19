/*-----------------------------------------------------------------------------
  파 일 : mmc-s5pv210.c
  설 명 : 
  작 성 : frog@falinux.com
          freefrug@falinux.com
  날 짜 : 2012-06-29
  주 의 :
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


#include <zb_blk.h>
#include <zbi.h>
#include <zb_mmc.h>
#include "mmc.h"
#include "fsl_esdhc.h"
#include <linux/dma-mapping.h>
#include <zb_snapshot.h>



#define STORAGE_NAME			"mmc driver for i.mx6"
#define WRITE_PROTECT			0			// never write


struct zb_priv_mmc *zb_mmc;

typedef ulong lbaint_t;





/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------

	

/// @}


static struct mmc zero_mmc;

static u32  mmc_block_size =  512;  
#define MMC_START_BLOCK			(ZB_STORAGE_OFFSET/mmc_block_size)
#define MMC_TOTAL_BLOCK_COUNT	1024 //FIXME

static int mmc_send_cmd(struct mmc *mmc,struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	//struct mmc_data backup;
	int ret;
#if 0	
	int i;
	u8 *ptr;
#endif

	//memset(&backup, 0, sizeof(backup));

#if 0


	printk("CMD_SEND:%d\n", cmd->cmdidx);
	printk("\t\tARG\t\t\t 0x%08X\n", cmd->cmdarg);
	ret = esdhc_send_cmd( mmc,cmd, data);
	switch (cmd->resp_type) {
		case MMC_RSP_NONE:
			printk("\t\tMMC_RSP_NONE\n");
			break;
		case MMC_RSP_R1:
			printk("\t\tMMC_RSP_R1,5,6,7 \t 0x%08X \n",
				cmd->response[0]);
			break;
		case MMC_RSP_R1b:
			printk("\t\tMMC_RSP_R1b\t\t 0x%08X \n",
				cmd->response[0]);
			break;
		case MMC_RSP_R2:
			printk("\t\tMMC_RSP_R2\t\t 0x%08X \n",
				cmd->response[0]);
			printk("\t\t          \t\t 0x%08X \n",
				cmd->response[1]);
			printk("\t\t          \t\t 0x%08X \n",
				cmd->response[2]);
			printk("\t\t          \t\t 0x%08X \n",
				cmd->response[3]);
			printk("\n");
			printk("\t\t\t\t\tDUMPING DATA\n");
			for (i = 0; i < 4; i++) {
				int j;
				printk("\t\t\t\t\t%03d - ", i*4);
				ptr = (u8 *)&cmd->response[i];
				ptr += 3;
				for (j = 0; j < 4; j++)
					printk("%02X ", *ptr--);
				printk("\n");
			}
			break;
		case MMC_RSP_R3:
			printk("\t\tMMC_RSP_R3,4\t\t 0x%08X \n",
				cmd->response[0]);
			break;
		default:
			printk("\t\tERROR MMC rsp not supported\n");
			break;
	}
#else
	ret = esdhc_send_cmd(mmc,cmd, data);
#endif
	return ret;
}

static int mmc_go_idle(struct mmc *mmc)
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

void mmc_set_clock(struct mmc *mmc, uint clock)
{
	if (clock > mmc->f_max)
		clock = mmc->f_max;

	if (clock < mmc->f_min)
		clock = mmc->f_min;

	mmc->clock = clock;

	esdhc_set_ios(mmc);
}

static void mmc_set_bus_width(struct mmc *mmc,unsigned int width)
{
	mmc->bus_width = width;
	esdhc_set_ios(mmc);
}



static int mmc_send_status(struct mmc *mmc,int timeout)
{
	struct mmc_cmd cmd;
	int err, retries = 5;
#ifdef CONFIG_MMC_TRACE
	int status;
#endif

	cmd.cmdidx = MMC_CMD_SEND_STATUS;
	cmd.resp_type = MMC_RSP_R1;
	if (!mmc_host_is_spi(mmc))
		cmd.cmdarg = mmc->rca << 16;

	do {
		err = mmc_send_cmd(mmc,&cmd, NULL);
		if (!err) {
			if ((cmd.response[0] & MMC_STATUS_RDY_FOR_DATA) &&
			    (cmd.response[0] & MMC_STATUS_CURR_STATE) !=
			     MMC_STATE_PRG)
				break;
			else if (cmd.response[0] & MMC_STATUS_MASK) {
				printk("Status Error: 0x%08X\n",
					cmd.response[0]);
				return COMM_ERR;
			}
		} else if (--retries < 0)
			return err;
		printk("SEND STATUS WAIT..\n");
		udelay(1000);

	} while (timeout--);

#ifdef CONFIG_MMC_TRACE
	status = (cmd.response[0] & MMC_STATUS_CURR_STATE) >> 9;
	printk("CURR STATE:%d\n", status);
#endif
	if (timeout <= 0) {
		printk("Timeout waiting card ready\n");
		return TIMEOUT;
	}

	return 0;
}

static int mmc_set_blocklen(struct mmc *mmc, int len)
{
	struct mmc_cmd cmd;

	cmd.cmdidx = MMC_CMD_SET_BLOCKLEN;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = len;

	return mmc_send_cmd(mmc, &cmd, NULL);
}




static ulong mmc_erase_t( struct mmc *mmc,ulong start, lbaint_t blkcnt)
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

	if ( mmc->version & SD_VERSION_SD ) {
		start_cmd = SD_CMD_ERASE_WR_BLK_START;
		end_cmd = SD_CMD_ERASE_WR_BLK_END;
	} else {
		start_cmd = MMC_CMD_ERASE_GROUP_START;
		end_cmd = MMC_CMD_ERASE_GROUP_END;
	}

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
mmc_berase(struct mmc *mmc, unsigned long start, lbaint_t blkcnt)
{
	int err = 0;
	lbaint_t blk = 0, blk_r = 0;
	int timeout = 1000;

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
		if ((mmc,timeout))
			return 0;
	}

	return blk;
}

static ulong
mmc_write_blocks( struct mmc *mmc,ulong start, lbaint_t blkcnt, const void*src)
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
	data.phys_addr = mmc->data_buf_phys;
	if (mmc_send_cmd(mmc,&cmd, &data)) {
		printk("mmc write failed\n");
		return 0;
	}


	/* SPI multiblock writes terminate using a special
	 * token, not a STOP_TRANSMISSION request.
	 */
	if (!mmc_host_is_spi(mmc) && blkcnt > 1) {
	if ( blkcnt > 1 ) {	
		cmd.cmdidx = MMC_CMD_STOP_TRANSMISSION;
		cmd.cmdarg = 0;
		cmd.resp_type = MMC_RSP_R1b;
		if (mmc_send_cmd(mmc,&cmd, NULL)) {
			printk("mmc fail to send stop cmd\n");
			return 0;
		}
	}
	}

	/* Waiting for the ready status */
	if (mmc_send_status(mmc, timeout))
		return 0;

	return blkcnt;
}

static ulong
mmc_bwrite( struct mmc *mmc,ulong start, lbaint_t blkcnt, const void*src)
{
	lbaint_t cur, blocks_todo = blkcnt;

	//if (mmc_set_blocklen(mmc,  mmc->write_bl_len))
	//	return 0;

	do {
		cur = (blocks_todo > mmc->b_max) ?   mmc->b_max : blocks_todo;
		if(mmc_write_blocks(mmc,start, cur, src) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		src += cur *  mmc->write_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}


static int mmc_read_blocks( struct mmc *mmc,void *dst, ulong start,
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
	data.phys_addr = mmc->data_buf_phys;

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

static ulong mmc_bread(struct mmc *mmc, ulong start, lbaint_t blkcnt, void *dst)
{
	lbaint_t cur, blocks_todo = blkcnt;

	if (blkcnt == 0)
		return 0;

	
#if 0
	if ((start + blkcnt) > mmc->block_dev.lba) {
		printk("MMC: block number 0x%lx exceeds max(0x%lx)\n",
			start + blkcnt, mmc->block_dev.lba);
		return 0;
	}
#endif

	if (mmc_set_blocklen(mmc, mmc->read_bl_len))
		return 0;

	do {
		cur = (blocks_todo >  mmc->b_max) ?   mmc->b_max : blocks_todo;
		if(mmc_read_blocks(mmc,dst, start, cur) != cur)
			return 0;
		blocks_todo -= cur;
		start += cur;
		dst += cur * mmc->read_bl_len;
	} while (blocks_todo > 0);

	return blkcnt;
}


static int mmc_send_ext_csd(struct mmc *mmc, u8 *ext_csd)
{
	struct mmc_cmd cmd;
	struct mmc_data data;
	int err;

	/* Get the Card Status Register */
	cmd.cmdidx = MMC_CMD_SEND_EXT_CSD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	data.dest = (char *)mmc->ext_csd;
	data.blocks = 1;
	data.blocksize = 512;
	data.flags = MMC_DATA_READ;
	data.phys_addr = mmc->ext_csd_phys;

	err = mmc_send_cmd(mmc,&cmd, &data);

	return err;
}

static int mmc_send_if_cond(struct mmc *mmc)
{
	struct mmc_cmd cmd;
	int err;

	cmd.cmdidx = SD_CMD_SEND_IF_COND;
	/* We set the bit if the host supports voltages between 2.7 and 3.6 V */
	cmd.cmdarg = ((mmc->voltages & 0xff8000) != 0) << 8 | 0xaa;
	cmd.resp_type = MMC_RSP_R7;

	err = mmc_send_cmd(mmc, &cmd, NULL);

	if (err)
		return err;

	if ((cmd.response[0] & 0xff) != 0xaa)
		return UNUSABLE_ERR;
	else
		mmc->version = SD_VERSION_2;

	return 0;
}

static int sd_send_op_cond(struct mmc *mmc)
{
	int timeout = 1000;
	int err;
	struct mmc_cmd cmd;

	do {
		cmd.cmdidx = MMC_CMD_APP_CMD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		cmd.cmdidx = SD_CMD_APP_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;

		/*
		 * Most cards do not answer if some reserved bits
		 * in the ocr are set. However, Some controller
		 * can set bit 7 (reserved for low voltages), but
		 * how to manage low voltages SD card is not yet
		 * specified.
		 */
		cmd.cmdarg = mmc_host_is_spi(mmc) ? 0 :
			(mmc->voltages & 0xff8000);

		if (mmc->version == SD_VERSION_2)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
	} while ((!(cmd.response[0] & OCR_BUSY)) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	if (mmc->version != SD_VERSION_2)
		mmc->version = SD_VERSION_1_0;

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}

	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

static int mmc_send_op_cond(struct mmc *mmc)
{
	int timeout = 10000;
	struct mmc_cmd cmd;
	int err;

	/* Some cards seem to need this */
	mmc_go_idle(mmc);

 	/* Asking to the card its capabilities */
 	cmd.cmdidx = MMC_CMD_SEND_OP_COND;
 	cmd.resp_type = MMC_RSP_R3;
 	cmd.cmdarg = 0;

 	err = mmc_send_cmd(mmc, &cmd, NULL);

 	if (err)
 		return err;

 	udelay(1000);

	do {
		cmd.cmdidx = MMC_CMD_SEND_OP_COND;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = (mmc_host_is_spi(mmc) ? 0 :
				(mmc->voltages &
				(cmd.response[0] & OCR_VOLTAGE_MASK)) |
				(cmd.response[0] & OCR_ACCESS_MODE));

		if (mmc->host_caps & MMC_MODE_HC)
			cmd.cmdarg |= OCR_HCS;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;

		udelay(1000);
	} while (!(cmd.response[0] & OCR_BUSY) && timeout--);

	if (timeout <= 0)
		return UNUSABLE_ERR;

	if (mmc_host_is_spi(mmc)) { /* read OCR for spi */
		cmd.cmdidx = MMC_CMD_SPI_READ_OCR;
		cmd.resp_type = MMC_RSP_R3;
		cmd.cmdarg = 0;

		err = mmc_send_cmd(mmc, &cmd, NULL);

		if (err)
			return err;
	}

	mmc->version = MMC_VERSION_UNKNOWN;
	mmc->ocr = cmd.response[0];

	mmc->high_capacity = ((mmc->ocr & OCR_HCS) == OCR_HCS);
	mmc->rca = 0;

	return 0;
}

static int mmc_switch(struct mmc *mmc, u8 set, u8 index, u8 value)
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

static int sd_switch(struct mmc *mmc, int mode, int group, u8 value, u8 *resp)
{
	struct mmc_cmd cmd;
	struct mmc_data data;

	/* Switch the frequency */
	cmd.cmdidx = SD_CMD_SWITCH_FUNC;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = (mode << 31) | 0xffffff;
	cmd.cmdarg &= ~(0xf << (group * 4));
	cmd.cmdarg |= value << (group * 4);

	data.dest = (char *)resp;
	data.blocksize = 64;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.phys_addr = mmc->switch_status_phys;
	
	return mmc_send_cmd(mmc, &cmd, &data);
}


static int sd_change_freq(struct mmc *mmc)
{
	int err;
	struct mmc_cmd cmd;
	struct mmc_data data;
	int timeout;
	mmc->card_caps = 0;

	if (mmc_host_is_spi(mmc))
		return 0;
	/* Read the SCR to find out if this card supports higher speeds */
	cmd.cmdidx = MMC_CMD_APP_CMD;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = mmc->rca << 16;

	err = mmc_send_cmd(mmc, &cmd, NULL);
	if (err)
		return err;

	cmd.cmdidx = SD_CMD_APP_SEND_SCR;
	cmd.resp_type = MMC_RSP_R1;
	cmd.cmdarg = 0;

	timeout = 3;

retry_scr:
	data.dest = (char *)mmc->scr;
	data.blocksize = 8;
	data.blocks = 1;
	data.flags = MMC_DATA_READ;
	data.phys_addr = mmc->scr_phys;

	err = mmc_send_cmd(mmc, &cmd, &data);
	if (err) {
		if (timeout--)
			goto retry_scr;

		return err;
	}

	mmc->scr[0] = __be32_to_cpu(mmc->scr[0]);
	mmc->scr[1] = __be32_to_cpu(mmc->scr[1]);

	printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);

	switch ((mmc->scr[0] >> 24) & 0xf) {
		case 0:
			mmc->version = SD_VERSION_1_0;
			break;
		case 1:
			mmc->version = SD_VERSION_1_10;
			break;
		case 2:
			mmc->version = SD_VERSION_2;
			if ((mmc->scr[0] >> 15) & 0x1)
				mmc->version = SD_VERSION_3;
			break;
		default:
			mmc->version = SD_VERSION_1_0;
			break;
	}
	if (mmc->scr[0] & SD_DATA_4BIT)
		mmc->card_caps |= MMC_MODE_4BIT;

	/* Version 1.0 doesn't support switching */
	if (mmc->version == SD_VERSION_1_0)
		return 0;

	timeout = 4;

	while (timeout--) {
		err = sd_switch(mmc, SD_SWITCH_CHECK, 0, 1,
				(u8 *)mmc->switch_status);
		if (err)
			return err;
		/* The high-speed function is busy.  Try again */
		if (!(__be32_to_cpu(mmc->switch_status[7]) & SD_HIGHSPEED_BUSY))
			break;
	}
	/* If high-speed isn't supported, we return */
	if (!(__be32_to_cpu(mmc->switch_status[3]) & SD_HIGHSPEED_SUPPORTED))
		return 0;

	/*
	 * If the host doesn't support SD_HIGHSPEED, do not switch card to
	 * HIGHSPEED mode even if the card support SD_HIGHSPPED.
	 * This can avoid furthur problem when the card runs in different
	 * mode between the host.
	 */
	if (!((mmc->host_caps & MMC_MODE_HS_52MHz) &&
		(mmc->host_caps & MMC_MODE_HS)))
		return 0;
	err = sd_switch(mmc, SD_SWITCH_SWITCH, 0, 1, (u8 *)mmc->switch_status);
	if (err)
		return err;
	if ((__be32_to_cpu(mmc->switch_status[4]) & 0x0f000000) == 0x01000000)
		mmc->card_caps |= MMC_MODE_HS;
	return 0;
}

static int mmc_change_freq(struct mmc *mmc)
{
	char cardtype;
	int err;

	mmc->card_caps = 0;
	
	 printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);

	if (mmc_host_is_spi(mmc))
		return 0;
 printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);
	/* Only version 4 supports high-speed */
	if (mmc->version < MMC_VERSION_4)
		return 0;
 printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);
	err = mmc_send_ext_csd(mmc, mmc->ext_csd);

	if (err)
		return err;
 printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);
	cardtype = mmc->ext_csd[EXT_CSD_CARD_TYPE] & 0xf;
 printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);
	err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL, EXT_CSD_HS_TIMING, 1);
 printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);
	if (err)
		return err;
 printk("FUNC: %s, LINE: %d\n", __func__, __LINE__);
	/* Now check to see that it worked */
	err = mmc_send_ext_csd(mmc, mmc->ext_csd);

	if (err)
		return err;

	/* No high-speed support */
	if (!mmc->ext_csd[EXT_CSD_HS_TIMING]) {
		 printk("CARD CAPS :  NOT HIGH SPEED\n");
		return 0;
	}
	/* High Speed is set, there are two types: 52MHz and 26MHz */
	if (cardtype & MMC_HS_52MHZ) {
		mmc->card_caps |= MMC_MODE_HS_52MHz | MMC_MODE_HS;
		 printk("CARD CAPS :  MMC_HS_52MHZ\n");
	}
	else
		 {
			 mmc->card_caps |= MMC_MODE_HS;
			 printk("CARD CAPS : 52MHZ\n"); }
	
	return 0;
}


/* frequency bases */
/* divided by 10 to be nice to platforms without floating point */
static const int fbase[] = {
	10000,
	100000,
	1000000,
	10000000,
};

/* Multiplier values for TRAN_SPEED.  Multiplied by 10 to be nice
 * to platforms without floating point.
 */
static const int multipliers[] = {
	0,	/* reserved */
	10,
	12,
	13,
	15,
	20,
	25,
	30,
	35,
	40,
	45,
	50,
	55,
	60,
	70,
	80,
};


int mmc_detect(struct mmc *mmc)// DETECT & Parameter config
{
	
	int err;
	uint mult, freq;
	u64 cmult, csize, capacity;
	struct mmc_cmd cmd;
	int timeout = 1000;


	
	mmc->ext_csd = dma_alloc_coherent(NULL,512,&mmc->ext_csd_phys,GFP_DMA);

	mmc_set_bus_width(mmc, 1);
	mmc_set_clock(mmc, 1);
	
	err = mmc_go_idle(mmc);

	if (err) {
		printk("CARD RESET ERROR!!\n");
		return err;
	}
	
	/* Test for SD version 2 */
	err = mmc_send_if_cond(mmc);
	/* Now try to get the SD card's operating condition */
	err = sd_send_op_cond(mmc);

	/* If the command timed out, we check for an MMC card */
	if (err == TIMEOUT) {
		err = mmc_send_op_cond(mmc);

		if (err) {
			printk("Card did not respond to voltage select!\n");
			return UNUSABLE_ERR;
		}
	}

	/* Put the Card in Identify Mode */

	cmd.cmdidx = MMC_CMD_ALL_SEND_CID; 
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg = 0;

	err = mmc_send_cmd(mmc,&cmd, NULL);

	if (err)
		return err;

	memcpy( mmc->cid, cmd.response, 16);

	/*
	 * For MMC cards, set the Relative Address.
	 * For SD cards, get the Relatvie Address.
	 * This also puts the cards into Standby State
	 */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = SD_CMD_SEND_RELATIVE_ADDR;
		cmd.cmdarg = mmc->rca << 16;
		cmd.resp_type = MMC_RSP_R6;

		err = mmc_send_cmd(mmc,&cmd, NULL);

		if (err)
			return err;
		
		if (IS_SD(mmc))
			mmc->rca = (cmd.response[0] >> 16) & 0xffff;
	}
	/* Get the Card-Specific Data */
	cmd.cmdidx = MMC_CMD_SEND_CSD;
	cmd.resp_type = MMC_RSP_R2;
	cmd.cmdarg =  mmc->rca << 16;

	err = mmc_send_cmd(mmc,&cmd, NULL);

	/* Waiting for the ready status */
	mmc_send_status(mmc, timeout);

	if (err)
		return err;

	 mmc->csd[0] = cmd.response[0];
	 mmc->csd[1] = cmd.response[1];
	 mmc->csd[2] = cmd.response[2];
	 mmc->csd[3] = cmd.response[3];

	if ( mmc->version == MMC_VERSION_UNKNOWN) {
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


	/* divide frequency by 10, since the mults are 10x bigger */
	freq = fbase[(cmd.response[0] & 0x7)];
	mult = multipliers[((cmd.response[0] >> 3) & 0xf)];

	mmc->tran_speed = freq * mult;

	mmc->read_bl_len = 1 << ((cmd.response[1] >> 16) & 0xf);


	if (IS_SD(mmc))
		 mmc->write_bl_len =  mmc->read_bl_len;
	else
		 mmc->write_bl_len = 1 << ((cmd.response[3] >> 22) & 0xf);

	if ( mmc->high_capacity) {
		csize = ( mmc->csd[1] & 0x3f) << 16
			| ( mmc->csd[2] & 0xffff0000) >> 16;
		cmult = 8;
	} else {
		csize = ( mmc->csd[1] & 0x3ff) << 2
			| ( mmc->csd[2] & 0xc0000000) >> 30;
		cmult = ( mmc->csd[2] & 0x00038000) >> 15;
	}

	 mmc->capacity = (csize + 1) << (cmult + 2);
	 mmc->capacity *=  mmc->read_bl_len;

	if ( mmc->read_bl_len > 512)
		 mmc->read_bl_len = 512;

	if ( mmc->write_bl_len > 512)
		 mmc->write_bl_len = 512;

	/* Select the card, and put it into Transfer Mode */
	if (!mmc_host_is_spi(mmc)) { /* cmd not supported in spi */
		cmd.cmdidx = MMC_CMD_SELECT_CARD;
		cmd.resp_type = MMC_RSP_R1;
		cmd.cmdarg =  mmc->rca << 16;
		err = mmc_send_cmd(mmc,&cmd, NULL);

		if (err)
			return err;
	}

	/*
	 * For SD, its erase group is always one sector
	 */
	 mmc->erase_grp_size = 1;
	 mmc->part_config = MMCPART_NOAVAILABLE;
	if (!IS_SD( mmc) && ( mmc->version >= MMC_VERSION_4)) {
		/* check  ext_csd version and capacity */
		err = mmc_send_ext_csd(mmc,mmc->ext_csd);
		if (!err && (mmc->ext_csd[EXT_CSD_REV] >= 2)) {
			/*
			 * According to the JEDEC Standard, the value of
			 * ext_csd's capacity is valid if the value is more
			 * than 2GB
			 */
			capacity = mmc->ext_csd[EXT_CSD_SEC_CNT] << 0
					| mmc->ext_csd[EXT_CSD_SEC_CNT + 1] << 8
					| mmc->ext_csd[EXT_CSD_SEC_CNT + 2] << 16
					| mmc->ext_csd[EXT_CSD_SEC_CNT + 3] << 24;
			capacity *= 512;
			if ((capacity >> 20) > 2 * 1024)
				 mmc->capacity = capacity;
		}

		switch (mmc->ext_csd[EXT_CSD_REV]) {
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
		 * Check whether GROUP_DEF is set, if yes, read out
		 * group size from ext_csd directly, or calculate
		 * the group size from the csd value.
		 */
		if (mmc->ext_csd[EXT_CSD_ERASE_GROUP_DEF])
			 mmc->erase_grp_size =
				  mmc->ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] * 512 * 1024;
		else {
			int erase_gsz, erase_gmul;
			erase_gsz = ( mmc->csd[2] & 0x00007c00) >> 10;
			erase_gmul = ( mmc->csd[2] & 0x000003e0) >> 5;
			 mmc->erase_grp_size = (erase_gsz + 1)
				* (erase_gmul + 1);
		}

		/* store the partition info of emmc */
		if ((mmc->ext_csd[EXT_CSD_PARTITIONING_SUPPORT] & PART_SUPPORT) ||
			mmc->ext_csd[EXT_CSD_BOOT_MULT])
			 mmc->part_config = mmc->ext_csd[EXT_CSD_PART_CONF];
	}

	if (IS_SD(mmc))
		err = sd_change_freq(mmc);
		//printk("BUG - IS IT SD??????????????\n");
	else
		err = mmc_change_freq(mmc);

	if (err)
		return err;

	
	if (IS_SD(mmc)) {
			if (mmc->card_caps & MMC_MODE_4BIT) {
				cmd.cmdidx = MMC_CMD_APP_CMD;
				cmd.resp_type = MMC_RSP_R1;
				cmd.cmdarg = mmc->rca << 16;
	
				err = mmc_send_cmd(mmc, &cmd, NULL);
				if (err)
					return err;
	
				cmd.cmdidx = SD_CMD_APP_SET_BUS_WIDTH;
				cmd.resp_type = MMC_RSP_R1;
				cmd.cmdarg = 2;
				err = mmc_send_cmd(mmc, &cmd, NULL);
				if (err)
					return err;
	
				mmc_set_bus_width(mmc, 4);
			}
	
			if (mmc->card_caps & MMC_MODE_HS)
				mmc->tran_speed = 50000000;
			else
				mmc->tran_speed = 25000000;
		} else {
			int idx;
	
			/* An array of possible bus widths in order of preference */
			static unsigned ext_csd_bits[] = {
				EXT_CSD_BUS_WIDTH_8,
				EXT_CSD_BUS_WIDTH_4,
				EXT_CSD_BUS_WIDTH_1,
			};
	
			/* An array to map CSD bus widths to host cap bits */
			static unsigned ext_to_hostcaps[] = {
				[EXT_CSD_BUS_WIDTH_4] = MMC_MODE_4BIT,
				[EXT_CSD_BUS_WIDTH_8] = MMC_MODE_8BIT,
			};
	
			/* An array to map chosen bus width to an integer */
			static unsigned widths[] = {
				8, 4, 1,
			};
	
			for (idx=0; idx < ARRAY_SIZE(ext_csd_bits); idx++) {
				unsigned int extw = ext_csd_bits[idx];
	
				/*
				 * Check to make sure the controller supports
				 * this bus width, if it's more than 1
				 */
				if (extw != EXT_CSD_BUS_WIDTH_1 &&
						!(mmc->host_caps & ext_to_hostcaps[extw]))
					continue;
	
				err = mmc_switch(mmc, EXT_CSD_CMD_SET_NORMAL,
						EXT_CSD_BUS_WIDTH, extw);
	
				if (err)
					continue;
	
				mmc_set_bus_width(mmc, widths[idx]);
	
				/*err = mmc_send_ext_csd(mmc, test_csd);
				if (!err && mmc->ext_csd[EXT_CSD_PARTITIONING_SUPPORT] \
						== test_csd[EXT_CSD_PARTITIONING_SUPPORT]
					 && mmc->ext_csd[EXT_CSD_ERASE_GROUP_DEF] \
						== test_csd[EXT_CSD_ERASE_GROUP_DEF] \
					 && mmc->ext_csd[EXT_CSD_REV] \
						== test_csd[EXT_CSD_REV]
					 && mmc->ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] \
						== test_csd[EXT_CSD_HC_ERASE_GRP_SIZE]
					 && mmc->ext_csd(&ext_csd[EXT_CSD_SEC_CNT], \
						&test_csd[EXT_CSD_SEC_CNT], 4) == 0) {
	
					mmc->card_caps |= ext_to_hostcaps[extw];
					break;
				}*/
			}

			mmc->card_caps |= MMC_MODE_HS; //FIXME
			mmc->tran_speed = 52000000;
			/*
			if (mmc->card_caps & MMC_MODE_HS) {
				if (mmc->card_caps & MMC_MODE_HS_52MHz)
					mmc->tran_speed = 52000000;
				else
					mmc->tran_speed = 26000000;
			}*/
		}
	
		mmc_set_clock(mmc, mmc->tran_speed);
	return 0;
}

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
  @brief   ZERO 부트를 위하여 할당된 파티션 영역을 모두 지운다.
  @remark  
*///----------------------------------------------------------------------------
static int mmc_all_erase(struct mmc *mmc )
{
	//mmc_berase(mmc,MMC_START_BLOCK,MMC_TOTAL_BLOCK_COUNT);
	mmc_berase(mmc,MMC_START_BLOCK,1);
	return 0;
}


/*------------------------------------------------------------------------------
  @brief   시작하기 전에 필요한 일을 한다.
  @remark  사용할 플래시를 모두 지운다.
*///----------------------------------------------------------------------------
static int mmc_setup_first( void )
{
	
	return mmc_all_erase(&zero_mmc);
}
/*------------------------------------------------------------------------------
  @brief   메모리 4K 페이지 단위로 쓴다.
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

	memcpy(zero_mmc.data_buf,page_buf, count*mmc_block_size);

	mmc_bwrite(&zero_mmc,block_index,count,zero_mmc.data_buf);
	if ( (done % 100 ) == 0 ) printk("*");
	done++;
	return count;
}

/*------------------------------------------------------------------------------
  @brief   메모리 4K 페이지 단위로 읽는다.
  @remark  
*///----------------------------------------------------------------------------
static int mmc_read_block_4k( u32 mem_page,  u8 *page_buf )
{
	u32 count;
	u32 block_index;
	u8	*data;

	block_index 	= mem_index_to_mmc_phys(mem_page);;
	data		= page_buf;
	count		= PAGE_SIZE/mmc_block_size;

	mmc_bread(&zero_mmc,block_index,count,zero_mmc.data_buf);
	memcpy(data,zero_mmc.data_buf, count*mmc_block_size);
	
		
	return count;	
}

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

void mmc_test(void)
{
	int error_count= 0; 
	int page_no;
	int i;
	char *test_data;
	char *data;
	int test_count =50;

	mmc_all_erase(&zero_mmc);
	test_data = kmalloc(4096,GFP_KERNEL);
	data = kmalloc(4096,GFP_KERNEL);
#if 1	
	for ( page_no = 0; page_no<test_count; page_no++) {
		
		memset(test_data,0xff,4096);
		memset(data,0,4096);
		for (i=0; i<4096;i++ )
			test_data[i]=i;
		/*for ( i=0; i<256; i=i+16) {
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
		}*/
#if 1		
		printk(" Page %8x test started\n", page_no);
		mmc_write_block_4k(page_no,test_data);	
		mmc_read_block_4k(page_no,data);
		
		if ( memcmp(test_data,data,sizeof(test_data)) != 0 ) {
			
			error_count++;
			printk("Failed...........\n");
		
			for ( i=0; i<256; i=i+16)
					printk("data=%2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x %2x \n",data[i],data[i+1],data[i+2],data[i+3],data[i+4],data[i+5],data[i+6],data[i+7]
					,data[i+8],data[i+9],data[i+10],data[i+11],data[i+12],data[i+13],data[i+14],data[i+15]);

		}
#endif


	}
	
	printk("mmc operation Count %d Pages,	Error Count: %d\n",test_count, error_count);
#endif



}

static int zb_setup_private_info(void)
{
	
	zb_mmc->regs = (unsigned int) zero_mmc.regs;
	zb_mmc->data_buf =  (unsigned int)zero_mmc.data_buf;
	zb_mmc->data_buf_phys =  (unsigned int)zero_mmc.data_buf_phys;
	zb_mmc->scr =  (unsigned int)zero_mmc.scr;
	zb_mmc->scr_phys =  (unsigned int)zero_mmc.scr_phys;
	zb_mmc->switch_status =  (unsigned int)zero_mmc.switch_status;
	zb_mmc->switch_status_phys =  (unsigned int)zero_mmc.switch_status_phys;
	zb_mmc->ext_csd =  (unsigned int)zero_mmc.ext_csd;
	zb_mmc->ext_csd_phys =  (unsigned int) zero_mmc.ext_csd_phys;

	printk("regs : %8x\n", zb_mmc->regs);
	printk("zb_mmc->data_buf : %8x\n", 	zb_mmc->data_buf);
	printk("zb_mmc->data_buf_phys: %8x\n",zb_mmc->data_buf_phys);
	printk("zb_mmc->scr : %8x\n", zb_mmc->scr);
	printk("zb_mmc->scr_phys : %8x\n", zb_mmc->scr_phys);
	printk("zb_mmc->switch_status : %8x\n", zb_mmc->switch_status);
	printk("zb_mmc->switch_status_phys : %8x\n", zb_mmc->switch_status_phys);
	printk("zb_mmc->ext_csd : %8x\n", zb_mmc->ext_csd);
	printk("zb_mmc->ext_csd_phys : %8x\n", zb_mmc->ext_csd_phys);
	
	
	zb_pre_setup_dma_area((u8 *)zero_mmc.data_buf_phys, zero_mmc.data_buf_phys, PAGE_SIZE);
	zb_pre_setup_dma_area((u8 *)zero_mmc.scr_phys, zero_mmc.scr, PAGE_SIZE);
	zb_pre_setup_dma_area((u8 *)zero_mmc.switch_status_phys, zero_mmc.switch_status, PAGE_SIZE );
	zb_pre_setup_dma_area((u8 *)zero_mmc.ext_csd_phys, zero_mmc.ext_csd, PAGE_SIZE );

	return 0;
}


/*------------------------------------------------------------------------------
  @brief   초기화
  @remark   
*///----------------------------------------------------------------------------
int zb_storage_init( struct zblk_ops *zblk_ops )
{
	dma_addr_t phys;
	printk( "STORAGE %s READY\n", STORAGE_NAME );

	memset(&zero_mmc,0,sizeof(struct mmc));
	
	fsl_esdhc_mmc_init(&zero_mmc);
	zero_mmc.data_buf = dma_alloc_coherent(NULL,PAGE_SIZE,&phys,GFP_DMA);
	zero_mmc.data_buf_phys = phys;
	//printk("DATA_BUF_VIRT: %8x, DATA_BUF_PHYS: %8x\n",zero_mmc.data_buf,zero_mmc.data_buf_phys);
	zero_mmc.scr = dma_alloc_coherent(NULL,PAGE_SIZE,&phys,GFP_DMA); // FIXME -- size
	zero_mmc.scr_phys = phys;
	//printk("scr: %8x, scr_PHYS: %8x\n",zero_mmc.scr,zero_mmc.scr_phys);
	zero_mmc.switch_status = dma_alloc_coherent(NULL,PAGE_SIZE,&phys,GFP_DMA);
	zero_mmc.switch_status_phys = phys;
	//printk("switch_status: %8x, switch_status_phys: %8x\n",zero_mmc.switch_status,zero_mmc.switch_status_phys);
	if ( zero_mmc.data_buf == NULL ) {
		printk("DMA ALLOC MEMORY FAILED!!!\n");
		return 0;
	}
	
	if (mmc_detect(&zero_mmc) == 0) {
			printk("MMC DETECT SUCCESS\n");
			
			printk("mmc_read_bl_len: %8x\n",zero_mmc.read_bl_len);
			printk("mmc_write_bl_len: %8x\n",zero_mmc.write_bl_len);
			printk("mmc_erase_grp_size: %8x\n",zero_mmc.erase_grp_size);
			printk("mmc_capacity: %llu\n",zero_mmc.capacity);
			printk("mmc_rca: %8x\n",zero_mmc.rca);
			printk("mmc_high_capacity: %8x\n",zero_mmc.high_capacity);
			printk("mmc_version: %8x\n",zero_mmc.version);
			printk("mmc_b_max: %8x\n",zero_mmc.b_max);

			//mmc_test();
			
			zblk_ops->setup_first		  = mmc_setup_first	   ;	// 준비 작업(처음 한번 호출된다.)
			zblk_ops->get_storage_offset  = mmc_get_storage_offset;	// 저장소(난드플래시)의 시작 주소(바이트 단위)
			zblk_ops->page_write		  = mmc_write_block_4k	   ;	// 4KiB 단위의 버퍼 쓰기
			zblk_ops->page_read 		  = mmc_read_block_4k	   ;	// 4KiB 단위의 버퍼 읽기
			zblk_ops->get_io_vaddr		  = mmc_get_io_vaddr	   ;	// nand_base 주소를 얻는 함수
			zblk_ops->setup_post_zeroboot = mmc_post_zeroboot	   ;	// 제로부트 이후에 할일
				
			zb_mmc = (struct zb_priv_mmc *)get_storage_priv_offset();
			printk(" zb_MMC_ADDRESS : %8x\n",zb_mmc);
			zb_setup_private_info();

		} else {
			printk( " MMC DETECT FAILED !! \n" );
			return 0; // FIXME RETURN CODE
			
		}
	return 0;


}

/*------------------------------------------------------------------------------
  @brief   해제
  @remark  
*///----------------------------------------------------------------------------
void zb_storage_exit( void )
{
	//if ( nand_base ) iounmap( nand_base );
	
	printk( "STORAGE MMC-EXIT\n" );
}
