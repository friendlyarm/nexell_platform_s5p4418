//------------------------------------------------------------------------------
//
//  Copyright (C) 2013 Nexell Co. All Rights Reserved
//  Nexell Co. Proprietary & Confidential
//
//  NEXELL INFORMS THAT THIS CODE AND INFORMATION IS PROVIDED "AS IS" BASE
//  AND WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING
//  BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS
//  FOR A PARTICULAR PURPOSE.
//
//  Module      :
//  File        :
//  Description :
//  Author      : 
//  Export      :
//  History     :
//
//------------------------------------------------------------------------------

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "NX_Queue.h"
#include "NX_Semaphore.h"
#include "NX_GTService.h"

#include <nx_fourcc.h>

#include "vr_deinterlace.h"
#include "nx_graphictools.h"

#include "dbgmsg.h"

#define MAX_SERVICE_NUM		6
#define MAX_CMD_SIZE		MAX_SERVICE_NUM + 1

////////////////////////////////////////////////////////////////////////////////
//
// Debugging tools
//
//#define DEBUG

#ifdef DEBUG
#include <sys/time.h>

static uint64_t gNxStartTime   = 0;
static uint64_t gNxStopTime    = 0;

uint64_t NX_GetTickCount( void )
{
	uint64_t ret;
	struct timeval  tv;
	struct timezone zv;
	gettimeofday( &tv, &zv );
	ret = ((uint64_t)tv.tv_sec)*1000 + tv.tv_usec/1000;
	return ret;
}

#define NxDbgCheckTimeStart()       gNxStartTime = NX_GetTickCount()
#define NxDbgCheckTimeStop(A)       do {												\
										gNxStopTime = NX_GetTickCount();				\
										ErrMsg("%s Process Time : %lld msec\n",			\
										A, (uint64_t)(gNxStopTime - gNxStartTime));		\
									} while(0)
#else
#define NxDbgCheckTimeStart()		do {} while(0)
#define NxDbgCheckTimeStop(A)       do {} while(0)
#endif

////////////////////////////////////////////////////////////////////////////////
//
// Debug Function
//
#define	ALIGN(X,N)	( (X+N-1) & (~(N-1)) )

static void mem_save_to_file(char* pbuf, const char *filename, unsigned int width, unsigned int height)
{
	char sName[256];
	FILE* fp_deinterlace;
	unsigned int bytes_size;

	bytes_size = ALIGN(width, 16) * ( ALIGN(height, 16) + ALIGN(height, 16) / 2 );

	if(!bytes_size)
	{
		printf("can't save image, bytes_size is zero!!!\n");
		return;
	}
	
	sprintf(sName, "./rst_%s_%dx%d.yuv", filename, width, height);
	
	fp_deinterlace = fopen(sName, "w");	
	fwrite(pbuf, bytes_size , 1, fp_deinterlace);
	printf("mem_save_to_file(0x%x, %dx%d)\n", bytes_size, width, height);
	fflush(fp_deinterlace);		
	fclose(fp_deinterlace);
}


////////////////////////////////////////////////////////////////////////////////
//
// Graphic tools engine
//



////////////////////////////////////////////////////////////////////////////////
//
//	Deinterlace Service Routine
//
struct NX_GT_DEINT_INFO {
	int32_t isInit;
	int32_t srcWidth;
	int32_t srcHeight;
	int32_t dstWidth;
	int32_t dstHeight;

	int32_t maxInOutBufSize;
	int32_t isScaleMode;

	NX_VID_MEMORY_INFO inputImage[MAX_GRAPHIC_BUF_SIZE];	//	Input Image
	HSURFSOURCE hInSurface[MAX_GRAPHIC_BUF_SIZE];			//	Input Image Surface

	NX_VID_MEMORY_INFO outputImage[MAX_GRAPHIC_BUF_SIZE];	//	Output Image
	HSURFTARGET hOutSurface[MAX_GRAPHIC_BUF_SIZE];			//	Output Image Surface
};

static void *GTServiceDeintOpen( NX_GT_PARAM_OPEN *pParam )
{
	NX_GT_DEINT_HANDLE handle = (NX_GT_DEINT_HANDLE)malloc(sizeof(struct NX_GT_DEINT_INFO));

	if( handle )
	{
		memset( handle, 0, sizeof(struct NX_GT_DEINT_INFO) );
		handle->isInit		= 1;
		handle->srcWidth	= pParam->srcWidth;
		handle->srcHeight	= pParam->srcHeight;
		handle->dstWidth	= pParam->srcWidth;
		handle->dstHeight	= pParam->srcHeight;
		handle->maxInOutBufSize = pParam->maxInOutBufSize;
		pParam->handle      = handle;
	}
	return (void*)handle;
}

static int32_t GTServiceDeintDoDeinterlace(NX_GT_PARAM_DO *pParam)
{
	int32_t i, ret = ERR_NONE;
	HSURFSOURCE hInSurface = NULL;
	HSURFTARGET hOutSurface = NULL;
	NX_GT_DEINT_HANDLE handle = (NX_GT_DEINT_HANDLE)pParam->handle;
	NX_VID_MEMORY_INFO *pInBuf = (NX_VID_MEMORY_INFO *)pParam->pInBuf;
	NX_VID_MEMORY_INFO *pOutBuf = (NX_VID_MEMORY_INFO *)pParam->pOutBuf;

	if( !handle || !handle->isInit )
	{
		return ERR_NOT_INIT;
	}

	//	Check Input Buffer in Cache
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->inputImage[i].luPhyAddr == pInBuf->luPhyAddr )
		{
			hInSurface = handle->hInSurface[i];
			break;
		}
	}
	if( hInSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->inputImage[i].luPhyAddr )
			{
				hInSurface = handle->hInSurface[i] = vrCreateDeinterlaceSource( pInBuf->luStride, handle->srcWidth, handle->srcHeight, (NX_MEMORY_HANDLE)pInBuf->privateDesc[0] );
				if( hInSurface == NULL )
				{
					if( hInSurface )		vrDestroyDeinterlaceSource( hInSurface );
					ret = ERR_SURFACE_CREATE;
					goto ErrorExit;
				}
				handle->inputImage[i] = *pInBuf;
				break;
			}
		}
	}

	//	Check Output Buffer in Cache
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->outputImage[i].luPhyAddr == pOutBuf->luPhyAddr )
		{
			hOutSurface = handle->hOutSurface[i];
			break;
		}
	}
	if( hOutSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->outputImage[i].luPhyAddr )
			{
				hOutSurface = handle->hOutSurface[i] = vrCreateDeinterlaceTarget( pOutBuf->luStride, handle->srcWidth, handle->srcHeight, (NX_MEMORY_HANDLE)pOutBuf->privateDesc[0], 0 );
				if( hOutSurface == NULL )
				{
					if( hOutSurface )	vrDestroyDeinterlaceTarget( hOutSurface, 0 );
					ret = ERR_SURFACE_CREATE;
					goto ErrorExit;
				}
				handle->outputImage[i] = *pOutBuf;
				break;
			}
		}
	}

	if( NULL==hOutSurface || NULL==hInSurface )
	{
		ret = ERR_BUF_SIZE;
		goto ErrorExit;
	}

	//	Deinterlace
	vrRunDeinterlace(hOutSurface, hInSurface);

	return ret;

ErrorExit:
	return ret;
}

static void GTServiceDeintClose(NX_GT_PARAM_CLOSE *pParam)
{
	int i;
	NX_GT_DEINT_HANDLE handle = (NX_GT_DEINT_HANDLE)pParam->handle;
	if( handle )
	{
		if( handle->isInit )
		{
			for( i = 0 ; i < handle->maxInOutBufSize ; i++)
			{
				if( handle->inputImage[i].luPhyAddr )
				{
					vrDestroyDeinterlaceSource( handle->hInSurface[i] );
				}
				if( handle->outputImage[i].luPhyAddr )
				{
					vrDestroyDeinterlaceTarget( handle->hOutSurface[i], 0 );
				}
			}
		}
		free( handle );
	}
}
//
//	End of Deinterlace Service
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
//	Scaler Service
//
struct NX_GT_SCALER_INFO {
	int32_t isInit;
	int32_t srcWidth;
	int32_t srcHeight;
	int32_t dstWidth;
	int32_t dstHeight;
	int32_t maxInOutBufSize;

	NX_VID_MEMORY_INFO inputImage[MAX_GRAPHIC_BUF_SIZE];	//	Input Image
	HSURFSOURCE hInSurface[MAX_GRAPHIC_BUF_SIZE];			//	Input Image Surface

	NX_VID_MEMORY_INFO outputImage[MAX_GRAPHIC_BUF_SIZE];	//	Target Output Buffer
	HSURFTARGET hOutSurface[MAX_GRAPHIC_BUF_SIZE];			//	Target Output Surface
};

static void *GTServiceSclOpen( NX_GT_PARAM_OPEN *pParam )
{
	NX_GT_SCALER_HANDLE handle = (NX_GT_SCALER_HANDLE)malloc(sizeof(struct NX_GT_SCALER_INFO));
	
	if( handle )
	{
		memset( handle, 0, sizeof(struct NX_GT_SCALER_INFO) );
		handle->isInit		= 1;
		handle->srcWidth	= pParam->srcWidth;
		handle->srcHeight	= pParam->srcHeight;
		handle->dstWidth	= pParam->dstWidth;
		handle->dstHeight	= pParam->dstHeight;
		handle->maxInOutBufSize = pParam->maxInOutBufSize;
		pParam->handle 		= handle;
	}

	return (void*)handle;
}

static int32_t GTServiceSclDoScale(NX_GT_PARAM_DO *pParam )
{
	int32_t i;
	HSURFSOURCE hInSurface = NULL;
	HSURFTARGET hOutSurface = NULL;
	NX_GT_SCALER_HANDLE handle = (NX_GT_SCALER_HANDLE)pParam->handle;

	NX_VID_MEMORY_INFO *pInBuf = (NX_VID_MEMORY_INFO *)pParam->pInBuf;
	NX_VID_MEMORY_INFO *pOutBuf = (NX_VID_MEMORY_INFO *)pParam->pOutBuf;

	if( !handle || !handle->isInit )
	{
		return -1;
	}

	//	Check surface
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->inputImage[i].luPhyAddr == pInBuf->luPhyAddr )
		{
			hInSurface = handle->hInSurface[i];
			break;
		}
	}
	//	Create Input(Source) Surface
	if( hInSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->inputImage[i].luPhyAddr )
			{
				hInSurface = handle->hInSurface[i] = vrCreateScaleSource( pInBuf->luStride, handle->srcWidth, handle->srcHeight, (NX_MEMORY_HANDLE)pInBuf->privateDesc[0] );
				handle->inputImage[i] = *pInBuf;
				break;
			}
		}
	}

	//	Check output surface
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->outputImage[i].luPhyAddr == pOutBuf->luPhyAddr )
		{
			hOutSurface = handle->hOutSurface[i];
			break;
		}
	}
	//	Create Output Surface
	if( hOutSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->outputImage[i].luPhyAddr )
			{
				hOutSurface = handle->hOutSurface[i] = vrCreateScaleTarget( pOutBuf->luStride, handle->dstWidth, handle->dstHeight, (NX_MEMORY_HANDLE)pOutBuf->privateDesc[0], 0 );
				handle->outputImage[i] = *pOutBuf;
				break;
			}
		}
	}

	//	Scale Image
	// static void mem_save_to_file(char* pbuf, const char *filename, unsigned int width, unsigned int height)
	//mem_save_to_file( (char*)pInBuf->luVirAddr, "test", handle->srcWidth, handle->srcHeight);

	vrRunScale(hOutSurface, hInSurface);
	//mem_save_to_file( (char*)pOutBuf->luVirAddr, "test", pOutBuf->luStride, handle->dstHeight);

	return 0;
}

static void GTServiceSclClose(NX_GT_PARAM_CLOSE *pParam)
{
	int i;
	NX_GT_SCALER_HANDLE handle = (NX_GT_SCALER_HANDLE)pParam->handle;

	if( handle )
	{
		if( handle->isInit )
		{
			for( i = 0 ; i < handle->maxInOutBufSize ; i++)
			{
				if( handle->inputImage[i].luPhyAddr )
				{
					vrDestroyScaleSource( handle->hInSurface[i] );
				}
				if( handle->outputImage[i].luPhyAddr )
				{
					vrDestroyScaleTarget( handle->hOutSurface[i], 0 );
				}
			}
		}
		free( handle );
	}
}
//
//			End of Scaler Service
//
////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////
//
//	RGB to YUV Service
//	Input  : RGB Surface's Ion Handle
//	OUtput : NX_VID_MEMORY_INFO
//
struct NX_GT_RGB2YUV_INFO {
	int32_t isInit;
	int32_t srcWidth;
	int32_t srcHeight;
	int32_t dstWidth;
	int32_t dstHeight;
	int32_t maxInOutBufSize;

	int					inputFd[MAX_GRAPHIC_BUF_SIZE];		//	Input Image
	HSURFSOURCE	hInSurface[MAX_GRAPHIC_BUF_SIZE];			//	RGB Source Surface, 0(Y Source) 1(UV Source)

	NX_VID_MEMORY_INFO	outputImage[MAX_GRAPHIC_BUF_SIZE];	//	Target Output Buffer
	HSURFTARGET	hOutSurface[MAX_GRAPHIC_BUF_SIZE];			//	Target YUV Surface, 0(Y Target) 1(UV Target)
};

static void *GTServiceRgb2YuvOpen( NX_GT_PARAM_OPEN *pParam )
{
	NX_GT_RGB2YUV_HANDLE handle = (NX_GT_RGB2YUV_HANDLE)malloc(sizeof(struct NX_GT_RGB2YUV_INFO));
	
	if( handle )
	{
		memset( handle, 0, sizeof(struct NX_GT_RGB2YUV_INFO) );
		handle->isInit		= 1;
		handle->srcWidth	= pParam->srcWidth;
		handle->srcHeight	= pParam->srcHeight;
		handle->dstWidth	= pParam->dstWidth;
		handle->dstHeight	= pParam->dstHeight;
		handle->maxInOutBufSize = pParam->maxInOutBufSize;
		pParam->handle 		= handle;
	}

	return (void*)handle;
}

static int32_t GTServiceRgb2YuvDoConvert(NX_GT_PARAM_DO *pParam )
{
	int32_t i;
	HSURFSOURCE hInSurface = NULL;
	HSURFTARGET hOutSurface = NULL;
	NX_GT_RGB2YUV_HANDLE handle = (NX_GT_RGB2YUV_HANDLE)pParam->handle;

	int inputFd  = (int)pParam->pInBuf;
	NX_VID_MEMORY_INFO *pOutBuf = (NX_VID_MEMORY_INFO *)pParam->pOutBuf;

	if( !handle || !handle->isInit )
	{
		return -1;
	}

	//	Check Input ION File Descriptor
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->inputFd[i] == inputFd )
		{
			hInSurface = handle->hInSurface[i];
			break;
		}
	}

	//	If is not exist in cache, craete input surface.
	if( hInSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->inputFd[i] )
			{
				handle->hInSurface[i] = vrCreateCvt2YuvSource ( pOutBuf->luStride, handle->srcWidth, handle->srcHeight, (NX_MEMORY_INFO*)&inputFd );
				hInSurface = handle->hInSurface[i];
				handle->inputFd[i] = inputFd;
				break;
			}
		}
	}

	//	Check output surface
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->outputImage[i].luPhyAddr == pOutBuf->luPhyAddr )
		{
			hOutSurface = handle->hOutSurface[i];
			break;
		}
	}
	//	Create Output Surface
	if( hOutSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->outputImage[i].luPhyAddr )
			{
				handle->hOutSurface[i] = vrCreateCvt2YuvTarget ( pOutBuf->luStride, handle->dstWidth, handle->dstHeight, (NX_MEMORY_HANDLE)pOutBuf->privateDesc[0], 0 );
				hOutSurface = handle->hOutSurface[i];
				handle->outputImage[i] = *pOutBuf;
				break;
			}
		}
	}

	//	Run RGB 2 YUV 
	vrRunCvt2Yuv (hOutSurface, hInSurface);

	// static void mem_save_to_file(char* pbuf, const char *filename, unsigned int width, unsigned int height)
	//mem_save_to_file( (char*)pOutBuf->luVirAddr, "test", handle->dstWidth, handle->dstHeight);

	return 0;
}

static void GTServiceRgb2YuvClose(NX_GT_PARAM_CLOSE *pParam)
{
	int i;
	NX_GT_RGB2YUV_HANDLE handle = (NX_GT_RGB2YUV_HANDLE)pParam->handle;

	if( handle )
	{
		if( handle->isInit )
		{
			for( i = 0 ; i < handle->maxInOutBufSize ; i++)
			{
				if( handle->inputFd[i] )
				{
					vrDestroyCvt2YuvSource( handle->hInSurface[i] );
				}
				if( handle->outputImage[i].luPhyAddr )
				{
					vrDestroyCvt2YuvTarget( handle->hOutSurface[i], 0 );
				}
			}
		}
		free( handle );
	}
}
//
//			End of RGB to YUV(NV12) Service
//
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
//	YUV to RGB Service
//	Input  : NX_VID_MEMORY_INFO
//	OUtput : RGB Surface's Ion Handle
//
struct NX_GT_YUV2RGB_INFO {
	int32_t isInit;
	int32_t srcWidth;
	int32_t srcHeight;
	int32_t dstWidth;
	int32_t dstHeight;
	int32_t maxInOutBufSize;

	NX_VID_MEMORY_INFO	inputImage[MAX_GRAPHIC_BUF_SIZE];		//	Input Image
	HSURFSOURCE			hInSurface[MAX_GRAPHIC_BUF_SIZE];		//	Input Image's Surface

	int					outputFd[MAX_GRAPHIC_BUF_SIZE];			//	Target Buffer's Ion File Descriptor
	HSURFTARGET			hOutSurface[MAX_GRAPHIC_BUF_SIZE];		//	Target RGB Surface
};

static void *GTServiceYuv2RgbOpen( NX_GT_PARAM_OPEN *pParam )
{
	NX_GT_YUV2RGB_HANDLE handle = (NX_GT_YUV2RGB_HANDLE)malloc(sizeof(struct NX_GT_YUV2RGB_INFO));
	
	if( handle )
	{
		memset( handle, 0, sizeof(struct NX_GT_YUV2RGB_INFO) );
		handle->isInit		= 1;
		handle->srcWidth	= pParam->srcWidth;
		handle->srcHeight	= pParam->srcHeight;
		handle->dstWidth	= pParam->dstWidth;
		handle->dstHeight	= pParam->dstHeight;
		handle->maxInOutBufSize = pParam->maxInOutBufSize;
		pParam->handle 		= handle;
	}

	return (void*)handle;
}

static int32_t GTServiceYuv2RgbDoConvert(NX_GT_PARAM_DO *pParam )
{
	int32_t i;
	HSURFSOURCE hInSurface  = NULL;
	HSURFTARGET hOutSurface = NULL;
	NX_GT_YUV2RGB_HANDLE handle = (NX_GT_YUV2RGB_HANDLE)pParam->handle;

	NX_VID_MEMORY_INFO *pInBuf = (NX_VID_MEMORY_INFO *)pParam->pInBuf;
	int outFd  = (int)pParam->pOutBuf;

	if( !handle || !handle->isInit )
	{
		return -1;
	}

	//	Check Input ION File Descriptor
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->inputImage[i].luPhyAddr == pInBuf->luPhyAddr )
		{
			hInSurface = handle->hInSurface[i];
			break;
		}
	}

	//	If is not exist in cache, craete input surface.
	if( hInSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->inputImage[i].luPhyAddr )
			{
				hInSurface = handle->hInSurface[i] = vrCreateCvt2RgbaSource ( pInBuf->luStride, handle->srcWidth, handle->srcHeight, (NX_MEMORY_HANDLE)pInBuf->privateDesc[0] );
				handle->inputImage[i] = *pInBuf;
				break;
			}
		}
	}

	//	Check output surface
	for( i=0 ; i<handle->maxInOutBufSize ; i++ )
	{
		if( handle->outputFd[i] == outFd )
		{
			hOutSurface = handle->hOutSurface[i];
			break;
		}
	}
	//	Create Output Surface
	if( hOutSurface==NULL )
	{
		for( i=0 ; i<handle->maxInOutBufSize ; i++ )
		{
			if( 0 == handle->outputFd[i] )
			{
				NX_MEMORY_INFO memHandle;
				memHandle.privateDesc = (void*)outFd;
				handle->hOutSurface[i] = vrCreateCvt2RgbaTarget ( pInBuf->luStride, handle->dstWidth, handle->dstHeight, (NX_MEMORY_HANDLE)&memHandle, 0 );
				hOutSurface = handle->hOutSurface[i];
				handle->outputFd[i] = outFd;
				break;
			}
		}
	}

	//	Run RGB 2 YUV 
	vrRunCvt2Rgba(hOutSurface, hInSurface);

	return 0;
}

static void GTServiceYuv2RgbClose(NX_GT_PARAM_CLOSE *pParam)
{
	int i;
	NX_GT_YUV2RGB_HANDLE handle = (NX_GT_YUV2RGB_HANDLE)pParam->handle;

	if( handle )
	{
		if( handle->isInit )
		{
			for( i = 0 ; i < handle->maxInOutBufSize ; i++)
			{
				if( handle->inputImage[i].luPhyAddr )
				{
					vrDestroyCvt2RgbaSource( handle->hInSurface[i] );
				}
				if( handle->outputFd[i] )
				{
					vrDestroyCvt2RgbaTarget( handle->hOutSurface[i], 0 );
				}
			}
		}
		free( handle );
	}
}
//
//			End of YUV to RGB Service
//
////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////
//
// Graphic tools Service
//
typedef struct tag_NX_GT_SERVICE {
	void*		(*GTOpen)	( NX_GT_PARAM_OPEN *param );
	int32_t		(*GTDo)		( NX_GT_PARAM_DO *param );
	void		(*GTClose)	( NX_GT_PARAM_CLOSE *param );
} NX_GT_SERVICE;

typedef struct tag_NX_GT_SERVICE_INFO {
	pthread_t			hThread;
	int32_t				bThreadRun;
	pthread_mutex_t		hLock;
	NX_QUEUE_HANDLE		hCmd;
	NX_SEM_HANDLE		hSemCmd;
	NX_SEM_HANDLE		hSemResult;
} NX_GT_SERVICE_INFO;

typedef struct tag_NX_GT_SERVICE_CMD {
	//	Input
	void		*pHandle;
	int32_t		id;
	int32_t		cmd;
	void		*data;
	//	Output
	int32_t		retCode;
} NX_GT_SERVICE_CMD;

void *GTServiceThread( void *arg )
{
	int32_t retCode = 0;
	NX_GT_SERVICE_INFO	*pService	= (NX_GT_SERVICE_INFO*)arg;
	NX_GT_SERVICE_CMD	*pCmd		= NULL;

	NX_GT_PARAM_OPEN	*pParamOpen = NULL;
	NX_GT_PARAM_DO		*pParamDo	= NULL;
	NX_GT_PARAM_CLOSE	*pParamClose= NULL;

	// Graphic Tool Engine Registration 
	NX_GT_SERVICE serviceList[MAX_SERVICE_NUM];

	serviceList[ GT_SERVICE_ID_DEINTERLACE ].GTOpen	 = &GTServiceDeintOpen;
	serviceList[ GT_SERVICE_ID_DEINTERLACE ].GTDo    = &GTServiceDeintDoDeinterlace;
	serviceList[ GT_SERVICE_ID_DEINTERLACE ].GTClose = &GTServiceDeintClose;
	serviceList[ GT_SERVICE_ID_SCALER      ].GTOpen  = &GTServiceSclOpen;
	serviceList[ GT_SERVICE_ID_SCALER      ].GTDo    = &GTServiceSclDoScale;
	serviceList[ GT_SERVICE_ID_SCALER      ].GTClose = &GTServiceSclClose;
	serviceList[ CT_SERVICE_ID_RGB2YUV     ].GTOpen  = &GTServiceRgb2YuvOpen;
	serviceList[ CT_SERVICE_ID_RGB2YUV     ].GTDo    = &GTServiceRgb2YuvDoConvert;
	serviceList[ CT_SERVICE_ID_RGB2YUV     ].GTClose = &GTServiceRgb2YuvClose;
	serviceList[ CT_SERVICE_ID_YUV2RGB     ].GTOpen  = &GTServiceYuv2RgbOpen;
	serviceList[ CT_SERVICE_ID_YUV2RGB     ].GTDo    = &GTServiceYuv2RgbDoConvert;
	serviceList[ CT_SERVICE_ID_YUV2RGB     ].GTClose = &GTServiceYuv2RgbClose;

	// GL Surface Initialize
	vrInitializeGLSurface();
	
	while( pService->bThreadRun )
	{
		pCmd = NULL;

		if( !NX_QueueGetCount( pService->hCmd ) )
			NX_SemaporePend( pService->hSemCmd );
		
		NX_QueuePop( pService->hCmd, (void**)&pCmd );
		
		// Command Parser..
		if( !pCmd ) continue;
		
		switch( pCmd->cmd )
		{
		case GT_SERVICE_CMD_OPEN :
			pParamOpen	= (NX_GT_PARAM_OPEN*)pCmd->data;
			retCode = (int32_t)serviceList[pCmd->id].GTOpen( pParamOpen );
			pCmd->retCode = retCode;
			break;
		case GT_SERVICE_CMD_DO :
			pParamDo = (NX_GT_PARAM_DO*)pCmd->data;
			retCode = serviceList[pCmd->id].GTDo( pParamDo );
			pCmd->retCode = retCode;
			break;
		case GT_SERVICE_CMD_CLOSE :
			pParamClose	= (NX_GT_PARAM_CLOSE*)pCmd->data;
			serviceList[pCmd->id].GTClose( pParamClose );
			pCmd->retCode = 0;
			break;
		default :
			break;
		}
		NX_SemaporePost( pService->hSemResult );
	}

	// GL Surface Terminate
	vrTerminateGLSurface();

	return (void*)0xDEADDEAD;
}

NX_GT_SERVICE_HANDLE NX_GTServiceInit( void )
{
	NX_GT_SERVICE_INFO *pService = (NX_GT_SERVICE_INFO*)malloc( sizeof( NX_GT_SERVICE_INFO) );
	
	if( !pService ) {
		ErrMsg("%s(): GTService Create failed.\n", __func__);
		return NULL;
	}

	memset( pService, 0x00, sizeof( NX_GT_SERVICE_INFO) );

	pService->hThread		= 0;
	pService->bThreadRun	= false;
	pService->hCmd			= NX_QueueInit( MAX_CMD_SIZE );
	pService->hSemCmd		= NX_SemaporeInit( MAX_CMD_SIZE, 0 );
	pService->hSemResult	= NX_SemaporeInit( MAX_CMD_SIZE, 0 );
	pthread_mutex_init( &pService->hLock, NULL);

	return (NX_GT_SERVICE_HANDLE)pService;
}

void NX_GTServiceDeinit( NX_GT_SERVICE_HANDLE hService )
{
	NX_GT_SERVICE_INFO *pService = (NX_GT_SERVICE_INFO*)hService;

	if( !pService ) {
		ErrMsg("%s(): GTService Handle is NULL.\n", __func__);
		return;
	}
	
	if( pService->bThreadRun ) {
		NX_GTServiceStop( pService );
	}

	NX_QueueDeinit( pService->hCmd );
	NX_SemaporeDeinit( pService->hSemCmd );
	NX_SemaporeDeinit( pService->hSemResult );
	pthread_mutex_destroy( &pService->hLock );

	free( pService );
}

int32_t NX_GTServiceStart( NX_GT_SERVICE_HANDLE hService )
{
	NX_GT_SERVICE_INFO *pService = (NX_GT_SERVICE_INFO*)hService;

	if( pService->bThreadRun ) {
		//ErrMsg("%s(): Fail, Already running.\n", __func__);
		return -1;
	}

	pService->bThreadRun = true;
	if( 0 > pthread_create( &pService->hThread, NULL, &GTServiceThread, (void*)hService ) ) {
		ErrMsg("%s(): Fail, Create thread.\n", __func__);
		return -1;
	}
	return 0;
}

int32_t NX_GTServiceStop( NX_GT_SERVICE_HANDLE hService )
{
	NX_GT_SERVICE_INFO *pService = (NX_GT_SERVICE_INFO*)hService;

	if( !pService->bThreadRun ) {
		ErrMsg("%s(): Fail, Already stopping.\n", __func__);
		return -1;
	}

	pService->bThreadRun = false;
	NX_SemaporePost( pService->hSemCmd );	// send dummy
	pthread_join( pService->hThread, NULL );

	return 0;
}

int32_t NX_GTServiceCommand( NX_GT_SERVICE_HANDLE hService, int32_t serviceID, int32_t serviceCmd, void *serviceData )
{
	NX_GT_SERVICE_CMD	cmd; 
	NxDbgCheckTimeStart();
	
	pthread_mutex_lock( &hService->hLock );

	if( !hService->bThreadRun )
	{
		ErrMsg("%s(): Service is not running.\n", __func__);
		pthread_mutex_unlock( &hService->hLock );
		return -1;
	}

	cmd.id		= serviceID;
	cmd.cmd		= serviceCmd;
	cmd.data	= serviceData;
	cmd.retCode = 0;
	NX_QueuePush( hService->hCmd, &cmd );
	NX_SemaporePost( hService->hSemCmd );

	// wait result..
	NX_SemaporePend( hService->hSemResult );
	pthread_mutex_unlock( &hService->hLock );

	NxDbgCheckTimeStop("Command");
	return cmd.retCode;
}

