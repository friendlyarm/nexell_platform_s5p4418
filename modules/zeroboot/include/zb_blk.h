/*-----------------------------------------------------------------------------
  파 일 : zb_blk.h
  설 명 : 
  작 성 : boggle70@falinux.com
  날 짜 : 2012-01-4
  주 의 :

-------------------------------------------------------------------------------*/

#ifndef __ZB_BLK_H__
#define __ZB_BLK_H__

#define ZB_PAGE_INVALID		0xffffffff
#define ZB_PAGE_VALID		0

// zb 에서 사용하는 페이지 자료는 인덱스 베이스로 부터
// 오프셋값을 디바이스로 부터 얻어 사용하게 된다.
////////////////////////////////////////
// 데이타 저장 디바이스 API 구조
////////////////////////////////////////

struct zblk_ops {
	void				*priv;								// 디바이스에서 사용하는 내부 자료 이 자료형은 device 내부에서만 사용한다.

	int					(*probe)		(void);				// 저장할 디바이스가 있는지 확인 
	int					(*setup_first)	(void);				// 현재의 낸드 방식처럼 사용전 모두 지우거나 빈 함수로 채워도 무방하다
	int					(*setup_post_zeroboot)(void);		// 제로부트 이후에 할일

	u32					(*get_storage_offset)	(void);		// 저장 디바이스의 offset 주소를 구한다
	u32					(*get_io_vaddr)	(void);				// 디바이스의 virtual 주소를 구하는 함수
	u32					(*get_index)	(u32 idx);			// 사용가능한 인덱스를 하나 얻는 함수
	
	int					(*page_write)	(u32 page, u8 *buf);	// 4K page 를 디바이스에 쓰는 함수
	int					(*page_read)	(u32 page, u8 *buf);	// 4K page 를 디바이스에서 읽는 함수
	

	// 옵셔널 함수, 현재는 사용하지 않는다
	int					(*burst_write)	(u32 page, u8 *buf, u32 size);		// 주어진 size 만큼 디바이스에 연속으로 쓰는 함수
	int					(*burst_read)	(u32 page, u8 *buf, u32 size);		// 주어진 size 만큼 디바이스에서 연속으로 읽는 함수

	// storage dev reset api
	int					(*reset)		(int);

	// 임시 함수

};

extern struct zblk_ops zops;

extern int zblk_probe(void);
extern int zblk_setup_first(void);
extern int zblk_setup_post_zeroboot(void);

extern u32 zblk_get_storage_offset(void);
extern u32 zblk_get_io_vaddr(void);
extern u32 zblk_get_index(u32 page);

extern int zblk_page_write(u32 page, u8 *buf);
extern int zblk_page_read(u32 page, u8 *buf);

extern int zblk_burst_read(u32 page, u8 *buf, u32 size);
extern int zblk_burst_write(u32 page, u8 *buf, u32 size);

extern int	zb_blk_init(void);
extern int zb_blk_exit(void);
extern struct zblk_ops *get_zb_blk_info(void);

#if 0
extern u32 zb_nand_get_io_vaddr     ( void );

extern int zb_nand_detect           ( void );
extern int zb_nand_get_page_size    ( void );
extern int zb_nand_is_bad_block     ( u32 block_start_page );
extern int zb_nand_is_bad_page      ( u32 page );

extern int zb_nand_erase            ( u32 block_start_page );
extern int zb_nand_write_page       ( u32 page,  u8 *buf );
extern int zb_nand_read_page        ( u32 page,  u8 *buf );

extern int zb_nand_all_erase        ( void );
extern int zb_nand_write_page_of_mem( u32 nand_page,  u8 *page_buf );
extern int zb_nand_read_page_of_mem ( u32 nand_page,  u8 *page_buf );
#endif


#endif  // __ZB_BLK_H__

