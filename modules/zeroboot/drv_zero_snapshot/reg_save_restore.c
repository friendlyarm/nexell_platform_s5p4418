/*-----------------------------------------------------------------------------

  파 일 : reg_save_restore.c

  설 명 : 레지스터를 저장하고 복구하는 함수

  작 성 : freefrug@falinux.com

  날 짜 : 2011-07 ~

  주 의 :

		 ver 0.2.2

-------------------------------------------------------------------------------*/
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

//#include <mach/map.h>

#include <reg_save_restore.h>

#define PROC_NAME   "zb_hwlist"

void zeroboot_show_hw_list( void );

//--------------------------------------------------------------------------------------------
// 리스트로 관리하기 위한 변수

static zbhw_list_t *zbhw_list = NULL;
static u32  zbhw_list_cnt = 0;
//--------------------------------------------------------------------------------------------

//#define  _READ_REG(x)	{ (x) = __raw_readl(iomem+reg->addr); }
#define  _READ_REG(x)	{	\
		if ( 0 == (reg->flags & (RF_BIT16|RF_BIT08)) )	\
		{                                               \
			(x) = __raw_readl(iomem+reg->addr);   		\
		}                                               \
		else if ( reg->flags & RF_BIT16 )               \
		{                                               \
			(x) = __raw_readw(iomem+reg->addr);   		\
		}                                               \
		else                                            \
		{                                               \
			(x) = __raw_readb(iomem+reg->addr);   		\
		}                                               \
}

//#define  _WRITE_REG(x)	{ __raw_writel( (x), iomem+reg->addr); }
#define  _WRITE_REG(x)	{	\
		if ( 0 == (reg->flags & (RF_BIT16|RF_BIT08)) )	\
		{                                               \
			__raw_writel( (x), iomem+reg->addr);   		\
		}                                               \
		else if ( reg->flags & RF_BIT16 )               \
		{                                               \
			__raw_writew( (x), iomem+reg->addr);   		\
		}                                               \
		else                                            \
		{                                               \
			__raw_writeb( (x), iomem+reg->addr);   		\
		}                                               \
}


//------------------------------------------------------------------------------
/** @brief   레지스터를 저장한다. 
	@remark  
*///----------------------------------------------------------------------------
int zeroboot_save_reg(void *mem, reg_t *reg)
{
	volatile void *iomem = mem;
	unsigned int flags;

	while ( reg->addr != 0xFFFFFFFF ) 
	{
		// 주소를 순서대로 저장하는 형태라면
		if ( reg->addr == REG_ADDR_SEQ_MARK )
		{
			reg_t *cp_reg, *wk_reg;
			unsigned int addr, e_addr, step;
			
			cp_reg = reg + 1;
			wk_reg = reg + 2;
			addr   = reg->val;	// 시작주소
			e_addr = reg->mask; // 끝 주소
			step   = 4;
			if ( reg->flags & RF_BIT16 ) step = 2;
			if ( reg->flags & RF_BIT08 ) step = 1;
    	
			do
			{
				wk_reg->addr  = addr;
				wk_reg->val   = cp_reg->val;
				wk_reg->mask  = cp_reg->mask;
				wk_reg->flags = cp_reg->flags;
				
				wk_reg ++;
				addr += step;
    	
			} while ( addr <= e_addr ); 
    	
			reg += 2;
		}

		if ( 0 == (reg->flags & RF_RD) )
		{
			_READ_REG(reg->val);

			reg->val &= ~(reg->mask);
		}
		else
		{
			flags = reg->flags & ~RF_RD;
			
			if ( flags & RF_IGNORE_MASK_AS_READ )
			{
				_READ_REG(reg->val);
			}
			else if ( flags & RF_NOSAVE_AS_READ )
			{
				// nothing	
			}
		}
		reg++;
	}
	
	return 0;
}
//------------------------------------------------------------------------------
/** @brief   레지스터를 복구한다. 
	@remark  
*///----------------------------------------------------------------------------
int zeroboot_restore_reg(void *mem, reg_t *reg)
{
	volatile void *iomem = (volatile void *)mem;
	unsigned int val, flags;

	while ( reg->addr != 0xFFFFFFFF ) 
	{
		if ( reg->addr == REG_ADDR_SEQ_MARK ) reg += 2;
			
		if ( 0 == (reg->flags & RF_WR) )
		{
			_WRITE_REG(reg->val);
		}
		else
		{
			flags = reg->flags & ~RF_WR;
			
			if ( flags & RF_OR_MASK_AS_WRITE )
			{
				val = reg->val | reg->mask;
				_WRITE_REG(val);
				reg->val = val;
			}
			else if ( flags & RF_READ_BACK_AS_WRITE )
			{
				_READ_REG(val);
				val |= reg->mask;
				
				_WRITE_REG(val);
				reg->val = val;
			}
			else if ( flags & RF_READ_ONLY )
			{
				// nothing
			}
		}
		
		reg++;
	};
	
	return 0;
}
//------------------------------------------------------------------------------
/** @brief   레지스터를 모두 보여준다. 
	@remark  
*///----------------------------------------------------------------------------
int zeroboot_show_reg(void *mem, reg_t *reg)
{
	volatile void *iomem = (volatile void *)mem;

	if ( reg->addr == REG_ADDR_SEQ_MARK ) reg += 2;

	while ( reg->addr != 0xFFFFFFFF ) 
	{
		if ( 0 == (reg->flags & (RF_BIT16|RF_BIT08)) )	
		{                                               
			printk("  offset=0x%04X \t val=0x%08X \t cur=0x%08X\n", reg->addr, reg->val         , __raw_readl(iomem+reg->addr) );
		}                                               
		else if ( reg->flags & RF_BIT16 )               
		{                                               
			printk("  offset=0x%04X \t val=0x%04X \t\t cur=0x%04X\n", reg->addr, reg->val & 0xffff, __raw_readw(iomem+reg->addr) );
		}                                               
		else                                            
		{                                               
			printk("  offset=0x%04X \t val=0x%02X \t\t cur=0x%02X\n", reg->addr, reg->val & 0xff  , __raw_readb(iomem+reg->addr) );
		}                                               

		reg++;
	}
	
	return 0;
}
//------------------------------------------------------------------------------
/** @brief   proc 읽기를 지원한다. 
	@remark  
*///----------------------------------------------------------------------------
static int regsave_proc_read(char *buf, char **start, off_t fpos, int lenght, int *eof, void *data)
{
	char *p;
    
	p = buf;
	p += sprintf(p, "\nresotre io list\n\n" );
	p += sprintf(p, " %16s\tactive\tphys\n", "name" );
	p += sprintf(p, "===========================================\n");
	
	{
		zbhw_list_t  *list = zbhw_list;
		int count = zbhw_list_cnt;
		
		while( count-- )
		{
			p += sprintf(p, " %16s\t%d\t0x%08x\n", list->name, list->active ? 1:0, list->phys_base  );
			list ++;
		}		
	}
	
	p += sprintf(p, "\n\n");
	p += sprintf(p, " usage : echo 'hwabc=0'    > /proc/%s\n\n", PROC_NAME );
	p += sprintf(p, "         echo 'hwabc=1'    > /proc/%s\n\n", PROC_NAME );
	p += sprintf(p, "         echo 'hwabc=show' > /proc/%s\n\n", PROC_NAME );
	p += sprintf(p, "         echo 'showall'    > /proc/%s\n\n", PROC_NAME );
	*eof = 1;

	return p - buf;
}
//------------------------------------------------------------------------------
/** @brief   proc 쓰기를 지원한다. 
	@remark  
*///----------------------------------------------------------------------------
static int regsave_proc_write( struct file *file, const char __user *buffer, unsigned long count, void *data)
{
	char	cmd[128];
	int		len = count;
	int     idx, val = -1;
	int     show = 0;
	int     restore = 0;

	if ( len > 256 ) len = 256;
	if(copy_from_user( cmd, buffer, len ))
		return -EFAULT;
	cmd[len-1] = 0;	// CR 제거
	
	if ( 0 == strncmp( "showall", cmd, 7 ) )
	{
		printk( "** showall reg list ----------------------------------------\n" );
		zeroboot_show_hw_list();
		return count;		
	}
	                    
	for ( idx=0; idx<len; idx++ )
	{
		if ( '=' == cmd[idx] )
		{
			// 명령어 show 분석
			if ( 0 == strncmp( "show", &cmd[idx+1], 4 ) )
			{				
				show = 1;
			}
			// 명령어 restore 분석
			else if ( 0 == strncmp( "restore", &cmd[idx+1], 7 ) )
			{				
				restore = 1;
			}
			else	// 숫자값 분석
			{		
				val = simple_strtoul( &cmd[idx+1], NULL, 10 );
			}

			cmd[idx] = 0;
			break;
		} 	
	}
	
	if ( 0 <= val || show || restore )
	{
		zbhw_list_t  *list = zbhw_list;
		int  cmd_len;
		int  listcnt = zbhw_list_cnt;
		
		cmd_len = strlen(cmd);
		
		while( listcnt-- )
		{
			// cmd 길이만큼만 비교한다. 앞이 같은 문자열도 동일한 것으로 간주한다.
			//   예) "hdmi_core", "hdmi_ctrl" 두개의 이름은 "hdmi" 로 모두 선택할 수 있다.
			if ( strncmp( list->name, cmd, cmd_len  ) == 0 ) 
			{   
				if ( show )
				{
					printk( ">> %s\n",  list->name );
					zeroboot_show_reg( list->iomem, list->regs );
				}
				else if ( restore )
				{
					printk( ">> IO restore %s\n",  list->name );
					zeroboot_restore_reg( list->iomem, list->regs );
				}
				else
				{
					if ( REG_MUST_ACTIVE != list->active )
					{
						list->active = val & 0x1;
						printk( " OK %s = %d\n", list->name, list->active );	
					}
				}	
				
				//break;
			}
			
			list ++;
		}		
	}
	
	return count;
}
//------------------------------------------------------------------------------
/** @brief   proc 등록
	@remark  
*///----------------------------------------------------------------------------
static void regsave_register_proc( void )
{
	struct proc_dir_entry *procdir;
	
	procdir = create_proc_entry( PROC_NAME, S_IFREG | S_IRUGO, 0);
	procdir->read_proc  = regsave_proc_read;
	procdir->write_proc = regsave_proc_write;
}
//------------------------------------------------------------------------------
/** @brief   proc 해제
	@remark  
*///----------------------------------------------------------------------------
static void regsave_unregister_proc( void )
{
	remove_proc_entry( PROC_NAME,0 );
}

//------------------------------------------------------------------------------
/** @brief   레지스터 관리항목 등록
	@remark  
*///----------------------------------------------------------------------------
void zeroboot_checkin_hw_list( zbhw_list_t *list, u32 count )
{
	zbhw_list = list;
	zbhw_list_cnt = count;
	while( count-- )
	{
		if (list->phys_base && list->phys_size)
			list->iomem  = ioremap( list->phys_base, list->phys_size );
		list ++;
	}
	
	regsave_register_proc();
}
//------------------------------------------------------------------------------
/** @brief   레지스터 관리항목 해제
	@remark  
*///----------------------------------------------------------------------------
void zeroboot_checkout_hw_list( void )
{
	zbhw_list_t  *list = zbhw_list;

	while( zbhw_list_cnt-- )
	{
		iounmap( list->iomem );
		list ++;
	}
	
	regsave_unregister_proc();
}
//------------------------------------------------------------------------------
/** @brief   레지스터 관리항목 저장
	@remark  
*///----------------------------------------------------------------------------
void zeroboot_save_hw_list( void )
{
	zbhw_list_t  *list = zbhw_list;
	int count = zbhw_list_cnt;
	while( count-- )
	{
		if ( list->active )
		{
			printk("\thwlist_save\tphys\tiomem=0x%08X\t0x%p\t%s\n", list->phys_base, list->iomem, list->name );
			zeroboot_save_reg( list->iomem, list->regs );
		}
		list ++;
	}
}
//------------------------------------------------------------------------------
/** @brief   레지스터 관리항목 복구
	@remark  
*///----------------------------------------------------------------------------
void zeroboot_restore_hw_list( void )
{
	zbhw_list_t  *list = zbhw_list;
	int count = zbhw_list_cnt;
		printk("***************Restore Hw List Count: %d\n",count);
	while( count-- )
	{
		if ( list->active == REG_ACTIVE || list->active == REG_MUST_ACTIVE )
		{
			printk("hwlist_restore iomem 0x%p phys=0x%08X\t%s", list->iomem, list->phys_base, list->name );
			zeroboot_restore_reg( list->iomem, list->regs );
			printk(" done...\n");
		}
		list ++;
	}
}
//------------------------------------------------------------------------------
/** @brief   레지스터 관리항목 늦은 복구
	@remark  
*///----------------------------------------------------------------------------
void zeroboot_restore_late_hw_list( void )
{
	zbhw_list_t  *list;
	int count;
	int max, cur;
	
	cur = REG_LATE_ACTIVE;
	max = REG_LATE_ACTIVE;

	do 
	{
		list = zbhw_list; 
		count = zbhw_list_cnt;
		
		while( count-- )
		{
			if ( list->active < REG_MUST_ACTIVE )
			{
				if ( list->active > max  ) 
				{
					max = list->active; 
				}
				else if ( list->active == cur )
				{	
					printk("hwlist_restore phys=0x%08X\t%s\n", list->phys_base, list->name );
					zeroboot_restore_reg( list->iomem, list->regs );
				}
			}
			list ++;
		}
		
	} while ( ++cur <= max );
}
//------------------------------------------------------------------------------
/** @brief   레지스터 관리항목 표출
	@remark  
*///----------------------------------------------------------------------------
void zeroboot_show_hw_list( void )
{
	zbhw_list_t  *list = zbhw_list;
	int count = zbhw_list_cnt;
	
	while( count-- )
	{
		printk( ">> %s\n",  list->name );
		zeroboot_show_reg( list->iomem, list->regs );
		list ++;
	}
}
//------------------------------------------------------------------------------
/** @brief   등록된 관리항목에서 이름에 맞는 리스트 아이템 포인터를 넘겨준다.
	@remark  
*///----------------------------------------------------------------------------
zbhw_list_t *zeroboot_get_hw_list( const char *name )
{
	zbhw_list_t  *list = zbhw_list;
	int count = zbhw_list_cnt;
	
	while( count-- )
	{
		if ( strcmp( list->name, name ) == 0 ) 
		{   
			return list;
		}
		list ++;
	}
	
	return NULL;
}



