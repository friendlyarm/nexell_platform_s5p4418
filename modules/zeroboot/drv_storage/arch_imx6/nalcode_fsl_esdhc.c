/*
 * Copyright 2007, 2010-2013 Freescale Semiconductor, Inc.
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
#define ARCH_MXC  // FROME arch/arm/include/asm/arch-mx6/imx-regs.h
#define CONFIG_FSL_ESDHC // include/configs/sabrelite.h
#define CONFIG_FSL_USDHC


#define AIPS2_ARB_BASE_ADDR             0x02100000
#define ATZ2_BASE_ADDR              AIPS2_ARB_BASE_ADDR
#define AIPS2_OFF_BASE_ADDR         (ATZ2_BASE_ADDR + 0x80000)
#define USDHC1_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x10000)
#define USDHC2_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x14000)
#define USDHC3_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x18000)
#define USDHC4_BASE_ADDR            (AIPS2_OFF_BASE_ADDR + 0x1C000)
#define USDHC_REGS_SIZE			0x4000


#define USDHC_CLK				0xBEBC200  //200MHZ
#define MMC_CLK					0x2FAF080  // 50MHZ
//#define MMC_CLK					0x61a80  // 4MHZ
#define SDHC_F_MIN	 400000

// ADDED

#include <typedef.h>
#include "mmc.h"
#include "fsl_esdhc.h"
#include "imx-regs.h"
#include <stdio.h>
#include <string.h>
#include <uart.h>


static inline u32 readl(const volatile u32 *addr)
{
  return *(const volatile u32  *) addr;
}

static inline void writel(u32 b, volatile u32  *addr)
{
       *(volatile u32  *) addr = b;
}

static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
		"bne 1b" : "=r" (loops) : "0"(loops));
}


//#define out_le32(a, v) __raw_writel(__cpu_to_le32(v), (a))
//#define in_le32(a) __le32_to_cpu(__raw_readl(a))

#define out_le32(a, v) writel((v), (a))
#define in_le32(a) readl(a)


#define clrsetbits(type, addr, clear, set) \
         out_##type((addr), (in_##type(addr) & ~(clear)) | (set))

#define clrsetbits_le32(addr, clear, set) clrsetbits(le32, addr, clear, set)


#define clrbits(type, addr, clear) \
	out_##type((addr), in_##type(addr) & ~(clear))

#define setbits(type, addr, set) \
	out_##type((addr), in_##type(addr) | (set))

#define clrbits_le32(addr, clear) clrbits(le32, addr, clear)
#define setbits_le32(addr, set) setbits(le32, addr, set)


struct fsl_esdhc {
	uint	dsaddr;
	uint	blkattr;
	uint	cmdarg;
	uint	xfertyp;
	uint	cmdrsp0;
	uint	cmdrsp1;
	uint	cmdrsp2;
	uint	cmdrsp3;
	uint	datport;
	uint	prsstat;
	uint	proctl;
	uint	sysctl;
	uint	irqstat;
	uint	irqstaten;
	uint	irqsigen;
	uint	autoc12err;
	uint	hostcapblt;
	uint	wml;
	uint    mixctrl;
	char    reserved1[4];
	uint	fevt;
	char	reserved2[168];
	uint	hostver;
	char	reserved3[780];
	uint	scr;
};



/* Return the XFERTYP flags for a given command and data packet */
static uint esdhc_xfertyp(struct mmc_cmd *cmd, struct mmc_data *data)
{
	uint xfertyp = 0;

	if (data) {
		xfertyp |= XFERTYP_DPSEL;
		xfertyp |= XFERTYP_DMAEN;
		if (data->blocks > 1) {
			xfertyp |= XFERTYP_MSBSEL;
			xfertyp |= XFERTYP_BCEN;
		}

		if (data->flags & MMC_DATA_READ)
			xfertyp |= XFERTYP_DTDSEL;
	}

	if (cmd->resp_type & MMC_RSP_CRC)
		xfertyp |= XFERTYP_CCCEN;
	if (cmd->resp_type & MMC_RSP_OPCODE)
		xfertyp |= XFERTYP_CICEN;
	if (cmd->resp_type & MMC_RSP_136)
		xfertyp |= XFERTYP_RSPTYP_136;
	else if (cmd->resp_type & MMC_RSP_BUSY)
		xfertyp |= XFERTYP_RSPTYP_48_BUSY;
	else if (cmd->resp_type & MMC_RSP_PRESENT)
		xfertyp |= XFERTYP_RSPTYP_48;

	return XFERTYP_CMD(cmd->cmdidx) | xfertyp;
}


static int esdhc_setup_data( struct mmc *mmc,struct mmc_data *data)
{
	int timeout;
		uint wml_value;
	
		wml_value = data->blocksize/4;
	
		if (data->flags & MMC_DATA_READ) {
			if (wml_value > WML_RD_WML_MAX)
				wml_value = WML_RD_WML_MAX_VAL;
	
			esdhc_clrsetbits32(&mmc->regs->wml, WML_RD_WML_MASK, wml_value);
			esdhc_write32(&mmc->regs->dsaddr, (u32)data->phys_addr);
			//printk("[DEBUG1] Block size: %8x,phys:%8x, reg Address: %8x, wml_value=%8x\n", data->blocksize, (u32)data->phys_addr,&regs->dsaddr,&regs->wml);
		} else {
			
	
			if (wml_value > WML_WR_WML_MAX)
				wml_value = WML_WR_WML_MAX_VAL;
			if ((esdhc_read32(&mmc->regs->prsstat) & PRSSTAT_WPSPL) == 0) {
				uart_puts("\nThe SD card is locked. Can not write to a locked card.\n\n");
				return TIMEOUT;
			}
	
			esdhc_clrsetbits32(&mmc->regs->wml, WML_WR_WML_MASK,
						wml_value << 16);
			esdhc_write32(&mmc->regs->dsaddr, (u32)data->phys_addr);
			//printk("[DEBUG2] reg Address: %8x, wml_value=%8x\n",&regs->dsaddr,&regs->wml);
		}

	
	esdhc_write32(&mmc->regs->blkattr, data->blocks << 16 | data->blocksize);

	timeout = 14; //FIXME

	if (timeout > 14)
		timeout = 14;

	if (timeout < 0)
		timeout = 0;



	esdhc_clrsetbits32(&mmc->regs->sysctl, SYSCTL_TIMEOUT_MASK, timeout << 16);

	return 0;
}


/*
 * Sends a command out on the bus.  Takes the mmc pointer,
 * a command pointer, and an optional data pointer.
 */
int esdhc_send_cmd(struct mmc *mmc,struct mmc_cmd *cmd, struct mmc_data *data)
{
	uint	xfertyp;
	uint	irqstat;
	int timeout = 1000;


	//esdhc_reg_dump();

	//printk("CLK SDHC : %d\n",get_usdhc_clk(3));

	esdhc_write32(&mmc->regs->irqstat, -1);

	//sync(); //FIXME

	/* Wait for the bus to be idle */
	while ((esdhc_read32(&mmc->regs->prsstat) & PRSSTAT_CICHB) ||
			(esdhc_read32(&mmc->regs->prsstat) & PRSSTAT_CIDHB))
		;

	while (esdhc_read32(&mmc->regs->prsstat) & PRSSTAT_DLA)
		;

	/* Wait at least 8 SD clock cycles before the next command */
	/*
	 * Note: This is way more than 8 cycles, but 1ms seems to
	 * resolve timing issues with some cards
	 */
	delay(1000);

	/* Set up for a data transfer if we have one */
	if (data) {
		int err;
		
		err = esdhc_setup_data(mmc, data);
		if(err)
			return err;
	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	/* Send the command */
	esdhc_write32(&mmc->regs->cmdarg, cmd->cmdarg);
	esdhc_write32(&mmc->regs->mixctrl,
	(esdhc_read32(&mmc->regs->mixctrl) & 0xFFFFFF80) | (xfertyp & 0x7F));
	esdhc_write32(&mmc->regs->xfertyp, xfertyp & 0xFFFF0000);


	/* Mask all irqs */
	esdhc_write32(&mmc->regs->irqsigen, 0);

	/* Wait for the command to complete */
	while (!(esdhc_read32(&mmc->regs->irqstat) & (IRQSTAT_CC | IRQSTAT_CTOE)) && --timeout ) {
		delay(1000);
	}	
	if ( timeout <= 0 ) {
		uart_puts( " SEND COMMAND FAIL.. RETURNED \n");
		return TIMEOUT;
	}
	//esdhc_reg_dump();
	irqstat = esdhc_read32(&mmc->regs->irqstat);
	esdhc_write32(&mmc->regs->irqstat, irqstat);

	/* Reset CMD and DATA portions on error */
	if (irqstat & (CMD_ERR | IRQSTAT_CTOE)) {
		esdhc_write32(&mmc->regs->sysctl, esdhc_read32(&mmc->regs->sysctl) |
			      SYSCTL_RSTC);
		while (esdhc_read32(&mmc->regs->sysctl) & SYSCTL_RSTC)
			;

		if (data) {
			esdhc_write32(&mmc->regs->sysctl,
				      esdhc_read32(&mmc->regs->sysctl) |
				      SYSCTL_RSTD);
			while ((esdhc_read32(&mmc->regs->sysctl) & SYSCTL_RSTD))
				;
		}
	}

	if (irqstat & CMD_ERR) {
		uart_puts("CMD ERR!\n");
		return COMM_ERR;
	}

	if (irqstat & IRQSTAT_CTOE) {
			uart_puts("TIMEOUT ERR2222!\n");
		return TIMEOUT;
		}
	/* Workaround for ESDHC errata ENGcm03648 */
	if (!data && (cmd->resp_type & MMC_RSP_BUSY)) {
		int timeout = 2500;

		/* Poll on DATA0 line for cmd with busy signal for 250 ms */
		while (timeout > 0 && !(esdhc_read32(&mmc->regs->prsstat) &
					PRSSTAT_DAT0)) {
			delay(1000);
			timeout--;
		}

		if (timeout <= 0) {
			uart_puts("Timeout waiting for DAT0 to go high!\n");
			return TIMEOUT;
		}
	}

	/* Copy the response to the response buffer */
	if (cmd->resp_type & MMC_RSP_136) {
		u32 cmdrsp3, cmdrsp2, cmdrsp1, cmdrsp0;

		cmdrsp3 = esdhc_read32(&mmc->regs->cmdrsp3);
		cmdrsp2 = esdhc_read32(&mmc->regs->cmdrsp2);
		cmdrsp1 = esdhc_read32(&mmc->regs->cmdrsp1);
		cmdrsp0 = esdhc_read32(&mmc->regs->cmdrsp0);
		cmd->response[0] = (cmdrsp3 << 8) | (cmdrsp2 >> 24);
		cmd->response[1] = (cmdrsp2 << 8) | (cmdrsp1 >> 24);
		cmd->response[2] = (cmdrsp1 << 8) | (cmdrsp0 >> 24);
		cmd->response[3] = (cmdrsp0 << 8);
	} else
		cmd->response[0] = esdhc_read32(&mmc->regs->cmdrsp0);


#if 0 //CHECK ME..
	timeout = 100;
	/* Wait until all of the blocks are transferred */
	if (data) {
	
		do {
			irqstat = esdhc_read32(&mmc->regs->irqstat);

			if (irqstat & IRQSTAT_DTOE)
				return TIMEOUT;

			if (irqstat & DATA_ERR)
				return COMM_ERR;
			timeout--;
			delay(100);
		} while (((irqstat & DATA_COMPLETE) != DATA_COMPLETE) && timeout );

	
	}
#endif

	esdhc_write32(&mmc->regs->irqstat, -1);

	return 0;
}


static void set_sysctl(struct mmc *mmc, uint clock)
{
	int div, pre_div;
	int sdhc_clk = USDHC_CLK;
	uint clk;

	if (clock < mmc->f_min)
		clock = mmc->f_min;
	if (sdhc_clk / 16 > clock) {
		for (pre_div = 2; pre_div < 256; pre_div *= 2)
			if ((sdhc_clk / pre_div) <= (clock * 16))
				break;
	} else
		pre_div = 2;

	for (div = 1; div <= 16; div++)
		if ((sdhc_clk / (div * pre_div)) <= clock)
			break;

	pre_div >>= 1;
	div -= 1;

	clk = (pre_div << 8) | (div << 4);

	esdhc_clrbits32(&mmc->regs->sysctl, SYSCTL_CKEN);

	esdhc_clrsetbits32(&mmc->regs->sysctl, SYSCTL_CLOCK_MASK, clk);

	delay(1000);

	clk = SYSCTL_PEREN | SYSCTL_CKEN;

	esdhc_setbits32(&mmc->regs->sysctl, clk);
}



void esdhc_set_ios(struct mmc *mmc)
{
	
	/* Set the clock speed */
	set_sysctl(mmc, mmc->clock);

	/* Set the bus width */
	esdhc_clrbits32(&mmc->regs->proctl, PROCTL_DTW_4 | PROCTL_DTW_8);

	if ( mmc->bus_width == 4 ) 
		esdhc_setbits32(&mmc->regs->proctl, PROCTL_DTW_4); 
	else if(mmc->bus_width ==8 )
		esdhc_setbits32(&mmc->regs->proctl, PROCTL_DTW_8);

	//esdhc_reg_dump();

}

int fsl_esdhc_mmc_init(struct mmc *mmc)
{
	
	int timeout = 1000;
	
		
	mmc->host_caps = MMC_MODE_4BIT | MMC_MODE_8BIT | MMC_MODE_HC | MMC_MODE_HS_52MHz | MMC_MODE_HS;
	mmc->f_min = 400000;
	mmc->f_max = 52000000;
	mmc->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->b_max = 65535;
	
	/* Reset the entire host controller */
	esdhc_write32(&mmc->regs->sysctl, SYSCTL_RSTA);


	/* Wait until the controller is available */
	while ((esdhc_read32(&mmc->regs->sysctl) & SYSCTL_RSTA) && --timeout)
		delay(1000);
	

	if (!timeout)
		uart_puts("MMC/SD: Reset never completed.\n");

	/*esdhc_setbits32(&regs->sysctl, SYSCTL_PEREN | SYSCTL_HCKEN
				| SYSCTL_IPGEN | SYSCTL_CKEN);*/


#ifndef ARCH_MXC
	/* Enable cache snooping */
	esdhc_write32(&mmc->regs->scr, 0x00000040);
#endif

	esdhc_write32(&mmc->regs->sysctl, SYSCTL_HCKEN | SYSCTL_IPGEN);

	/* Set the initial clock speed */
	mmc_set_clock(mmc, 400000);


	/* Disable the BRR and BWR bits in IRQSTAT */
	esdhc_clrbits32(&mmc->regs->irqstaten, IRQSTATEN_BRR | IRQSTATEN_BWR);
	

	/* Put the PROCTL reg back to the default */
	esdhc_write32(&mmc->regs->proctl, PROCTL_INIT);

	/* Set timout to the maximum value */
	esdhc_clrsetbits32(&mmc->regs->sysctl, SYSCTL_TIMEOUT_MASK, 14 << 16);
	


	return 0;
}



