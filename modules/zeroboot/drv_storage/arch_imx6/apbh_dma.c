/*
 * Freescale i.MX28 APBH DMA driver
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * Based on code from LTIB:
 * Copyright (C) 2010 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
 

#include <linux/list.h>

//#include <common.h>
//#include <malloc.h>

#define CONFIG_MX6
#define CONFIG_SYS_DCACHE_OFF  //fix-me
#define CONFIG_APBH_DMA_BURST
#define CONFIG_APBH_DMA_BURST8


#include <asm/errno.h>
#include <asm/io.h>
#include "clock.h"
#include "imx-regs.h"
#include "sys_proto.h"
#include "dma.h"
#include "regs-apbh.h"
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <asm/delay.h>

 #define DMA_ERROR_CODE  (~0)

#define roundup(x, y)		((((x) + ((y) - 1)) / (y)) * (y))

extern void __iomem *mxs_apbh_base_virt; 

static struct mxs_dma_chan mxs_dma_channels[MXS_MAX_DMA_CHANNELS];


/*
 * Test is the DMA channel is valid channel
 */
int mxs_dma_validate_chan(int channel)
{
	struct mxs_dma_chan *pchan;
	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;
	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -EINVAL;

	return 0;
}

/*
 * Return the address of the command within a descriptor.
 */
static unsigned int mxs_dma_cmd_address(struct mxs_dma_desc *desc)
{
	return desc->address;// + offsetof(struct mxs_dma_desc, cmd);
}

/*
 * Read a DMA channel's hardware semaphore.
 *
 * As used by the MXS platform's DMA software, the DMA channel's hardware
 * semaphore reflects the number of DMA commands the hardware will process, but
 * has not yet finished. This is a volatile value read directly from hardware,
 * so it must be be viewed as immediately stale.
 *
 * If the channel is not marked busy, or has finished processing all its
 * commands, this value should be zero.
 *
 * See mxs_dma_append() for details on how DMA command blocks must be configured
 * to maintain the expected behavior of the semaphore's value.
 */
static int mxs_dma_read_semaphore(int channel)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;
	uint32_t tmp;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	tmp = readl(&apbh_regs->ch[channel].hw_apbh_ch_sema);

	tmp &= APBH_CHn_SEMA_PHORE_MASK;
	tmp >>= APBH_CHn_SEMA_PHORE_OFFSET;

	return tmp;
}


/*
 * Enable a DMA channel.
 *
 * If the given channel has any DMA descriptors on its active list, this
 * function causes the DMA hardware to begin processing them.
 *
 * This function marks the DMA channel as "busy," whether or not there are any
 * descriptors to process.
 */
static int mxs_dma_enable(int channel)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;
	unsigned int sem;
	struct mxs_dma_chan *pchan;
	struct mxs_dma_desc *pdesc;
	int ret;
	struct mxs_dma_cmd *mxs_dma_cmd_debug;
	
	

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	if (pchan->pending_num == 0) {
		pchan->flags |= MXS_DMA_FLAGS_BUSY;
		return 0;
	}

	pdesc = list_first_entry(&pchan->active, struct mxs_dma_desc, node);

	
	if (pdesc == NULL)
		return -EFAULT;	

	if (pchan->flags & MXS_DMA_FLAGS_BUSY) {
	
		if (!(pdesc->cmd.data & MXS_DMA_DESC_CHAIN))
			return 0;

		sem = mxs_dma_read_semaphore(channel);
		if (sem == 0)
			return 0;

		if (sem == 1) {
			pdesc = list_entry(pdesc->node.next,
					   struct mxs_dma_desc, node);
			writel( mxs_dma_cmd_address(pdesc),
				&apbh_regs->ch[channel].hw_apbh_ch_nxtcmdar);
		}
		writel(pchan->pending_num,
			&apbh_regs->ch[channel].hw_apbh_ch_sema);
		pchan->active_num += pchan->pending_num;
		pchan->pending_num = 0;
	} else {
	
		pchan->active_num += pchan->pending_num;
		pchan->pending_num = 0;
		writel(mxs_dma_cmd_address(pdesc),  
			&apbh_regs->ch[channel].hw_apbh_ch_nxtcmdar);
		writel(pchan->active_num,
			&apbh_regs->ch[channel].hw_apbh_ch_sema);
		writel(1 << (channel + APBH_CTRL0_CLKGATE_CHANNEL_OFFSET),
			&apbh_regs->hw_apbh_ctrl0_clr);
	}
	pchan->flags |= MXS_DMA_FLAGS_BUSY;
	return 0;
}

/*
 * Disable a DMA channel.
 *
 * This function shuts down a DMA channel and marks it as "not busy." Any
 * descriptors on the active list are immediately moved to the head of the
 * "done" list, whether or not they have actually been processed by the
 * hardware. The "ready" flags of these descriptors are NOT cleared, so they
 * still appear to be active.
 *
 * This function immediately shuts down a DMA channel's hardware, aborting any
 * I/O that may be in progress, potentially leaving I/O hardware in an undefined
 * state. It is unwise to call this function if there is ANY chance the hardware
 * is still processing a command.
 */
static int mxs_dma_disable(int channel)
{
	struct mxs_dma_chan *pchan;
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	if (!(pchan->flags & MXS_DMA_FLAGS_BUSY))
		return -EINVAL;

	writel(1 << (channel + APBH_CTRL0_CLKGATE_CHANNEL_OFFSET),
		&apbh_regs->hw_apbh_ctrl0_set);

	pchan->flags &= ~MXS_DMA_FLAGS_BUSY;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	list_splice_init(&pchan->active, &pchan->done);

	return 0;
}

/*
 * Resets the DMA channel hardware.
 */
static int mxs_dma_reset(int channel)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;
	int ret;

	uint32_t setreg = (uint32_t)(&apbh_regs->hw_apbh_channel_ctrl_set);
	uint32_t offset = APBH_CHANNEL_CTRL_RESET_CHANNEL_OFFSET;


	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;
	writel(1 << (channel + offset), setreg);

	return 0;
}

/*
 * Enable or disable DMA interrupt.
 *
 * This function enables the given DMA channel to interrupt the CPU.
 */
static int mxs_dma_enable_irq(int channel, int enable)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;
	
	if (enable)
		writel(1 << (channel + APBH_CTRL1_CH_CMDCMPLT_IRQ_EN_OFFSET),
			&apbh_regs->hw_apbh_ctrl1_set);
	else
		writel(1 << (channel + APBH_CTRL1_CH_CMDCMPLT_IRQ_EN_OFFSET),
			&apbh_regs->hw_apbh_ctrl1_clr);
	return 0;
}

/*
 * Clear DMA interrupt.
 *
 * The software that is using the DMA channel must register to receive its
 * interrupts and, when they arrive, must call this function to clear them.
 */
static int mxs_dma_ack_irq(int channel)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	writel(1 << channel, &apbh_regs->hw_apbh_ctrl1_clr);
	writel(1 << channel, &apbh_regs->hw_apbh_ctrl2_clr);

	return 0;
}

/*
 * Request to reserve a DMA channel
 */
static int mxs_dma_request(int channel)
{
	struct mxs_dma_chan *pchan;

	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -EINVAL;

	pchan = mxs_dma_channels + channel;
	if ((pchan->flags & MXS_DMA_FLAGS_VALID) != MXS_DMA_FLAGS_VALID)
		return -ENODEV;

	if (pchan->flags & MXS_DMA_FLAGS_ALLOCATED)
		return -EBUSY;

	pchan->flags |= MXS_DMA_FLAGS_ALLOCATED;
	pchan->active_num = 0;
	pchan->pending_num = 0;

	INIT_LIST_HEAD(&pchan->active);
	INIT_LIST_HEAD(&pchan->done);

	return 0;
}

/*
 * Release a DMA channel.
 *
 * This function releases a DMA channel from its current owner.
 *
 * The channel will NOT be released if it's marked "busy" (see
 * mxs_dma_enable()).
 */
int mxs_dma_release(int channel)
{
	struct mxs_dma_chan *pchan;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	if (pchan->flags & MXS_DMA_FLAGS_BUSY)
		return -EBUSY;

	pchan->dev = 0;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	pchan->flags &= ~MXS_DMA_FLAGS_ALLOCATED;

	return 0;
}

/*
 * Allocate DMA descriptor
 */
struct mxs_dma_desc *mxs_dma_desc_alloc()
{
	struct mxs_dma_desc *pdesc;
	uint32_t size;
	dma_addr_t phy=NULL;
	
	size = roundup(sizeof(struct mxs_dma_desc), MXS_DMA_ALIGNMENT);
	pdesc = dma_alloc_coherent(NULL,PAGE_SIZE,&phy,GFP_DMA);
		
	if (pdesc == NULL)
		return NULL;

	memset(pdesc, 0, sizeof(*pdesc));
	pdesc->address = phy;
	
	return pdesc;
};

/*
 * Free DMA descriptor
 */
void mxs_dma_desc_free(struct mxs_dma_desc *pdesc)
{
	if (pdesc == NULL)
		return;
	dma_free_coherent(NULL,PAGE_SIZE,pdesc,pdesc->address);
}

int mxs_dma_desc_append(int channel, struct mxs_dma_desc *pdesc)
{
	struct mxs_dma_chan *pchan;
	struct mxs_dma_desc *last;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	pdesc->cmd.next = mxs_dma_cmd_address(pdesc); //JHYOON
	pdesc->flags |= MXS_DMA_DESC_FIRST | MXS_DMA_DESC_LAST;

	if (!list_empty(&pchan->active)) {
		last = list_entry(pchan->active.prev, struct mxs_dma_desc,
					node);

		pdesc->flags &= ~MXS_DMA_DESC_FIRST;
		last->flags &= ~MXS_DMA_DESC_LAST;

		last->cmd.next = mxs_dma_cmd_address(pdesc); //PHYS
		last->cmd.data |= MXS_DMA_DESC_CHAIN;
		
	}
	pdesc->flags |= MXS_DMA_DESC_READY;
	if (pdesc->flags & MXS_DMA_DESC_FIRST)
		pchan->pending_num++;
	list_add_tail(&pdesc->node, &pchan->active);

	return ret;
}


static int mxs_dma_finish(int channel, struct list_head *head)
{
	int sem;
	struct mxs_dma_chan *pchan;
	struct list_head *p, *q;
	struct mxs_dma_desc *pdesc;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	sem = mxs_dma_read_semaphore(channel);
	if (sem < 0)
		return sem;

	if (sem == pchan->active_num)
		return 0;

	list_for_each_safe(p, q, &pchan->active) {
		if ((pchan->active_num) <= sem)
			break;

		pdesc = list_entry(p, struct mxs_dma_desc, node);
		pdesc->flags &= ~MXS_DMA_DESC_READY;

		if (head)
			list_move_tail(p, head);
		else
			list_move_tail(p, &pchan->done);

		if (pdesc->flags & MXS_DMA_DESC_LAST)
			pchan->active_num--;
	}

	if (sem == 0)
		pchan->flags &= ~MXS_DMA_FLAGS_BUSY;

	return 0;
}

/*
 * Wait for DMA channel to complete
 */
static int mxs_dma_wait_complete(uint32_t timeout, unsigned int chan)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;
	int ret;
	
	ret = mxs_dma_validate_chan(chan);
	if (ret)
		return ret;

	if (mxs_wait_mask_set(&apbh_regs->hw_apbh_ctrl1_reg,
				1 << chan, timeout)) {
						
		ret = -ETIMEDOUT;
		mxs_dma_reset(chan);
	}

	return ret;
}

/*
 * Execute the DMA channel
 */
int mxs_dma_go(int chan)
{
	//uint32_t timeout = 10000000;
	uint32_t timeout = 100000;
	//uint32_t timeout = 1000;
	int ret;
	
	LIST_HEAD(tmp_desc_list);		
	//mxs_reset_block(&apbh_regs->hw_apbh_ctrl0);		
	//mxs_dma_enable_irq(chan, 1);
	
	mxs_dma_enable(chan);
	
	/* Wait for DMA to finish. */
	ret = mxs_dma_wait_complete(timeout, chan);
	/* Clear out the descriptors we just ran. */
	mxs_dma_finish(chan, &tmp_desc_list);
	/* Shut the DMA channel down. */
	mxs_dma_ack_irq(chan);
	mxs_dma_reset(chan);
	mxs_dma_enable_irq(chan, 0);
	mxs_dma_disable(chan);	

	//udelay(1000);
	
	return ret;
}

/*
 * Initialize the DMA hardware
 */
void mxs_dma_init(void)
{
	
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;

	
	mxs_reset_block(&apbh_regs->hw_apbh_ctrl0_reg);

#ifdef CONFIG_APBH_DMA_BURST8
	writel(APBH_CTRL0_AHB_BURST8_EN,
		&apbh_regs->hw_apbh_ctrl0_set);
#else
	writel(APBH_CTRL0_AHB_BURST8_EN,
		&apbh_regs->hw_apbh_ctrl0_clr);
#endif

#ifdef CONFIG_APBH_DMA_BURST
	writel(APBH_CTRL0_APB_BURST_EN,
		&apbh_regs->hw_apbh_ctrl0_set);
#else
	writel(APBH_CTRL0_APB_BURST_EN,
		&apbh_regs->hw_apbh_ctrl0_clr);
#endif

}

int mxs_dma_init_channel(int channel)
{
	struct mxs_dma_chan *pchan;
	int ret;
		struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)mxs_apbh_base_virt;


		
	pchan = mxs_dma_channels + channel;
	pchan->flags = MXS_DMA_FLAGS_VALID;
	ret = mxs_dma_request(channel);
	if (ret) {
		printk("MXS DMA: Can't acquire DMA channel %i\n",
			channel);
		return ret;
	}
	mxs_dma_reset(channel);
	mxs_dma_ack_irq(channel);
	
	
	return 0;
}
