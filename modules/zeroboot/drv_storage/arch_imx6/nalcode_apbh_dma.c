/**    
    @file     nalcode_apbh_dma.c
    @date     2014/03/27
    @author   jonghooy@sevo-systems Sevo Systems Co.,Ltd.
    @brief    i.mx6q nalcode nand storage  driver
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
#include "list.h"
#include "clock.h"
#include "imx-regs.h"
#include "dma.h"
#include "regs-apbh.h"
#include "nalcode_mxs_nand.h"


extern struct mxs_nand_info nand_info;


static struct mxs_dma_chan mxs_dma_channels[MXS_MAX_DMA_CHANNELS];




/*
 * Test is the DMA channel is valid channel
 */
int mxs_dma_validate_chan(int channel)
{
	struct mxs_dma_chan *pchan;
	if ((channel < 0) || (channel >= MXS_MAX_DMA_CHANNELS))
		return -1;
	pchan = mxs_dma_channels + channel;
	if (!(pchan->flags & MXS_DMA_FLAGS_ALLOCATED))
		return -1;

	return 0;
}

/*
 * Return the address of the command within a descriptor.
 */
static unsigned int mxs_dma_cmd_address(struct mxs_dma_desc *desc)
{
	return desc->address;// + offsetof(struct mxs_dma_desc, cmd);
}

static int mxs_dma_read_semaphore(int channel)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
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
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
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

	
	if (pdesc == ((void*)0))
		return -1;	

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

static int mxs_dma_disable(int channel)
{
	struct mxs_dma_chan *pchan;
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
	int ret;

	ret = mxs_dma_validate_chan(channel);
	if (ret)
		return ret;

	pchan = mxs_dma_channels + channel;

	if (!(pchan->flags & MXS_DMA_FLAGS_BUSY))
		return -1;

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
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
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
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
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
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
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
		return -1;

	pchan = mxs_dma_channels + channel;
	if ((pchan->flags & MXS_DMA_FLAGS_VALID) != MXS_DMA_FLAGS_VALID)
		return -1;

	if (pchan->flags & MXS_DMA_FLAGS_ALLOCATED)
		return -1;

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
		return -1;

	pchan->dev = 0;
	pchan->active_num = 0;
	pchan->pending_num = 0;
	pchan->flags &= ~MXS_DMA_FLAGS_ALLOCATED;

	return 0;
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


int mxs_wait_mask_clr(struct mxs_register_32 *reg, uint32_t mask)
{
	while (1) {
		if ((readl(&reg->reg) & mask) == 0)
			break;
		delay(1);
	}

	return 0;
}

int mxs_wait_mask_set(struct mxs_register_32 *reg, uint32_t mask)
{
		
	while (1) {
		if ((readl(&reg->reg) & mask) == mask) 
			break;
	
	}

	return 0;
}

/*
 * Wait for DMA channel to complete
 */

static int mxs_dma_wait_complete(unsigned int chan)
{
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
	int ret;
	
	ret = mxs_dma_validate_chan(chan);
	if (ret)
		return ret;

	if (mxs_wait_mask_set(&apbh_regs->hw_apbh_ctrl1_reg,
				1 << chan)) {
						
		ret = -1;
		mxs_dma_reset(chan);
	}

	return ret;
}

/*
 * Execute the DMA channel
 */
int mxs_dma_go(int chan)
{
	int ret;

	LIST_HEAD(tmp_desc_list);	

	mxs_dma_enable(chan);

	
	/* Wait for DMA to finish. */
	ret = mxs_dma_wait_complete(chan);
	//uart_puts("dma wait complete .");
	/* Clear out the descriptors we just ran. */
	mxs_dma_finish(chan, &tmp_desc_list);


	/* Shut the DMA channel down. */
	mxs_dma_ack_irq(chan);
	mxs_dma_reset(chan);
	mxs_dma_enable_irq(chan, 0);
	mxs_dma_disable(chan);	
	
	return ret;
}

int mxs_reset_block(struct mxs_register_32 *reg)
{
	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_clr);

	if (mxs_wait_mask_clr(reg, MXS_BLOCK_SFTRST))
		return 1;
	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, &reg->reg_clr);
	/* Set SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_set);
	/* Wait for CLKGATE being set */
	if (mxs_wait_mask_set(reg, MXS_BLOCK_CLKGATE))
		return 1;
	/* Clear SFTRST */
	writel(MXS_BLOCK_SFTRST, &reg->reg_clr);
	if (mxs_wait_mask_clr(reg, MXS_BLOCK_SFTRST))
		return 1;
	/* Clear CLKGATE */
	writel(MXS_BLOCK_CLKGATE, &reg->reg_clr);
	if (mxs_wait_mask_clr(reg, MXS_BLOCK_CLKGATE))
		return 1;
	return 0;
}


/*
 * Initialize the DMA hardware
 */
void mxs_dma_init(void)
{

	
	struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;

	mxs_reset_block(&apbh_regs->hw_apbh_ctrl0_reg);

	writel(APBH_CTRL0_AHB_BURST8_EN,
		&apbh_regs->hw_apbh_ctrl0_set);
	writel(APBH_CTRL0_APB_BURST_EN,
		&apbh_regs->hw_apbh_ctrl0_set);


}

int mxs_dma_init_channel(int channel)
{
	struct mxs_dma_chan *pchan;
	int ret;
		struct mxs_apbh_regs *apbh_regs =
		(struct mxs_apbh_regs *)nand_info.mxs_apbh_base;
		
	pchan = mxs_dma_channels + channel;
	pchan->flags = MXS_DMA_FLAGS_VALID;
	ret = mxs_dma_request(channel);
	if (ret) {
		uart_puts("MXS DMA: Can't acquire DMA channel \n");

		return ret;
	}
	mxs_dma_reset(channel);
	mxs_dma_ack_irq(channel);
	
	
	return 0;
}
