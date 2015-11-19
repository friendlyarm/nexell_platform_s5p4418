//================================================================================================
/**
@file : typedef.h
@project: 
@data : 2007년 4월 27일
@author : 유영창
@brief : ezboot에서 사용하는 기본 타입을 정의한다.  
@version :
@todo :
@remark :  
@warning :
@modifylnfo :
*///==================================================================================================

#ifndef _TYPEDEF__HEADER_
#define _TYPEDEF__HEADER_

//------------------------------------------------------------------------------
/**
@brief : 기본형에 대한 정의
*///------------------------------------------------------------------------------

#define	KB			(1024)
#define	MB			(1024*1024)

#ifndef __ASSEMBLY__

typedef signed 		char 	s8;
typedef unsigned 	char 	u8;
typedef unsigned 	char 	uint8_t;

typedef signed 		short 	s16;
typedef unsigned 	short 	u16;
typedef unsigned 	short 	uint16_t;

typedef signed 		int 	s32;
typedef unsigned 	int 	u32;
typedef unsigned 	int 	uint32_t;

typedef signed 		long 	s64;
typedef unsigned 	long 	u64;
typedef unsigned 	long 	uint64_t;

typedef u64 				dma_addr_t;
typedef u64 				dma64_addr_t;

typedef unsigned 	int BOOL;
typedef unsigned 	int bool;

typedef unsigned 	short	ushort;
typedef unsigned 	int		uint;
typedef unsigned 	long 	ulong;

#define FALSE		0
#define TRUE		-1 
#define false		FALSE
#define true		TRUE
#define NULL		((void *)0)

#define	BUS_8		1		// 값을 변경하지 말것
#define	BUS_16		2		// 값을 변경하지 말것
#define	BUS_32		4		// 값을 변경하지 말것

#define SIZE_ALIGN(x,a) 	((((x)+((a)-1))/(a))*(a))

#define __swab16(x) \
          ((u16)( \
                (((u16)(x) & (u16)0x00ffU) << 8) | \
                (((u16)(x) & (u16)0xff00U) >> 8) ))
#define __swab32(x) \
          ((u32)( \
                (((u32)(x) & (u32)0x000000ffUL) << 24) | \
                (((u32)(x) & (u32)0x0000ff00UL) <<  8) | \
                (((u32)(x) & (u32)0x00ff0000UL) >>  8) | \
                (((u32)(x) & (u32)0xff000000UL) >> 24) ))

#define __be32_to_cpu(x)	__swab32((x))
#define __be16_to_cpu(x)	__swab16((x))

#endif // not __ASSEMBLY__ 

//------------------------------------------------------------------------------
/**
@brief : 레지스터 명에 대한 정의
*///------------------------------------------------------------------------------
#ifndef __ASSEMBLY__

	#define 	__io_p2v(paddr)   (paddr)						//   @FIXME 나중에 MMU 도입후 제대로 지원할 것
	#define 	__io_v2p(vaddr)   (vaddr)						//   @FIXME 나중에 MMU 도입후 제대로 지원할 것

	#define 	__REG(x)	(*((volatile u32 *)__io_p2v(x)))
	#define 	__REG_B(x)	(*((volatile u8 *) __io_p2v(x)))
	#define 	__REG2(x,y)	(*(volatile u32 *)((u32)&__REG(x) + (y)))

#else

  	#define 	__REG(x) 	(x)
	#define 	__REG_B(x)	(x) 
  	#define 	__REG2(x) 	(x)

#endif


#define FUNC            		printf( "TRACE <%s %d>\n", __FUNCTION__, __LINE__ )
#define TRACE            		printf( "TRACE <%s %d>\n", __FILE__, __LINE__ )
#define tprintf(fmt,args...)  	printf( "TRACE <%s:%d> " fmt, __FILE__,__LINE__, ## args )

#endif // _TYPEDEF__HEADER_
