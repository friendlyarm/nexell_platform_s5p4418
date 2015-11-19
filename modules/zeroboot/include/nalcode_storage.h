/*-----------------------------------------------------------------------------
  파 일 : nalcode_stroage.h
  설 명 : 
  작 성 : freefrug@falinux.com
  날 짜 : 2012-01-11
  주 의 :

	linux/mtd/nand/nand_base.c 에서 사용될 때는 이 헤더파일을 해당 디렉토리에 
	복사하여 사용한다.
-------------------------------------------------------------------------------*/
#ifndef __NALCODE_STROAGE_H__
#define __NALCODE_STROAGE_H__

#if 1
	#include <zb_app_inf.h>					// 커널소스에 복사하여 사용할 경우 이 라인을 지우고 아래의 내용을 활성화 시킨다.
#else  
	// <제로부트 상태>
	enum {
		ZB_STATUS_NORMALBOOT                = 0,
		ZB_STATUS_SNAPSHOT_READY            = 10,
		ZB_STATUS_SNAPSHOT_DONE             = 20,
		ZB_STATUS_ZEROBOOT                  = 30,
		ZB_STATUS_ZEROBOOT_RECOVERY_DONE    = 40,
	};
	
	// <저장소 상태>
	enum {
		STORAGE_ACCESS_MARK_DONE			= 0,
		STORAGE_ACCESS_MARK_KERNEL			= 1,
		STORAGE_ACCESS_MARK_KERNEL_ERASE	= 2,
		STORAGE_ACCESS_MARK_KERNEL_WRITE	= 3,
		STORAGE_ACCESS_MARK_NALCODE			= 0xfa,
	};
#endif

#ifndef MODULE							// 모듈일경우 참조되지 않는다.

//-----------------------------------------------------------------------------
#ifndef	__KERNEL__

#include <zbi.h>
#define ZBI_INDEX_ZB_STATUS				(2048-4)	// zbi->align_2048[2044] 데이타
#define ZBI_INDEX_STORAGE_ACCESS_FLAG	(2048-3)	// zbi->align_2048[2047] 데이타

static zbi_t  		*zbi_head = (zbi_t *)(ZBI_VADDR);
#define NAL_2_NAND	 zbi_head->align_2048[ZBI_INDEX_STORAGE_ACCESS_FLAG]

extern void nalcode_stroage_read_4k_page( u32 nand_page, u8 *mem_buf, void *vaddr , unsigned int addr);

typedef struct
{
	u8	align_2048[2048];
	
} simple_zbi_t;

#define BUF_ZEROBOOT_MAGIC		(*(unsigned int *)(zbi_head->align_2048))
#define BUF_STORAGE_ACCESS		zbi_head->align_2048[ZBI_INDEX_STORAGE_ACCESS_FLAG]
#define BUF_ZEROBOOT_STATUS		zbi_head->align_2048[ZBI_INDEX_ZB_STATUS]


static inline void zb_storage_access_enter_mark( u8 mark_type )
{
	BUF_STORAGE_ACCESS = mark_type;
}


//-----------------------------------------------------------------------------
#else									// zbi.h 파일의 내용중 발췌된 것이다
														
#define ZEROBOOT_MAGIC					(0xFA0000B2)
#define ZBI_HEADER_VADDR				(PAGE_OFFSET + 0x00008000)

#define ZBI_INDEX_ZB_STATUS				(2048-4)	// zbi->align_2048[2044] 데이타
#define ZBI_INDEX_STORAGE_ACCESS_FLAG	(2048-3)	// zbi->align_2048[2047] 데이타

typedef struct
{
	u8	align_2048[2048];
	
} simple_zbi_t;

static simple_zbi_t *zbi_head = (simple_zbi_t *)(ZBI_HEADER_VADDR);	
#define BUF_ZEROBOOT_MAGIC		(*(unsigned int *)(zbi_head->align_2048))
#define BUF_STORAGE_ACCESS		zbi_head->align_2048[ZBI_INDEX_STORAGE_ACCESS_FLAG]
#define BUF_ZEROBOOT_STATUS		zbi_head->align_2048[ZBI_INDEX_ZB_STATUS]

#if 0
  #define IF_ZEROBOOT_DBG(fmt,args...)	if ( BUF_ZEROBOOT_MAGIC == ZEROBOOT_MAGIC ) printk( fmt, ## args )
  #define ZEROBOOT_DBG(fmt,args...)		printk( fmt, ## args )
  #define ZEROBOOT_WRITE_VERIFY			
#else
  #define IF_ZEROBOOT_DBG(fmt,args...)	do { } while(0)
  #define ZEROBOOT_DBG(fmt,args...)		do { } while(0)
  #undef  ZEROBOOT_WRITE_VERIFY			
#endif

static inline void zb_storage_access_enter_mark( u8 mark_type )
{
	BUF_STORAGE_ACCESS = mark_type;
}

static inline int zb_storage_is_dirty( u8 mark_type, int page )
{
	int rtn = 0;
	
	if ( BUF_STORAGE_ACCESS != mark_type ) // NAND_ACCESS_MARK_NALCODE
	{
		ZEROBOOT_DBG( " ..nand dirty type=%d [%02x] [%08x] page=%d\n", mark_type, BUF_STORAGE_ACCESS, ZEROBOOT_MAGIC, page );
		//if ( BUF_ZEROBOOT_MAGIC == ZEROBOOT_MAGIC ) printk ( " ..nand dirty type=%d [%02x] [%08x]\n", mark_type, BUF_STORAGE_ACCESS, ZEROBOOT_MAGIC );
		rtn = -1;
		//BUF_STORAGE_ACCESS = STORAGE_ACCESS_MARK_KERNEL;	
		//printk( "    b=%02x\n", STORAGE_ACCESS_MARK );
	}
	else
	{
		//IF_ZEROBOOT_DBG( " ..%d:%d ok\n", mark_type, BUF_STORAGE_ACCESS );
	}

	BUF_STORAGE_ACCESS = STORAGE_ACCESS_MARK_DONE;

	return rtn;
}

#define zbstorage_enter_loop(j,m)			label_zbstorage_enter_##j:   zb_storage_access_enter_mark(m)
#define if_zbstorage_is_clean_exit(j,m,p)		if ( zb_storage_is_dirty(m,p) ) goto label_zbstorage_enter_##j

// 정상상태라면 0 아니면 -1
//     ECC 복구 메세지를 보이지 않도록 하기위해 함수, export 한다.
static int check_zeroboot_state_normal( void )
{
	//if ( ZB_STATUS_NORMALBOOT == BUF_ZEROBOOT_STATUS ) return 0;
	return -1;
}
EXPORT_SYMBOL(check_zeroboot_state_normal);

#endif	// __KERNEL__
//-----------------------------------------------------------------------------
#endif  // MODULE

#endif  // __NALCODE_STROAGE_H__

