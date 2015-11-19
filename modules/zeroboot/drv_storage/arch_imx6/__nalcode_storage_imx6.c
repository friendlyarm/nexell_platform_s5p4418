/**    
    @file     nalcode_storage_s5pv210.c
    @date     2011/07/05
    @author   오재경 freefrug@falinux.com  FALinux.Co.,Ltd.
    @brief    .S 영역에서 호출된다.
			  최대한 빠르게 동작하도록 작성되었다.
    @todo     
    @bug     
    @remark   ECC 동작은 배제되었다.
    
    @warning 
*/
//
//  저작권    에프에이리눅스(주)
//            외부공개 금지
//
//----------------------------------------------------------------------------

#include <typedef.h>

#define OFS_NFCONF 			(0x00)	// R/W Configuration register								0xX000_100X 		
#define OFS_NFCONT 			(0x04)	// R/W Control register                          			0x0001_00C6         
#define OFS_NFCMD  			(0x08)	// R/W Command register                          			0x0000_0000         
#define OFS_NFADDR 			(0x0c)	// R/W Address register                          			0x0000_0000         
#define OFS_NFDATA8			(0x10)	// R/W Data register                             			0xXXXX_XXXX         
#define OFS_NFSTAT 			(0x28)	// R/W status registet                      				0x0080_001D         

#define NFCONF  			(*(volatile u32 *)(nand_base + OFS_NFCONF		 ))
#define NFCONT 				(*(volatile u32 *)(nand_base + OFS_NFCONT		 ))
#define NFCMD            	(*(volatile u8  *)(nand_base + OFS_NFCMD         ))
#define NFADDR            	(*(volatile u8  *)(nand_base + OFS_NFADDR        ))
#define NFDATA8            	(*(volatile u8  *)(nand_base + OFS_NFDATA8       ))
#define NFSTAT            	(*(volatile u32 *)(nand_base + OFS_NFSTAT        ))

#define NAND_DISABLE_CE()	(NFCONT |= (1 << 1))
#define NAND_ENABLE_CE()	(NFCONT &= ~(1 << 1))

#define NF_TRANSRnB()		do { while(!(NFSTAT & (1 << 0))); } while(0)

#define NAND_CMD_READ0		0x00
#define NAND_CMD_READSTART	0x30
#define NAND_CMD_READID		0x90

#define NAND_PAGE_SIZE		2048
#define NAND_PAGE_SHIFT		11


volatile void *nand_base; 	

//
// asmcopy_page_2048
//
void asmcopy_page_2048( void *buffer, u32 nand_io_base );
asm("   	            				\n\
.align  2								\n\
.text									\n\
.global asmcopy_page_2048				\n\
.type asmcopy_page_2048, %function		\n\
asmcopy_page_2048:						\n\
 			stmfd sp!, {r1-r9, lr}	 	\n\
@			ldr		r1, =0x70200010	 	\n\
			;							\n\
	mov	r6, #(2048/16)  				\n\
loop:			  	    				\n\
			;							\n\
			ldrb	r2, [r1]			\n\
 			ldrb	r3, [r1]            \n\
 			ldrb	r4, [r1]            \n\
 			ldrb	r5, [r1]            \n\
			orr		r2, r3, lsl#8       \n\
			orr		r2, r4, lsl#16      \n\
			orr		r2, r5, lsl#24      \n\
			;							\n\
			ldrb	r7, [r1]			\n\
 			ldrb	r3, [r1]            \n\
 			ldrb	r4, [r1]            \n\
 			ldrb	r5, [r1]            \n\
			orr		r7, r3, lsl#8       \n\
			orr		r7, r4, lsl#16      \n\
			orr		r7, r5, lsl#24      \n\
			;							\n\
			ldrb	r8, [r1]			\n\
 			ldrb	r3, [r1]            \n\
 			ldrb	r4, [r1]            \n\
 			ldrb	r5, [r1]            \n\
			orr		r8, r3, lsl#8       \n\
			orr		r8, r4, lsl#16      \n\
			orr		r8, r5, lsl#24      \n\
			;							\n\
			ldrb	r9, [r1]			\n\
 			ldrb	r3, [r1]            \n\
 			ldrb	r4, [r1]            \n\
 			ldrb	r5, [r1]            \n\
			orr		r9, r3, lsl#8       \n\
			orr		r9, r4, lsl#16      \n\
			orr		r9, r5, lsl#24      \n\
			;							\n\
	stmia r0!, { r2,r7,r8,r9 }			\n\
			;							\n\
	subs r6, r6, #1   					\n\
	bne  loop         					\n\
			;							\n\
			ldmfd sp!, {r1-r9, pc}		\n\
");


void nand_read_page( unsigned char *buf, unsigned long page )
{
	int 	lp;

	NAND_ENABLE_CE();

    NFCMD = NAND_CMD_READ0;

	
    NFADDR = 0;						// col #1
	NFADDR = 0;						// col #2
	NFADDR = (page)		 ; 			// row #1
	NFADDR = (page >> 8) ; 			// row #2              
	NFADDR = (page >> 16); 			// row #3              
	
	NFCMD = NAND_CMD_READSTART;
    NF_TRANSRnB();
	
	
	//for( lp=0; lp < NAND_PAGE_SIZE; lp++) 
	//{
    //	*buf++ = NFDATA8;
    //}
	
	//for( lp=0; lp < NAND_PAGE_SIZE/16; lp++) 
	//{
    //	asmcopy_16byte_nand( buf );
    //	buf += 16;
    //}

	asmcopy_page_2048( buf , (u32)(nand_base+0x10) );

    NAND_DISABLE_CE();
}
/*
unsigned long get_nand_base( void );
asm("   	       					                \n\
.align  5						                    \n\
.text                                               \n\
.global get_nand_base								\n\
get_nand_base:										\n\
	mrc 	p15, 0, r0, c1, c0						\n\
	tst 	r0, #1									\n\
	ldreq	r0, =0x70200000							\n\
	ldrne	r0, =0xF5005000							\n\
	mov		pc, lr									\n\
                                                    \n\
");

void nalcode_stroage_set_base( u32 phys )
{
	nand_base = (void *)get_nand_base();
}
*/

// 외부 호출함수 ------------------------------------------------------------------------
void nalcode_stroage_read_4k_page( u32 mem_page_offset, u8 *mem_buf, void *vaddr )
{
	u32  nand_page;

	nand_base = vaddr;
	nand_page = (ZB_STORAGE_OFFSET>>NAND_PAGE_SHIFT) + mem_page_offset*(4096/NAND_PAGE_SIZE);

	nand_read_page( mem_buf                , nand_page   ); 
	nand_read_page( mem_buf + NAND_PAGE_SIZE, nand_page+1 );
}

