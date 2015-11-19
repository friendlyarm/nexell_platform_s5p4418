/**    
    @file     sd_s5pv210.c
    @date     2011/10/06
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

//#include <plat/regs-lcd.h>
#include <plat/regs-timer.h>
#include <plat/clock.h>

#include <reg_save_restore.h>


#define SDHCI_ARGUMENT			0x08
#define SDHCI_TRANSFER_MODE		0x0c
#define SDHCI_COMMAND			0x0E
#define SDHCI_RESPONSE			0x10
#define SDHCI_INT_STATUS		0x30
#define SDHCI_ERR_STATUS		0x32
#define SDHCI_PRESENT_STATE		0x24
#define SDHCI_CLOCK_CONTROL		0x2C		// MMC0 : 0xEB00_002C
#define SDHCI_TIMEOUT_CONTROL	0x2e
#define SDHCI_SOFTWARE_RESET	0x2f
#define S3C64XX_SDHCI_CONTROL4  0x8C

#define SDHCI_INT_RESPONSE		0x00000001
#define SDHCI_INT_ERROR			0x00008000
#define SDHCI_CMD_INHIBIT		0x00000001
#define SDHCI_INT_TIMEOUT		0x00010000

#define SDHCI_CLOCK_INT_EN		(1<<0)
#define SDHCI_CLOCK_INT_STABLE	(1<<1)
#define SDHCI_CLOCK_CARD_EN		(1<<2)
#define SDHCI_CLOCK_EXT_STABLE	(1<<3)




static int mmc_wait_response( void __iomem *mmc_base, unsigned int *resp, int resp_cnt )
{
	unsigned int intmask, idx = 0;
	int  rtn = 0;
	

//printk( "  ..enter mmc wait\n" );	
	while( 1 )
	{
		intmask = __raw_readl(mmc_base + SDHCI_INT_STATUS);	
		__raw_writel(intmask, mmc_base + SDHCI_INT_STATUS);	

		//printk( "  mmc intsts=%04x\n", intmask & 0xffff );
		
		if ( intmask & SDHCI_INT_ERROR )
		{
			//printk( "  error mmc intsts=%08x\n", intmask );
			return rtn;
		}
		
		if ( intmask & SDHCI_INT_TIMEOUT )
		{
			//printk( "  timeout mmc intsts=%08x\n", intmask );
			return rtn;
		}
		
		if ( intmask & SDHCI_INT_RESPONSE )
		{
			if ( resp_cnt == 4 )
			{
				for (idx = 0;idx < 4;idx++) 
				{
					resp[idx] = __raw_readl(mmc_base + SDHCI_RESPONSE + (3-idx)*4 );
            	    resp[idx] = resp[idx] << 8;
            
					if (idx != 3)
						resp[idx] |= __raw_readb(mmc_base + SDHCI_RESPONSE + (3-idx)*4-1);
				}			
			}
			else
			{
				resp[0] = __raw_readl(mmc_base + SDHCI_RESPONSE );
			}
		
			rtn = resp_cnt;
			break;
		}
	}

//printk( "  end int=%04x err=%04x\n",__raw_readw(mmc_base + SDHCI_INT_STATUS),__raw_readw(mmc_base + SDHCI_ERR_STATUS) );

	return rtn;
} 

static void mmc_send_cmd( void __iomem *mmc_base, unsigned short cmd, unsigned int arg )
{
	__raw_writel( arg, mmc_base + SDHCI_ARGUMENT);
	__raw_writew( cmd, mmc_base + SDHCI_COMMAND );

	if(0)
		printk( " cmd=%04x  arg=%08x\n", cmd, arg );
}

static unsigned int mmc_send_cmd_with_response( void __iomem *mmc_base, unsigned short cmd, unsigned int arg )
{
	unsigned int resp[16], dumy, wait_loop, i;  
	unsigned char bcmd;
	
	wait_loop = 30;
	while ( (dumy = __raw_readl( mmc_base + SDHCI_PRESENT_STATE) & 0x07) ) 
	{
		printk( "wait.... %08x\n", dumy );
		if ( 0 == wait_loop -- ) break;
	}

	if ( cmd & (1<<3) ) // MMC_RSP_BUSY
	{
		__raw_writeb( 0xf, mmc_base + SDHCI_TIMEOUT_CONTROL);
	}
	
	__raw_writel( arg, mmc_base + SDHCI_ARGUMENT);
	
	// clear status
	{
		dumy = __raw_readl( mmc_base + SDHCI_INT_STATUS);	
		__raw_writel( dumy, mmc_base + SDHCI_INT_STATUS);	
    	
		for(i=0; i<0x1000000; i++) 
		{
			dumy = __raw_readl( mmc_base + S3C64XX_SDHCI_CONTROL4 );
			if(!(dumy & (1<<0)))
				break;
		}
	}

	__raw_writew( cmd, mmc_base + SDHCI_COMMAND );


	bcmd = (cmd>>8)&0xff; 
	switch( bcmd )
	{
	case 9 : // CMD9 SEND_CSD
	case 2 : // CMD2 SEND_CID
		{
			if ( 4 == mmc_wait_response( mmc_base, resp, 4 ) )		// 128 bit
			{
				//printk( "mmc cmd=%d(%s) resp=%08x %08x %08x %08x\n", bcmd, (bcmd == 9) ? "CSD":"CID", resp[3], resp[2], resp[1], resp[0] );
			}
		}
		break;
	default :
		{
			if ( 1 == mmc_wait_response( mmc_base, resp, 1 ) )
			{
				//printk( "mmc cmd=%d  resp=%08x\n", bcmd, resp[0] );
			}	
		}
		break;
	}
	
	//printk( " cmd=%04x  arg=%08x  prnsts=%08x rsp=%08x\n", cmd, arg, __raw_readl( mmc_base + SDHCI_PRESENT_STATE), resp[0] );

	return resp[0];
}

extern int zeroboot_mmc_init( int channel, void *iomem );

void mmc_active_SD4GB( void __iomem *mmc_base )
{
	unsigned int resp, card_addr;
	unsigned short  clk;

// for test
	zeroboot_mmc_init( 0, mmc_base );
	return;


	// CLOCK low speed
	{
		int timeout = 100;
		
		__raw_writew( 0, mmc_base + SDHCI_TRANSFER_MODE );

		//__raw_writew( 0, mmc_base + SDHCI_CLOCK_CONTROL );	
		clk = 0x8000 | SDHCI_CLOCK_INT_EN | SDHCI_CLOCK_CARD_EN;
		__raw_writew( clk, mmc_base + SDHCI_CLOCK_CONTROL );
		
		while ( timeout-- )
		{
			clk = __raw_readw( mmc_base + SDHCI_CLOCK_CONTROL );
			if ( clk & SDHCI_CLOCK_INT_STABLE ) break;
			udelay(100);
			
			if (timeout == 0) 
			{
				printk("---mmc err clk low-speed\n" );
				break;
			}
		}
    	
		//clk |= SDHCI_CLOCK_CARD_EN;
		//writew( clk, mmc_base + SDHCI_CLOCK_CONTROL);
	}

	// CMD0		idle
	mmc_send_cmd( mmc_base, 0x0000, 0x00000000 );
//	udelay(1000);
//	udelay(1000);

	// CMD8
	mmc_send_cmd_with_response( mmc_base, 0x081a, 0x000001aa );

	// CMD5
	mmc_send_cmd_with_response( mmc_base, 0x0502, 0x000001aa );

	// CMD55
	mmc_send_cmd_with_response( mmc_base, 0x371a, 0x00000000 );
	// ACMD41
	mmc_send_cmd_with_response( mmc_base, 0x2902, 0x00000000 );



	// CMD0		idle
	mmc_send_cmd( mmc_base, 0x0000, 0x00000000 );
	udelay(1000);

	// CMD8
	resp = mmc_send_cmd_with_response( mmc_base, 0x081a, 0x000001aa );
	
//printk(" ---%d\n", __LINE__ );
	// CMD55, ACMD41	OCR
	while ( 1 )
	{
		// CMD55 --> ACMD41	OCR
		mmc_send_cmd_with_response( mmc_base, 0x371a, 0x00000000 );
		resp = mmc_send_cmd_with_response( mmc_base, 0x2902, 0x40300000 );

		if ( 0xc0ff8000 == resp )
		{
			break;	
		}

		if ( 0x80ff8000 == resp )
		{
			break;	
		}
	}

//printk( "-----%s(%d)\n", __func__,__LINE__ );
	// CMD2 ALL_SEND_CID
	mmc_send_cmd_with_response( mmc_base, 0x0209, 0x00000000 );


//printk( "-----%s(%d)\n", __func__,__LINE__ );
	// CMD3 SEND_RELATIVE_ADDR
	card_addr  = mmc_send_cmd_with_response( mmc_base, 0x031a, 0x00000000 );
	card_addr &= 0xffff0000;
	
	printk(" sd card addr=0x%08x\n", card_addr );

//printk( "-----%s(%d)\n", __func__,__LINE__ );
	// CMD9 SEND_CSD
	mmc_send_cmd_with_response( mmc_base, 0x0909, card_addr );

//printk( "-----%s(%d)\n", __func__,__LINE__ );
	// CMD7	SELECT Card
	mmc_send_cmd_with_response( mmc_base, 0x071a, card_addr );

	// CMD55 --> ACMD51	 SCR
	//mmc_send_cmd_with_response( mmc_base, 0x371a, card_addr );
	//mmc_send_cmd_with_response( mmc_base, 0x333a, 0x00000000 );

	// CMD6		Driver Strength 설정
	//mmc_send_cmd_with_response( mmc_base, 0x063a, 0x00fffff1 );

	// CMD6		Driver Strength 설정
	//mmc_send_cmd_with_response( mmc_base, 0x063a, 0x80fffff1 );

//printk( "-----%s(%d)\n", __func__,__LINE__ );
	// CMD55 --> ACMD6	SET_BUS_WIDTH
	mmc_send_cmd_with_response( mmc_base, 0x371a, card_addr );
	mmc_send_cmd_with_response( mmc_base, 0x061a, 0x00000002 );



	// CLOCK high speed
	{
		int timeout = 100;
		
		clk = 0x0200 | SDHCI_CLOCK_INT_EN | SDHCI_CLOCK_CARD_EN;
		__raw_writew( clk, mmc_base + SDHCI_CLOCK_CONTROL );
		
		while ( 1 )
		{
			clk = __raw_readw( mmc_base + SDHCI_CLOCK_CONTROL );
			if ( clk & SDHCI_CLOCK_INT_STABLE ) break;
			udelay(100);
			
			if (timeout == 0) 
			{
				printk("---mmc err clk high-speed\n" );
				//clk = 0x0200 | SDHCI_CLOCK_INT_EN;
				clk += 0x100;
				writew( clk, mmc_base + SDHCI_CLOCK_CONTROL);
				break;
			}
		}
    	
		//clk |= SDHCI_CLOCK_CARD_EN;
		//writew( clk, mmc_base + SDHCI_CLOCK_CONTROL);
	}

//printk("  PRNSTS\t%08x\n", __raw_readl( mmc_base + 0x24 ) );	
//printk("  NORINTSTS\t%04x\n", __raw_readw( mmc_base + 0x30 ) );	
//printk("  ERRINTSTS\t%04x\n", __raw_readw( mmc_base + 0x32 ) );	

printk("..mmc end\n" );	
}


/* capture
-----mmc (mmc0) SDHCI_CLOCK_CONTROL=00008007
--> opcode=52  CMD=0x341a  ARG=0x00000c00
--> opcode=52  CMD=0x341a  ARG=0x80000c08
--> opcode=0  CMD=0x0000  ARG=0x00000000
--> opcode=8  CMD=0x081a  ARG=0x000001aa
                rsp=0x1aa  prnsts=01ff0000
--> opcode=5  CMD=0x0502  ARG=0x00000000
--> opcode=5  CMD=0x0502  ARG=0x00000000
--> opcode=5  CMD=0x0502  ARG=0x00000000
--> opcode=5  CMD=0x0502  ARG=0x00000000
--> opcode=55  CMD=0x371a  ARG=0x00000000
                rsp=0x400120  prnsts=01ff0000
--> opcode=41  CMD=0x2902  ARG=0x00000000
                rsp=0xff8000  prnsts=01ff0000

--> opcode=0  CMD=0x0000  ARG=0x00000000
--> opcode=8  CMD=0x081a  ARG=0x000001aa
                rsp=0x1aa  prnsts=01ff0000
--> opcode=55  CMD=0x371a  ARG=0x00000000
                rsp=0x120  prnsts=01ff0000
--> opcode=41  CMD=0x2902  ARG=0x40300000
                rsp=0xff8000  prnsts=01ff0000
--> opcode=55  CMD=0x371a  ARG=0x00000000
                rsp=0x120  prnsts=01ff0000
--> opcode=41  CMD=0x2902  ARG=0x40300000
                rsp=0x80ff8000  prnsts=01ff0000
                
--> opcode=2  CMD=0x0209  ARG=0x00000000
                rsp=0x3534453 0x44303247 0x80c02bae 0x3b00ba00 prnsts=01ff0000
--> opcode=3  CMD=0x031a  ARG=0x00000000
                rsp=0xe6240520  prnsts=01ff0000
--> opcode=9  CMD=0x0909  ARG=0xe6240000
                rsp=0x260032 0x5f5a83ae 0xfefbcfff 0x92804000 prnsts=01ff0000
--> opcode=7  CMD=0x071a  ARG=0xe6240000
                rsp=0x700  prnsts=01ff0000
--> opcode=55  CMD=0x371a  ARG=0xe6240000
                rsp=0x920  prnsts=01ff0000
--> opcode=51  CMD=0x333a  ARG=0x00000000
                rsp=0x920  prnsts=01ff0000
--> opcode=6  CMD=0x063a  ARG=0x00fffff1
                rsp=0x900  prnsts=01ff0000
-----mmc (mmc0) SDHCI_CLOCK_CONTROL=00000207
--> opcode=55  CMD=0x371a  ARG=0xe6240000
                rsp=0x920  prnsts=01ff0000
--> opcode=6  CMD=0x061a  ARG=0x00000002
                rsp=0x920  prnsts=01ff0000

mmc0: new SD card at address e624
--> opcode=16  CMD=0x101a  ARG=0x00000200
                rsp=0x900  prnsts=01ff0000
mmcblk0: mmc0:e624 SD02G 1.84 GiB 
 mmcblk0:--> opcode=18  CMD=0x123a  ARG=0x00000000
 p1 p2 p3 p4


*/


