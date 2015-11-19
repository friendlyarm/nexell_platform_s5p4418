//------------------------------------------------------------------------------
// 화일명 : sbi.h
// 설  명 : 드라이버의 스냅샷 부팅 헤더파일이다.
// 
// 작성자 : 유영창 에프에이리눅스(주) frog@falinux.com
// 작성일 : 2001년 10월 8일
// 저작권 : 에프에이리눅스(주)
// 주  의 : 
//------------------------------------------------------------------------------

#ifndef __SBI_H__
#define __SBI_H__

//
//  SBI   - zeroboot Boot Information
//  SBIH  - zeroboot Boot Header
//  SBID  - zeroboot Boot Data
// 
//          SBI                 SBIH
//          +---------+  +----> +-------------------+---+ -+--      
//   Fix    |  SBIH   +--+      | Magic             | 4 |  |        
//   -------+---------+         +-------------------+---+  |        
//   CHUNK  |         |         | version           | 4 |  |        
//          |  DUMP   |         +-------------------+---+  |        
//          |  DATA   |   +-----+ chunk start       | 4 |  |        
//          |         |   |     +-------------------+---+  |        
//          +---------+<--+     | chunk count       | 4 |  |        
//       +--+ SBID(1) |         +-------------------+---+  |  4K    
//       |  +---------+         | sbid count        | 4 |  |        
//       |  |  ...    |         +-------------------+---+  |        
//       |  +---------+         |                   |   |  |        
//       |  | SBID(n) |         | Reserved          | N |  |        
//       |  +---------+         |                   |   |  |        
//       |                      +-------------------+---+ -+--      
//       |                      |                   |   |  |        
//       |                      | CPU save          | N |  |        
//       |                      |                   |   |  |        
//       |                      +-------------------+---+  |        
//       |                      |                   |   |  |        
//       |                      | CO-Prosessor save | N |  | 4K     
//       |                      |                   |   |  |        
//       |                      +-------------------+---+  |        
//       |                      |                   |   |  |        
//       |                      | Reserved          | N |  |        
//       |                      |                   |   |  |        
//       |                      +-------------------+---+ -+--      
//       |                                                             
//       |              
//       +------------------->  SBID                                
//                              +-------------------+---+              
//                              | Command           | 4 |           
//                              +-------------------+---+              
//                              | V1                | 4 |           
//                              +-------------------+---+              
//                              | V2                | 4 |           
//                              +-------------------+---+              
//                              | V3                | 4 |           
//                              +-------------------+---+           
//
//    * Magic ID : 0xFADEADAF
//    * chunk size -> page size (4K)
//    * chunk Start -> SBIH + 4K align
//  
//  SBID Command    
//
//   ① END       
//   ② COPY      
//   ③ FILL_ZERO 
//   ④ FILL_DATA 
//
//
//    SBID Stgruct and List
//
//   ① END
//   +--------------+      +---------------+
//   |          0x0 |      |   0xFFFFFFFF  |
//   +--------------+      +---------------+
//   |            0 |      |   0xFFFFFFFF  |
//   +--------------+      +---------------+
//   |            0 |      |   0xFFFFFFFF  |
//   +--------------+      +---------------+
//   |            0 |      |   0xFFFFFFFF  |
//   +--------------+      +---------------+
//     
//    
//   ② COPY
//   +--------------------+   
//   |                0x1 |   
//   +--------------------+   
//   |     Target Address |   
//   +--------------------+   
//   |  Target Total Size |   
//   +--------------------+   
//   |  NAND Page Adderss |   
//   +--------------------+   
//
//    
//   FILL_ZERO                FILL_DATA             
//   +--------------------+   +--------------------+   
//   |                0x2 |   |                0x3 |   
//   +--------------------+   +--------------------+   
//   |     Target Address |   |     Target Address |   
//   +--------------------+   +--------------------+   
//   |  Target Total Size |   |  Target Total Size |   
//   +--------------------+   +--------------------+   
//   |           Reserved |   |          FILL_DATA |   
//   +--------------------+   +--------------------+   
//   
//   MEM_VALUE_LONG           MEM_MOVE_LONG
//   +--------------------+   +--------------------+   
//   |                0x4 |   |                0x7 |
//   +--------------------+   +--------------------+   
//   |     Target Address |   |     Target Address |   
//   +--------------------+   +--------------------+   
//   |              Value |   |     Source Address |   
//   +--------------------+   +--------------------+   
//   |              Count |   |              Count |   
//   +--------------------+   +--------------------+   
//   
//   MEM_VALUE_SHOT           MEM_MOVE_SHOT
//   +--------------------+   +--------------------+   
//   |                0x5 |   |                0x8 |
//   +--------------------+   +--------------------+   
//   |     Target Address |   |     Target Address |   
//   +--------------------+   +--------------------+   
//   |              Value |   |     Source Address |   
//   +--------------------+   +--------------------+   
//   |              Count |   |              Count |   
//   +--------------------+   +--------------------+   
//   
//   MEM_VALUE_BYTE           MEM_MOVE_BYTE
//   +--------------------+   +--------------------+   
//   |                0x6 |   |                0x9 |
//   +--------------------+   +--------------------+   
//   |     Target Address |   |     Target Address |   
//   +--------------------+   +--------------------+   
//   |              Value |   |     Source Address |   
//   +--------------------+   +--------------------+   
//   |              Count |   |              Count |   
//   +--------------------+   +--------------------+   
//   

#define NAND_SAVE_TOTAL_SIZE 	(32*1024*1024)			// 32M 

#define SIZE_SBI_UNIT  			(4*1024)

#define SIZE_SBI_SBIH 			(2*SIZE_SBI_UNIT)
#define SIZE_SBI_CHUNK  		(SIZE_SBI_UNIT)

#define SIZE_SBI_SBIH_CPU_SAVE	(1024)
#define SIZE_SBI_SBIH_CO_SAVE 	(1024)

#define MAX_SBID_COUNT     		((NAND_SAVE_TOTAL_SIZE/PAGE_SIZE)-(sizeof(sbih_t)/sizeof(sbid_t)))
#define SBID_COUNT_PER_CHUNK	(SIZE_SBI_CHUNK/sizeof(sbid_t))
#define SBI_MAGIC_ID			0xFADEADAF
#define SBI_VERSION				100

typedef struct
{
	unsigned long 	magic;							// 0xFADEADAF
	unsigned long  	version;             			// SBI_VERSION
	unsigned long  	chunk_start;
	unsigned long  	chunk_count;             
	unsigned long  	sbid_count;
	unsigned long  	*copy_marks;
	unsigned short 	*phys_to_page_info;
    unsigned char   rev_area1	[SIZE_SBI_UNIT-(7*4)];  
    
	unsigned char  	cpu_save	[SIZE_SBI_SBIH_CPU_SAVE];
	unsigned char   co_save		[SIZE_SBI_SBIH_CO_SAVE];
	unsigned char   rev_area2	[SIZE_SBI_UNIT-(SIZE_SBI_SBIH_CPU_SAVE+SIZE_SBI_SBIH_CO_SAVE)];
	
} sbih_t;


typedef struct cpu_save
{
	unsigned long	svc_r1;		// __recover_ptr
	unsigned long	svc_r2;		// cpsr
	unsigned long	svc_r3;		// spsr
	unsigned long	svc_r4;
	unsigned long	svc_r5;
	unsigned long	svc_r6;
	unsigned long	svc_r7;
	unsigned long	svc_r8;
	unsigned long	svc_r9;
	unsigned long	svc_r10;
	unsigned long	svc_r11;
	unsigned long	svc_r12;
	unsigned long	svc_sp;
	unsigned long	svc_lr;
	unsigned long	svc_nsp;
	
	unsigned long	sys_r1;		// spsr
	unsigned long	sys_r13;	// sp
	unsigned long	sys_r14;	// lr

	unsigned long	fiq_r1;		// spsr
	unsigned long	fiq_r8;		// 
	unsigned long	fiq_r9;		// 
	unsigned long	fiq_r10;
	unsigned long	fiq_r11;
	unsigned long	fiq_r12;
	unsigned long	fiq_r13;	// sp
	unsigned long	fiq_r14;	// lr
	
	unsigned long	irq_r1;		// spsr
	unsigned long	irq_r13;	// sp
	unsigned long	irq_r14;	// lr
	
	unsigned long	abt_r1;		// spsr
	unsigned long	abt_r13;	// sp
	unsigned long	abt_r14;	// lr
	
	unsigned long	und_r1;		// spsr
	unsigned long	und_r13;	// sp
	unsigned long	und_r14;	// lr

} cpu_save_t;


typedef struct
{
	unsigned long cmd;
	unsigned long v1;
	unsigned long v2;
	unsigned long v3;
} sbid_t;

#define SBID_CMD_END1				0
#define SBID_CMD_END2				0xFFFFFFFF
#define SBID_CMD_COPY				1
#define SBID_CMD_FILL_ZERO 			2
#define SBID_CMD_FILL_DATA 			3
#define SBID_CMD_READ_CHUNK			4
#define SBID_CMD_PAGE_INFO 			5


#define SBID_CMD_COPY_NORMAL				0

#define SBID_FLAG_MMU_BASE       		(1<<31)
#define SBID_FLAG_MMU_COARSE       		(1<<30)
#define SBID_FLAG_FAULT_PAGE       		(1<<29)
#define SBID_FLAG_COPY_PAGE       		(1<<28)
#define SBID_FLAG_KERNEL_STACK_PAGE     (1<<27)

#define SBID_MASK_PID              0xFFFF


typedef struct
{
	unsigned long cmd;		// SBID_CMD_END1 or SBID_CMD_END2
	unsigned long zero1;	// 0x0 or 0xffffffff
	unsigned long zero2;	// 0x0 or 0xffffffff
	unsigned long zero3;	// 0x0 or 0xffffffff
} sbid_end_t;

typedef struct
{
	unsigned long cmd;					// SBID_CMD_COPY , SBID_CMD_COPY_INFO
	unsigned long target_address;		// 
	unsigned long target_total_size;	// 
	unsigned long nand_page_address;	// 
} sbid_copy_t;

typedef struct
{
	unsigned long cmd;					// SBID_CMD_FILL_ZERO
	unsigned long target_address;		// 
	unsigned long target_total_size;	// 
	unsigned long reserved;				// 
} sbid_fill_zero_t;

typedef struct
{
	unsigned long cmd;					// SBID_CMD_FILL_ZERO
	unsigned long target_address;		// 
	unsigned long target_total_size;	// 
	unsigned long fill_data;			// 
} sbid_fill_data_t;

typedef struct
{
	unsigned long cmd;					// SBID_CMD_READ_CHUNK
	unsigned long chunk_start;			//
	unsigned long chunk_count;			//
	unsigned long sbid_count;			// 
} sbid_read_chunk_t;

typedef struct
{
	unsigned long cmd;					// SBID_CMD_PAGE_INFO
	unsigned long page_address;			// 
	unsigned long nand_page_address;	// 
	unsigned long state;				// 
} sbid_page_info_t;

//#define ZEROBOOT_NAND_BASE_OFFSET        (32*1024*1024)

#endif // __SBI_H__

