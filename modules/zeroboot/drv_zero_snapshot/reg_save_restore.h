/*-----------------------------------------------------------------------------

  파 일 : reg_save_restore.h

  설 명 : 레지스터를 저장하고 복구하는 함수

  작 성 : freefrug@falinux.com

  날 짜 : 2011-07 ~

  주 의 :



-------------------------------------------------------------------------------*/

#ifndef _REG_SAVE_RESTORE_H_
#define _REG_SAVE_RESTORE_H_

// 레지스터 저장 복구를 위한 구조체 ----------------------
typedef struct
{
	unsigned int addr;
	unsigned int val;
	unsigned int mask;		// 읽기 일 경우 flags=0 이면  save_val = *reg & ~mask   <== 마스크 반전임!!
	                        // 쓰기 일 경우 flags 옵션이 있으면 마스크 사용
	unsigned int flags;
	
} reg_t;

#define RF_RD					(1<<31)
#define RF_WR					(1<<30)
#define RF_BIT16				(1<<29)				// 16비트 레지스터
#define RF_BIT08				(1<<28)				//  8비트 레지스터

#define RF_RD_MASK				0x000000ff			// read  bit D7..D0
#define RF_WR_MASK				0x00ffff00			// write bit D23..D8
#define RF_RD_CMD(x)			((x+1)<<0)
#define RF_WR_CMD(x)			((x+1)<<8)


#define REG_ADDR_SEQ_MARK		0xfafafa55			// 주소가 이값일 경우 val 의 주소를 시작으로 mask 주소값까지 진행한다. 
#define REG_ADDR_SEQ_COPY_DATA	0xfafafa57			// REG_ADDR_SEQ_MARK 에 의해 복사되는 데이타의 maks, flag 값


// read flags
#define RF_NOSAVE_AS_READ		RF_RD_CMD(0)		// 저장하지 않는다.
#define RF_IGNORE_MASK_AS_READ	RF_RD_CMD(1)		// 저장할때 mask 값을 무시한다.


// write flags
#define RF_OR_MASK_AS_WRITE		RF_WR_CMD(0)		// 복구할때 mask 값을 val 값에 or 하여 실제 레지스터값에 넣는다.
													//    ex)  *addr = val | mask;
#define RF_READ_BACK_AS_WRITE	RF_WR_CMD(1)		// 복구할때 read-back 을 한 후  mask 값을 or  하여 레지스터에 넣는다.
													//    저장된 val 값은 무시된다
#define RF_READ_ONLY			RF_WR_CMD(2)		// 읽기 전용레지스터이므로 복구하지 않는다.

#define	RF_RO_ARMMP				(RF_WR_CMD(0x40 | RF_READ_ONLY)	// ARM MPCore 
													
#define	RF_WR_ARMMP				(RF_WR_CMD(0x80))	// ARM MPCore



// 복구를 위한 reg_t 리스트들을 등록하는 함수
typedef struct
{
	char    name[32];	// 복구 하드웨어의 이름이며 /proc/zbhw/name  으로 사용된다.
	u32     active;		// 복구 유무를 나타낸다
	
	u32     phys_base;	// 물리주소
	u32     phys_size;	// 물리주소 크기
	void   *iomem;		// 가상주소
	reg_t  *regs;		// 복구 데이타 포인터 이다.
	
} zbhw_list_t;

#define REG_DEACTIVE		0
#define REG_ACTIVE			1
#define REG_LATE_ACTIVE		2
#define REG_MUST_ACTIVE		100


#endif // _REG_SAVE_RESTORE_H_

extern int zeroboot_save_reg   (void *mem, reg_t *reg);
extern int zeroboot_restore_reg(void *mem, reg_t *reg);
extern int zeroboot_show_reg   (void *mem, reg_t *reg);

extern void zeroboot_checkin_hw_list( zbhw_list_t *list, u32 count );
extern void zeroboot_checkout_hw_list( void );
extern void zeroboot_save_hw_list( void );
extern void zeroboot_restore_hw_list( void );
extern void zeroboot_restore_late_hw_list( void );
extern void zeroboot_show_hw_list( void );
extern zbhw_list_t *zeroboot_get_hw_list( const char *name );
extern void zeroboot_registe_proc_cmd( char *cmd, int (*func)(char *) );

