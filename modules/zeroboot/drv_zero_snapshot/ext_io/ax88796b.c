/**    
    @file     ax88796b.c
    @date     2011/10/10
    @author   오재경 freefrug@falinux.com  FALinux.Co.,Ltd.
    @brief    

    @todo     
    @bug     
    @remark  
    @warning 
*/
//
//  저작권    에프에이리눅스(주)
//            외부공개 금지
//
//----------------------------------------------------------------------------

#ifndef __KERNEL__
#define __KERNEL__
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
#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>

#include <mach/map.h>
#include <linux/time.h>			
#include <linux/timer.h>		
#include <linux/clk.h>

#include <asm/mach/map.h>

#include <plat/regs-timer.h>
#include <plat/clock.h>

#include <reg_save_restore.h>

#include "ax88796b.h"

static u32 AX88796B_BASE = 0;
static u32 AX_ADDR_SHIFT = 1;	

static u8 mac_addr[6];


/**
 * 설명 : Byte 단위 read.
 */
static inline unsigned char ax_readb(unsigned long addr)
{
	unsigned char v;

	v = *((volatile unsigned char*)(AX88796B_BASE + addr));
	return v;
}

/**
 * 설명 : Byte 단위 write.
 */
 static inline void ax_writeb(unsigned char value, unsigned long addr)
{
    *((volatile unsigned char*)(AX88796B_BASE + addr)) = value;
}
/**
 * 설명 : Word 단위 read.
 */
static inline unsigned short ax_readw(unsigned long addr)
{
	unsigned short v;

	v = *((volatile unsigned short*)(AX88796B_BASE + addr));
	return v;
}
/**
 * 설명 : Word 단위 write.
 */
static inline void ax_writew(unsigned short value, unsigned long addr)
{
    *((volatile unsigned short*)(AX88796B_BASE + addr)) = value;
}

/*======================================================================
    MII interface support
======================================================================*/
#define MDIO_SHIFT_CLK		0x01
#define MDIO_DATA_WRITE0	0x00
#define MDIO_DATA_WRITE1	0x08
#define MDIO_DATA_READ		0x04
#define MDIO_MASK			0x0f
#define MDIO_ENB_IN			0x02

static void mdio_sync(void)
{
    int bits;

    for (bits = 0; bits < 32; bits++) {
		ax_writeb(MDIO_DATA_WRITE1, AX88796_MII_EEPROM);
		ax_writeb(MDIO_DATA_WRITE1 | MDIO_SHIFT_CLK, AX88796_MII_EEPROM);
    }
}

static void mdio_clear(void)
{
    int bits;

    for (bits = 0; bits < 16; bits++) {
		ax_writeb(MDIO_DATA_WRITE0, AX88796_MII_EEPROM);
		ax_writeb(MDIO_DATA_WRITE0 | MDIO_SHIFT_CLK, AX88796_MII_EEPROM);
    }
}

static int mdio_read(int phy_id, int loc)
{
    unsigned int cmd = (0xf6<<10)|(phy_id<<5)|loc;
    int i, retval = 0;

	mdio_clear();
    mdio_sync();

    for (i = 14; i >= 0; i--) {
		int dat = (cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		ax_writeb(dat, AX88796_MII_EEPROM);
		ax_writeb(dat | MDIO_SHIFT_CLK, AX88796_MII_EEPROM);
    }
    for (i = 19; i > 0; i--) {
		ax_writeb(MDIO_ENB_IN, AX88796_MII_EEPROM);
		retval = (retval << 1) | ((ax_readb(AX88796_MII_EEPROM) & MDIO_DATA_READ) != 0);
		ax_writeb(MDIO_ENB_IN | MDIO_SHIFT_CLK, AX88796_MII_EEPROM);
    }
    return (retval>>1) & 0xffff;
}

static void mdio_write(int phy_id, int loc, int value)
{
    unsigned int cmd = (0x05<<28)|(phy_id<<23)|(loc<<18)|(1<<17)|value;
    int i;

	mdio_clear();
    mdio_sync();

    for (i = 31; i >= 0; i--) {
	int dat = (cmd&(1<<i)) ? MDIO_DATA_WRITE1 : MDIO_DATA_WRITE0;
		ax_writeb(dat, AX88796_MII_EEPROM);
		ax_writeb(dat | MDIO_SHIFT_CLK, AX88796_MII_EEPROM);
    }
    for (i = 1; i >= 0; i--) {
		ax_writeb(MDIO_ENB_IN, AX88796_MII_EEPROM);
		ax_writeb(MDIO_ENB_IN | MDIO_SHIFT_CLK, AX88796_MII_EEPROM);
    }
}

//static unsigned char XmitPage;

/**
 * 설명 : AX88796 RESET 함수이다.
 */
void ax88796b_reset(void)
{
	unsigned char value;

	value = ax_readb(EN0_RESET);
	ax_writeb(value, EN0_RESET);

	value = ax_readb(EN0_ISR);
	if(value != ENISR_RESET)
		printk("AX88796B Reset Failure\r\n");
}

static int media_mode = MEDIA_100FULL;

/*
 * ----------------------------------------------------------------------------
 * Function Name: ax88796_PHY_init
 * Purpose:
 * Params:
 * Returns:
 * Note:
 * ----------------------------------------------------------------------------
 */
void ax88796_PHY_init(void)
{
	u8 tmp_data;

	/* Enable AX88796B FOLW CONTROL */
	ax_writeb(ENFLOW_ENABLE, EN0_FLOW);

	/* Enable PHY PAUSE */
	mdio_write(0x10,0x04,(mdio_read(0x10,0x04) | 0x400));
	mdio_write(0x10,0x00,0x1200);

	/* Enable AX88796B TQC */
	tmp_data = ax_readb(EN0_MCR);
	ax_writeb( tmp_data | ENTQC_ENABLE, EN0_MCR);

	/* Enable AX88796B Transmit Buffer Ring */
	ax_writeb(E8390_NODMA+E8390_PAGE3+E8390_STOP, E8390_CMD);
	ax_writeb(ENTBR_ENABLE, EN3_TBR);
	ax_writeb(E8390_NODMA+E8390_PAGE0+E8390_STOP, E8390_CMD);

	switch (media_mode) {
	default:
	case MEDIA_AUTO:
//		printf("AX88796B: The media mode is autosense.\n");
		break;

	case MEDIA_100FULL:
//		printf("AX88796B: The media mode is forced to 100full.\n");
		mdio_write(0x10,0x00,0x2300);
		break;

	case MEDIA_100HALF:
//		printf("AX88796B: The media mode is forced to 100half.\n");
		mdio_write(0x10,0x00,0x2200);
		break;

	case MEDIA_10FULL:
//		printf("AX88796B: The media mode is forced to 10full.\n");
		mdio_write(0x10,0x00,0x0300);
		break;

	case MEDIA_10HALF:
//		printf("AX88796B: The media mode is forced to 10half.\n");
		mdio_write(0x10,0x00,0x0200);
		break;
	}
}


static int ax88796b_init( u32 base )
{
	unsigned char i;

	AX88796B_BASE = base;
	AX_ADDR_SHIFT = 1;
	
	// reset ax88796 chip
	ax88796b_reset();
	
	// stop MAC, Page0
	ax_writeb((E8390_STOP | E8390_NODMA | E8390_PAGE0), E8390_CMD);
	udelay(1000);
	//udelay(1000);
	//udelay(1000);
	//udelay(1000);
	//udelay(1000);

	ax_writeb(0x48|ENDCFG_WTS, EN0_DCFG);	// 0x49

	// clear the remote byte count registers
	ax_writeb(0, EN0_RCNTLO);
	ax_writeb(0, EN0_RCNTHI);

	// set to monitor and loopback mode
	//ax_writeb(ENRCR_MONITOR,   EN0_RXCR);
	ax_writeb(ENRCR_BROADCAST, EN0_RXCR);
	//ax_writeb(ENTCR_LOCAL,  	EN0_TXCR);
	ax_writeb(ENTCR_FDU,       EN0_TXCR);

	// set the transmit page and receive ring
	ax_writeb(NESM_START_PG,	EN0_TPSR);
	ax_writeb(NESM_RX_START_PG,	EN0_STARTPG);
	//ax_writeb(NESM_RX_START_PG,	EN0_BOUNDARY );
	ax_writeb(NESM_STOP_PG-1,	EN0_BOUNDARY );
	ax_writeb(NESM_STOP_PG,		EN0_STOPPG);

	// clear the pending interrupts and mask
	ax_writeb(0xff,	EN0_ISR);
	ax_writeb(0,	EN0_IMR);

	// copy the station address into the DS8390 registers
	ax_writeb((E8390_STOP | E8390_NODMA | E8390_PAGE1), E8390_CMD);
	udelay(1000);
	//udelay(1000);
	//udelay(1000);
	//udelay(1000);
	//udelay(1000);


	//Write out the current receive buffer to receive into
	ax_writeb(NESM_RX_START_PG + 1, EN1_CURPAG);

	// set MAC address
	//printk("  AX88796B MAC  : [ ");
	for (i=0; i<ETHER_ADDR_LEN; i++)
	{   
		ax_writeb( mac_addr[i] , (i+1)<<1);
		if(ax_readb((i+1)<<1) != mac_addr[i])
			
		{
			ax_writeb(mac_addr[i], (i+1)<<1);
			if(ax_readb((i+1)<<1) != mac_addr[i])
			{
				printk("  Mac address set failurei ]\n");
				return -1;
			}
		}
		//printk("%02X ", ax_readb((i+1)<<1));		
	}
	//printk("]\n");

	// change to page0
	ax_writeb((E8390_STOP | E8390_NODMA | E8390_PAGE0), E8390_CMD);

	ax88796_PHY_init();
	ax_writeb(0xff,  EN0_ISR);
	ax_writeb(ENISR_ALL, EN0_IMR);
	ax_writeb(E8390_NODMA+E8390_PAGE0+E8390_START, E8390_CMD);
	ax_writeb(E8390_TXCONFIG, EN0_TXCR); /* xmit on. */
	/* 3c503 TechMan says rxconfig only after the NIC is started. */
	ax_writeb(E8390_RXCONFIG, EN0_RXCR); /* rx on,  */

	//mdelay(2000);		
	//printk("  AX88796B_Init : OK!\n");

	return 0;
}



void ax88796b_save( void __iomem *base )
{
	int i;
	
	AX88796B_BASE = (u32)base;
	AX_ADDR_SHIFT = 1;

	// copy the station address into the DS8390 registers
	ax_writeb((E8390_STOP | E8390_NODMA | E8390_PAGE1), E8390_CMD);

	// set MAC address
	printk("  AX88796B MAC  : [ ");
	for (i=0; i<ETHER_ADDR_LEN; i++)
	{   
		mac_addr[i] = ax_readb( (i+1)<<AX_ADDR_SHIFT );
		
		printk("%02X ", mac_addr[i] );		
	}
	printk("]\n");
	
}


void ax88796b_restore( void __iomem *base )
{
	ax88796b_init( (u32)base );	
}









