//------------------------------------------------------------------------------
// 파 일 명 : string.h
// 설    명 : ezboot에서 사용하는 문자열 처리 관련 헤더 
// 작 성 자 : 유영창
// 작 성 일 : 2001년 11월 3일
// 수 정 일 : 
// 주    의 : 이 헤더 화일의 대부분의 내용은  리눅스 string.h에서
//            에서 가져 왔다. 
//------------------------------------------------------------------------------

#ifndef _STRING_HEADER_
#define _STRING_HEADER_

#ifndef __MODULE__

extern char * ___strtok;
extern char * strpbrk(const char *,const char *);
extern char * strtok(char *,const char *);
extern char * strsep(char **,const char *);
extern __kernel_size_t strspn(const char *,const char *);
extern char * strcpy(char *,const char *);
extern char * strncpy(char *,const char *, __kernel_size_t);
extern char * strcat(char *, const char *);
extern char * strncat(char *, const char *, __kernel_size_t);
extern int strcmp(const char *,const char *);
extern int strncmp(const char *,const char *,__kernel_size_t);
extern int strnicmp(const char *, const char *, __kernel_size_t);
extern char * strchr(const char *,int);
extern char * strrchr(const char *,int);
extern char * strstr(const char *,const char *);
extern __kernel_size_t strlen(const char *);
extern __kernel_size_t strnlen(const char *,__kernel_size_t);
extern void * memset(void *,int,__kernel_size_t);
extern void * memcpy(void *,const void *,__kernel_size_t);
extern void * memmove(void *,const void *,__kernel_size_t);
extern void * memscan(void *,int,__kernel_size_t);
extern int memcmp(const void *,const void *,__kernel_size_t);
extern void * memchr(const void *,int,__kernel_size_t);
extern unsigned long strtoul(const char *p, char **out_p, int base);

extern void upper_str( char *Str );
extern void lower_str( char *Str );

#else

#define strpbrk    call._strpbrk   
#define strtok     call._strtok    
#define strsep     call._strsep    
#define strspn     call._strspn    
#define strcpy     call._strcpy    
#define strncpy    call._strncpy   
#define strcat     call._strcat    
#define strncat    call._strncat   
#define strcmp     call._strcmp    
#define strncmp    call._strncmp   
#define strnicmp   call._strnicmp  
#define strchr     call._strchr    
#define strrchr    call._strrchr   
#define strstr     call._strstr    
#define strlen     call._strlen    
#define strnlen    call._strnlen   
#define memset     call._memset    
#define memcpy     call._memcpy    
#define memmove    call._memmove   
#define memscan    call._memscan   
#define memcmp     call._memcmp    
#define memchr     call._memchr    
#define strtoul    call._strtoul   
#define upper_str  call._upper_str 
#define lower_str  call._lower_str 

#endif

#endif // _STRING_HEADER_

