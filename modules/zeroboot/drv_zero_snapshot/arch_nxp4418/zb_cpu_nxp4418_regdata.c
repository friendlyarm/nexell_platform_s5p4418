/*-----------------------------------------------------------------------------
  파 일 : zb_cpu_s5pv210_regdata.c
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-01-04
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
#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>
#include <asm/io.h>
#include <asm/irq.h>
#include <asm/delay.h>

#include <reg_save_restore.h>

reg_t regv_uart1[]= {
	{ .addr=0x00000080, .val=0x00000000, .mask=0,		.flags=0 }, 	 //Uart1 Control Register
	{ .addr=0x00000084, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000088, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000008c, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x00000090, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x0000009c, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0,		.flags=0 },
};

reg_t regv_uart2[]= {
	{ .addr=0x00000080, .val=0x00000000, .mask=0,		.flags=0 }, 	 //Uart2 Control Register
	{ .addr=0x00000084, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000088, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000008c, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x00000090, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x0000009c, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0,		.flags=0 },
};


reg_t regv_uart3[]= {
	{ .addr=0x00000080, .val=0x00000000, .mask=0,		.flags=0 }, 	 //Uart3 Control Register
	{ .addr=0x00000084, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000088, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000008c, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x00000090, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x0000009c, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0,		.flags=0 },
};


reg_t regv_uart4[]= {
	{ .addr=0x00000080, .val=0x00000000, .mask=0,		.flags=0 }, 	 //Uart4 Control Register
	{ .addr=0x00000084, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000088, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000008c, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x00000090, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x0000009c, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0,		.flags=0 },
};

reg_t regv_uart5[]= {
	{ .addr=0x00000080, .val=0x00000000, .mask=0,		.flags=0 }, 	 //Uart4 Control Register
	{ .addr=0x00000084, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000088, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000008c, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x00000090, .val=0x00000000, .mask=0,		.flags=0 }, 	
	{ .addr=0x0000009c, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000a8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b0, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000000b8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0,		.flags=0 },
};


reg_t regv_gpt[]= {
	{ .addr=0x00000000, .val=0x00000000, .mask=1,		.flags=0 }, //General Purpose Timer
	{ .addr=0x00000004, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000008, .val=0x00000000, .mask=0,		.flags=RF_READ_ONLY },           //status, 1480p
	{ .addr=0x0000000C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000010, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000014, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000018, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000024, .val=0x00000000, .mask=0,		.flags=RF_READ_ONLY },  
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0,		.flags=0 },
};


reg_t regv_epit[]= {
	{ .addr=0x00000000, .val=0x00000000, .mask=0,		.flags=0 }, //EPIT Control Register
	{ .addr=0x00000004, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000008, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00004000, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00004004, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00004008, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000400c, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0,		.flags=0 },
};

#if 1
// ARM MP Register
reg_t regv_armmp[] = {
	// SCU registers ( 0~ FCh ) size : 0xFC
	{ .addr=0x00000000, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00000004, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000008, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000040, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000044, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00000050, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00000054, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },

	//Interrupt Controller Interface ( 100 ~ 1FFh ) size : 0x100 , 83page
	{ .addr=0x00000100, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00000100, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00000104, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00000108, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },

	//Global timer ( 200 ~ 2FFh ) size : 0x100
	{ .addr=0x00000200, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000204, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000208, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000210, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000214, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000218, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	

	//PrivateTimer and watchdogs registers ( 600 ~6FFh ) size: 0x100
	{ .addr=0x00000600, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	//Private timer and watchdog registers,87page
	{ .addr=0x00000604, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000608, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000060C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000620, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000624, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000628, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x0000062C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00000630, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },

	//Interrupt Distributor Register ( 1000 ~ 1FFFh ) size : 0x1000
	{ .addr=0x00001000, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },//Distributor register,68page

	{ .addr=0x00001080, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001084, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001088, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000108C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000108f, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001090, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001094, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001098, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000109C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },

	{ .addr=0x00001100, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001104, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001108, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000110C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000110f, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001110, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001114, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001118, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000111C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
//	ok

/* not working -> only used secure mode 
	{ .addr=0x00001180, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001184, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001188, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000118C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000118f, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001190, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001194, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001198, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000119C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
*/

	
//ok -below
	{ .addr=0x00001200, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001204, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001208, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000120C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000120f, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001210, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },
	{ .addr=0x00001214, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x00001218, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },	
	{ .addr=0x0000121C, .val=0x00000000, .mask=0, 		.flags=RF_WR_ARMMP },

	{ .addr=0x00001400, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001404, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001408, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000140C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000140f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001410, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001414, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001418, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000141C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000141f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001420, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001424, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001428, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000142C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000142f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001430, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001434, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001438, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000143C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000143f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001440, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001444, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001448, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000144C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000144f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001450, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001454, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001458, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000145C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000145f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001460, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001464, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001468, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000146C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000146f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001470, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001474, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001478, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000147C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000147f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001480, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001484, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001488, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000148C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000148f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001490, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001494, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001498, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000149C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000149f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014a0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014a4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014a8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014aC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014af, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014b0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014b4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014b8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014bC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014bf, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014c0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014c4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014c8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014cC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014cf, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014d0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014d4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014d8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014dC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014df, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014e0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014e4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014e8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014eC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014ef, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014f0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000014f4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014f8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000014fC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },

	{ .addr=0x00001800, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001804, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001808, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000180C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000180f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001810, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001814, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001818, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000181C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000181f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001820, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001824, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001828, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000182C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000182f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001830, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001834, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001838, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000183C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000183f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001840, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001844, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001848, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000184C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000184f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001850, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001854, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001858, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000185C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000185f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001860, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001864, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001868, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000186C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000186f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001870, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001874, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001878, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000187C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000187f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001880, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001884, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001888, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000188C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000188f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001890, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001894, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001898, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x0000189C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x0000189f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018a0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018a4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018a8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018aC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018af, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018b0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018b4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018b8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018bC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018bf, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018c0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018c4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018c8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018cC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018cf, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018d0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018d4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018d8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018dC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018df, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018e0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018e4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018e8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018eC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018ef, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018f0, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x000018f4, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018f8, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x000018fC, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c00, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c04, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c08, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c0C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c0f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c10, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c14, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c18, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c1C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c1f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c20, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c24, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c28, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c2C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c2f, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c30, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },
	{ .addr=0x00001c34, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c38, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP }, 
	{ .addr=0x00001c3C, .val=0x00000000, .mask=0,		.flags=RF_WR_ARMMP },


	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, 		.flags=RF_WR_ARMMP },
};

#else
// Platform Register Base : A0 0000h ~ A0 1FFFh 
reg_t regv_scu[]={  // SCU registers ( 0~ FCh ) size : 0xFC
	{ .addr=0x00000000, .val=0x00000000, .mask=0, 		.flags=0 },	//SCU Control Register
	{ .addr=0x00000004, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000008, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000040, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000044, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000050, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000054, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, 		.flags=0 },
};

reg_t regv_ici[]={ //Interrupt Controller Interface ( 100 ~ 1FFh ) size : 0x100 , 83page
	{ .addr=0x00000100, .val=0x00000000, .mask=0, 		.flags=0 },

	{ .addr=0x00000100, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000104, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000108, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, 		.flags=0 },
};

reg_t regv_gtimer[]={ //Global timer ( 200 ~ 2FFh ) size : 0x100
	{ .addr=0x00000000, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000004, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000008, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000010, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000014, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000018, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, 		.flags=0 },
};


reg_t regv_ptimer_watchdog[]={ //PrivateTimer and watchdogs registers ( 600 ~6FFh ) size: 0x100
	{ .addr=0x00000000, .val=0x00000000, .mask=0, 		.flags=0 },	//Private timer and watchdog registers,87page
	{ .addr=0x00000004, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000008, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000020, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000024, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000028, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000030, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, 		.flags=0 },
};

reg_t regv_interrupt_distibutor[]={ //Interrupt Distributor Register ( 1000 ~ 1FFFh ) size : 0x1000
	{ .addr=0x00000000, .val=0x00000000, .mask=0, 		.flags=0 },//Distributor register,68page

	{ .addr=0x00000080, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000084, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000088, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000008C, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000008f, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000090, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000094, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000098, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000009C, .val=0x00000000, .mask=0, 		.flags=0 },

	{ .addr=0x00000100, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000104, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000108, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000010C, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000010f, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000110, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000114, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000118, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000011C, .val=0x00000000, .mask=0, 		.flags=0 },
//	ok

/* not working -> only used secure mode 
	{ .addr=0x00000180, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000184, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000188, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000018C, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000018f, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000190, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000194, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000198, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000019C, .val=0x00000000, .mask=0, 		.flags=0 },
*/

	
//ok -below
	{ .addr=0x00000200, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000204, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000208, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000020C, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000020f, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000210, .val=0x00000000, .mask=0, 		.flags=0 },
	{ .addr=0x00000214, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x00000218, .val=0x00000000, .mask=0, 		.flags=0 },	
	{ .addr=0x0000021C, .val=0x00000000, .mask=0, 		.flags=0 },

	{ .addr=0x00000400, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000404, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000408, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000040C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000040f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000410, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000414, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000418, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000041C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000041f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000420, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000424, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000428, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000042C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000042f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000430, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000434, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000438, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000043C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000043f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000440, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000444, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000448, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000044C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000044f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000450, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000454, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000458, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000045C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000045f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000460, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000464, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000468, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000046C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000046f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000470, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000474, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000478, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000047C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000047f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000480, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000484, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000488, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000048C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000048f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000490, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000494, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000498, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000049C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000049f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004a0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004a4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004a8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004aC, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004af, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004b0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004b4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004b8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004bC, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004bf, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004c0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004c4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004c8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004cC, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004cf, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004d0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004d4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004d8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004dC, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004df, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004e0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004e4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004e8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004eC, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004ef, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004f0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000004f4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004f8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000004fC, .val=0x00000000, .mask=0,		.flags=0 },

	{ .addr=0x00000800, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000804, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000808, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000080C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000080f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000810, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000814, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000818, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000081C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000081f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000820, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000824, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000828, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000082C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000082f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000830, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000834, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000838, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000083C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000083f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000840, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000844, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000848, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000084C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000084f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000850, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000854, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000858, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000085C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000085f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000860, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000864, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000868, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000086C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000086f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000870, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000874, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000878, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000087C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000087f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000880, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000884, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000888, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000088C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000088f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000890, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000894, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000898, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x0000089C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x0000089f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008a0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008a4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008a8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008aC, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008af, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008b0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008b4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008b8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008bC, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008bf, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008c0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008c4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008c8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008cC, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008cf, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008d0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008d4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008d8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008dC, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008df, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008e0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008e4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008e8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008eC, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008ef, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008f0, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x000008f4, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008f8, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x000008fC, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c00, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c04, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c08, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c0C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c0f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c10, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c14, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c18, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c1C, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c1f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c20, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c24, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c28, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c2C, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c2f, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c30, .val=0x00000000, .mask=0,		.flags=0 },
	{ .addr=0x00000c34, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c38, .val=0x00000000, .mask=0,		.flags=0 }, 
	{ .addr=0x00000c3C, .val=0x00000000, .mask=0,		.flags=0 },


	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, 		.flags=0 },
};
#endif


#if 0
// RTC Timer  0xE2800000, size=0x100 
reg_t regv_rtctimer[] = {
	{ .addr=0x00000040, .val=0x00000000, .mask=(1<<8), 	.flags=RF_OR_MASK_AS_WRITE },	// RTCCON	: d8(TICEN) = 0
	{ .addr=0x00000044, .val=0x00000000, .mask=0, 		.flags=0 },		// TICCNT
	{ .addr=0x00000050, .val=0x00000000, .mask=0, 		.flags=0 },		// RTCALM
	{ .addr=0x00000054, .val=0x00000000, .mask=0, 		.flags=0 },		// ALMSEC
	{ .addr=0x00000058, .val=0x00000000, .mask=0, 		.flags=0 },		// ALMMIN
	{ .addr=0x0000005C, .val=0x00000000, .mask=0, 		.flags=0 },		// ALMHOUR
	{ .addr=0x00000060, .val=0x00000000, .mask=0, 		.flags=0 },		// ALMDAY
	{ .addr=0x00000064, .val=0x00000000, .mask=0, 		.flags=0 },		// ALMMON
	{ .addr=0x00000068, .val=0x00000000, .mask=0, 		.flags=0 },		// ALMYEAR

	// for start timer
	{ .addr=0x00000040, .val=0x00000000, .mask=0, 		.flags=0 },		// RTCCON 	: d8(TICEN) = (?)

	{ .addr=0x00000030, .val=0x00000000, .mask=0, 		.flags=0 },		// INTP
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, 		.flags=0 }
};

// PWM Timer  0xE2500000, size=0x100 
reg_t regv_pwmtimer[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// TCFG0
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// TCFG1
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// TCNTB0
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// TCMPB0
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// TCNTB1
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// TCMPB1
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// TCNTB2
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// TCNTB3
	{ .addr=0x0000003C, .val=0x00000000, .mask=0, .flags=0 },	// TCNTB4

	// for start timer
	{ .addr=0x00000008, .val=0x00000000, .mask=((1<<20)|(1<<16)|(1<<12)|(1<<8)|(1<<0)), .flags=0 },	                    // TCON  timer stop
	{ .addr=0x00000008, .val=0x00000000, .mask=((1<<21)|(1<<17)|(1<<13)|(1<<9)|(1<<1)), .flags=RF_READ_BACK_AS_WRITE },	// TCON  update=1
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// TCON  update=0, timer start

	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=0 },	// TINT_CSTAT
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// System Timer  0xE2600000, size=0x100 
reg_t regv_systimer[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// TCFG
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// TCON
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// TICNTB
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// TICNTO
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// TFCNTB
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// ICNTB
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// ICNTBO
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_RO_ARMMP },	// TINT_CSTAT
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// VIC0 0xF2000000, size=0x1000 
reg_t regv_vic0[] = {
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
//	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENCLEAR
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// VICxPROTECTION
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// VICxSWPRIORITYMASK
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// VICxPRIORITYDAISY
	
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000104, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000010C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000114, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000118, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000011C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000120, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000124, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000128, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000012C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000130, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000134, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000138, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000013C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000140, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000144, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000148, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000014C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000150, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000154, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000158, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000015C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000160, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000164, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000168, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000016C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000170, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000174, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000178, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000017C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0x00000200, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000204, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000208, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000020C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000210, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000214, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000218, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000021C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000220, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000224, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000228, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000022C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000230, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000234, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000238, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000023C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000240, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000244, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000248, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000024C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000250, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000254, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000258, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000025C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000260, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000264, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000268, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000026C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000270, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000274, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000278, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000027C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// VIC1 0xF2100000, size=0x1000 
reg_t regv_vic1[] = {
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
//	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENCLEAR
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// VICxPROTECTION
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// VICxSWPRIORITYMASK
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// VICxPRIORITYDAISY
	
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000104, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000010C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000114, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000118, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000011C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000120, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000124, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000128, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000012C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000130, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000134, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000138, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000013C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000140, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000144, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000148, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000014C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000150, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000154, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000158, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000015C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000160, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000164, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000168, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000016C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000170, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000174, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000178, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000017C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0x00000200, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000204, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000208, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000020C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000210, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000214, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000218, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000021C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000220, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000224, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000228, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000022C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000230, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000234, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000238, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000023C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000240, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000244, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000248, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000024C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000250, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000254, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000258, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000025C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000260, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000264, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000268, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000026C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000270, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000274, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000278, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000027C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// VIC2 0xF2200000, size=0x1000 
reg_t regv_vic2[] = {
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
//	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENCLEAR
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// VICxPROTECTION
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// VICxSWPRIORITYMASK
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// VICxPRIORITYDAISY
	
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000104, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000010C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000114, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000118, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000011C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000120, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000124, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000128, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000012C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000130, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000134, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000138, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000013C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000140, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000144, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000148, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000014C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000150, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000154, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000158, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000015C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000160, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000164, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000168, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000016C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000170, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000174, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000178, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000017C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0x00000200, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000204, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000208, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000020C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000210, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000214, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000218, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000021C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000220, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000224, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000228, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000022C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000230, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000234, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000238, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000023C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000240, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000244, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000248, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000024C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000250, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000254, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000258, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000025C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000260, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000264, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000268, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000026C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000270, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000274, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000278, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000027C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// VIC3 0xF2300000, size=0x1000 
reg_t regv_vic3[] = {
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
//	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENCLEAR
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// VICxPROTECTION
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// VICxSWPRIORITYMASK
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// VICxPRIORITYDAISY
	
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000104, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000010C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000114, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000118, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000011C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000120, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000124, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000128, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000012C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000130, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000134, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000138, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000013C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000140, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000144, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000148, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000014C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000150, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000154, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000158, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000015C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000160, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000164, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000168, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000016C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000170, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000174, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000178, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000017C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0x00000200, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000204, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000208, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000020C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000210, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000214, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000218, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000021C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000220, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000224, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000228, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000022C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000230, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000234, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000238, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000023C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000240, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000244, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000248, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000024C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000250, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000254, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000258, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000025C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000260, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000264, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000268, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000026C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000270, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000274, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000278, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000027C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// TZIC0 0xF2800000, size=0x1000 
reg_t regv_tzic0[] = {
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// TZIC0 0xF2900000, size=0x1000 
reg_t regv_tzic1[] = {
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// TZIC0 0xF2A00000, size=0x1000 
reg_t regv_tzic2[] = {
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// TZIC0 0xF2B00000, size=0x1000 
reg_t regv_tzic3[] = {
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTSELECT
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VICxINTENABLE
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINT
//	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// VICxSOFTINTCLEAR

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};                                                			

// GPIO  0xE0200000 ~ 0xE0200F80, size=0x1000
reg_t regv_gpio[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x0F80, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	

	[(0x0F80/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

// Clock Gate  0xE0100000, size=0x8000 
reg_t regv_sysclk[] = {
	{ .addr=0x00000200, .val=0x00000000, .mask=0, .flags=0 },	// CLK_SRC0
	{ .addr=0x00000204, .val=0x00000000, .mask=0, .flags=0 },   // CLK_SRC1
	{ .addr=0x00000208, .val=0x00000000, .mask=0, .flags=0 },   // CLK_SRC2
	{ .addr=0x0000020C, .val=0x00000000, .mask=0, .flags=0 },   // CLK_SRC3
	{ .addr=0x00000210, .val=0x00000000, .mask=0, .flags=0 },   // CLK_SRC4
	{ .addr=0x00000214, .val=0x00000000, .mask=0, .flags=0 },   // CLK_SRC5
	{ .addr=0x00000218, .val=0x00000000, .mask=0, .flags=0 },   // CLK_SRC6
	{ .addr=0x00000280, .val=0x00000000, .mask=0, .flags=0 },	// CLK_SRC_MASK0
	{ .addr=0x00000284, .val=0x00000000, .mask=0, .flags=0 },   // CLK_SRC_MASK1
	{ .addr=0x00000300, .val=0x00000000, .mask=0, .flags=0 },	// CLK_DIV0
	{ .addr=0x00000304, .val=0x00000000, .mask=0, .flags=0 },   // CLK_DIV1
	{ .addr=0x00000308, .val=0x00000000, .mask=0, .flags=0 },   // CLK_DIV2
	{ .addr=0x0000030C, .val=0x00000000, .mask=0, .flags=0 },   // CLK_DIV3
	{ .addr=0x00000310, .val=0x00000000, .mask=0, .flags=0 },   // CLK_DIV4
	{ .addr=0x00000314, .val=0x00000000, .mask=0, .flags=0 },   // CLK_DIV5
	{ .addr=0x00000318, .val=0x00000000, .mask=0, .flags=0 },   // CLK_DIV6
	{ .addr=0x0000031C, .val=0x00000000, .mask=0, .flags=0 },   // CLK_DIV7
	{ .addr=0x00000444, .val=0x00000000, .mask=0, .flags=0 },   // CLK_GATE_SCLK
	{ .addr=0x00000460, .val=0x00000000, .mask=0, .flags=0 },	// CLK_GATE_IP0
	{ .addr=0x00000464, .val=0x00000000, .mask=0, .flags=0 },   // CLK_GATE_IP1
	{ .addr=0x00000468, .val=0x00000000, .mask=0, .flags=0 },   // CLK_GATE_IP2
	{ .addr=0x0000046C, .val=0x00000000, .mask=0, .flags=0 },   // CLK_GATE_IP3
	{ .addr=0x00000470, .val=0x00000000, .mask=0, .flags=0 },   // CLK_GATE_IP4
	{ .addr=0x00000480, .val=0x00000000, .mask=0, .flags=0 },   // CLK_GATE_BLOCK		LCD(d3)
	{ .addr=0x00000484, .val=0x00000000, .mask=0, .flags=0 },   // CLK_GATE_IP5
	{ .addr=0x00000500, .val=0x00000000, .mask=0, .flags=0 },   // CLK_OUT
	{ .addr=0x00007008, .val=0x00000000, .mask=0, .flags=0 },   // DISPLAY_CONTROL
	{ .addr=0x0000700C, .val=0x00000000, .mask=0, .flags=0 },   // AUDIO_ENDIAN
	
	{ .addr=0x00008000, .val=0x00000000, .mask=0, .flags=0 },   // OSC_CON
	{ .addr=0x0000C000, .val=0x00000000, .mask=0, .flags=0 },   // PWR_CFG
	{ .addr=0x0000C004, .val=0x00000000, .mask=0, .flags=0 },   // EINT_WAKEUP_MASK
	{ .addr=0x0000C008, .val=0x00000000, .mask=0, .flags=0 },   // WAKEUP_MASK
	{ .addr=0x0000C00C, .val=0x00000000, .mask=0, .flags=0 },   // PWR_MODE
	{ .addr=0x0000C010, .val=0x00000000, .mask=0, .flags=0 },   // NORMAL_CFG
	{ .addr=0x0000C020, .val=0x00000000, .mask=0, .flags=0 },   // IDLE_CFG
	{ .addr=0x0000C030, .val=0x00000000, .mask=0, .flags=0 },   // STOP_CFG
	{ .addr=0x0000C034, .val=0x00000000, .mask=0, .flags=0 },   // STOP_MEM_CFG
	{ .addr=0x0000C040, .val=0x00000000, .mask=0, .flags=0 },   // SLEEP_CFG
	{ .addr=0x0000C100, .val=0x00000000, .mask=0, .flags=0 },   // OSC_FREQ
	{ .addr=0x0000C104, .val=0x00000000, .mask=0, .flags=0 },   // OSC_STABLE
	{ .addr=0x0000C108, .val=0x00000000, .mask=0, .flags=0 },   // PWR_STABLE
	{ .addr=0x0000C110, .val=0x00000000, .mask=0, .flags=0 },   // MTC_STABLE
	{ .addr=0x0000C114, .val=0x00000000, .mask=0, .flags=0 },   // CLAMP_STABLE
	{ .addr=0x0000C200, .val=0x00000000, .mask=0, .flags=0 },   // WAKEUP_STAT

	{ .addr=0x0000E000, .val=0x00000000, .mask=0, .flags=0 },   // OTHERS
	{ .addr=0x0000E804, .val=0x00000000, .mask=0, .flags=0 },   // HDMI_CONTROL
	{ .addr=0x0000E80C, .val=0x00000000, .mask=0, .flags=0 },   // USB_PHY_CONTROL
	{ .addr=0x0000E810, .val=0x00000000, .mask=0, .flags=0 },   // DAC_CONTROL
	{ .addr=0x0000E814, .val=0x00000000, .mask=0, .flags=0 },   // MIPI_DPHY_CONTROL
	{ .addr=0x0000E818, .val=0x00000000, .mask=0, .flags=0 },   // ADC_CONTROL
	{ .addr=0x0000E81C, .val=0x00000000, .mask=0, .flags=0 },   // PS_HOLD_CONTROL
	
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SROM  0xE8000000, size=0x100 
reg_t regv_srom[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// SROM_BW
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// SROM_BC0
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// SROM_BC1
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// SROM_BC2
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// SROM_BC3
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// SROM_BC4
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// SROM_BC5
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }	
};

// TSADC  0xE1700000, size=0x2000 
reg_t regv_tsadc[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// TSADCC0N0
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// TSADC0
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// TSDLY0
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },	// TSPENSTAT0
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },	// CLRINTADC0
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },	// CLRINTPEN0
	{ .addr=0x00001000, .val=0x00000000, .mask=0, .flags=0 },	// TSADCC0N0
	{ .addr=0x00001004, .val=0x00000000, .mask=0, .flags=0 },	// TSADC0
	{ .addr=0x00001008, .val=0x00000000, .mask=0, .flags=0 },	// TSDLY0
	{ .addr=0x00001014, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },	// TSPENSTAT0
	{ .addr=0x00001018, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },	// CLRINTADC0
	{ .addr=0x00001020, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },	// CLRINTPEN0
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }	
};

// OneNAND  0xB0600000, size=0x2000 
reg_t regv_onenand[] = {
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// ONENAND_IF_CTRL
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },   // ONENAND_IF_ASYNC_TIMING_CTRL
	{ .addr=0x00001024, .val=0x00000000, .mask=0, .flags=0 },   // INTC_DMA_MASK
	{ .addr=0x00001028, .val=0x00000000, .mask=0, .flags=0 },   // INTC_ONENAND_MASK
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// AC97  0xE2200000, size=0x100 
reg_t regv_ac97[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0x7f000000, .flags=RF_OR_MASK_AS_WRITE },	// AC_GLBCTRL
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};
// todo DMA

// I2S  0xEEE30000, size=0x100 
reg_t regv_i2s[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=(7<<0), .flags=0 },	// IISCON	I2SACTIVE=0 TXDMA=0 RXDMA=0
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// IISMOD
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// IISFIC
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// IISPSR
	//{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// IISTXD
	//{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// IISRXD
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// IISFICS 
	//{ .addr=0x0000001C, .val=0x00000000, .mask=0x7f000000, .flags=0 },	// IISTXDS
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// IISAHB
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// IISSRT0
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// IISSIZE
	//{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },	// IISTRNCNT
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// IISLVL0ADDR
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },	// IISLVL1ADDR 
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=0 },	// IISLVL2ADDR
	{ .addr=0x0000003C, .val=0x00000000, .mask=0, .flags=0 },	// IISLVL3ADDR
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0 },	// IISSTR1

	{ .addr=0x00000000, .val=0x00000000, .mask=(6<<0), .flags=0 },	// IISCON  I2SACTIVE=? TXDMA=0 RXDMA=0
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// I2C  0xE1800000, 0xFAB00000, 0xE1A00000,  size=0x100 
reg_t regv_i2c0[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// I2CCON
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CSTAT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CLC
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=RF_BIT08 }
};
reg_t regv_i2c1[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// I2CCON
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CSTAT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CLC
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=RF_BIT08 }
};
reg_t regv_i2c2[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// I2CCON
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CSTAT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CLC
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=RF_BIT08 }
};

// UART  0xE2900000,  size=0x1000 
reg_t regv_uart[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// ULCON0
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },   // UCON0
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },   // UFCON0
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },   // UMCON0
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },   // UBRDIV0
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },   // UDIVSLOT0
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// UINTP0
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },   // UINTSP0
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=0 },   // UINTM0

	{ .addr=0x00000400, .val=0x00000000, .mask=0, .flags=0 },	// ULCON1
	{ .addr=0x00000404, .val=0x00000000, .mask=0, .flags=0 },   // UCON1
	{ .addr=0x00000408, .val=0x00000000, .mask=0, .flags=0 },   // UFCON1
	{ .addr=0x0000040C, .val=0x00000000, .mask=0, .flags=0 },   // UMCON1
	{ .addr=0x00000428, .val=0x00000000, .mask=0, .flags=0 },   // UBRDIV1
	{ .addr=0x0000042C, .val=0x00000000, .mask=0, .flags=0 },   // UDIVSLOT1
	{ .addr=0x00000430, .val=0x00000000, .mask=0, .flags=0 },	// UINTP1
	{ .addr=0x00000434, .val=0x00000000, .mask=0, .flags=0 },   // UINTSP1
	{ .addr=0x00000438, .val=0x00000000, .mask=0, .flags=0 },   // UINTM1

	{ .addr=0x00000800, .val=0x00000000, .mask=0, .flags=0 },	// ULCON2
	{ .addr=0x00000804, .val=0x00000000, .mask=0, .flags=0 },   // UCON2
	{ .addr=0x00000808, .val=0x00000000, .mask=0, .flags=0 },   // UFCON2
	{ .addr=0x0000080C, .val=0x00000000, .mask=0, .flags=0 },   // UMCON2
	{ .addr=0x00000828, .val=0x00000000, .mask=0, .flags=0 },   // UBRDIV2
	{ .addr=0x0000082C, .val=0x00000000, .mask=0, .flags=0 },   // UDIVSLOT2
	{ .addr=0x00000830, .val=0x00000000, .mask=0, .flags=0 },	// UINTP2
	{ .addr=0x00000834, .val=0x00000000, .mask=0, .flags=0 },   // UINTSP2
	{ .addr=0x00000838, .val=0x00000000, .mask=0, .flags=0 },   // UINTM2

	{ .addr=0x00000C00, .val=0x00000000, .mask=0, .flags=0 },	// ULCON3
	{ .addr=0x00000C04, .val=0x00000000, .mask=0, .flags=0 },   // UCON3
	{ .addr=0x00000C08, .val=0x00000000, .mask=0, .flags=0 },   // UFCON3
	{ .addr=0x00000C0C, .val=0x00000000, .mask=0, .flags=0 },   // UMCON3
	{ .addr=0x00000C28, .val=0x00000000, .mask=0, .flags=0 },   // UBRDIV3
	{ .addr=0x00000C2C, .val=0x00000000, .mask=0, .flags=0 },   // UDIVSLOT3
	{ .addr=0x00000C30, .val=0x00000000, .mask=0, .flags=0 },	// UINTP3
	{ .addr=0x00000C34, .val=0x00000000, .mask=0, .flags=0 },   // UINTSP3
	{ .addr=0x00000C38, .val=0x00000000, .mask=0, .flags=0 },   // UINTM3
                                                                   
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// MMC0  0xEB000000,  size=0x100
reg_t regv_mmc0[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0            },			// SDMASYSAD
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKSIZE	16bit
	{ .addr=0x00000006, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKCNT	16bit
	//{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0            },			// ARGUMENT
	{ .addr=0x0000000c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// TRNMOD	16bit
	//{ .addr=0x0000000e, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// CMDREG	16bit
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#0
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#1
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#2
	{ .addr=0x0000001c, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#3
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },		// BDATA
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// PRNSTS
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// HOSTCTL
	{ .addr=0x00000029, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// PWRCON
	{ .addr=0x0000002a, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// BLKGAP
	{ .addr=0x0000002b, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// WAKCON
	{ .addr=0x0000002c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// CLKCON
	{ .addr=0x0000002e, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// TIMEOUTCON
	{ .addr=0x0000002f, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// SWRST
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// NORINTSTS
	{ .addr=0x00000032, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ERRINTSTS
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSTSEN
	{ .addr=0x00000036, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSTSEN
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSIGEN
	{ .addr=0x0000003a, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSIGEN
	{ .addr=0x0000003c, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ACMD12ERRSTS
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0                 },		// CAPAREG
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY      },		// MAXCURR
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL2
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL3
	{ .addr=0x0000008c, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL4
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=RF_BIT16          },		// FEAER
	{ .addr=0x00000052, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_NOSAVE_AS_READ },	// FEERR
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0                 },		// ADMAERR
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0                 },		// ADMASYSADDR
	{ .addr=0x000000fe, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// HCVER

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 },
};

// MMC1  0xEB100000,  size=0x100
reg_t regv_mmc1[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0            },			// SDMASYSAD
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKSIZE	16bit
	{ .addr=0x00000006, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKCNT	16bit
	//{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0            },			// ARGUMENT
	{ .addr=0x0000000c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// TRNMOD	16bit
	//{ .addr=0x0000000e, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// CMDREG	16bit
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#0
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#1
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#2
	{ .addr=0x0000001c, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#3
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },		// BDATA
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// PRNSTS
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// HOSTCTL
	{ .addr=0x00000029, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// PWRCON
	{ .addr=0x0000002a, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// BLKGAP
	{ .addr=0x0000002b, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// WAKCON
	{ .addr=0x0000002c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// CLKCON
	{ .addr=0x0000002e, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// TIMEOUTCON
	{ .addr=0x0000002f, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// SWRST
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// NORINTSTS
	{ .addr=0x00000032, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ERRINTSTS
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSTSEN
	{ .addr=0x00000036, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSTSEN
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSIGEN
	{ .addr=0x0000003a, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSIGEN
	{ .addr=0x0000003c, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ACMD12ERRSTS
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0                 },		// CAPAREG
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY      },		// MAXCURR
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL2
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL3
	{ .addr=0x0000008c, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL4
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=RF_BIT16          },		// FEAER
	{ .addr=0x00000052, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_NOSAVE_AS_READ },	// FEERR
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0                 },		// ADMAERR
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0                 },		// ADMASYSADDR
	{ .addr=0x000000fe, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// HCVER

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 },
};

// MMC2  0xEB200000,  size=0x100
reg_t regv_mmc2[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0            },			// SDMASYSAD
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKSIZE	16bit
	{ .addr=0x00000006, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKCNT	16bit
	//{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0            },			// ARGUMENT
	{ .addr=0x0000000c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// TRNMOD	16bit
	//{ .addr=0x0000000e, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// CMDREG	16bit
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#0
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#1
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#2
	{ .addr=0x0000001c, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#3
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },		// BDATA
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// PRNSTS
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// HOSTCTL
	{ .addr=0x00000029, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// PWRCON
	{ .addr=0x0000002a, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// BLKGAP
	{ .addr=0x0000002b, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// WAKCON
	{ .addr=0x0000002c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// CLKCON
	{ .addr=0x0000002e, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// TIMEOUTCON
	{ .addr=0x0000002f, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// SWRST
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// NORINTSTS
	{ .addr=0x00000032, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ERRINTSTS
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSTSEN
	{ .addr=0x00000036, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSTSEN
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSIGEN
	{ .addr=0x0000003a, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSIGEN
	{ .addr=0x0000003c, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ACMD12ERRSTS
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0                 },		// CAPAREG
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY      },		// MAXCURR
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL2
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL3
	{ .addr=0x0000008c, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL4
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=RF_BIT16          },		// FEAER
	{ .addr=0x00000052, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_NOSAVE_AS_READ },	// FEERR
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0                 },		// ADMAERR
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0                 },		// ADMASYSADDR
	{ .addr=0x000000fe, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// HCVER

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 },
};

// MMC3  0xEB300000,  size=0x100
reg_t regv_mmc3[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0            },			// SDMASYSAD
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKSIZE	16bit
	{ .addr=0x00000006, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// BLKCNT	16bit
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0            },			// ARGUMENT
	{ .addr=0x0000000c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// TRNMOD	16bit
	//{ .addr=0x0000000e, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// CMDREG	16bit
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#0
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#1
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#2
	{ .addr=0x0000001c, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// RSPREG#3
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_NOSAVE_AS_READ },		// BDATA
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY },			// PRNSTS
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// HOSTCTL
	{ .addr=0x00000029, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// PWRCON
	{ .addr=0x0000002a, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// BLKGAP
	{ .addr=0x0000002b, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// WAKCON
	{ .addr=0x0000002c, .val=0x00000000, .mask=0, .flags=RF_BIT16     },			// CLKCON
	{ .addr=0x0000002e, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// TIMEOUTCON
	{ .addr=0x0000002f, .val=0x00000000, .mask=0, .flags=RF_BIT08     },			// SWRST
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// NORINTSTS
	{ .addr=0x00000032, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ERRINTSTS
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSTSEN
	{ .addr=0x00000036, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSTSEN
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// NORINTSIGEN
	{ .addr=0x0000003a, .val=0x00000000, .mask=0, .flags=RF_BIT16              },	// ERRINTSIGEN
	{ .addr=0x0000003c, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// ACMD12ERRSTS
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0                 },		// CAPAREG
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=RF_READ_ONLY      },		// MAXCURR
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL2
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL3
	{ .addr=0x0000008c, .val=0x00000000, .mask=0, .flags=0                 },		// CONTROL4
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=RF_BIT16          },		// FEAER
	{ .addr=0x00000052, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_NOSAVE_AS_READ },	// FEERR
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0                 },		// ADMAERR
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0                 },		// ADMASYSADDR
	{ .addr=0x000000fe, .val=0x00000000, .mask=0, .flags=RF_BIT16|RF_READ_ONLY },	// HCVER

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 },
};

// LCD  0xF8000000,  size=0x8000
reg_t regv_lcd[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=(1<<1)|(1<<0), .flags=0 },			// VIDCON0	  VIDCON0_ENVID_DISABLE, VIDCON0_ENVID_F_DISABLE
	{ .addr=0x00000004, .val=0x00000000, .mask=(1<<1)|(1<<0), .flags=0 },			// VIDCON1
	{ .addr=0x00000008, .val=0x00000000, .mask=(1<<1)|(1<<0), .flags=0 },			// VIDCON2
	{ .addr=0x0000000C, .val=0x00000000, .mask=(1<<1)|(1<<0), .flags=0 },			// VIDCON3
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },						// VIDTCON0
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },						// VIDTCON1
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },						// VIDTCON2
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },						// VIDTCON3
	{ .addr=0x00000020, .val=0x00000000, .mask=(1<<22)|(1<<20)|(1<<0), .flags=0 },	// WINCON0    WINCONx_ENWIN_F_ENABLE, WINCONx_ENLOCAL_DMA
	{ .addr=0x00000024, .val=0x00000000, .mask=(1<<22)|(1<<20)|(1<<0), .flags=0 },	// WINCON1
	{ .addr=0x00000028, .val=0x00000000, .mask=(1<<22)|(1<<20)|(1<<0), .flags=0 },	// WINCON2
	{ .addr=0x0000002c, .val=0x00000000, .mask=(1<<22)|(1<<20)|(1<<0), .flags=0 },	// WINCON3
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },						// WINCON4
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },						// SHADOWCON
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0 },						// VIDOSD0A
	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=0 },						// VIDOSD0B
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=0 },						// VIDOSD0C
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x0000005c, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000060, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000064, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000068, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x0000006c, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000070, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000074, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000078, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000088, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000a0, .val=0x00000000, .mask=0, .flags=0 },	// VIDW00ADD0B0    Window 0’s buffer start address register, buffer 0
	{ .addr=0x000000a4, .val=0x00000000, .mask=0, .flags=0 },	// VIDW00ADD0B1    Window 0’s buffer start address register, buffer 1
	{ .addr=0x000020a0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000a8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000ac, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020a8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000b0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000b4, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020b0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000b8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000bc, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020b8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000c0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000c4, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020c0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000d0, .val=0x00000000, .mask=0, .flags=0 },	// VIDW00ADD1B0		Window 0’s buffer end address register, buffer 0
	{ .addr=0x000000d4, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020d0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000d8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000dc, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020d8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000e0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000e4, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020e0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000e8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000ec, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020e8, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000f0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000000f4, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x000020f0, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// VIDW00ADD2     virts size : Window 0’s buffer size register
	{ .addr=0x00000104, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x0000010c, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000130, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000134, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000140, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000144, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000148, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x0000014c, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000150, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000154, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000158, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x0000015c, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000160, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000164, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000168, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x0000016c, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000170, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000180, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000184, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000188, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x0000018c, .val=0x00000000, .mask=0, .flags=0 },
	{ .addr=0x00000190, .val=0x00000000, .mask=0, .flags=0 },
	
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// WINCON0    WINCONx_ENWIN_F_ENABLE, WINCONx_ENLOCAL_DMA
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// WINCON1
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// WINCON2
	{ .addr=0x0000002c, .val=0x00000000, .mask=0, .flags=0 },	// WINCON3

	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// VIDCON0	  VIDCON0_ENVID_DISABLE, VIDCON0_ENVID_F_DISABLE
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// VIDCON1
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// VIDCON2
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// VIDCON3     
	
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};



// Video  0xF9100000,  size=0x400		
reg_t regv_video[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=(1<<0), .flags=0 },	// VP_ENABLE
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// VP_SRESET
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000003CC, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000004C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000005C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000060, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000064, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000068, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000006C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000070, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000074, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000078, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000007C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000088, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000008C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000090, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000094, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000098, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000009C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000000A0, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000000A4, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x000000A8, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x000000EC, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000000F0, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000000F4, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x000000F8, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x000000FC, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000104, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000010C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000114, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000118, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000011C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000120, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000124, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000128, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000012C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000100, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000104, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000108, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000010C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000114, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000118, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000011C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000120, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000124, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000128, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000012C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000130, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000134, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000138, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x0000013C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000140, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000144, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x00000148, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x000001D4, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x000001D8, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x000001DC, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000001E0, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000001E4, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x000001F0, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x000001EC, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x000001E8, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x000001F4, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000200, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000020C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000210, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000218, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000021C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000220, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000224, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000228, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000022C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000230, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000238, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000023C, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// VP_ENABLE
	//{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// VP_SRESET
	
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// Mixer  0xF9200000,  size=0x100		
reg_t regv_mixer[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=(1<<1)|(1<<0), .flags=0 },	// MIXER_STATUS
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// MIXER_CFG
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x0000004C, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000064, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000068, .val=0x00000000, .mask=0, .flags=0 },	// 	
	{ .addr=0x0000006C, .val=0x00000000, .mask=0, .flags=0 },	//		
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000088, .val=0x00000000, .mask=0, .flags=0 },	// 

	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// MIXER_STATUS
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI_ctrl  0xFA100000,  size=0x100
reg_t regv_hdmi_ctrl[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI_core  0xFA110000,  size=0x800
reg_t regv_hdmi_core[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=(1<<0), .flags=RF_BIT08 },	// HDMI_CON_0
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 | RF_READ_BACK_AS_WRITE }, // STATUS					
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000060, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000064, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000068, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000006C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//					
	{ .addr=0x000000A0, .val=0x00000000, .mask=0, .flags=RF_BIT16 },	// 
	{ .addr=0x000000A4, .val=0x00000000, .mask=0, .flags=RF_BIT16 },	// 
	{ .addr=0x000000B0, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000000B4, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000000B8, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000000C0, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000000C4, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000000C8, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000000E4, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x000000E8, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000110, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000114, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000118, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000120, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000124, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000128, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000130, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000134, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000138, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000140, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000144, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000148, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000150, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000154, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000158, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000160, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000164, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000170, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000174, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000178, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x0000017C, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000180, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000184, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000188, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x0000018C, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000190, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000194, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x00000198, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000001A0, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000001A4, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000001A8, .val=0x00000000, .mask=0, .flags=0        },	// 
	{ .addr=0x000001B0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x000001B4, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x000001B8, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x000001BC, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//					
	{ .addr=0x000001C0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// GCP
	{ .addr=0x000001D0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// GCP
	{ .addr=0x000001D4, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// GCP
	{ .addr=0x000001D8, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// GCP
	{ .addr=0x000001E0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// ACP
	{ .addr=0x000001F0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// ACP

	{ .addr=0x00000300, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000310, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000320, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000324, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000328, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x0000032C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000330, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000334, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000338, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x0000033C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000340, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000344, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000348, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x0000034C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  
	{ .addr=0x00000350, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AVI  

	{ .addr=0x00000360, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// AUI     

	
	{ .addr=0x000003A0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// MPG  
	{ .addr=0x000003B0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// MPG  
	{ .addr=0x000003C0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// MPG  
	{ .addr=0x000003C4, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// MPG  
	{ .addr=0x000003C8, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// MPG  
	{ .addr=0x000003CC, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// MPG  
	{ .addr=0x000003D0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// MPG  
	
	{ .addr=0x00000400, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000414, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000418, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000420, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000424, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000428, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x0000042C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000430, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000434, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000438, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x0000043C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000440, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000444, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000448, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x0000044C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000450, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000454, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000458, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x0000045C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000460, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000464, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000468, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x0000046C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000470, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000474, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000478, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x0000047C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000480, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000484, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x00000488, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  
	{ .addr=0x0000048C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// SPD  

	{ .addr=0x000005C4, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x000005C8, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	
	//{ .addr=0x00000500, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// GMUT 
	//{ .addr=0x00000600, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// HDCP 

	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// HDMI_CON_0
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI-spdif  0xFA130000,  size=0x100
reg_t regv_hdmi_spdif[] = {
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//				
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//				
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI_i2s  0xFA140000,  size=0x100
reg_t regv_hdmi_i2s[] = {
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//				
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//				
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000005C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//				
	{ .addr=0x00000060, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	//{ .addr=0x00000064, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	//  ~
	//{ .addr=0x000000D8, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x000000DC, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//				

	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// I2S_CLK_CON
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI_tg  0xFA150000,  size=0x200
reg_t regv_hdmi_tg[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=(1<<0), .flags=RF_BIT08 },	//  TG_CMD
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//					
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000003C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000004C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000005C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000060, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000064, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000078, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000007C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000088, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000008C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000090, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000094, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000017C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000180, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000184, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 

	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//  TG_CMD
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI_efuse  0xFA160000,  size=0x100
reg_t regv_hdmi_efuse[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	//
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI_cec  0xE1B00000,  size=0x200
reg_t regv_hdmi_cec[] = {
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000180, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000184, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// 
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// CEC_TX_CTRL
	{ .addr=0x000000C0, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// CEC_RX_CTRL

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// HDMI_i2c  0xFA900000,  size=0x100
reg_t regv_hdmi_i2c[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=RF_BIT08 },	// I2CCON
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CSTAT
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=RF_BIT08 },   // I2CLC
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};




/*

// SGX_GSR 0x0xF3000000,  size=0x100
reg_t regv_sgx_GSR[] = {
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// FGGB_CACHECTL
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// FGGB_RST
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0 },	// FGGB_INTPENDING
	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=0 },	// FGGB_INTMASK
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=0 },	// FGGB_PIPEMASK
	{ .addr=0x0000004C, .val=0x00000000, .mask=0, .flags=0 },	// FGGB_PIPETGTSTATE
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SGX_GSR 0x0xF3008000,  size=0x100
reg_t regv_sgx_HISR[] = {
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_CONTROL
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_IDXOFFSET
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_VBADDR
	{ .addr=0x00000040, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB0
	{ .addr=0x00000044, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB1
	{ .addr=0x00000048, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB2
	{ .addr=0x0000004C, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB3
	{ .addr=0x00000050, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB4
	{ .addr=0x00000054, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB5
	{ .addr=0x00000058, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB6
	{ .addr=0x0000005C, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB7
	{ .addr=0x00000060, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB8
	{ .addr=0x00000064, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB9
	{ .addr=0x00000080, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB0_VBCTRL
	{ .addr=0x00000084, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB1_VBCTRL
	{ .addr=0x00000088, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB2_VBCTRL
	{ .addr=0x0000008C, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB3_VBCTRL
	{ .addr=0x00000090, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB4_VBCTRL
	{ .addr=0x00000094, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB5_VBCTRL
	{ .addr=0x00000098, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB6_VBCTRL
	{ .addr=0x0000009C, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB7_VBCTRL
	{ .addr=0x000000A0, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB8_VBCTRL
	{ .addr=0x000000A4, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB9_VBCTRL
	{ .addr=0x000000C0, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB0_VBBASE
	{ .addr=0x000000C4, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB1_VBBASE
	{ .addr=0x000000C8, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB2_VBBASE
	{ .addr=0x000000CC, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB3_VBBASE
	{ .addr=0x000000D0, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB4_VBBASE
	{ .addr=0x000000D4, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB5_VBBASE
	{ .addr=0x000000D8, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB6_VBBASE
	{ .addr=0x000000DC, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB7_VBBASE
	{ .addr=0x000000E0, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB8_VBBASE
	{ .addr=0x000000E4, .val=0x00000000, .mask=0, .flags=0 },	// FGHI_ATTRIB9_VBBASE

	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SGX_HISR FGHI_VBDATA    0xF300E000,  size=0x1000 
reg_t regv_sgx_FGHI_VBDATA[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x0FFC, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	
	[(0x0FFC/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

// SGX_HISR FGVS_INSTMEM   0xF3010000,  size=0x2000    
reg_t regv_sgx_FGVS_INSTMEM[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x1FFC, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	
	[(0x1FFC/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

// SGX_HISR FGVS_CFLOAT  0xF3014000,  size=0x1000    
reg_t regv_sgx_FGVS_CFLOAT[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x0FFC, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	
	[(0x0FFC/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

// SGX_HISR FGVS_COTHERS  0xF3018000,  size=0xC000    
reg_t regv_sgx_FGVS_COTHERS[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// FGVS_CINT
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000003C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000400, .val=0x00000000, .mask=0, .flags=0 },	// FGVS_CBOOL
	
	{ .addr=0x00008000, .val=0x00000000, .mask=0, .flags=0 },	// FGVS_PCRange
	{ .addr=0x00008004, .val=0x00000000, .mask=0, .flags=0 },	// FGVS_AttributeNum
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SGX_HISR FGPE   0xF3030000,  size=0x100    
reg_t regv_sgx_FGPE[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// FGPE_VERTEX_CONTEXT
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// FGPE_VIEWPORT_OX
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// FGPE_VIEWPORT_OY
	{ .addr=0x0000000A, .val=0x00000000, .mask=0, .flags=0 },	// FGPE_VIEWPORT_HALF_PX
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// FGPE_VIEWPORT_HALF_PY
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// FGPE_DEPTHRANGE_HALF_F_SUB_N
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// FGPE_DEPTHRANGE_HALF_F_ADD_N
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SGX_HISR FGRA   0xF3038000,  size=0x800    
reg_t regv_sgx_FGRA[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_PixSamp
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_DOffEn
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_DOffFactor
	{ .addr=0x0000000A, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_DOffUnits
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_DOffRIn
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_BFCULL
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_YCLIP
	{ .addr=0x00000400, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_LODCTL
	{ .addr=0x00000404, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_CLIPX
	{ .addr=0x0000041C, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_PWIDTH
	{ .addr=0x00000420, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_PSIZE_MIN
	{ .addr=0x00000424, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_PSIZE_MAX
	{ .addr=0x00000428, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_COORDREPLACE
	{ .addr=0x0000042C, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_LWIDTH
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SGX_HISR FGPS   0xF3040000,  size=0x2000   
reg_t regv_sgx_FGPS_INSTMEM[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x1FFC, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	
	[(0x1FFC/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

// SGX_HISR FGPS_CFLOAT  0xF3044000,  size=0x1000    
reg_t regv_sgx_FGPS_CFLOAT[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x0FFC, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	
	[(0x0FFC/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

// SGX_HISR FGPS_COTHERS  0xF3048000,  size=0x1000    
reg_t regv_sgx_FGRA_COTHERS[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// FGRA_CINT
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x0000003C, .val=0x00000000, .mask=0, .flags=0 },	// 
	{ .addr=0x00000400, .val=0x00000000, .mask=0, .flags=0 },	// FGVS_CBOOL
	{ .addr=0x00000800, .val=0x00000000, .mask=0, .flags=0 },	// FGPS_ExeMode
	{ .addr=0x00000804, .val=0x00000000, .mask=0, .flags=0 },	// FGPS_PCStart
	{ .addr=0x00000808, .val=0x00000000, .mask=0, .flags=0 },	// FGPS_PCEnd
	{ .addr=0x0000080C, .val=0x00000000, .mask=0, .flags=0 },	// FGPS_PCCopy
	{ .addr=0x00000810, .val=0x00000000, .mask=0, .flags=0 },	// FGPS_AttributeNum
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SGX_HISR FGTU   0xF3060000,  size=0x400 
reg_t regv_sgx_FGTU[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x02DC, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	
	[(0x02DC/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

// SGX_HISR FGPF  0xF3070000,  size=0x100    
reg_t regv_sgx_FGPF[] = {
	{ .addr=0x00000000, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_SCISSOR_X
	{ .addr=0x00000004, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_SCISSOR_Y
	{ .addr=0x00000008, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_ALPHAT
	{ .addr=0x0000000C, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_FRONTST
	{ .addr=0x00000010, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_BACKST
	{ .addr=0x00000014, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_DEPTHT
	{ .addr=0x00000018, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_CCLR
	{ .addr=0x0000001C, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_BLEND
	{ .addr=0x00000020, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_LOGOP
	{ .addr=0x00000024, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_CBMSK
	{ .addr=0x00000028, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_DBMSK
	{ .addr=0x0000002C, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_FBCTL
	{ .addr=0x00000030, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_DBADDR
	{ .addr=0x00000034, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_CBADDR
	{ .addr=0x00000038, .val=0x00000000, .mask=0, .flags=0 },	// FGPF_FBW
	{ .addr=0xFFFFFFFF, .val=0xFFFFFFFF, .mask=0, .flags=0 }
};

// SGX_HISR FGHI_DWENTRY   0xF300C000,  size=0x2000
reg_t regv_sgx_FGHI_DWENTRY[] = {
	[0] = { .addr=REG_ADDR_SEQ_MARK     , .val=0x0000    , .mask=0x1FFC, .flags=0 },
	[1] = { .addr=REG_ADDR_SEQ_COPY_DATA, .val=0         , .mask=0     , .flags=0 },	
	[(0x1FFC/4)+3] = { .addr=0xFFFFFFFF , .val=0xFFFFFFFF, .mask=0     , .flags=0 }
};

*/
#endif
