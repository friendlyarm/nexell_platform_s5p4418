#ifndef	__NX_DbgMsg_h__
#define	__NX_DbgMsg_h__

#include <stdio.h>		//	printf

#ifndef	DTAG
#define	DTAG	"[Nexell|PLAYER] "
#endif


//	Debug Flags
#define	DBG_MSG				1
#define	VBS_MSG				0
#define	DBG_FUNCTION		0
#define	DEB_MESSAGE			0
#define	ERR_BREAK			0

#if VBS_MSG
#define VbsMsg				g_print
#else
#define	VbsMsg(FORMAT,...)	do{}while(0)
#endif	//	VBS_MSG


#if	DBG_MSG

#define	DbgMsg				g_print
#if ERR_BREAK
#define	ErrMsg				g_error			//	critical error
#else
#define	ErrMsg				g_printerr			//	critical error
#endif


#if	DBG_FUNCTION
#define	FUNC_IN()			g_print("%s() In\n", __func__)
#define	FUNC_OUT()			g_print("%s() Out\n", __func__)
#else
#define	FUNC_IN()			do{}while(0)
#define	FUNC_OUT()			do{}while(0)
#endif	//	DBG_FUNCTION

#else	//	!DBG_MSG

#define	ErrMsg				g_printerr			//	critical error

#define	DbgMsg(FORMAT,...)	do{}while(0)
#define	VbsMsg(FORMAT,...)	do{}while(0)
#define	FUNC_IN()			do{}while(0)
#define	FUNC_OUT()			do{}while(0)

#endif	//	DBG_MSG

#endif	//	__NX_DbgMsg_h__
