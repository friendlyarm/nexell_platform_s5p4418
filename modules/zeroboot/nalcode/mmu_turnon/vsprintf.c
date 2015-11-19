//------------------------------------------------------------------------------
// 파 일 명 : vsprintf.c
// 프로젝트 : ez-jtag
// 설    명 : 표출 출력 포맷 함수 관련 내용
// 작 성 자 : 유영창(frog@falinux.com)
// 작 성 일 : 2007년 5월 6일
// 수 정 일 : 
// 수 정 일 : 
// 주    의 : 
// 라이센스 : BSD
//------------------------------------------------------------------------------
#include <stdio.h>
#include <string.h>

//------------------------------------------------------------------------------
// 파 일 명 : vsprintf.c
// 프로젝트 : ezboot
// 설    명 : ezboot에서 사용하는 표출 출력 포맷 함수 관련 내용
// 작 성 자 : 유영창
// 작 성 일 : 2001년 11월 3일
// 수 정 일 : 
// 주    의 : 이 헤더 화일의 대부분의 내용은 리눅스 쏘스상에 
//            lib/vsprintf.c 에서 가져 왔다. 
//------------------------------------------------------------------------------

//******************************************************************************
// 헤더 정의
//******************************************************************************
#include <stdio.h>
#include <string.h>

//******************************************************************************
//
// 함수 정의
//
//******************************************************************************
//------------------------------------------------------------------------------
// 설명 : 수치형 문자열에 해당하는 정수를 반환한다. 
// 매계 : s : 수치형 문자가 포함된 문자열 주소 
// 반환 : 만들어진 문자열의 길이 
// 주의 : 없음 
//------------------------------------------------------------------------------
static int skip_atoi(const char **s)
{
	int i=0;

	while (isdigit(**s))	i = i*10 + *((*s)++) - '0';
	return i;
}
//------------------------------------------------------------------------------
// 설명 : 수치 표현 포맷 문자열 처리 함수 
// 매계 : 
// 반환 : 
// 주의 : 없음 
//------------------------------------------------------------------------------
static char * number(char * str, long long num, int base, int size, int precision, int type)
{
	char c,sign,tmp[66];
	const char *digits="0123456789abcdefghijklmnopqrstuvwxyz";
	int i;

	if (type & LARGE)
		digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	if (type & LEFT)
		type &= ~ZEROPAD;
	if (base < 2 || base > 36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ';
	sign = 0;
	if (type & SIGN) {
		if (num < 0) {
			sign = '-';
			num = -num;
			size--;
		} else if (type & PLUS) {
			sign = '+';
			size--;
		} else if (type & SPACE) {
			sign = ' ';
			size--;
		}
	}
	if (type & SPECIAL) {
		if (base == 16)
			size -= 2;
		else if (base == 8)
			size--;
	}
	i = 0;
	if (num == 0)
		tmp[i++]='0';
	else while (num != 0)
		tmp[i++] = digits[do_div(num,base)];
	if (i > precision)
		precision = i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT)))
		while(size-->0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type & SPECIAL) {
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	}
	if (!(type & LEFT))
		while (size-- > 0)
			*str++ = c;
	while (i < precision--)
		*str++ = '0';
	while (i-- > 0)
		*str++ = tmp[i];
	while (size-- > 0)
		*str++ = ' ';
	return str;
}

//------------------------------------------------------------------------------
// 설명 : 버퍼에 포맷 문자열에 맞는 문자열을 만든다. 
// 매계 : buf  : 작성된 문자열이 쓰여질 버퍼 
//        fmt  : 포맷 문자열
//        args : 포맷 문자열에 해당하는 매계변수   
// 반환 : 만들어진 문자열의 길이 
// 주의 : 없음 
//------------------------------------------------------------------------------
int vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	unsigned long num;
	int i, base;
	char * str;
	const char *s;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	for (str=buf ; *fmt ; ++fmt) {
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}
			
		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
				}
		
		/* get field width */
		field_width = -1;
		if (isdigit(*fmt))
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			++fmt;
			/* it's the next argument */
			field_width = va_arg(args, int);
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;	
			if (isdigit(*fmt))
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				++fmt;
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		/* default base */
		base = 10;

		switch (*fmt) {
		case 'c':
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			continue;

		case 's':
			s = va_arg(args, char *);
			if (!s)
				s = "<NULL>";

			len = strnlen(s, precision);

			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			continue;

		case 'p':
			if (field_width == -1) {
				field_width = 2*sizeof(void *);
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			continue;


		case 'n':
			if (qualifier == 'l') {
				long * ip = va_arg(args, long *);
				*ip = (str - buf);
			} else {
				int * ip = va_arg(args, int *);
				*ip = (str - buf);
			}
			continue;

		/* integer number formats - set up the flags and "break" */
		case 'o':
			base = 8;
			break;

		case 'X':
			flags |= LARGE;
		case 'x':
			base = 16;
			break;

		case 'd':
		case 'i':
			flags |= SIGN;
		case 'u':
			break;

		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			continue;
		}
		if (qualifier == 'l')
			num = va_arg(args, unsigned long);
		else if (qualifier == 'h') {
			if (flags & SIGN)
				num = va_arg(args, short);
			else
				num = va_arg(args, unsigned short);
		} else if (flags & SIGN)
			num = va_arg(args, int);
		else
			num = va_arg(args, unsigned int);
		str = number(str, num, base, field_width, precision, flags);
	}
	*str = '\0';
	return str-buf;
}
//------------------------------------------------------------------------------
// 설명 : 버퍼에 포맷 문자열에 맞는 문자열을 만든다. 
// 매계 : buf  : 작성된 문자열이 쓰여질 버퍼 
//        fmt  : 포맷 문자열
//        args : 포맷 문자열에 해당하는 매계변수   
// 반환 : 만들어진 문자열의 길이 
// 주의 : 없음 
//------------------------------------------------------------------------------
int sprintf(char * buf, const char *fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i=vsprintf(buf,fmt,args);
	va_end(args);
	return i;
}
