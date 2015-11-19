/*-----------------------------------------------------------------------------
  파 일 : zb_mmu_s5pv210.c
  설 명 : 
  작 성 : frog@falinux.com
  날 짜 : 2011-07-29
  주 의 :

-------------------------------------------------------------------------------*/
#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef MODULE
#define MODULE
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
#include <linux/poll.h>     // poll
#include <linux/proc_fs.h>
#include <linux/irq.h>
#include <linux/gfp.h>

#include <asm/system.h>     
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/ioctl.h>
#include <asm/unistd.h>

#include <linux/cdev.h>          
#include <linux/platform_device.h>
#include <linux/err.h>

#include <asm/gpio.h>
#include <linux/irqflags.h>

#include <linux/mm.h>
#include <linux/swap.h>
#include <linux/bootmem.h>

#include <asm/setup.h>

#include <linux/mmzone.h> // search_all_pg_data()

#include <linux/delay.h>
#include <asm/unistd.h>
#include <asm/io.h>

#include <asm/pgalloc.h>
#include <asm/page.h>
#include <asm/tlbflush.h>         
#include <linux/gfp.h>

#include "zbi.h"
#include "zb_debug.h"

#include <zb_mmu.h>
#include <zb_core_reg.h>
#include <zb_nalcode.h>

/// @{
/// @brief  변수정의
///-----------------------------------------------------------------------------
u32 *zb_coarse_tables = NULL;

/// @}

/*------------------------------------------------------------------------------
  @brief   두번째 TTB를 설정한다.
  @remark  
*///----------------------------------------------------------------------------
u32 mcu_get_cpsr( void );
asm("   	            					\n\
											\n\
.align  5									\n\
.text                                       \n\
.global mcu_get_cpsr                        \n\
mcu_get_cpsr:                               \n\
				mrs r0,cpsr                 \n\
				mov	pc,lr                   \n\
");


/*------------------------------------------------------------------------------
  @brief   모든 TLB 캐쉬를 비운다. 
  @remark  
*///----------------------------------------------------------------------------
void v7_flush_dcache_all( void );
asm("   	            					                            			\n\
.align  5									                            			\n\
.text                                                                   			\n\
.global v7_flush_dcache_all			                                                \n\
v7_flush_dcache_all:                                                       			\n\
				dmb                 @ ensure ordering with previous memory accesses \n\
				mrc p15, 1, r0, c0, c0, 1       @ read clidr						\n\
				ands    r3, r0, #0x7000000      @ extract loc from clidr			\n\
				mov r3, r3, lsr #23         @ left align loc bit field				\n\
				beq finished            @ if loc is 0, then no need to clean		\n\
				mov r10, #0             @ start clean at cache level 0				\n\
loop1:																				\n\
				add r2, r10, r10, lsr #1        @ work out 3x current cache level	\n\
				mov r1, r0, lsr r2          @ extract cache type bits from clidr	\n\
				and r1, r1, #7          @ mask of the bits for current cache only	\n\
				cmp r1, #2              @ see what cache we have at this level		\n\
				blt skip                @ skip if no cache, or just i-cache			\n\
				mcr p15, 2, r10, c0, c0, 0      @ select current cache level in cssr	\n\
				isb                 @ isb to sych the new cssr&csidr				\n\
				mrc p15, 1, r1, c0, c0, 0       @ read the new csidr				\n\
				and r2, r1, #7          @ extract the length of the cache lines		\n\
				add r2, r2, #4          @ add 4 (line length offset)				\n\
				ldr r4, =0x3ff														\n\
				ands    r4, r4, r1, lsr #3      @ find maximum number on the way size	\n\
				clz r5, r4              @ find bit position of way size increment	\n\
				ldr r7, =0x7fff														\n\
				ands    r7, r7, r1, lsr #13     @ extract max number of the index size	\n\
loop2:																				\n\
				mov r9, r4              @ create working copy of max way size		\n\
loop3:																				\n\
				orr r11, r10, r9, lsl r5        @ factor way and cache number into r11	\n\
				orr r11, r11, r7, lsl r2        @ factor index number into r11		\n\
				mcr p15, 0, r11, c7, c14, 2     @ clean & invalidate by set/way		\n\
				subs    r9, r9, #1          @ decrement the way						\n\
				bge loop3                                                           \n\
				subs    r7, r7, #1          @ decrement the index                   \n\
				bge loop2                                                           \n\
skip:                                                                               \n\
				add r10, r10, #2            @ increment cache number                \n\
				cmp r3, r10                                                         \n\
				bgt loop1                                                           \n\
finished:                                                                           \n\
				mov r10, #0             @ swith back to cache level 0				\n\
				mcr p15, 2, r10, c0, c0, 0      @ select current cache level in cssr\n\
				dsb																	\n\
				isb																	\n\
				mov pc, lr															\n\
");
 

void all_tbl_clean( void );
asm("   	            					                            			\n\
											                            			\n\
.align  5									                            			\n\
.text                                                                   			\n\
.global all_tbl_clean                                                   			\n\
all_tbl_clean:                                                          			\n\
				stmfd   sp!, {r4-r5, r7, r9-r11, lr}                                \n\
				bl  v7_flush_dcache_all                                             \n\
				mov r0, #0                                                          \n\
				mcr p15, 0, r0, c7, c5, 0       @ I+BTB cache invalidate            \n\
				ldmfd   sp!, {r4-r5, r7, r9-r11, lr}                                \n\
				mov pc, lr 															\n\
");

/*------------------------------------------------------------------------------
  @brief   메인 TTB를 구한다. 
  @remark  
*///----------------------------------------------------------------------------
u32 mmu_get_ttb_base( void );
asm("   	            					\n\
											\n\
.align  5									\n\
.text                                       \n\
.global mmu_get_ttb_base                    \n\
mmu_get_ttb_base:                           \n\
				mrc	p15, 0, r0, c2, c0, 0   \n\
				mov	pc,lr                   \n\
");

/*------------------------------------------------------------------------------
  @brief   두번째 TTB를 구한다. 
  @remark  
*///----------------------------------------------------------------------------
u32 mmu_get_ttb2_base( void );
asm("   	            					\n\
											\n\
.align  5									\n\
.text                                       \n\
.global mmu_get_ttb2_base                   \n\
mmu_get_ttb2_base:                          \n\
				mrc	p15, 0, r0, c2, c0, 1   \n\
				mov	pc,lr                   \n\
");

/*------------------------------------------------------------------------------
  @brief   TTBC를 구한다. 
  @remark  
*///----------------------------------------------------------------------------
u32 mmu_get_ttbc( void );
asm("   	            					\n\
											\n\
.align  5									\n\
.text                                       \n\
.global mmu_get_ttbc                        \n\
mmu_get_ttbc:                               \n\
				mrc	p15, 0, r0, c2, c0, 2   \n\
				mov	pc,lr                   \n\
");

/*------------------------------------------------------------------------------
  @brief   메인 TTB를 설정한다. 
  @remark  
*///----------------------------------------------------------------------------
void mmu_set_ttb_base( u32 value );
asm("   	            					\n\
											\n\
.align  5									\n\
.text                                       \n\
.global mmu_set_ttb_base                    \n\
mmu_set_ttb_base:                           \n\
				mcr	p15, 0, r0, c2, c0, 0   \n\
				mov	pc,lr                   \n\
");

/*------------------------------------------------------------------------------
  @brief   두번째 TTB를 설정한다.
  @remark  
*///----------------------------------------------------------------------------
void mmu_set_ttb2_base( u32 value );
asm("   	            					\n\
											\n\
.align  5									\n\
.text                                       \n\
.global mmu_set_ttb2_base                   \n\
mmu_set_ttb2_base:                          \n\
				mcr	p15, 0, r0, c2, c0, 1   \n\
				mov	pc,lr                   \n\
");

/*------------------------------------------------------------------------------
  @brief   코프로세서를 스냅샷 하여 저장한다. 
  @remark  
*///----------------------------------------------------------------------------

u8 *mmu_snapshot_coprocessor( u8 *save_buffer );
asm("   	            					                 \n\
											                 \n\
.align  5									                 \n\
.text                                                        \n\
.global mmu_snapshot_coprocessor                             \n\
mmu_snapshot_coprocessor:                                    \n\
				mrc p15, 0, r1, c1, c0,  0 	@  CR            \n\
				str	r1, [r0], #4                             \n\
    			                                             \n\
				mrc p15, 0, r1, c1, c0,  1 	@ Auxiliary      \n\
				str	r1, [r0], #4                             \n\
    			                                             \n\
				mrc p15, 0, r1, c1, c0,  2 	@ CACR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c1, c1,  1 	@ SDER           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c1, c1,  3 	@ VCR            \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c2, c0,  0 	@ TTB_0R         \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c2, c0,  1 	@ TTB_1R         \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c2, c0,  2 	@ TTBCR          \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c3, c0,  0 	@ DACR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c7, c4,  0 	@ PAR            \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c10, c0, 0 	@ D_TLBLR        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c10, c2, 0 	@ PRRR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c10, c2, 1 	@ NRRR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c12, c0, 0 	@ VBAR        	 \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c12, c0, 1 	@ MVBAR        	 \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 0 	@ FCSE           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 1 	@ CID            \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 2 	@ URWTPID        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 3 	@ UROTPID        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 4 	@ POTPID         \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c15, c7, 2 	@ MTLBAR         \n\
				str	r1, [r0], #4                             \n\
				                                             \n\
				mov	pc,lr                                    \n\
");
#if 0
asm("   	            					                 \n\
											                 \n\
.align  5									                 \n\
.text                                                        \n\
.global mmu_snapshot_coprocessor                             \n\
mmu_snapshot_coprocessor:                                    \n\
				mrc p15, 0, r1, c1, c0,  0 	@  CR            \n\
				str	r1, [r0], #4                             \n\
    			                                             \n\
				mrc p15, 0, r1, c1, c0,  1 	@ Auxiliary      \n\
				str	r1, [r0], #4                             \n\
    			                                             \n\
				mrc p15, 0, r1, c1, c0,  2 	@ CACR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c2, c0,  0 	@ TTB_0R         \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c2, c0,  1 	@ TTB_1R         \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c2, c0,  2 	@ TTBCR          \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c3, c0,  0 	@ DACR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c5, c0,  0 	@ D_FSR          \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c5, c0,  1 	@ I_FSR          \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c6, c0,  0 	@ D_FAR          \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c6, c0,  2 	@ I_FAR          \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c7, c4,  0 	@ PAR            \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c10, c0, 0 	@ D_TLBLR        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c10, c2, 0 	@ PRRR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c10, c2, 1 	@ NRRR           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c11, c1, 0 	@ PLEUAR         \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c11, c2, 0 	@ PLECNR         \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c11, c4, 0 	@ PLECR          \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c11, c5, 0 	@ PLEISAR        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c11, c7, 0 	@ PLEIEAR        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c11, c15,0 	@ PLECIDR        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c12, c0, 0 	@ SNSVBAR        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 0 	@ FCSE           \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 1 	@ CID            \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 2 	@ URWTPID        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 3 	@ UROTPID        \n\
				str	r1, [r0], #4                             \n\
				mrc p15, 0, r1, c13, c0, 4 	@ POTPID         \n\
				str	r1, [r0], #4                             \n\
				                                             \n\
				mov	pc,lr                                    \n\
");
#endif

void dump_coprocessor_value(unsigned char *save_buffer, int opt)
{
	struct core_reg *coreg;

	coreg = (struct core_reg *)save_buffer;

	if (opt) {
		printk("reload coprocessor value\n");
		mmu_snapshot_coprocessor_new(save_buffer);
	}
		
	printk(" Dump coprocessor value at 0x%p\n", coreg);
	
	printk(" coreg->c0.c0_0_2	0x%08lX\n", coreg->c1.c0_0	);

	printk(" coreg->c1.c0_0		0x%08lX\n", coreg->c1.c0_0	);
	printk(" coreg->c1.c0_1		0x%08lX\n", coreg->c1.c0_1	);
	printk(" coreg->c1.c0_2		0x%08lX\n", coreg->c1.c0_2	);

	printk(" coreg->c1.c1_0		0x%08lX\n", coreg->c1.c1_0	);
	printk(" coreg->c1.c1_1		0x%08lX\n", coreg->c1.c1_1	);
	printk(" coreg->c1.c1_2		0x%08lX\n", coreg->c1.c1_2	);
	printk(" coreg->c1.c1_3		0x%08lX\n", coreg->c1.c1_3	);

	printk(" coreg->c2.c0_0		0x%08lX\n", coreg->c2.c0_0	);
	printk(" coreg->c2.c0_1		0x%08lX\n", coreg->c2.c0_1	);
	printk(" coreg->c2.c0_2		0x%08lX\n", coreg->c2.c0_2	);

	printk(" coreg->c3.c0_0		0x%08lX\n", coreg->c3.c0_0	);

	printk(" coreg->c7.c4_0		0x%08lX\n", coreg->c7.c4_0	);

	printk(" coreg->c9.c12_0	0x%08lX\n", coreg->c9.c12_0	);
	printk(" coreg->c9.c12_1	0x%08lX\n", coreg->c9.c12_1	);
	printk(" coreg->c9.c12_2	0x%08lX\n", coreg->c9.c12_2	);
	printk(" coreg->c9.c12_3	0x%08lX\n", coreg->c9.c12_3	);
	printk(" coreg->c9.c12_5	0x%08lX\n", coreg->c9.c12_5	);

	printk(" coreg->c9.c13_0	0x%08lX\n", coreg->c9.c13_0	);
	printk(" coreg->c9.c13_1	0x%08lX\n", coreg->c9.c13_1	);
	printk(" coreg->c9.c13_2	0x%08lX\n", coreg->c9.c13_2	);

	printk(" coreg->c9.c14_0	0x%08lX\n", coreg->c9.c14_0	);
	printk(" coreg->c9.c14_1	0x%08lX\n", coreg->c9.c14_1	);
	printk(" coreg->c9.c14_2	0x%08lX\n", coreg->c9.c14_2	);

	printk(" coreg->c10.c0_0	0x%08lX\n", coreg->c10.c0_0  );
	printk(" coreg->c10.c2_0	0x%08lX\n", coreg->c10.c2_0  );
	printk(" coreg->c10.c2_0	0x%08lX\n", coreg->c10.c2_0  );

	printk(" coreg->c12.c0_0	0x%08lX\n", coreg->c12.c0_0  );
	printk(" coreg->c12.c0_1	0x%08lX\n", coreg->c12.c0_1  );
	printk(" coreg->c12.c1_1	0x%08lX\n", coreg->c12.c1_1  );

	printk(" coreg->c13.c0_0	0x%08lX\n", coreg->c13.c0_0  );
	printk(" coreg->c13.c0_1	0x%08lX\n", coreg->c13.c0_1  );
	printk(" coreg->c13.c0_2	0x%08lX\n", coreg->c13.c0_2  );
	printk(" coreg->c13.c0_3	0x%08lX\n", coreg->c13.c0_3  );
	printk(" coreg->c13.c0_4	0x%08lX\n", coreg->c13.c0_4  );

	printk(" coreg->c15.c7_2_5	0x%08lX\n", coreg->c15.c7_2_5);
}

unsigned char *	mmu_snapshot_coprocessor_new(u8 *save_buffer)
{
	struct core_reg *coreg;

	coreg = (struct core_reg *)save_buffer;

	memset(coreg, 0, sizeof(struct core_reg));

	return save_buffer + sizeof(struct core_reg);

	/* CR0 */
	asm volatile ("mrc p15, 2, %0, c0, c0, 0" : "=r"(coreg->c0.c0_0_2));	// cssr
	
	/* CR1 */
	asm volatile ("mrc p15, 0, %0, c1 , c0, 0" : "=r"(coreg->c1.c0_0));		// cr
	asm volatile ("mrc p15, 0, %0, c1 , c0, 1" : "=r"(coreg->c1.c0_1));		// auxiliary
	asm volatile ("mrc p15, 0, %0, c1 , c0, 2" : "=r"(coreg->c1.c0_2));		// cacr
	asm volatile ("mrc p15, 0, %0, c1 , c1, 0" : "=r"(coreg->c1.c1_0));		// sder
	asm volatile ("mrc p15, 0, %0, c1 , c1, 1" : "=r"(coreg->c1.c1_1));		// sder
	asm volatile ("mrc p15, 0, %0, c1 , c1, 2" : "=r"(coreg->c1.c1_2));		// nsacr
	asm volatile ("mrc p15, 0, %0, c1 , c1, 3" : "=r"(coreg->c1.c1_3));		// vcr

	/* CR2 */
	asm volatile ("mrc p15, 0, %0, c2 , c0, 0" : "=r"(coreg->c2.c0_0));		// ttb_0r
	asm volatile ("mrc p15, 0, %0, c2 , c0, 1" : "=r"(coreg->c2.c0_1));		// ttb_1r
	asm volatile ("mrc p15, 0, %0, c2 , c0, 2" : "=r"(coreg->c2.c0_2));		// ttb_cr

	/* CR3 */
	asm volatile ("mrc p15, 0, %0, c3 , c0, 0" : "=r"(coreg->c3.c0_0));		// dacr

	/* CR5 */
	asm volatile ("mrc p15, 0, %0, c5, c0, 0" : "=r"(coreg->c5.c0_0));		// d fsr
	asm volatile ("mrc p15, 0, %0, c5, c0, 1" : "=r"(coreg->c5.c0_1));		// i fsr
	asm volatile ("mrc p15, 0, %0, c5, c1, 0" : "=r"(coreg->c5.c1_0));		// d afsr
	asm volatile ("mrc p15, 0, %0, c5, c1, 1" : "=r"(coreg->c5.c1_1));		// i afsr
	
	/* CR6 */
	asm volatile ("mrc p15, 0, %0, c6, c0, 0" : "=r"(coreg->c6.c0_0));		// d far
	asm volatile ("mrc p15, 0, %0, c6, c0, 2" : "=r"(coreg->c6.c0_2));		// i far

	/* CR7 */
	asm volatile ("mrc p15, 0, %0, c7, c4, 0" : "=r"(coreg->c7.c4_0));		// par

	/* CR9 */
	asm volatile ("mrc p15, 0, %0, c9, c12, 0" : "=r"(coreg->c9.c12_0));	// pmcontrol
	asm volatile ("mrc p15, 0, %0, c9, c12, 1" : "=r"(coreg->c9.c12_1));	// cesr
	asm volatile ("mrc p15, 0, %0, c9, c12, 2" : "=r"(coreg->c9.c12_2));	// cecr
	asm volatile ("mrc p15, 0, %0, c9, c12, 3" : "=r"(coreg->c9.c12_3));	// ofsr
	asm volatile ("mrc p15, 0, %0, c9, c12, 5" : "=r"(coreg->c9.c12_5));	// pcsr
	asm volatile ("mrc p15, 0, %0, c9, c13, 0" : "=r"(coreg->c9.c13_0));	// ccr
	asm volatile ("mrc p15, 0, %0, c9, c13, 1" : "=r"(coreg->c9.c13_1));	// esr
	asm volatile ("mrc p15, 0, %0, c9, c13, 2" : "=r"(coreg->c9.c13_2));	// pmcounter
	asm volatile ("mrc p15, 0, %0, c9, c14, 0" : "=r"(coreg->c9.c14_0));	// uer
	asm volatile ("mrc p15, 0, %0, c9, c14, 1" : "=r"(coreg->c9.c14_1));	// iesr
	asm volatile ("mrc p15, 0, %0, c9, c14, 2" : "=r"(coreg->c9.c14_2));	// iecr

	/* CR10 */
	asm volatile ("mrc p15, 0, %0, c10, c0, 0" : "=r"(coreg->c10.c0_0));		// d_tlblr
	asm volatile ("mrc p15, 0, %0, c10, c2, 0" : "=r"(coreg->c10.c2_0));		// prrr
	asm volatile ("mrc p15, 0, %0, c10, c2, 1" : "=r"(coreg->c10.c2_1));		// nrrr

	/* CR12 */
	asm volatile ("mrc p15, 0, %0, c12, c0, 0" : "=r"(coreg->c12.c0_0));		// vbar
	asm volatile ("mrc p15, 0, %0, c12, c0, 1" : "=r"(coreg->c12.c0_1));		// mvbar
	asm volatile ("mrc p15, 0, %0, c12, c1, 1" : "=r"(coreg->c12.c1_1));		// vir

	/* CR13 */
	asm volatile ("mrc p15, 0, %0, c13, c0, 0" : "=r"(coreg->c13.c0_0));		// fcse
	asm volatile ("mrc p15, 0, %0, c13, c0, 1" : "=r"(coreg->c13.c0_1));		// cid
	asm volatile ("mrc p15, 0, %0, c13, c0, 2" : "=r"(coreg->c13.c0_2));		// upwtpid
	asm volatile ("mrc p15, 0, %0, c13, c0, 3" : "=r"(coreg->c13.c0_3));		// urotpid
	asm volatile ("mrc p15, 0, %0, c13, c0, 4" : "=r"(coreg->c13.c0_4));		// potpid

	/* CR15*/
	asm volatile ("mrc p15, 5, %0, c15, c7, 2" : "=r"(coreg->c15.c7_2_5));		// mtlbar

	return save_buffer + sizeof(struct core_reg);
}

void active_dcache(unsigned char *save_buffer)
{
	struct core_reg *coreg;

	coreg = (struct core_reg *)save_buffer;

	asm volatile ("mcr p15, 0, %0, c1, c0, 0" : : "r"(coreg->c1.c0_0));
}

/*------------------------------------------------------------------------------
  @brief   TTB 테이블에서 가상주소 vaddr 의 L1 ENTRY 값을 구한다. 
  @remark  
*///----------------------------------------------------------------------------
u32 mmu_get_l1_entry	( u32 *ttb, u32 vaddr )
{
	u32 l1_index;
	
	l1_index = (vaddr>>MMU_L1_SHIFT )&0xFFF;
	return  ttb[l1_index];
}

/*------------------------------------------------------------------------------
  @brief   TTB 테이블에서 가상주소 vaddr 의 L2 ENTRY 값을 구한다. 
  @remark  COARSE 만 다룬다. 
*///----------------------------------------------------------------------------
u32 mmu_get_l2_entry	( u32 *ttb, u32 vaddr )
{
	u32 l1_index;
	u32 l1_entry;
	u32 *pte;
	u32 l2_index;
	u32 l2_entry;
	
	l1_index 	= (vaddr>>MMU_L1_SHIFT )&0xFFF;
	l1_entry	= ttb[l1_index];
	
	l2_entry	= 0xFFFFFFFF;
	
	if( MMU_L1_COARSE == ( l1_entry & MMU_L1_MASK_TYPE ) )
	{
		pte = (u32 *) mmu_get_vaddr_of_coarse_from_l1_entry(l1_entry);
		l2_index = (vaddr>>MMU_L2_SHIFT) & 0xFF;
		l2_entry = pte[l2_index];
	}		
	
	return l2_entry; 
}

u32  mmu_set_fault_l2_entry( u32 *ttb, u32 vaddr )
{
	u32 l1_index;
	u32 l1_entry;
	u32 *pte;
	u32 l2_index;
	u32 l2_entry;
	
	l1_index 	= (vaddr>>MMU_L1_SHIFT )&0xFFF;
	l1_entry	= ttb[l1_index];
	
	l2_entry	= 0xFFFFFFFF;
	
	if( MMU_L1_COARSE == ( l1_entry & MMU_L1_MASK_TYPE ) )
	{
		pte = (u32 *) mmu_get_vaddr_of_coarse_from_l1_entry(l1_entry);
		l2_index = (vaddr>>MMU_L2_SHIFT) & 0xFF;
		l2_entry = pte[l2_index];
		
		l2_entry &= (~0x3);
//		pte[l2_index] = l2_entry;
		pte[l2_index] = 0;
		
//		clean_dcache_area( pte, PAGE_SIZE );	
		
	}		

	all_tbl_clean();
	
	return l2_entry; 
}

/*------------------------------------------------------------------------------
  @brief   L1 엔트리가 COARSE 인가를 본다. 
  @remark  COARSE 만 다룬다. 
*///----------------------------------------------------------------------------
s32	mmu_is_coarse( u32 l1_entry )
{
	if( (l1_entry & MMU_L1_MASK_TYPE ) == MMU_L1_COARSE ) return 1;
	return 0;
}

/*------------------------------------------------------------------------------
  @brief   현재 동작중인 MMU 테이블에서 가상주소 vaddr 의 물리주소를 구한다. 
  @remark  COARSE 만 다룬다. 
*///----------------------------------------------------------------------------
u32 mmu_virt_to_phys( u32 vaddr )
{
	u32 ttb_paddr;
	u32 *ttb;
	u32 l2_entry;
	
	ttb_paddr = mmu_get_ttb_base();
	ttb = (u32 *) phys_to_virt( ttb_paddr & MMU_TTB_BASE_ADDR_MASK );
//	printk( "TTB  PADDR = %08X, VADDR = %08X\n",ttb_paddr,ttb );
	
	l2_entry = mmu_get_l2_entry( ttb, vaddr );
	
	return MMU_L2_PADDR_SMALL(l2_entry);
}

/*------------------------------------------------------------------------------
  @brief   DRAM 메모리 폴트 처리를 위하여 MMU의 section 정보를 coarse 로 변경
           하기 위한 MMU L2 ENTRY PAGE 를 할당하고 정보를 작성한다. 
           
  @remark  
*///----------------------------------------------------------------------------
u32 zb_dumy_read( u32 data )
{
	return data;
}

void zb_make_coarse_dram_entrys( u32 dram_phys_base, u32 dram_size )
{
	volatile u32 *pte_datas;
	u32 phys_addr, vaddr;
	
	printk( "order = %d\n", get_order((dram_size/1024)) );
	if( zb_coarse_tables == NULL )
	{
		zb_coarse_tables =  (u32 *) __get_free_pages(GFP_ATOMIC | GFP_DMA, get_order((dram_size/1024)));
	}
	
	if( zb_coarse_tables == NULL )
	{
		printk( "Oops! Page Table Memory Allocation Error - %s()\n",__FUNCTION__ );
		return ;
	}
	printk( "zb_coarse_tables VADD = %08X , PADDR = %08X\n", (int) zb_coarse_tables, (int) virt_to_phys( zb_coarse_tables ) );
	
	pte_datas = zb_coarse_tables;
	vaddr = PAGE_OFFSET;
	for( vaddr = PAGE_OFFSET; vaddr < (PAGE_OFFSET + dram_size); vaddr += PAGE_SIZE )
	{
//		printk( "*pte_datas = %08X\n", *pte_datas );
		phys_addr = virt_to_phys((void *)vaddr);
		*pte_datas = ZBI_L2_FAULT_MARK(phys_addr); 	// nG  S APX  TEX AP C B 1 XN
		                                   			//  0  0   0   00 10 1 1 1  0 = 0 000 0010 1110 = 0x2E
													// 0x01E

//		zb_dumy_read( *pte_datas );
		pte_datas++;                     
	}
	
//	clean_dcache_area( zb_coarse_tables, (dram_size/1024) );
}

/*------------------------------------------------------------------------------
  @brief   DRAM 메모리 폴트 처리를 위하여 MMU의 section 정보를 coarse 로 변경
           한 MMU L2 테이블을 각 프로세서 별 MMU의 L1 테이블에 적용한다.
  @remark  
*///----------------------------------------------------------------------------
void zb_reset_mmu_by_coarse_dram_entrys( u32 *mmu_base, u32 dram_size )
{
	u32 pte_paddress;
	u32 l1_index;
	u32 new_l1_entry;

	
	if( zb_coarse_tables == NULL )
	{
		printk( "Oops! Page Table NULL Program Bug %s:%d\n",__FUNCTION__, __LINE__ );
		return;
	}
	
	pte_paddress = virt_to_phys( zb_coarse_tables );
	
	all_tbl_clean();

	for( l1_index =  ZBI_DRAM_MMU_L1_INDEX; 
	     l1_index < (ZBI_DRAM_MMU_L1_INDEX+(dram_size/(1024*1024))); 
	     l1_index++ )
	{
		new_l1_entry  = (pte_paddress & (~0x3FF));
		new_l1_entry |= (0<<5); // DOMAIN
		new_l1_entry |= MMU_L1_COARSE;
		new_l1_entry |= (01<<4);
		//printk( "old value 0x%08X l1_index = %08X new_l1_entry = %08X\n", mmu_base[l1_index], l1_index, new_l1_entry );	
		
		mmu_base[l1_index] = new_l1_entry;
//		zb_dumy_read( mmu_base[l1_index] );

		pte_paddress += 1024;
	}	
	
//	clean_dcache_area( mmu_base, PTRS_PER_PGD * sizeof(pgd_t) );
	
}

/*------------------------------------------------------------------------------
  @brief   DRAM 메모리 폴트 처리를 위하여 section을 page 로 변경 처리 MMU 재 설정
  @remark  
*///----------------------------------------------------------------------------
void zb_rebuild_mmu_by_coarse_dram_area( u32 dram_phys_base, u32 dram_size )
{
	struct task_struct 	*p;
	unsigned int opt;

#ifdef FULL_RESTORE_AT_BOOT
   printk("Full Restore at boot debug...\n");
   printk("Do not convert section to coarse\n");
   return;
#endif

	printk( "It is change from section to coarse about dram area of mmu table\n" );
	
	zb_make_coarse_dram_entrys( dram_phys_base, dram_size );

	opt = ((u32 *)KERNEL_SWAPPER_PG_DIR_VADDR)[ZBI_DRAM_MMU_L1_INDEX];
	opt &= SECTION_SIZE - 1;
	zbi_set_section_option(opt); 

	printk( "apply swapper_pg_dir\n" );
	zb_reset_mmu_by_coarse_dram_entrys( (u32 *) KERNEL_SWAPPER_PG_DIR_VADDR  , dram_size );

	flush_tlb_all();
	for_each_process(p)	
	{
		if( p->mm == NULL ) continue;
		printk( "apply task name [%s]\n", p->comm );
		zb_reset_mmu_by_coarse_dram_entrys( (u32 *) p->mm->pgd  , dram_size );
		flush_tlb_mm(p->mm);
	}
	
	all_tbl_clean();
}

void zb_show_process_name( u32 dram_phys_base, u32 dram_size )
{
	struct task_struct 	*p;

	printk( "It is change from section to coarse about dram area of mmu table\n" );
	
	for_each_process(p)	
	{
		printk( "task name [%s]\n", p->comm );
		printk( "task mm   [%p]\n", p->mm);
	}
}

/*------------------------------------------------------------------------------
  @brief   DRAM 메모리 폴트 처리를 위하여 coarse 되어 있것을 section로 재변경
  @remark  
*///----------------------------------------------------------------------------
void zb_reset_mmu_by_section_dram_entrys( u32 *mmu_base, u32 dram_size )
{
	u32 l1_index, start, end, base;
	u32 org_l1_entry;
	unsigned int opt;

	if (zb_coarse_tables == NULL) {
		printk( "Oops! Page Table NULL Program Bug %s:%d\n",__FUNCTION__, __LINE__ );
		return;
	}

	opt = zbi_get_section_option();
	all_tbl_clean();

	start = PAGE_OFFSET >> SECTION_SHIFT;
	end = start + (dram_size/SECTION_SIZE);
	base = PHYS_OFFSET >> SECTION_SHIFT;

	for (l1_index = start ; l1_index < end; l1_index++, base++) {
		org_l1_entry  = base << SECTION_SHIFT ;
		org_l1_entry  |= opt;
//		printk( "old value 0x%08X l1_index = %08X org_l1_entry = %08X\n", mmu_base[l1_index]l1_index, org_l1_entry );

		mmu_base[l1_index] = org_l1_entry;
	}
}
void zb_recover_mmu_by_section_dram_area(u32 dram_phys_base, u32 dram_size)
{
	struct task_struct 	*p;

	printk("It is change from coarse to section about dram area of mmu table\n");

	printk("apply swapper_pg_dir\n");
	zb_reset_mmu_by_section_dram_entrys((u32 *)KERNEL_SWAPPER_PG_DIR_VADDR, dram_size);

	for_each_process(p) {
		if (p->mm == NULL)
			continue;
		printk("apply task name [%s]\n", p->comm);
		zb_reset_mmu_by_section_dram_entrys((u32 *) p->mm->pgd, dram_size);
	}

	all_tbl_clean();
}

