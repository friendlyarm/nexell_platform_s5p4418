#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <pthread.h>

#include <nx_fourcc.h>
#include <nx_alloc_mem.h>

#include "vr_deinterlace.h"
#include "nx_graphictools.h"

#include "NX_GTService.h"

static int gstInstRefCnt = 0;
static NX_GT_SERVICE_HANDLE gstService = NULL;
static pthread_mutex_t gstInstMutex = PTHREAD_MUTEX_INITIALIZER;

//
//						Deinterlace Operation
//
//	Deinterlace Mode
//	Input Image --> Deinterlace --> Output Buffer[0]
//	                            --> Output Buffer[1]
//	                            --> Output Buffer[2]
//
//
NX_GT_DEINT_HANDLE NX_GTDeintOpen( int srcWidth, int srcHeight, int maxInOutBufSize )
{
	NX_GT_DEINT_HANDLE	handle = NULL;
	NX_GT_PARAM_OPEN	paramOpen;

	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt == 0 )
	{
		if( gstService == NULL )
		{
			gstService = NX_GTServiceInit();
		}
		NX_GTServiceStart( gstService );
	}
	gstInstRefCnt ++;
	pthread_mutex_unlock(&gstInstMutex);

	paramOpen.srcWidth 			= srcWidth;
	paramOpen.srcHeight 		= srcHeight;
	paramOpen.maxInOutBufSize	= maxInOutBufSize;
	NX_GTServiceCommand( gstService, GT_SERVICE_ID_DEINTERLACE, GT_SERVICE_CMD_OPEN, &paramOpen );
	handle = (NX_GT_DEINT_HANDLE)paramOpen.handle;
	return handle;
}

int32_t NX_GTDeintDoDeinterlace(NX_GT_DEINT_HANDLE handle, NX_VID_MEMORY_INFO *pInBuf, NX_VID_MEMORY_INFO *pOutBuf )
{
	NX_GT_PARAM_DO paramDo;
	paramDo.handle = handle;
	paramDo.pInBuf	= pInBuf;
	paramDo.pOutBuf	= pOutBuf;
	return 	NX_GTServiceCommand( gstService, GT_SERVICE_ID_DEINTERLACE, GT_SERVICE_CMD_DO, &paramDo );
}

void NX_GTDeintClose( NX_GT_DEINT_HANDLE handle )
{
	NX_GT_PARAM_CLOSE paramClose;
	paramClose.handle = handle;
	NX_GTServiceCommand( gstService, GT_SERVICE_ID_DEINTERLACE, GT_SERVICE_CMD_CLOSE, &paramClose );
	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt > 0)
	{
		gstInstRefCnt--;
		if( gstInstRefCnt==0 )
		{
			NX_GTServiceStop( gstService );
			//NX_GTServiceDeinit( gstService );
		}
	}
	pthread_mutex_unlock(&gstInstMutex);
}


//
//
//	Scaler Operation
//	Input Image --> Scaler --> Output Buffer[0]
//	                       --> Output Buffer[1]
//	                       --> Output Buffer[2]
//
//
NX_GT_SCALER_HANDLE NX_GTSclOpen(  int srcWidth, int srcHeight, int dstWidth, int dstHeight, int maxInOutBufSize )
{
	NX_GT_SCALER_HANDLE handle = NULL;
	NX_GT_PARAM_OPEN	paramOpen;

	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt == 0 )
	{
		gstService = NX_GTServiceInit();
		NX_GTServiceStart( gstService );
	}
	gstInstRefCnt ++;
	pthread_mutex_unlock(&gstInstMutex);

	paramOpen.srcWidth			= srcWidth;
	paramOpen.srcHeight			= srcHeight;
	paramOpen.dstWidth			= dstWidth;
	paramOpen.dstHeight			= dstHeight;
	paramOpen.maxInOutBufSize	= maxInOutBufSize;
	NX_GTServiceCommand( gstService, GT_SERVICE_ID_SCALER, GT_SERVICE_CMD_OPEN, &paramOpen );
	handle  = (NX_GT_SCALER_HANDLE)paramOpen.handle;
	return handle;
}

int32_t NX_GTSclDoScale(NX_GT_SCALER_HANDLE handle, NX_VID_MEMORY_INFO *pInBuf, NX_VID_MEMORY_INFO *pOutBuf )
{
	NX_GT_PARAM_DO paramDo;
	paramDo.handle = handle;
	paramDo.pInBuf	= pInBuf;
	paramDo.pOutBuf	= pOutBuf;
	return NX_GTServiceCommand( gstService, GT_SERVICE_ID_SCALER, GT_SERVICE_CMD_DO, &paramDo );
}

void NX_GTSclClose( NX_GT_SCALER_HANDLE handle )
{
	NX_GT_PARAM_CLOSE paramClose;
	paramClose.handle = handle;
	NX_GTServiceCommand( gstService, GT_SERVICE_ID_SCALER, GT_SERVICE_CMD_CLOSE, &paramClose );

	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt > 0)
	{
		gstInstRefCnt--;
		if( gstInstRefCnt==0 )
		{
			NX_GTServiceStop( gstService );
			//NX_GTServiceDeinit( gstService );
		}
	}
	pthread_mutex_unlock(&gstInstMutex);
}


//
//		RGB to YUV(NV12)
//
NX_GT_RGB2YUV_HANDLE NX_GTRgb2YuvOpen( int srcWidth, int srcHeight, int dstWidth, int dstHeight, int numOutBuf )
{
	NX_GT_RGB2YUV_HANDLE handle = NULL;
	NX_GT_PARAM_OPEN	paramOpen;

	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt == 0 )
	{
		gstService = NX_GTServiceInit();
		NX_GTServiceStart( gstService );
	}
	gstInstRefCnt ++;
	pthread_mutex_unlock(&gstInstMutex);

	paramOpen.srcWidth			= srcWidth;
	paramOpen.srcHeight			= srcHeight;
	paramOpen.dstWidth			= dstWidth;
	paramOpen.dstHeight			= dstHeight;
	paramOpen.maxInOutBufSize	= numOutBuf;
	NX_GTServiceCommand( gstService, CT_SERVICE_ID_RGB2YUV, GT_SERVICE_CMD_OPEN, &paramOpen );
	handle  = (NX_GT_RGB2YUV_HANDLE)paramOpen.handle;
	return handle;
}

int32_t NX_GTRgb2YuvDoConvert( NX_GT_RGB2YUV_HANDLE handle, int inputIonFd, NX_VID_MEMORY_INFO *pOutBuf )
{
	NX_GT_PARAM_DO paramDo;
	paramDo.handle = handle;
	paramDo.pInBuf	= (void*)inputIonFd;
	paramDo.pOutBuf	= pOutBuf;
	return NX_GTServiceCommand( gstService, CT_SERVICE_ID_RGB2YUV, GT_SERVICE_CMD_DO, &paramDo );
}

void NX_GTRgb2YuvClose( NX_GT_RGB2YUV_HANDLE handle )
{
	NX_GT_PARAM_CLOSE paramClose;
	paramClose.handle = handle;
	NX_GTServiceCommand( gstService, CT_SERVICE_ID_RGB2YUV, GT_SERVICE_CMD_CLOSE, &paramClose );

	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt > 0)
	{
		gstInstRefCnt--;
		if( gstInstRefCnt==0 )
		{
			NX_GTServiceStop( gstService );
			//NX_GTServiceDeinit( gstService );
		}
	}
	pthread_mutex_unlock(&gstInstMutex);
}


//
//		YUV to RGB
//
NX_GT_YUV2RGB_HANDLE NX_GTYuv2RgbOpen( int srcWidth, int srcHeight, int dstWidth, int dstHeight, int numOutBuf )
{
	NX_GT_YUV2RGB_HANDLE handle = NULL;
	NX_GT_PARAM_OPEN	paramOpen;

	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt == 0 )
	{
		gstService = NX_GTServiceInit();
		NX_GTServiceStart( gstService );
	}
	gstInstRefCnt ++;
	pthread_mutex_unlock(&gstInstMutex);

	paramOpen.srcWidth			= srcWidth;
	paramOpen.srcHeight			= srcHeight;
	paramOpen.dstWidth			= dstWidth;
	paramOpen.dstHeight			= dstHeight;
	paramOpen.maxInOutBufSize	= numOutBuf;
	NX_GTServiceCommand( gstService, CT_SERVICE_ID_YUV2RGB, GT_SERVICE_CMD_OPEN, &paramOpen );
	handle  = (NX_GT_YUV2RGB_HANDLE)paramOpen.handle;
	return handle;
}

int32_t NX_GTYuv2RgbDoConvert( NX_GT_YUV2RGB_HANDLE handle, NX_VID_MEMORY_INFO *pInBuf, int outMemIonFd )
{
	NX_GT_PARAM_DO paramDo;
	paramDo.handle = handle;
	paramDo.pInBuf	= pInBuf;
	paramDo.pOutBuf	= (void*)outMemIonFd;
	return NX_GTServiceCommand( gstService, CT_SERVICE_ID_YUV2RGB, GT_SERVICE_CMD_DO, &paramDo );
}

void NX_GTYuv2RgbClose( NX_GT_YUV2RGB_HANDLE handle )
{
	NX_GT_PARAM_CLOSE paramClose;
	paramClose.handle = handle;
	NX_GTServiceCommand( gstService, CT_SERVICE_ID_YUV2RGB, GT_SERVICE_CMD_CLOSE, &paramClose );

	pthread_mutex_lock(&gstInstMutex);
	if( gstInstRefCnt > 0)
	{
		gstInstRefCnt--;
		if( gstInstRefCnt==0 )
		{
			NX_GTServiceStop( gstService );
			//NX_GTServiceDeinit( gstService );
		}
	}
	pthread_mutex_unlock(&gstInstMutex);
}
