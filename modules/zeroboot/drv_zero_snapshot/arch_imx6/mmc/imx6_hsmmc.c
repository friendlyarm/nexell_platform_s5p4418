/*
 * Copyright 2007, Freescale Semiconductor, Inc
 * Andy Fleming
 *
 * Based vaguely on the pxa mmc code:
 * (C) Copyright 2003
 * Kyle Harris, Nexus Technologies, Inc. kharris@nexus-tech.net
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

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
#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>

#include <linux/time.h>			
#include <linux/timer.h>		
#include <linux/clk.h>
#include <mach/map.h>
#include <asm/mach/map.h>
#include <plat/regs-timer.h>
#include <plat/clock.h>

#include <reg_save_restore.h>

#include "mmc.h"
#include "s3c_hsmmc.h"

#define  MMC_MAX_CHANNEL     1

//#define dbg(x...)       printf(x)
#define dbg(x...)       do { } while (0)

#ifndef printf
	#define printf printk
#endif

#ifndef writel
	#define writel(v,a)		__raw_writel(v,a)
	#define readl(a)		__raw_readl(a)
	#define writew(v,a)		__raw_writew(v,a)	
	#define readw(a)		__raw_readw(a)	
	#define writeb(v,a)		__raw_writeb(v,a)	
	#define readb(a)		__raw_readb(a)	
#endif

extern void  zb_mdelay( int dly );



struct mmc mmc_channel[MMC_MAX_CHANNEL];
struct sdhci_host mmc_host[MMC_MAX_CHANNEL];

static void sdhci_prepare_data(struct sdhci_host *host, struct mmc_data *data)
{
	u8 ctrl;

	writeb(0xe, host->ioaddr + SDHCI_TIMEOUT_CONTROL);

//printf("================%s(%d)  data->dest=%p  blocks=%d, count=%d\n", __func__,__LINE__, data->dest, data->blocks, data->blocksize );

	/*
	 * Always uses SDMA
	 */
 	dbg("data->dest: %08x\n", data->dest);
	//writel(virt_to_phys((u32)data->dest), host->ioaddr + SDHCI_DMA_ADDRESS);
	writel( (u32)data->dest, host->ioaddr + SDHCI_DMA_ADDRESS );

	ctrl = readb(host->ioaddr + SDHCI_HOST_CONTROL);
	ctrl &= ~SDHCI_CTRL_DMA_MASK;
	writeb(ctrl, host->ioaddr + SDHCI_HOST_CONTROL);

	/* We do not handle DMA boundaries, so set it to max (512 KiB) */
	writew(SDHCI_MAKE_BLKSZ(7, data->blocksize), host->ioaddr + SDHCI_BLOCK_SIZE);
	writew(data->blocks, host->ioaddr + SDHCI_BLOCK_COUNT);
}

static void sdhci_set_transfer_mode(struct sdhci_host *host,
	struct mmc_data *data)
{
	u16 mode;

	mode = SDHCI_TRNS_BLK_CNT_EN | SDHCI_TRNS_DMA;
	if (data->blocks > 1)
		mode |= SDHCI_TRNS_MULTI | SDHCI_TRNS_ACMD12;
	if (data->flags & MMC_DATA_READ)
		mode |= SDHCI_TRNS_READ;

	writew(mode, host->ioaddr + SDHCI_TRANSFER_MODE);
}

/*
 * Sends a command out on the bus.  Takes the mmc pointer,
 * a command pointer, and an optional data pointer.
 */
static int
s3c_hsmmc_send_command(struct mmc *mmc, struct mmc_cmd *cmd,
			struct mmc_data *data)
{
	struct sdhci_host *host = mmc->priv;

	int flags, i;
	u32 mask;
	unsigned long timeout;

	/* Wait max 10 ms */
	timeout = 10;

	mask = SDHCI_CMD_INHIBIT;
	if ((data != NULL) || (cmd->flags & MMC_RSP_BUSY))
		mask |= SDHCI_DATA_INHIBIT;

	/* We shouldn't wait for data inihibit for stop commands, even
	   though they might use busy signaling */
	if (data)
		mask &= ~SDHCI_DATA_INHIBIT;

	while (readl(host->ioaddr + SDHCI_PRESENT_STATE) & mask) {
		if (timeout == 0) {
			printf("Controller never released " \
				"inhibit bit(s).\n");
			return -1;
		}
		timeout--;
		udelay(1000);//zb_mdelay(1);
	}

	if (data)
	{
		sdhci_prepare_data(host, data);
	}
	
	//dbg("cmd->arg: %08x\n", cmd->arg);	
	writel(cmd->arg, host->ioaddr + SDHCI_ARGUMENT);

	if (data)
	{
		sdhci_set_transfer_mode(host, data);
	}
	
	if ((cmd->resp_type & MMC_RSP_136) && (cmd->resp_type & MMC_RSP_BUSY)) {
		return -1;
	}

	if (!(cmd->resp_type & MMC_RSP_PRESENT))
		flags = SDHCI_CMD_RESP_NONE;
	else if (cmd->resp_type & MMC_RSP_136)
		flags = SDHCI_CMD_RESP_LONG;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		flags = SDHCI_CMD_RESP_SHORT_BUSY;
	else
		flags = SDHCI_CMD_RESP_SHORT;

	if (cmd->resp_type & MMC_RSP_CRC)
		flags |= SDHCI_CMD_CRC;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		flags |= SDHCI_CMD_INDEX;
	if (data)
		flags |= SDHCI_CMD_DATA;

	dbg(">>\n");
	dbg("cmd: %d (0x%04x) arg: 0x%08x\n", cmd->opcode, SDHCI_MAKE_CMD(cmd->opcode, flags), cmd->arg );
	
	writew(SDHCI_MAKE_CMD(cmd->opcode, flags), host->ioaddr + SDHCI_COMMAND);
	//printf( " \t\t\tcmd=0x%04x (%d)  arg=0x%08x\n", SDHCI_MAKE_CMD(cmd->opcode, flags), cmd->opcode, cmd->arg );

	for (i=0; i<0x100000; i++) {
		mask = readl(host->ioaddr + SDHCI_INT_STATUS);
		if (mask & SDHCI_INT_RESPONSE) {
			if (!data)
				writel(mask, host->ioaddr + SDHCI_INT_STATUS);
			break;
		}
	}
	
	if (0x100000 == i) {
		printf("FAIL: waiting for status update.\n");
		return TIMEOUT;
	}

	if (mask & SDHCI_INT_TIMEOUT) {
		dbg("timeout: %08x cmd %d\n", mask, cmd->opcode);
		return TIMEOUT;
	}
	else if (mask & SDHCI_INT_ERROR) {
		dbg("error: %08x cmd %d\n", mask, cmd->opcode);
		return -1;
	}
	
	if (cmd->resp_type & MMC_RSP_PRESENT) {
		if (cmd->resp_type & MMC_RSP_136) {
			/* CRC is stripped so we need to do some shifting. */
			for (i = 0;i < 4;i++) {
				cmd->resp[i] = readl(host->ioaddr +
					SDHCI_RESPONSE + (3-i)*4) << 8;
				dbg("cmd->resp[%d]: %08x\n", i, cmd->resp[i]);
				if (i != 3)
					cmd->resp[i] |=
						readb(host->ioaddr +
						SDHCI_RESPONSE + (3-i)*4-1);
				dbg("cmd->resp[%d]: %08x\n", i, cmd->resp[i]);				
			}
		} else if (cmd->resp_type & MMC_RSP_BUSY) {
			for (i = 0;i < 0x100000; i++)
				if (readl(host->ioaddr + SDHCI_PRESENT_STATE) & SDHCI_DATA_BIT(0))
					break;
			if (0x100000 == i) {
				printf("FAIL: card is still busy\n");
				return TIMEOUT;
			}
							
			cmd->resp[0] = readl(host->ioaddr + SDHCI_RESPONSE);
			dbg("cmd->resp[0]: %08x\n", cmd->resp[i]);
		} else {
			cmd->resp[0] = readl(host->ioaddr + SDHCI_RESPONSE);
			dbg("cmd->resp[0]: %08x\n", cmd->resp[i]);
		}
	}

	if (data) {
		
		mask = 0;
		while ( !(mask & (SDHCI_INT_DATA_END | SDHCI_INT_ERROR | SDHCI_INT_DMA_END)) ) 
		{
			mask = readl(host->ioaddr + SDHCI_INT_STATUS);
//printf( "-----%s(%d) mask=%08x  DATA_END=%d  DMA_END=%d  ERROR=%d\n", __func__,__LINE__, mask,
//				mask & SDHCI_INT_DATA_END ? 1:0, mask & SDHCI_INT_DMA_END ? 1:0, mask & SDHCI_INT_ERROR ? 1:0 );
		}
	
		writel(mask, host->ioaddr + SDHCI_INT_STATUS);
		if (mask & SDHCI_INT_ERROR) {
			printf("error during transfer: 0x%08x\n", mask);
			return -1;
		} else if (mask & SDHCI_INT_DMA_END) {
			printf("SDHCI_INT_DMA_END\n");
		} else {		
			dbg("r/w is done\n");
		}
	}

	udelay(1000);//zb_mdelay(1);
	return 0;
}

static void sdhci_change_clock(struct sdhci_host *host, uint clock)
{
	int div;
	u16 clk;
	unsigned long timeout;
	u32 ctrl2;

	/* Set SCLK_MMC from SYSCON as a clock source */
	ctrl2 = readl(host->ioaddr + S3C_SDHCI_CONTROL2);
	ctrl2 &= ~(3 << S3C_SDHCI_CTRL2_SELBASECLK_SHIFT);
	ctrl2 |= 2 << S3C_SDHCI_CTRL2_SELBASECLK_SHIFT;
	writew(ctrl2, host->ioaddr + S3C_SDHCI_CONTROL2);

	writew(0, host->ioaddr + SDHCI_CLOCK_CONTROL);

	/* XXX: we assume that clock is between 40MHz and 50MHz */
	if (clock == 0)
		goto out;
	else if (clock <= 400000)
		div = 0x80; // 0x40;
	// [FG]  클럭속도에 대해서 고민해야 한다.  커널에서 캡쳐한다.
	else
		div = 2;
	//else if (clock <= 20000000)
	//	div = 2;
	//else if (clock <= 26000000)
	//	div = 1;
	//else
	//	div = 0;

	clk = div << SDHCI_DIVIDER_SHIFT;
	clk |= SDHCI_CLOCK_INT_EN;

	dbg("div: %d  val=%04x\n", div, clk);

	writew(clk, host->ioaddr + SDHCI_CLOCK_CONTROL);

	/* Wait max 10 ms */
	timeout = 10;
	while (!((clk = readw(host->ioaddr + SDHCI_CLOCK_CONTROL))
		& SDHCI_CLOCK_INT_STABLE)) {
		if (timeout == 0) {
			printf("Internal clock never stabilised.\n");
			return;
		}
		timeout--;
		udelay(1000);//zb_mdelay(1);
	}

	clk |= SDHCI_CLOCK_CARD_EN;
	writew(clk, host->ioaddr + SDHCI_CLOCK_CONTROL);

	//printf( "	SDHCI_CLOCK_CONTROL=0x%08x\n", clk );

out:
	host->clock = clock;

	return;
}

static void s3c_hsmmc_set_ios(struct mmc *mmc)
{
	struct sdhci_host *host = mmc->priv;
	u8 ctrl;

	dbg("set_ios: bus_width: %x, clock: %d\n", mmc->bus_width, mmc->clock);
	//setup_sdhci0_cfg_card(host);
	{
		// todo
	}
	
	sdhci_change_clock(host, mmc->clock);

	ctrl = readb(host->ioaddr + SDHCI_HOST_CONTROL);
	
//printf( "---before %02x\n", ctrl );
	
	if (mmc->bus_width == 4)
		ctrl |= SDHCI_CTRL_4BITBUS;
	else
		ctrl &= ~SDHCI_CTRL_4BITBUS;

	ctrl &= ~SDHCI_CTRL_HISPD;

	writeb(ctrl, host->ioaddr + SDHCI_HOST_CONTROL);


//printf( "---after %02x\n", ctrl );
}

static void sdhci_reset(struct sdhci_host *host, u8 mask)
{
	u32 timeout;

	writeb(mask, host->ioaddr + SDHCI_SOFTWARE_RESET);

	if (mask & SDHCI_RESET_ALL)
		host->clock = 0;

	/* Wait max 100 ms */
	timeout = 100;

	/* hw clears the bit when it's done */
	while (readb(host->ioaddr + SDHCI_SOFTWARE_RESET) & mask) {
		if (timeout == 0) {
			printf("mmc: Reset 0x%x never completed.\n",
				(int)mask);
			return;
		}
		timeout--;
		udelay(1000);//zb_mdelay(1);
	}
}

static void sdhci_init(struct sdhci_host *host)
{
	u32 intmask;

	sdhci_reset(host, SDHCI_RESET_ALL);

	intmask = SDHCI_INT_BUS_POWER | SDHCI_INT_DATA_END_BIT |
		SDHCI_INT_DATA_CRC | SDHCI_INT_DATA_TIMEOUT | SDHCI_INT_INDEX |
		SDHCI_INT_END_BIT | SDHCI_INT_CRC | SDHCI_INT_TIMEOUT |
		SDHCI_INT_CARD_REMOVE | SDHCI_INT_CARD_INSERT |
		SDHCI_INT_DATA_AVAIL | SDHCI_INT_SPACE_AVAIL |
		SDHCI_INT_DATA_END | SDHCI_INT_RESPONSE |
		SDHCI_INT_ADMA_ERROR;

	writel(intmask, host->ioaddr + SDHCI_INT_ENABLE);
	writel(intmask, host->ioaddr + SDHCI_SIGNAL_ENABLE);
}

static int s3c_hsmmc_init(struct mmc *mmc)
{
	struct sdhci_host *host = (struct sdhci_host *)mmc->priv;

	sdhci_reset(host, SDHCI_RESET_ALL);

	host->version = readw(host->ioaddr + SDHCI_HOST_VERSION);
	sdhci_init(host);

	sdhci_change_clock(host, 400000);

	return 0;
}

//static int s3c_hsmmc_initialize(int channel)
static int s3c_hsmmc_initialize( int channel, void *iomem )
{
	struct mmc *mmc;

	mmc = &mmc_channel[channel];

	sprintf(mmc->name, "S3C_HSMMC%d", channel);
	mmc->priv = &mmc_host[channel];
	mmc->send_cmd = s3c_hsmmc_send_command;
	mmc->set_ios = s3c_hsmmc_set_ios;
	mmc->init = s3c_hsmmc_init;

	mmc->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->host_caps = MMC_MODE_4BIT | MMC_MODE_HS_52MHz | MMC_MODE_HS;

	mmc->f_min = 400000;
	mmc->f_max = 52000000;

	mmc_host[channel].clock = 0;

	switch(channel) {
	case 0:
		mmc_host[channel].ioaddr = (void *)iomem;	// for S5PV210
		// gpio alter
		//REG_GPG0CON  = 0x02222022;
		//REG_GPG0PUD  = 0x00000010;
		//REG_CLK_SRC0 = 0x10001111;
		//REG_CLK_SRC4 = 0x66667777;
		break;
	case 1:
		mmc_host[channel].ioaddr = (void *)iomem;	// for S5PV210
		break;
	case 2:
		mmc_host[channel].ioaddr = (void *)iomem;	// for S5PV210
		break;
	case 3:
		mmc_host[channel].ioaddr = (void *)iomem;	// for S5PV210
		break;
	default:
		printf("mmc err: not supported channel %d\n", channel);
	}

	return mmc_register(mmc);
}

int zb_s3c_mmc_init( int channel, void *iomem )
{
	int    retry = 3;
	struct mmc *mmc;
	
	while( retry-- )
	{
		if ( 0 > s3c_hsmmc_initialize(channel, iomem) ) return -1;
		if ( 0 > mmc_initialize() ) return -1;       
    	
		mmc = find_mmc_device(0);
		if ( mmc && mmc->rca == 0xaaaa )
		{
			printf( "mmc fail.. so retry\n" );
			continue;
		}
    	else
    	{
			printf( "mmc active\n" );
			return 0;
		}
	}
	
	return -1;
}


