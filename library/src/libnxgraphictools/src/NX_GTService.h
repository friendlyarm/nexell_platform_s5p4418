//------------------------------------------------------------------------------
//
//	Copyright (C) 2013 Nexell Co. All Rights Reserved
//	Nexell Co. Proprietary & Confidential
//
//	NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND	WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//	Module		:
//	File		:
//	Description	:
//	Author		: 
//	Export		:
//	History		:
//
//------------------------------------------------------------------------------

#ifndef __NX_GTSERVICE_H__
#define __NX_GTSERVICE_H__

#include <stdint.h>
#include <nx_alloc_mem.h>

enum {
	GT_SERVICE_ID_DEINTERLACE	= 0,
	GT_SERVICE_ID_SCALER		= 1,
	CT_SERVICE_ID_RGB2YUV		= 2,	//	RGB32 to NV12
	CT_SERVICE_ID_YUV2RGB		= 3,	//	YUV to RGB32
};

enum {
	GT_SERVICE_CMD_OPEN			= 0,
	GT_SERVICE_CMD_DO			= 1,
	GT_SERVICE_CMD_CLOSE		= 2,
};

//typedef void*		NX_GT_SERVICE_HANDLE;
typedef struct tag_NX_GT_SERVICE_INFO * NX_GT_SERVICE_HANDLE;

typedef struct tag_NX_GT_PARAM_OPEN {
	//	Input Parameters
	int32_t		srcWidth;
	int32_t		srcHeight;
	int32_t		dstWidth;
	int32_t		dstHeight;
	int32_t		maxInOutBufSize;
	//	Output Parameters
	void		*handle;
} NX_GT_PARAM_OPEN;

typedef struct tag_NX_GT_PARAM_DO {
	void 	*handle;		//	context handler
	void 	*pInBuf;
	void	*pOutBuf;
} NX_GT_PARAM_DO;

typedef struct tag_NX_GT_PARAM_CLOSE {
	void 				*handle;		//	context handler
} NX_GT_PARAM_CLOSE; 

#ifdef __cplusplus
extern "C" {
#endif	//	__cplusplus

// NX_GTServiceInit() -> NX_GTServiceStart() -> NX_GTServiceCommand() -> ... -> NX_GTServiceCommand() -> NX_GTServiceStop() -> NX_GTServiceDeinit()

NX_GT_SERVICE_HANDLE	NX_GTServiceInit	( void );
void					NX_GTServiceDeinit	( NX_GT_SERVICE_HANDLE hService );
int32_t					NX_GTServiceStart	( NX_GT_SERVICE_HANDLE hService );
int32_t					NX_GTServiceStop	( NX_GT_SERVICE_HANDLE hService );
int32_t 				NX_GTServiceCommand( NX_GT_SERVICE_HANDLE hService, int32_t serviceID, int32_t serviceCmd, void *serviceData );

#ifdef __cplusplus
};
#endif	//	__cplusplus

#endif	// __NX_GTSERVICE_H__