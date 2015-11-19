/*-----------------------------------------------------------------------------
  파 일 : hdmi_s5pv210.c
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-03-27
  주 의 :

-------------------------------------------------------------------------------*/
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
#include <linux/pm.h>
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

/// @{
/// @brief  define
///-----------------------------------------------------------------------------
#define PHY_I2C_ADDRESS       	0x70
#define PHY_REG_MODE_SET_DONE 	0x1F

#define HDMI_I2C_CON			(0x0000)
#define HDMI_I2C_STAT			(0x0004)
#define HDMI_I2C_ADD			(0x0008)
#define HDMI_I2C_DS				(0x000c)
#define HDMI_I2C_LC				(0x0010)

#define I2C_ACK					(1 << 7)
#define I2C_INT					(1 << 5)
#define I2C_PEND				(1 << 4)
#define I2C_INT_CLEAR			(0 << 4)
#define I2C_CLK					(0x41)
#define I2C_CLK_PEND_INT		(I2C_CLK | I2C_INT_CLEAR | I2C_INT)
#define I2C_ENABLE				(1 << 4)
#define I2C_START				(1 << 5)
#define I2C_MODE_MTX			0xC0
#define I2C_MODE_MRX			0x80
#define I2C_IDLE				0

#define STATE_IDLE 				0
#define STATE_TX_EDDC_SEGADDR	1
#define STATE_TX_EDDC_SEGNUM	2
#define STATE_TX_DDC_ADDR		3
#define STATE_TX_DDC_OFFSET		4
#define STATE_RX_DDC_ADDR		5
#define STATE_RX_DDC_DATA		6
#define STATE_RX_ADDR			7
#define STATE_RX_DATA			8
#define STATE_TX_ADDR			9
#define STATE_TX_DATA			10
#define STATE_TX_STOP			11
#define STATE_RX_STOP			12

// video format for HDMI HW (timings and AVI) and EDID
enum s5p_hdmi_v_fmt {
	v640x480p_60Hz = 0,
	v720x480p_60Hz,
	v1280x720p_60Hz,
	v1920x1080i_60Hz,
	v720x480i_60Hz,
	v720x240p_60Hz,
	v2880x480i_60Hz,
	v2880x240p_60Hz,
	v1440x480p_60Hz,
	v1920x1080p_60Hz,
	v720x576p_50Hz,
	v1280x720p_50Hz,
	v1920x1080i_50Hz,
	v720x576i_50Hz,
	v720x288p_50Hz,
	v2880x576i_50Hz,
	v2880x288p_50Hz,
	v1440x576p_50Hz,
	v1920x1080p_50Hz,
	v1920x1080p_24Hz,
	v1920x1080p_25Hz,
	v1920x1080p_30Hz,
	v2880x480p_60Hz,
	v2880x576p_50Hz,
	v1920x1080i_50Hz_1250,
	v1920x1080i_100Hz,
	v1280x720p_100Hz,
	v720x576p_100Hz,
	v720x576i_100Hz,
	v1920x1080i_120Hz,
	v1280x720p_120Hz,
	v720x480p_120Hz,
	v720x480i_120Hz,
	v720x576p_200Hz,
	v720x576i_200Hz,
	v720x480p_240Hz,
	v720x480i_240Hz,
	v720x480p_59Hz,
	v1280x720p_59Hz,
	v1920x1080i_59Hz,
	v1920x1080p_59Hz,
};
/// @}



/// @{
/// @brief  외부 함수,변수정의
///-----------------------------------------------------------------------------
static struct {
	s32    state;
	u8 	  *buffer;
	s32    bytes;
} i2c_hdmi_phy_context;


static void *i2c_hdmi_phy_base = NULL;

#include "hdmi_data.c"

/// @}


void wait_msec( int a )
{
	while ( a-- ) udelay(1000);
}

/*-----------------------------------------------------------------------------
  @brief  s5p_hdmi_i2c_phy_interruptwait
  @remark  
*///----------------------------------------------------------------------------
static s32 s5p_hdmi_i2c_phy_interruptwait(void)
{
	u8 status, reg;
	s32 retval = 0;

	do {
		status = __raw_readb(i2c_hdmi_phy_base + HDMI_I2C_CON);

		if (status & I2C_PEND) 
		{
			reg = __raw_readb(i2c_hdmi_phy_base + HDMI_I2C_STAT);
			break;
		}

	} while (1);

	return retval;
}

/*-----------------------------------------------------------------------------
  @brief  s5p_hdmi_i2c_phy_read
  @remark  
*///----------------------------------------------------------------------------
static s32 s5p_hdmi_i2c_phy_read( u8 addr, u8 nbytes, u8 *buffer )
{
	u8 reg;
	s32 ret = 0;
	u32 proc = 1;

	i2c_hdmi_phy_context.state  = STATE_RX_ADDR;
	i2c_hdmi_phy_context.buffer = buffer;
	i2c_hdmi_phy_context.bytes  = nbytes;

	__raw_writeb(I2C_CLK | I2C_INT | I2C_ACK          , i2c_hdmi_phy_base + HDMI_I2C_CON );
	__raw_writeb(I2C_ENABLE | I2C_MODE_MRX            , i2c_hdmi_phy_base + HDMI_I2C_STAT);
	__raw_writeb(addr & 0xFE                          , i2c_hdmi_phy_base + HDMI_I2C_DS  );
	__raw_writeb(I2C_ENABLE | I2C_START | I2C_MODE_MRX,	i2c_hdmi_phy_base + HDMI_I2C_STAT);

	while (proc) 
	{
		if (i2c_hdmi_phy_context.state != STATE_RX_STOP) 
		{
			if (s5p_hdmi_i2c_phy_interruptwait() != 0) 
			{
				printk("interrupt wait failed!!!\n");
				ret = -1;
				break;
			}
		}

		switch (i2c_hdmi_phy_context.state) 
		{
		case STATE_RX_DATA:
			reg = __raw_readb(i2c_hdmi_phy_base + HDMI_I2C_DS);
			*(i2c_hdmi_phy_context.buffer) = reg;

			i2c_hdmi_phy_context.buffer++;
			--(i2c_hdmi_phy_context.bytes);

			if (i2c_hdmi_phy_context.bytes == 1) 
			{
				i2c_hdmi_phy_context.state = STATE_RX_STOP;
				__raw_writeb(I2C_CLK_PEND_INT,	i2c_hdmi_phy_base + HDMI_I2C_CON);
			} 
			else 
			{
				writeb(I2C_CLK_PEND_INT | I2C_ACK,	i2c_hdmi_phy_base + HDMI_I2C_CON);
			}
			break;

		case STATE_RX_ADDR:
			i2c_hdmi_phy_context.state = STATE_RX_DATA;

			if (i2c_hdmi_phy_context.bytes == 1) 
			{
				i2c_hdmi_phy_context.state = STATE_RX_STOP;
				writeb(I2C_CLK_PEND_INT,	i2c_hdmi_phy_base + HDMI_I2C_CON);
			} 
			else 
			{
				writeb(I2C_CLK_PEND_INT | I2C_ACK,	i2c_hdmi_phy_base + HDMI_I2C_CON);
			}
			break;

		case STATE_RX_STOP:
			i2c_hdmi_phy_context.state = STATE_IDLE;

			reg = __raw_readb(i2c_hdmi_phy_base + HDMI_I2C_DS);

			*(i2c_hdmi_phy_context.buffer) = reg;

			__raw_writeb(I2C_MODE_MRX|I2C_ENABLE,	i2c_hdmi_phy_base + HDMI_I2C_STAT);
			__raw_writeb(I2C_CLK_PEND_INT,			i2c_hdmi_phy_base + HDMI_I2C_CON );
			__raw_writeb(I2C_MODE_MRX,				i2c_hdmi_phy_base + HDMI_I2C_STAT);

			while (__raw_readb(i2c_hdmi_phy_base + HDMI_I2C_STAT) & I2C_START)
			{
				wait_msec(1);
			}
			
			proc = 0;
			break;

		case STATE_IDLE:
		default:
			printk("error state!!!\n");
			ret = EINVAL;
			proc = 0;
			break;
		}

	}

	return ret;
}

/*-----------------------------------------------------------------------------
  @brief  s5p_hdmi_i2c_phy_write
  @remark  
*///----------------------------------------------------------------------------
static s32 s5p_hdmi_i2c_phy_write( u8 addr, u8 nbytes, u8 *buffer )
{
	u8 reg;
	s32 ret = 0;
	u32 proc = 1;

	i2c_hdmi_phy_context.state  = STATE_TX_ADDR;
	i2c_hdmi_phy_context.buffer = buffer;
	i2c_hdmi_phy_context.bytes  = nbytes;

	__raw_writeb(I2C_CLK | I2C_INT | I2C_ACK          , i2c_hdmi_phy_base + HDMI_I2C_CON );
	__raw_writeb(I2C_ENABLE | I2C_MODE_MTX            , i2c_hdmi_phy_base + HDMI_I2C_STAT);
	__raw_writeb(addr & 0xFE                          , i2c_hdmi_phy_base + HDMI_I2C_DS  );
	__raw_writeb(I2C_ENABLE | I2C_START | I2C_MODE_MTX,	i2c_hdmi_phy_base + HDMI_I2C_STAT);

	while (proc) 
	{
		if (s5p_hdmi_i2c_phy_interruptwait() != 0) 
		{
			printk("interrupt wait failed!!!\n");
			ret = -1;
			break;
		}

		switch (i2c_hdmi_phy_context.state) 
		{
		case STATE_TX_ADDR:
		case STATE_TX_DATA:
			i2c_hdmi_phy_context.state = STATE_TX_DATA;

			reg = *(i2c_hdmi_phy_context.buffer);

			__raw_writeb(reg, i2c_hdmi_phy_base + HDMI_I2C_DS);

			i2c_hdmi_phy_context.buffer++;
			--(i2c_hdmi_phy_context.bytes);

			if (i2c_hdmi_phy_context.bytes == 0) 
			{
				i2c_hdmi_phy_context.state = STATE_TX_STOP;
				__raw_writeb(I2C_CLK_PEND_INT,	i2c_hdmi_phy_base + HDMI_I2C_CON);
			} 
			else 
			{
				__raw_writeb(I2C_CLK_PEND_INT | I2C_ACK, i2c_hdmi_phy_base + HDMI_I2C_CON);
			}

			break;

		case STATE_TX_STOP:
			i2c_hdmi_phy_context.state = STATE_IDLE;

			__raw_writeb(I2C_MODE_MTX | I2C_ENABLE,	i2c_hdmi_phy_base + HDMI_I2C_STAT);
			__raw_writeb(I2C_CLK_PEND_INT,			i2c_hdmi_phy_base + HDMI_I2C_CON );
			__raw_writeb(I2C_MODE_MTX,				i2c_hdmi_phy_base + HDMI_I2C_STAT);

			while ( __raw_readb(i2c_hdmi_phy_base + HDMI_I2C_STAT) & I2C_START )
			{
				wait_msec(1);
			}

			proc = 0;
			break;

		case STATE_IDLE:
		default:
			printk("error state!!!\n");
			ret = EINVAL;
			proc = 0;
			break;
		}

	}

	return ret;
}

static int s5p_hdmi_phy_down(bool on, u8 addr, u8 offset, u8 *read_buffer)
{
	u8 buff[2] = {0};

	buff[0] = addr;
	buff[1] = (on) ? (read_buffer[addr] & (~(1 << offset))) :
			(read_buffer[addr] | (1 << offset));

	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 2, buff) != 0)
	{
		printk( "s5p_hdmi_i2c_phy_write failed.  (%d)\n", __LINE__ );
		return -1;
	}
	return 0;
}

int s5p_hdmi_phy_power(int on)
{
	u32 size;
	u8 *buffer;
	u8 read_buffer[0x40] = {0, };

	size   = sizeof(phy_config[0][0]) / sizeof(phy_config[0][0][0]);
	buffer = (u8 *) phy_config[0][0];

	/* write offset */
	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0)
	{
		printk( "s5p_hdmi_i2c_phy_write failed.  (%d)\n", __LINE__ );
		return -1;
	}
	
	/* read data */
	if (s5p_hdmi_i2c_phy_read(PHY_I2C_ADDRESS, size, read_buffer) != 0) 
	{
		printk( "s5p_hdmi_i2c_phy_read failed.  (%d)\n", __LINE__ );
		return -1;
	}

	// i can't get the information about phy setting
	if (on) {
		// on 
		// biaspd 
		s5p_hdmi_phy_down(1, 0x1, 0x5, read_buffer);
		// clockgenpd 
		s5p_hdmi_phy_down(1, 0x1, 0x7, read_buffer);
		// pllpd 
		s5p_hdmi_phy_down(1, 0x5, 0x5, read_buffer);
		// pcgpd 
		s5p_hdmi_phy_down(1, 0x17, 0x0, read_buffer);
		// txpd 
		s5p_hdmi_phy_down(1, 0x17, 0x1, read_buffer);
	} else {
		// off 
		// biaspd 
		s5p_hdmi_phy_down(0, 0x1, 0x5, read_buffer);
		// clockgenpd 
		s5p_hdmi_phy_down(0, 0x1, 0x7, read_buffer);
		// pllpd 
		s5p_hdmi_phy_down(0, 0x5, 0x5, read_buffer);
		// pcgpd
		s5p_hdmi_phy_down(0, 0x17, 0x0, read_buffer);
		// txpd 
		s5p_hdmi_phy_down(0, 0x17, 0x1, read_buffer);
	}

	return 0;
}


/*-----------------------------------------------------------------------------
  @brief   hdmi init 
  @remark  
*///----------------------------------------------------------------------------
void hdmi_s5pv210_init( void *iomem_i2c1, void *iomem_ctrl, void *iomem_core )
{
	u8  buffer[32] = {0, };
	u8  reg;
	s32 index, freq, size;

	printk( "hdmi init start.\n" );

	i2c_hdmi_phy_base = iomem_i2c1;

	// i2c_hdmi init - set i2c filtering
	__raw_writeb( 0x5, i2c_hdmi_phy_base + HDMI_I2C_LC );

	// i2c enable
	//writeb(I2C_ENABLE, i2c_hdmi_phy_base + HDMI_I2C_STAT);


	s5p_hdmi_phy_power(1);
	
		
	index = 0;
	freq  = v1920x1080p_60Hz;

	// i2c_hdmi init - set i2c filtering
	buffer[0] = PHY_REG_MODE_SET_DONE;
	buffer[1] = 0x00;
    
	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 2, buffer) != 0) 
	{
		printk( "s5p_hdmi_i2c_phy_write failed.  (%d)\n", __LINE__ );
		return;
	}

	// write config
	__raw_writeb( 0x5, i2c_hdmi_phy_base + HDMI_I2C_LC );

	size = sizeof(phy_config[freq][index]) / sizeof(phy_config[freq][index][0]);
	memcpy(buffer, phy_config[freq][index], sizeof(buffer));

	if ( s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, size, buffer) != 0 )
	{
		printk( "s5p_hdmi_i2c_phy_write failed.  (%d)\n", __LINE__ );
		return;
	}
	
	// write offset
	buffer[0] = 0x01;
	if (s5p_hdmi_i2c_phy_write(PHY_I2C_ADDRESS, 1, buffer) != 0) 
	{
		printk( "s5p_hdmi_i2c_phy_write failed.  (%d)\n", __LINE__ );
		return;
	}


	// 0x05 0x00 0xd8 0x10 0x9c 0xf8 0x40 0x6a
	// 0x18 0x00 0x51 0xff 0xf1 0x54 0xba 0x84
	// 0x00 0x10 0x38 0x00 0x08 0x10 0xe0 0x22
	// 0x40 0xa4 0x26 0x02 0x00 0x00 0x00
	if (1)
	{
		int i = 0;
		u8 read_buffer[0x40] = {0, };
	
		// read data
		if (s5p_hdmi_i2c_phy_read(PHY_I2C_ADDRESS, size, read_buffer) != 0) 
		{
			printk( "s5p_hdmi_i2c_phy_read failed.  (%d)\n", __LINE__ );
			return;
		}
	
		printk("read buffer :\n\t\t");
	
		for (i = 1; i < size; i++) {
			printk("0x%02x", read_buffer[i]);
	
			if (i % 8)
				printk(" ");
			else
				printk("\n\t\t");
		}
		printk("\n");
	}

	// s5p_hdmi_corereset(void)
	{
		__raw_writeb(0x0, iomem_ctrl + 0x20 );  // define	S5P_HDMI_CORE_RSTOUT			0x0020
		
		wait_msec( 30 ); // 10
	
		__raw_writeb(0x1, iomem_ctrl + 0x20 );	// define	S5P_HDMI_CORE_RSTOUT			0x0020
	}

	// wait ready
	do {
		reg = __raw_readb( iomem_core + 0x14 );	// #define	S5P_HDMI_PHY_STATUS				0x0014
	} while ( !(reg & 0x1) );					// #define	S5P_HDMI_PHY_STATUS_READY		(1)


	writeb(I2C_CLK_PEND_INT, i2c_hdmi_phy_base + HDMI_I2C_CON);

	// i2c disable
	writeb(I2C_IDLE, i2c_hdmi_phy_base + HDMI_I2C_STAT);

	printk( "hdmi init success.\n" );
	
	
}

/*


__raw_writel(pd_status, S5P_NORMAL_CFG);


static int s5pv210_pd_pwr_done(int ctrl)
{
	unsigned int cnt;

	cnt = 10000;

	do{
		if(__raw_readl(S5P_BLK_PWR_STAT) & ctrl)
			return 0;

	}while(cnt-- > 0);

	return -ETIME;
}
*/


