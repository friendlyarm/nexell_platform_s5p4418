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
#include <linux/jiffies.h>
// ADDED
#include "mmc.h"
#include "fsl_esdhc.h"
#include "imx-regs.h"
//#include "iomux-v3.h" //REMOVE
//#include "mx6q_pins.h"
//#include "crm_regs.h"



#define out_le32(a, v) __raw_writel(__cpu_to_le32(v), (a))
#define in_le32(a) __le32_to_cpu(__raw_readl(a))


#define clrsetbits(type, addr, clear, set) \
         out_##type((addr), (in_##type(addr) & ~(clear)) | (set))

#define clrsetbits_le32(addr, clear, set) clrsetbits(le32, addr, clear, set)


#define clrbits(type, addr, clear) \
	out_##type((addr), in_##type(addr) & ~(clear))

#define setbits(type, addr, set) \
	out_##type((addr), in_##type(addr) | (set))

#define clrbits_le32(addr, clear) clrbits(le32, addr, clear)
#define setbits_le32(addr, set) setbits(le32, addr, set)

struct fsl_esdhc *regs;// = (struct fsl_esdhc *)cfg->esdhc_base;


#if 0
struct mxc_ccm_reg *imx_ccm;// = 


#define USDHC_PAD_CTRL (PAD_CTL_PKE | PAD_CTL_PUE |	       \
	PAD_CTL_PUS_47K_UP  | PAD_CTL_SPEED_LOW |	       \
	PAD_CTL_DSE_80ohm   | PAD_CTL_SRE_FAST  | PAD_CTL_HYS)
	

iomux_v3_cfg_t const usdhc3_pads[] = {
	MX6_PAD_SD3_CLK__USDHC3_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_CMD__USDHC3_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT0__USDHC3_DAT0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT1__USDHC3_DAT1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT2__USDHC3_DAT2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT3__USDHC3_DAT3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD3_DAT5__GPIO_7_0    | MUX_PAD_CTRL(NO_PAD_CTRL), /* CD */
};

iomux_v3_cfg_t const usdhc4_pads[] = {
	MX6_PAD_SD4_CLK__USDHC4_CLK   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_CMD__USDHC4_CMD   | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT0__USDHC4_DAT0 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT1__USDHC4_DAT1 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT2__USDHC4_DAT2 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_SD4_DAT3__USDHC4_DAT3 | MUX_PAD_CTRL(USDHC_PAD_CTRL),
	MX6_PAD_NANDF_D6__GPIO_2_6    | MUX_PAD_CTRL(NO_PAD_CTRL), /* CD */
};
#endif

/*
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
	uint    admaerrstat;
	uint    admasysaddr;
	char    reserved2[4];
	uint    dllctrl;
	uint    dllstat;
	uint    clktunectrlstatus;
	char    reserved3[84];
	uint    vendorspec;
	uint    mmcboot;
	uint    vendorspec2;
	char	reserved4[48];
	uint	hostver;

};
*/

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

#if 0
static u32 get_usdhc_clk(u32 port)
{
	u32 root_freq = 0, usdhc_podf = 0, clk_sel = 0;
	u32 cscmr1 = __raw_readl(&imx_ccm->cscmr1);
	u32 cscdr1 = __raw_readl(&imx_ccm->cscdr1);

	switch (port) {
	case 0:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC1_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC1_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC1_CLK_SEL;

		break;
	case 1:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC2_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC2_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC2_CLK_SEL;

		break;
	case 2:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC3_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC3_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC3_CLK_SEL;

		break;
	case 3:
		usdhc_podf = (cscdr1 & MXC_CCM_CSCDR1_USDHC4_PODF_MASK) >>
					MXC_CCM_CSCDR1_USDHC4_PODF_OFFSET;
		clk_sel = cscmr1 & MXC_CCM_CSCMR1_USDHC4_CLK_SEL;

		break;
	default:
		break;
	}

	if (clk_sel)
		root_freq = PLL2_PFD0_FREQ;
	else
		root_freq = PLL2_PFD2_FREQ;

	return root_freq / (usdhc_podf + 1);
}
#endif

static int esdhc_reg_dump(struct mmc *mmc)
{
	printk("dsaddr = %8x\n",esdhc_read32(&mmc->regs->dsaddr));
	printk("blkattr = %8x\n",esdhc_read32(&mmc->regs->blkattr));
	printk("cmdarg = %8x\n",esdhc_read32(&mmc->regs->cmdarg));
	printk("xfertyp = %8x\n",esdhc_read32(&mmc->regs->xfertyp));
	printk("datport = %8x\n",esdhc_read32(&mmc->regs->datport));
	printk("prsstat = %8x\n",esdhc_read32(&mmc->regs->prsstat));
	printk("sysctl = %8x\n",esdhc_read32(&mmc->regs->sysctl));
	printk("proctl = %8x\n",esdhc_read32(&mmc->regs->proctl));
	printk("irqstat = %8x\n",esdhc_read32(&mmc->regs->irqstat));
	printk("irqstaten = %8x\n",esdhc_read32(&mmc->regs->irqstaten));
	printk("irqsigen = %8x\n",esdhc_read32(&mmc->regs->irqsigen));
	printk("autoc12err = %8x\n",esdhc_read32(&mmc->regs->autoc12err));
	printk("hostcapblt = %8x\n",esdhc_read32(&mmc->regs->hostcapblt));
	printk("wml = %8x\n",esdhc_read32(&mmc->regs->wml));
	printk("mixctrl = %8x\n",esdhc_read32(&mmc->regs->mixctrl));
	printk("hostver = %8x\n",esdhc_read32(&mmc->regs->hostver));
	printk("scr = %8x\n",esdhc_read32(&mmc->regs->scr));
	
	

}


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

#if 0
static inline int fls(int x)
{
         int r = 32;
 
        if (!x)
                 return 0;
         if (!(x & 0xffff0000u)) {
                 x <<= 16;
                 r -= 16;
         }
         if (!(x & 0xff000000u)) {
                 x <<= 8;
                 r -= 8;
         }
         if (!(x & 0xf0000000u)) {
                 x <<= 4;
                 r -= 4;
         }
         if (!(x & 0xc0000000u)) {
                x <<= 2;
                 r -= 2;
         }
         if (!(x & 0x80000000u)) {
                 x <<= 1;
                r -= 1;
        }
         return r;
 }
#endif

static int esdhc_setup_data(struct mmc *mmc, struct mmc_data *data)
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
				printk("\nThe SD card is locked. Can not write to a locked card.\n\n");
				return TIMEOUT;
			}
	
			esdhc_clrsetbits32(&mmc->regs->wml, WML_WR_WML_MASK,
						wml_value << 16);
			esdhc_write32(&mmc->regs->dsaddr, (u32)data->phys_addr);
			//printk("[DEBUG2] reg Address: %8x, wml_value=%8x\n",&regs->dsaddr,&regs->wml);
		}

	
	

	esdhc_write32(&mmc->regs->blkattr, data->blocks << 16 | data->blocksize);

	/* Calculate the timeout period for data transactions */
	/*
	 * 1)Timeout period = (2^(timeout+13)) SD Clock cycles
	 * 2)Timeout period should be minimum 0.250sec as per SD Card spec
	 *  So, Number of SD Clock cycles for 0.25sec should be minimum
	 *		(SD Clock/sec * 0.25 sec) SD Clock cycles
	 *		= (mmc->tran_speed * 1/4) SD Clock cycles
	 * As 1) >=  2)
	 * => (2^(timeout+13)) >= mmc->tran_speed * 1/4
	 * Taking log2 both the sides
	 * => timeout + 13 >= log2(mmc->tran_speed/4)
	 * Rounding up to next power of 2
	 * => timeout + 13 = log2(mmc->tran_speed/4) + 1
	 * => timeout + 13 = fls(mmc->tran_speed/4)
	 */
	 
	timeout = fls(mmc->tran_speed/4); //FIXME
	timeout -= 13;

	if (timeout > 14)
		timeout = 14;

	if (timeout < 0)
		timeout = 0;



	esdhc_clrsetbits32(&mmc->regs->sysctl, SYSCTL_TIMEOUT_MASK, timeout << 16);

	return 0;
}

#if 0
static void check_and_invalidate_dcache_range
	(struct mmc_cmd *cmd,
	 struct mmc_data *data) {
	 
	unsigned start = (unsigned)data->dest ;
	unsigned size = roundup(ARCH_DMA_MINALIGN,
				data->blocks*data->blocksize);
	unsigned end = start+size ;
	//invalidate_dcache_range(start, end);
}
#endif

/*
 * Sends a command out on the bus.  Takes the mmc pointer,
 * a command pointer, and an optional data pointer.
 */
int esdhc_send_cmd(struct mmc *mmc,struct mmc_cmd *cmd, struct mmc_data *data)
{
	uint	xfertyp;
	uint	irqstat;
	int timeout = 1000;


	//esdhc_reg_dump(mmc);

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
	//udelay(1);

	/* Set up for a data transfer if we have one */
	if (data) {
		int err;
		
		err = esdhc_setup_data( mmc, data);
		if(err)
			return err;
	}

	/* Figure out the transfer arguments */
	xfertyp = esdhc_xfertyp(cmd, data);

	/* Send the command */
	esdhc_write32(&mmc->regs->cmdarg, cmd->cmdarg);
#if defined(CONFIG_FSL_USDHC)
	esdhc_write32(&mmc->regs->mixctrl,
	(esdhc_read32(&mmc->regs->mixctrl) & 0xFFFFFF80) | (xfertyp & 0x7F));
	esdhc_write32(&mmc->regs->xfertyp, xfertyp & 0xFFFF0000);
#else
	esdhc_write32(&mmc->regs->xfertyp, xfertyp);
#endif

	/* Mask all irqs */
	esdhc_write32(&mmc->regs->irqsigen, 0);

	/* Wait for the command to complete */
	while (!(esdhc_read32(&mmc->regs->irqstat) & (IRQSTAT_CC | IRQSTAT_CTOE)) && --timeout ) {
		//printk("BUSY111\n");
		udelay(1);
	}	
	if ( timeout <= 0 ) {
		printk( " SEND COMMAND FAIL.. RETURNED \n");
		return TIMEOUT;
	}
	//esdhc_reg_dump(mmc);
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
		printk("CMD ERR!\n");
		return COMM_ERR;
	}

	if (irqstat & IRQSTAT_CTOE) {
			printk("TIMEOUT ERR!\n");
		return TIMEOUT;
		}
#if 0		
	/* Workaround for ESDHC errata ENGcm03648 */
	if (!data && (cmd->resp_type & MMC_RSP_BUSY)) {
		int timeout = 25000;

		/* Poll on DATA0 line for cmd with busy signal for 250 ms */
		while (timeout > 0 && !(esdhc_read32(&mmc->regs->prsstat) &
					PRSSTAT_DAT0)) {
			udelay(1);
			timeout--;
			printk("BUSY\n");
		}

		if (timeout <= 0) {
			printk("Timeout waiting for DAT0 to go high!\n");
			return TIMEOUT;
		}
	}
#endif
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

#if 0
	timeout = 1;
	/* Wait until all of the blocks are transferred */
	if (data) {
	
		do {
			irqstat = esdhc_read32(&mmc->regs->irqstat);

			if (irqstat & IRQSTAT_DTOE)
				return TIMEOUT;

			if (irqstat & DATA_ERR)
				return COMM_ERR;
			timeout--;
			udelay(1);
		} while (((irqstat & DATA_COMPLETE) != DATA_COMPLETE) && timeout );
		//if ( timeout <= 0 ) printk( " Wait Data Sending FAIL.. RETURNED \n");
	
	}
#endif

	esdhc_write32(&mmc->regs->irqstat, -1);

	return 0;
}



#if 0 //FOR DEBUG

static int esdhc_getcd(struct mmc *mmc)
{
	int timeout = 1000;

	while (!(esdhc_read32(&regs->prsstat) & PRSSTAT_CINS) && --timeout)
		udelay(1000);

	return timeout > 0;
}

static void esdhc_reset(struct fsl_esdhc *regs)
{
	unsigned long timeout = 100; /* wait max 100 ms */

	/* reset the controller */
	esdhc_write32(&regs->sysctl, SYSCTL_RSTA);

	/* hardware clears the bit when it is done */
	while ((esdhc_read32(&regs->sysctl) & SYSCTL_RSTA) && --timeout)
		udelay(1000);
	if (!timeout)
		printk("MMC/SD: Reset never completed.\n");
}

#endif


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

	udelay(1000);

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
		esdhc_setbits32(&mmc->regs->proctl, PROCTL_DTW_4); //BUS WIDTH ONLY 4BIT ??? FIXME?
	else if(mmc->bus_width ==8 )
		esdhc_setbits32(&mmc->regs->proctl, PROCTL_DTW_8); //BUS WIDTH ONLY 4BIT ??? FIXME?

	//esdhc_reg_dump(mmc);

}

int fsl_esdhc_mmc_init(struct mmc *mmc)
{
	
	int timeout = 1000;
	//mmc->regs = (struct fsl_esdhc *)ioremap_nocache(USDHC3_BASE_ADDR,USDHC_REGS_SIZE);
	mmc->regs = (struct fsl_esdhc *)ioremap_nocache(USDHC4_BASE_ADDR,USDHC_REGS_SIZE);


#if 0
	//imx_ccm = (struct mxc_ccm_reg *)ioremap_nocache(CCM_BASE_ADDR,0x20000);



	//imx_iomux_v3_setup_multiple_pads(
	//			usdhc3_pads, ARRAY_SIZE(usdhc3_pads));
	
	//imx_iomux_v3_setup_multiple_pads(
	//				usdhc4_pads, ARRAY_SIZE(usdhc4_pads));
#endif		

	mmc->host_caps = MMC_MODE_4BIT | MMC_MODE_8BIT | MMC_MODE_HC | MMC_MODE_HS_52MHz | MMC_MODE_HS;
	mmc->f_min = 400000;
	mmc->f_max = 52000000;
	mmc->voltages = MMC_VDD_32_33 | MMC_VDD_33_34;
	mmc->b_max = 65535;
	
	/* Reset the entire host controller */
	esdhc_write32(&mmc->regs->sysctl, SYSCTL_RSTA);


	/* Wait until the controller is available */
	while ((esdhc_read32(&mmc->regs->sysctl) & SYSCTL_RSTA) && --timeout)
		udelay(1000);
	

	if (!timeout)
		printk("MMC/SD: Reset never completed.\n");

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



