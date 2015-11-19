/**    
    @file     nalcode_mxs_nand.c
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
#include "imx-regs.h"
#include "regs-bch.h"
#include "regs-gpmi.h"
#include "crm_regs.h"
#include "dma.h"
#include "nalcode_mxs_nand.h"


#include <stdio.h>
#include <string.h>
#include <uart.h>



#define DATABUF_SIZE	NAND_MAX_PAGESIZE + NAND_MAX_OOBSIZE


#define	MXS_NAND_DMA_DESCRIPTOR_COUNT		4
#define	MXS_NAND_CHUNK_DATA_CHUNK_SIZE		512
#define	MXS_NAND_CHUNK_DATA_CHUNK_SIZE_SHIFT	2
#define	MXS_NAND_METADATA_SIZE			10
#define	MXS_NAND_COMMAND_BUFFER_SIZE		32
#define	MXS_NAND_BCH_TIMEOUT			10000



struct mxs_nand_info nand_info;


static void setup_gpmi_nand(void)
{
	uint32_t tmp;
	struct mxc_ccm_reg *mxc_ccm = (struct mxc_ccm_reg *)nand_info.ccm_base;

	/* gate ENFC_CLK_ROOT clock first,before clk source switch */
	clrbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* config gpmi and bch clock to 100 MHz */
	clrsetbits_le32(&mxc_ccm->cs2cdr,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED_MASK |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL_MASK,
			MXC_CCM_CS2CDR_ENFC_CLK_PODF(0) |
			MXC_CCM_CS2CDR_ENFC_CLK_PRED(3) |
			MXC_CCM_CS2CDR_ENFC_CLK_SEL(3));

	/* enable ENFC_CLK_ROOT clock */
	setbits_le32(&mxc_ccm->CCGR2, MXC_CCM_CCGR2_IOMUX_IPT_CLK_IO_MASK);

	/* enable gpmi and bch clock gating */
	setbits_le32(&mxc_ccm->CCGR4,
		     MXC_CCM_CCGR4_RAWNAND_U_BCH_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_BCH_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_BCH_INPUT_GPMI_IO_MASK |
		     MXC_CCM_CCGR4_RAWNAND_U_GPMI_INPUT_APB_MASK |
		     MXC_CCM_CCGR4_PL301_MX6QPER1_BCH_OFFSET);

	/* enable apbh clock gating */
	setbits_le32(&mxc_ccm->CCGR0, MXC_CCM_CCGR0_APBHDMA_MASK);
	tmp = readl(&mxc_ccm->CCGR0);
	
	
}

 

static struct mxs_dma_desc *mxs_nand_get_dma_desc(struct mxs_nand_info *info)
{
	struct mxs_dma_desc *desc;

	if (info->desc_index >= MXS_NAND_DMA_DESCRIPTOR_COUNT) {
		
		return 0; // FIXME
	}

	desc = info->desc[info->desc_index];
	info->desc_index++;

	return desc;
}

static void mxs_nand_return_dma_descs(struct mxs_nand_info *info)
{
	int i;
	struct mxs_dma_desc *desc;

	for (i = 0; i < info->desc_index; i++) {
		dma_addr_t temp;
		desc = info->desc[i];
		temp = desc->address; //backup
		memset(desc, 0, sizeof(struct mxs_dma_desc));
		
		desc->address = temp; //restore
		
	}

	info->desc_index = 0;
}

static uint32_t mxs_nand_ecc_chunk_cnt(uint32_t page_data_size)
{
	return page_data_size / MXS_NAND_CHUNK_DATA_CHUNK_SIZE;
}

static uint32_t mxs_nand_ecc_size_in_bits(uint32_t ecc_strength)
{
	return ecc_strength * 13;
}

static uint32_t mxs_nand_aux_status_offset(void)
{
	return (MXS_NAND_METADATA_SIZE + 0x3) & ~0x3;
}

static inline uint32_t mxs_nand_get_ecc_strength(uint32_t page_data_size,
						uint32_t page_oob_size)
{
	int ecc_strength;

	ecc_strength = ((page_oob_size - MXS_NAND_METADATA_SIZE) * 8)
			/ (13 * mxs_nand_ecc_chunk_cnt(page_data_size));

	/* We need the minor even number. */
	ecc_strength -= ecc_strength & 1;
	return ecc_strength;
}

static inline uint32_t mxs_nand_get_mark_offset(uint32_t page_data_size,
						uint32_t ecc_strength)
{
	uint32_t chunk_data_size_in_bits;
	uint32_t chunk_ecc_size_in_bits;
	uint32_t chunk_total_size_in_bits;
	uint32_t block_mark_chunk_number;
	uint32_t block_mark_chunk_bit_offset;
	uint32_t block_mark_bit_offset;

	chunk_data_size_in_bits = MXS_NAND_CHUNK_DATA_CHUNK_SIZE * 8;
	chunk_ecc_size_in_bits  = mxs_nand_ecc_size_in_bits(ecc_strength);

	chunk_total_size_in_bits =
			chunk_data_size_in_bits + chunk_ecc_size_in_bits;

	/* Compute the bit offset of the block mark within the physical page. */
	block_mark_bit_offset = page_data_size * 8;

	/* Subtract the metadata bits. */
	block_mark_bit_offset -= MXS_NAND_METADATA_SIZE * 8;

	/*
	 * Compute the chunk number (starting at zero) in which the block mark
	 * appears.
	 */
	block_mark_chunk_number =
			block_mark_bit_offset / chunk_total_size_in_bits;

	/*
	 * Compute the bit offset of the block mark within its chunk, and
	 * validate it.
	 */
	block_mark_chunk_bit_offset = block_mark_bit_offset -
			(block_mark_chunk_number * chunk_total_size_in_bits);

	if (block_mark_chunk_bit_offset > chunk_data_size_in_bits)
		return 1;

	/*
	 * Now that we know the chunk number in which the block mark appears,
	 * we can subtract all the ECC bits that appear before it.
	 */
	block_mark_bit_offset -=
		block_mark_chunk_number * chunk_ecc_size_in_bits;

	return block_mark_bit_offset;
}

static uint32_t mxs_nand_mark_byte_offset()
{
	uint32_t ecc_strength;
	ecc_strength = mxs_nand_get_ecc_strength(nand_info.writesize,nand_info.oobsize);
	return mxs_nand_get_mark_offset(nand_info.writesize, ecc_strength) >> 3;
}

static uint32_t mxs_nand_mark_bit_offset()
{
	uint32_t ecc_strength;
	ecc_strength = mxs_nand_get_ecc_strength(nand_info.writesize, nand_info.oobsize);
	return mxs_nand_get_mark_offset(nand_info.writesize, ecc_strength) & 0x7;
}

/*
 * Wait for BCH complete IRQ and clear the IRQ
 */
static int mxs_nand_wait_for_bch_complete(void)
{
	struct mxs_bch_regs *bch_regs = (struct mxs_bch_regs *)nand_info.mxs_bch_base;
	int timeout = MXS_NAND_BCH_TIMEOUT;
	int ret;

	ret = mxs_wait_mask_set(&bch_regs->hw_bch_ctrl_reg,
		BCH_CTRL_COMPLETE_IRQ, timeout);

	writel(BCH_CTRL_COMPLETE_IRQ, &bch_regs->hw_bch_ctrl_clr);
	
	return ret;
}

/*
 * This is the function that we install in the cmd_ctrl function pointer of the
 * owning struct nand_chip. The only functions in the reference implementation
 * that use these functions pointers are cmdfunc and select_chip.
 *
 * In this driver, we implement our own select_chip, so this function will only
 * be called by the reference implementation's cmdfunc. For this reason, we can
 * ignore the chip enable bit and concentrate only on sending bytes to the NAND
 * Flash.
 */
void mxs_nand_cmd_ctrl( int data, unsigned int ctrl)
{
	struct mxs_dma_desc *d;
	uint32_t channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0;
	int ret;

	if (ctrl & (NAND_ALE | NAND_CLE)) {
		if (data != NAND_CMD_NONE) {
			nand_info.cmd_buf[nand_info.cmd_queue_len++] = data;

		}

		return;
	}

	if (nand_info.cmd_queue_len == 0)
		return;

 	/* Compile the DMA descriptor -- a descriptor that sends command. */
	d = mxs_nand_get_dma_desc(&nand_info);
	d->cmd.data =
		MXS_DMA_DESC_COMMAND_DMA_READ | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_CHAIN | MXS_DMA_DESC_DEC_SEM |
		MXS_DMA_DESC_WAIT4END | (3 << MXS_DMA_DESC_PIO_WORDS_OFFSET) |
		(nand_info.cmd_queue_len << MXS_DMA_DESC_BYTES_OFFSET);

	
	d->cmd.address = (dma_addr_t)nand_info.cmd_buf_phys;

	
	
	d->cmd.pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WRITE |
		GPMI_CTRL0_WORD_LENGTH |
		(0 << GPMI_CTRL0_CS_OFFSET) |
		GPMI_CTRL0_ADDRESS_NAND_CLE |
		GPMI_CTRL0_ADDRESS_INCREMENT |
		nand_info.cmd_queue_len;
	d->cmd.pio_words[1] = 0;

	mxs_dma_desc_append(channel, d);

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel);
	if (ret)
		uart_puts("MXS NAND: Error sending command\n");



	mxs_nand_return_dma_descs(&nand_info);

	/* Reset the command queue. */
	nand_info.cmd_queue_len = 0;
}

/*
 * Test if the NAND flash is ready.
 */
int mxs_nand_device_ready()
{
	struct mxs_gpmi_regs *gpmi_regs =
		(struct mxs_gpmi_regs *)nand_info.mxs_gpmi_base;
	uint32_t tmp;

	tmp = readl(&gpmi_regs->hw_gpmi_stat);
	tmp >>= (GPMI_STAT_READY_BUSY_OFFSET );

	return tmp & 1;
}

/*
 * Handle block mark swapping.
 *
 * Note that, when this function is called, it doesn't know whether it's
 * swapping the block mark, or swapping it *back* -- but it doesn't matter
 * because the the operation is the same.
 */
static void mxs_nand_swap_block_mark(uint8_t *data_buf, uint8_t *oob_buf)
{
	uint32_t bit_offset;
	uint32_t buf_offset;

	uint32_t src;
	uint32_t dst;

	bit_offset = mxs_nand_mark_bit_offset();
	buf_offset = mxs_nand_mark_byte_offset();

	/*
	 * Get the byte from the data area that overlays the block mark. Since
	 * the ECC engine applies its own view to the bits in the page, the
	 * physical block mark won't (in general) appear on a byte boundary in
	 * the data.
	 */
	src = data_buf[buf_offset] >> bit_offset;
	src |= data_buf[buf_offset + 1] << (8 - bit_offset);

	dst = oob_buf[0];

	oob_buf[0] = src;

	data_buf[buf_offset] &= ~(0xff << bit_offset);
	data_buf[buf_offset + 1] &= 0xff << bit_offset;

	data_buf[buf_offset] |= dst << bit_offset;
	data_buf[buf_offset + 1] |= dst >> (8 - bit_offset);
}


/*
 * Read data from NAND.
 */
static void mxs_nand_read_buf(uint8_t *buf, int length)
{

	struct mxs_dma_desc *d;
	uint32_t channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0;
	int ret;
	
	if (length > NAND_MAX_PAGESIZE) {
		return;
	}

	if (!buf) {
		uart_puts("MXS NAND: DMA buffer is NULL\n");
		return;
	}

	/* Compile the DMA descriptor - a descriptor that reads data. */
	d = mxs_nand_get_dma_desc(&nand_info);
	d->cmd.data =
		MXS_DMA_DESC_COMMAND_DMA_WRITE | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_DEC_SEM | MXS_DMA_DESC_WAIT4END |
		(1 << MXS_DMA_DESC_PIO_WORDS_OFFSET) |
		(length << MXS_DMA_DESC_BYTES_OFFSET);

	d->cmd.address = (dma_addr_t)nand_info.data_buf_phys;

	d->cmd.pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		(0 << GPMI_CTRL0_CS_OFFSET) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		length;

	mxs_dma_desc_append(channel, d);

	/*
	 * A DMA descriptor that waits for the command to end and the chip to
	 * become ready.
	 *
	 * I think we actually should *not* be waiting for the chip to become
	 * ready because, after all, we don't care. I think the original code
	 * did that and no one has re-thought it yet.
	 */
	d = mxs_nand_get_dma_desc(&nand_info);
	d->cmd.data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_NAND_WAIT_4_READY | MXS_DMA_DESC_DEC_SEM |
		MXS_DMA_DESC_WAIT4END | (4 << MXS_DMA_DESC_PIO_WORDS_OFFSET);

	d->cmd.address = 0;

	d->cmd.pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		(0 << GPMI_CTRL0_CS_OFFSET) |
		GPMI_CTRL0_ADDRESS_NAND_DATA;

	mxs_dma_desc_append(channel, d);

	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel);
	if (ret) {
		uart_puts("MXS NAND: DMA read error\n");

		goto rtn;
	}

	memcpy(buf, nand_info.data_buf, length);

rtn:
	mxs_nand_return_dma_descs(&nand_info);
}


/*
 * Read a single byte from NAND.
 */
uint8_t mxs_nand_read_byte()
{
	uint8_t buf;
	mxs_nand_read_buf( &buf, 1);
	return buf;
}

/*
 * Read a page from NAND.
 */
int mxs_nand_ecc_read_page(uint8_t *buf, int oob_required, int page)
{
	// FIXME - oob_required...
	struct mxs_dma_desc *d;
	uint32_t channel = MXS_DMA_CHANNEL_AHB_APBH_GPMI0;  //FIXME
	uint32_t corrected = 0, failed = 0;
	
	uint8_t	*status;
	int i, ret;

	/* Compile the DMA descriptor - wait for ready. */
	d = mxs_nand_get_dma_desc(&nand_info);
	d->cmd.data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
		MXS_DMA_DESC_NAND_WAIT_4_READY | MXS_DMA_DESC_WAIT4END |
		(1 << MXS_DMA_DESC_PIO_WORDS_OFFSET);

	d->cmd.address = 0;

	d->cmd.pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		(0 << GPMI_CTRL0_CS_OFFSET) |
		GPMI_CTRL0_ADDRESS_NAND_DATA;

	mxs_dma_desc_append(channel, d);

	/* Compile the DMA descriptor - enable the BCH block and read. */
	d = mxs_nand_get_dma_desc(&nand_info);
	d->cmd.data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
		MXS_DMA_DESC_WAIT4END |	(6 << MXS_DMA_DESC_PIO_WORDS_OFFSET);

	d->cmd.address = 0;

	d->cmd.pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_READ |
		GPMI_CTRL0_WORD_LENGTH |
		(0 << GPMI_CTRL0_CS_OFFSET) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		(nand_info.writesize + nand_info.oobsize);
	d->cmd.pio_words[1] = 0;
	d->cmd.pio_words[2] =
		GPMI_ECCCTRL_ENABLE_ECC |
		GPMI_ECCCTRL_ECC_CMD_DECODE |
		GPMI_ECCCTRL_BUFFER_MASK_BCH_PAGE;
	d->cmd.pio_words[3] = nand_info.writesize + nand_info.oobsize;
	d->cmd.pio_words[4] = (dma_addr_t)nand_info.data_buf_phys;
	d->cmd.pio_words[5] = (dma_addr_t)nand_info.oob_buf_phys;

	mxs_dma_desc_append(channel, d);

	/* Compile the DMA descriptor - disable the BCH block. */
	d = mxs_nand_get_dma_desc(&nand_info);
	d->cmd.data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_CHAIN |
		MXS_DMA_DESC_NAND_WAIT_4_READY | MXS_DMA_DESC_WAIT4END |
		(3 << MXS_DMA_DESC_PIO_WORDS_OFFSET);

	d->cmd.address = 0;

	d->cmd.pio_words[0] =
		GPMI_CTRL0_COMMAND_MODE_WAIT_FOR_READY |
		GPMI_CTRL0_WORD_LENGTH |
		(0 << GPMI_CTRL0_CS_OFFSET) |
		GPMI_CTRL0_ADDRESS_NAND_DATA |
		(nand_info.writesize + nand_info.oobsize);
	d->cmd.pio_words[1] = 0;
	d->cmd.pio_words[2] = 0;

	mxs_dma_desc_append(channel, d);

	/* Compile the DMA descriptor - deassert the NAND lock and interrupt. */
	d = mxs_nand_get_dma_desc(&nand_info);
	d->cmd.data =
		MXS_DMA_DESC_COMMAND_NO_DMAXFER | MXS_DMA_DESC_IRQ |
		MXS_DMA_DESC_DEC_SEM;

	d->cmd.address = 0;

	mxs_dma_desc_append(channel, d);

	
	/* Execute the DMA chain. */
	ret = mxs_dma_go(channel);
	if (ret) {
		uart_puts("MXS NAND: DMA read error\n");

		goto rtn;
	}
	


	ret = mxs_nand_wait_for_bch_complete();
	if (ret) {
		uart_puts("MXS NAND: BCH read timeout\n");

		goto rtn;
	}

	
	/* Read DMA completed, now do the mark swapping. */
	mxs_nand_swap_block_mark( nand_info.data_buf, nand_info.oob_buf);

	/* Loop over status bytes, accumulating ECC status. */
	status = nand_info.oob_buf + mxs_nand_aux_status_offset();
	for (i = 0; i < mxs_nand_ecc_chunk_cnt(nand_info.writesize); i++) {
		if (status[i] == 0x00)
			continue;

		if (status[i] == 0xff)
			continue;

		if (status[i] == 0xfe) {
			failed++;
			continue;
		}

		corrected += status[i];
	}
	if ( failed ) {
		uart_puts("ECC Failed Count : ");
		uart_put_hex(failed);
		uart_puts("\n");
	}
	if ( corrected ) {
		uart_puts("ECC Corrected Count : ");
		uart_put_hex(corrected);
		uart_puts("\n");
	}
	/* Propagate ECC status to the owning MTD. */
	//mtd->ecc_stats.failed += failed; //FIXME -- Need it for NalCode base..??
	//mtd->ecc_stats.corrected += corrected;


	//memset(nand->oob_poi, 0xff, mtd->oobsize); // FIXME

	//nand->oob_poi[0] = nand_info->oob_buf[0];

	memcpy(buf, nand_info.data_buf, nand_info.writesize);


rtn:
	mxs_nand_return_dma_descs(&nand_info);
	
	return ret;
}

/*
 * Allocate DMA buffers
 */

 
int mxs_nand_alloc_buffers()
{
	memset(nand_info.data_buf, 0, nand_info.writesize + nand_info.oobsize);
	nand_info.oob_buf = nand_info.data_buf + nand_info.writesize;

	memset(nand_info.cmd_buf, 0, MXS_NAND_COMMAND_BUFFER_SIZE);
	nand_info.cmd_queue_len = 0;

	return 0;
}


int mxs_nand_scan_bbt()
{

	struct mxs_bch_regs *bch_regs = (struct mxs_bch_regs *)nand_info.mxs_bch_base;
	uint32_t tmp;

	if (nand_info.oobsize > MXS_NAND_CHUNK_DATA_CHUNK_SIZE) {
		return -1;
	}

	/* Configure BCH and set NFC geometry */
	mxs_reset_block(&bch_regs->hw_bch_ctrl_reg);

	 
	/* Configure layout 0 */
	tmp = (mxs_nand_ecc_chunk_cnt(nand_info.writesize) - 1)
		<< BCH_FLASHLAYOUT0_NBLOCKS_OFFSET;
	tmp |= MXS_NAND_METADATA_SIZE << BCH_FLASHLAYOUT0_META_SIZE_OFFSET;
	tmp |= (mxs_nand_get_ecc_strength(nand_info.writesize, nand_info.oobsize) >> 1)
		<< BCH_FLASHLAYOUT0_ECC0_OFFSET;
	tmp |= MXS_NAND_CHUNK_DATA_CHUNK_SIZE
		>> MXS_NAND_CHUNK_DATA_CHUNK_SIZE_SHIFT;
	writel(tmp, &bch_regs->hw_bch_flash0layout0);

	tmp = (nand_info.writesize + nand_info.oobsize)
		<< BCH_FLASHLAYOUT1_PAGE_SIZE_OFFSET;
	tmp |= (mxs_nand_get_ecc_strength(nand_info.writesize,nand_info.oobsize) >> 1)
		<< BCH_FLASHLAYOUT1_ECCN_OFFSET;
	tmp |= MXS_NAND_CHUNK_DATA_CHUNK_SIZE
		>> MXS_NAND_CHUNK_DATA_CHUNK_SIZE_SHIFT;
	writel(tmp, &bch_regs->hw_bch_flash0layout1);

	/* Set *all* chip selects to use layout 0 */
	writel(0, &bch_regs->hw_bch_layoutselect);

	return 0;
}

/*
 * Initializes the NFC hardware.
 */
int mxs_nand_init(struct mxs_nand_info *info)
{
	struct mxs_gpmi_regs *gpmi_regs =
		(struct mxs_gpmi_regs *)nand_info.mxs_gpmi_base;
	struct mxs_bch_regs *bch_regs =
		(struct mxs_bch_regs *)nand_info.mxs_bch_base;

	struct mxs_dma_desc	**desc = ( struct mxs_dma_desc	**)info->desc;
	
	int i = 0, j;

	
	mxs_dma_init();
	
					
	if (!desc)
		goto ERROR;
		
		

	/* Allocate the DMA descriptors. */
	for (i = 0; i < MXS_NAND_DMA_DESCRIPTOR_COUNT; i++) {
		desc[i]->address =(dma_addr_t) nand_info.desc_phys[i];
		if (!desc[i])
			goto ERROR;
	}

	/* Init the DMA controller. */
	for (j = MXS_DMA_CHANNEL_AHB_APBH_GPMI0;
		j <= MXS_DMA_CHANNEL_AHB_APBH_GPMI7; j++) {
		if (mxs_dma_init_channel(j))
			goto ERROR;
	}

	/* Reset the GPMI block. */
	mxs_reset_block(&gpmi_regs->hw_gpmi_ctrl0_reg);
	mxs_reset_block(&bch_regs->hw_bch_ctrl_reg);

	/*
	 * Choose NAND mode, set IRQ polarity, disable write protection and
	 * select BCH ECC.
	 */
	clrsetbits_le32(&gpmi_regs->hw_gpmi_ctrl1,
			GPMI_CTRL1_GPMI_MODE,
			GPMI_CTRL1_ATA_IRQRDY_POLARITY | GPMI_CTRL1_DEV_RESET |
			GPMI_CTRL1_BCH_MODE);

	return 0;

ERROR:
	uart_puts("NAND INIT ERROR..\n");

	return -1;
}


/*!
 * This function is called during the driver binding process.
 *
 * @param   pdev  the device structure used to store device specific
 *                information that is used by the suspend, resume and
 *                remove functions
 *
 * @return  The function always returns 0.
 */
int board_nand_init()
{
	int err;
	int i;

	setup_gpmi_nand();

	err = mxs_nand_alloc_buffers();
	if (err)
		goto ERROR;
	err = mxs_nand_init(&nand_info);
	if (err)
		goto ERROR;

	return 0;

ERROR:
	uart_puts("BOARD NAND INIT ERROR..\n");

	return -1;
}
