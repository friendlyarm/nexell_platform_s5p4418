//------------------------------------------------------------------------------
// 파 일 명 : typedef.h
// 설    명 : ezboot에서 사용하는 기본 타입을 정의한다. 
// 작 성 자 : 유영창
// 작 성 일 : 2007년 4월 27일
// 수 정 일 : 
//------------------------------------------------------------------------------

#ifndef _TYPEDEF__HEADER_
#define _TYPEDEF__HEADER_

//
// 기본형에 대한 정의 
//
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

#endif // __ASSEMBLY__ 

#define FUNC            		printf( "TRACE <%s %d>\n", __FUNCTION__, __LINE__ )
#define TRACE            		printf( "TRACE <%s %d>\n", __FILE__, __LINE__ )
#define tprintf(fmt,args...)  	printf( "TRACE <%s:%d> " fmt, __FILE__,__LINE__, ## args )

#endif // _TYPEDEF__HEADER_
