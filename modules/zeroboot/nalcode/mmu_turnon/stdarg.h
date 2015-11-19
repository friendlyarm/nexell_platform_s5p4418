//------------------------------------------------------------------------------
// 파 일 명 : stdarg.h
// 설    명 : ezboot에서 사용하는 C 프로그램의 가변인자에 대한 정의와 관련된 내용
// 작 성 자 : 유영창
// 작 성 일 : 2001년 11월 3일
// 수 정 일 : 
// 주    의 : 이 헤더 화일의 대부분의 내용은 
//            blob의 serial.h에서 가져 왔다. 
//            에서 가져 왔다. 
//------------------------------------------------------------------------------

#ifndef _STDARG_HEADER_
#define _STDARG_HEADER_

typedef char *va_list;

#define va_start(ap, p)		(ap = (char *) (&(p)+1))
#define va_arg(ap, type)	((type *) (ap += sizeof(type)))[-1]
#define va_end(ap)			((void)0)

#endif // _STDARG_HEADER_

