#ifndef __dbgmsg_h__
#define __dbgmsg_h__


#if 0

#ifdef ANDROID
#include <cutils/log.h>

#define DbgMsg	ALOGD 
#define ErrMsg 	ALOGE 

#else	//	Linux
#include <stdio.h>

#define DbgMsg	printf
#define ErrMsg 	printf

#endif

#else	//	#if 1

#define DbgMsg	printf
#define ErrMsg 	printf

#endif

#endif // __dbgmsg_h__

