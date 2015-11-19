

#ifndef _NALCODE_MXS_NAND_H_
#define _NALCODE_MXS_NAND_H_

#define DMA_ERROR_CODE  (~0)




#define NAND_MAX_OOBSIZE 	   744			//FIXME
#define NAND_MAX_PAGESIZE	   8192			//FIXME
#define NAND_PAGE_SIZE			2048
#define  CONFIG_SYS_NAND_RESET_CNT  100



/* from kernel 
 * Constants for hardware specific CLE/ALE/NCE function
 *
 * These are bits which can be or'ed to set/clear multiple
 * bits in one go.
 */
/* Select the chip by setting nCE to low */
#define NAND_NCE		0x01
/* Select the command latch by setting CLE to high */
#define NAND_CLE		0x02
/* Select the address latch by setting ALE to high */
#define NAND_ALE		0x04

#define NAND_CTRL_CLE		(NAND_NCE | NAND_CLE)
#define NAND_CTRL_ALE		(NAND_NCE | NAND_ALE)
#define NAND_CTRL_CHANGE	0x80

/*
 * Standard NAND flash commands
 */
#define NAND_CMD_READ0		0
#define NAND_CMD_READ1		1
#define NAND_CMD_RNDOUT		5
#define NAND_CMD_PAGEPROG	0x10
#define NAND_CMD_READOOB	0x50
#define NAND_CMD_ERASE1		0x60
#define NAND_CMD_STATUS		0x70
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_SEQIN		0x80
#define NAND_CMD_RNDIN		0x85
#define NAND_CMD_READID		0x90
#define NAND_CMD_ERASE2		0xd0
#define NAND_CMD_PARAM		0xec
#define NAND_CMD_RESET		0xff

#define NAND_CMD_LOCK		0x2a
#define NAND_CMD_UNLOCK1	0x23
#define NAND_CMD_UNLOCK2	0x24

/* Extended commands for large page devices */
#define NAND_CMD_READSTART	0x30
#define NAND_CMD_RNDOUTSTART	0xE0
#define NAND_CMD_CACHEDPROG	0x15

#define NAND_CMD_DEPLETE1	0x100
#define NAND_CMD_DEPLETE2	0x38
#define NAND_CMD_STATUS_MULTI	0x71
#define NAND_CMD_STATUS_ERROR	0x72
/* multi-bank error status (banks 0-3) */
#define NAND_CMD_STATUS_ERROR0	0x73
#define NAND_CMD_STATUS_ERROR1	0x74
#define NAND_CMD_STATUS_ERROR2	0x75
#define NAND_CMD_STATUS_ERROR3	0x76
#define NAND_CMD_STATUS_RESET	0x7f
#define NAND_CMD_STATUS_CLEAR	0xff

#define NAND_CMD_NONE		-1

/* Status bits */
#define NAND_STATUS_FAIL	0x01
#define NAND_STATUS_FAIL_N1	0x02
#define NAND_STATUS_TRUE_READY	0x20
#define NAND_STATUS_READY	0x40
#define NAND_STATUS_WP		0x80

#define	MXS_NAND_DMA_DESCRIPTOR_COUNT		4

#define	MXS_NAND_CHUNK_DATA_CHUNK_SIZE		512
#if defined(CONFIG_MX6)
#define	MXS_NAND_CHUNK_DATA_CHUNK_SIZE_SHIFT	2
#else
#define	MXS_NAND_CHUNK_DATA_CHUNK_SIZE_SHIFT	0
#endif
#define	MXS_NAND_METADATA_SIZE			10

#define	MXS_NAND_COMMAND_BUFFER_SIZE		32

//#define	MXS_NAND_BCH_TIMEOUT			10000
#define	MXS_NAND_BCH_TIMEOUT			10000

struct mxs_nand_info {
	uint32_t			cmd_queue_len;
	uint8_t				*cmd_buf;
	uint8_t				*data_buf;
	uint8_t				*oob_buf;
	uint8_t				*cmd_buf_phys;
	uint8_t				*data_buf_phys;
	uint8_t				*oob_buf_phys;
	
	//struct mxs_dma_desc	**desc;
	//struct mxs_dma_desc	**desc_phys;
	uint8_t				*desc[4];
	uint8_t				*desc_phys[4];
	
	uint32_t			desc_index;
	uint32_t			writesize;
	uint32_t    		oobsize;
	uint64_t			chipsize;

	uint8_t *mxs_apbh_base; 
	uint8_t *mxs_bch_base;
	uint8_t *mxs_gpmi_base;
	uint8_t *ccm_base;
};


/* 1 second delay should be plenty of time for block reset. */
#define	RESET_MAX_TIMEOUT	1000000

#define	MXS_BLOCK_SFTRST	(1 << 31)
#define	MXS_BLOCK_CLKGATE	(1 << 30)


int mxs_dma_go(int chan);

int mxs_nand_device_ready();
void mxs_nand_cmd_ctrl( int data, unsigned int ctrl);
int mxs_nand_ecc_read_page(uint8_t *buf, int oob_required, int page);
uint8_t mxs_nand_read_byte();
int board_nand_init();
int mxs_nand_scan_bbt();




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




#endif
