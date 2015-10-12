//
//	Nexel Video En/Decoder API
//
#include <stdlib.h>		//	malloc & free
#include <string.h>		//	memset
#include <unistd.h>		//	close

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include <nx_video_api.h>
#include <nx_video_rate_ctrl.h>

#include <nx_fourcc.h>
#include <vpu_drv_ioctl.h>		//	Device Driver IOCTL
#include <nx_alloc_mem.h>		//	Memory Allocation Information
#include "parser_vld.h"


// 14.12.05 initial version = 0.9.0
#define NX_VID_VER_MAJOR		0
#define NX_VID_VER_MINOR		9
#define NX_VID_VER_PATCH		0

#define	WORK_BUF_SIZE		(  80*1024)
#define	STREAM_BUF_SIZE		(1024*1024*4)
#define	PS_SAVE_SIZE		( 320*1024)


//
//	Debug Message Configuration
//
#define	NX_DTAG		"[VPU|API] "		//
#include "api_osapi.h"
#define	DBG_BUF_ALLOC		0
#define	DBG_ENC_OUT			0
#define DBG_BUF_INFO		0
#define	DBG_VBS				0
#define	DBG_WARNING			1
#define	DBG_USER_DATA		0

#define	DEV_NAME		"/dev/nx_vpu"
#define	RECON_CHROMA_INTERLEAVED	0

#define SLICE_SAVE_SIZE                 (MAX_DEC_PIC_WIDTH*MAX_DEC_PIC_HEIGHT*3/4)

//----------------------------------------------------------------------------
//	define static functions
static int32_t AllocateEncoderMemory( NX_VID_ENC_HANDLE hEnc );
static int32_t FreeEncoderMemory( NX_VID_ENC_HANDLE hEnc );
static int32_t AllocateDecoderMemory( NX_VID_DEC_HANDLE hDec);
static int32_t FreeDecoderMemory( NX_VID_DEC_HANDLE hDec );
static void DecoderFlushDispInfo( NX_VID_DEC_HANDLE hDec );
static void DecoderPutDispInfo( NX_VID_DEC_HANDLE hDec, int32_t iIndex, VPU_DEC_DEC_FRAME_ARG *pDecArg, uint64_t lTimeStamp, int32_t Reliable );
//static uint64_t DecoderGetTimeStamp( NX_VID_DEC_HANDLE hDec, int32_t iIndex, int32_t *piPicType );


//////////////////////////////////////////////////////////////////////////////
//
//		Video Encoder APIs
//

struct NX_VIDEO_ENC_INFO
{
	// open information
	int32_t hEncDrv;		                    // Device Driver Handle
	int32_t codecMode;		                    // (AVC_ENC = 0x10 / MP4_ENC = 0x12 / NX_JPEG_ENC=0x20 )
	int32_t instIndex;		                    // Instance Index

	NX_MEMORY_HANDLE hInstanceBuf;				// Encoder Instance Memory Buffer

	// Frame Buffer Information ( for Initialization )
	int32_t refChromaInterleave;				// Reconstruct & Referernce Buffer Chroma Interleaved
	NX_VID_MEMORY_HANDLE hRefReconBuf[2];		// Reconstruct & Referernce Buffer Information
	NX_MEMORY_HANDLE hSubSampleBuf[2];			// Sub Sample Buffer Address
	NX_MEMORY_HANDLE hBitStreamBuf;				// Bit Stream Buffer
	int32_t isInitialized;

	// Initialize Output Informations
	VPU_ENC_GET_HEADER_ARG seqInfo;

	// Encoder Options ( Default CBR Mode )
	int32_t width;
	int32_t height;
	int32_t gopSize;							// Group Of Pictures' Size
	int32_t frameRateNum;						// Framerate numerator
	int32_t frameRateDen;						// Framerate denominator
	int32_t bitRate;							// BitRate
	int32_t enableSkip;							// Enable skip frame

	int32_t userQScale;							// Default User Qunatization Scale

	int32_t GopFrmCnt;							// GOP frame counter

	// JPEG Specific
	uint32_t frameIndex;
	int32_t rstIntval;

	void *hRC;									// Rate Control Handle
};

NX_VID_ENC_HANDLE NX_VidEncOpen( VID_TYPE_E eCodecType, int32_t *piInstanceIdx )
{
	VPU_OPEN_ARG openArg;
	int32_t ret;

	//	Create Context
	NX_VID_ENC_HANDLE hEnc = (NX_VID_ENC_HANDLE)malloc( sizeof(struct NX_VIDEO_ENC_INFO) );
	memset( hEnc, 0, sizeof(struct NX_VIDEO_ENC_INFO) );
	memset( &openArg, 0, sizeof(openArg) );

	FUNC_IN();

	//	Open Device Driver
	hEnc->hEncDrv = open(DEV_NAME, O_RDWR);
	if( hEnc->hEncDrv < 0 )
	{
		NX_ErrMsg( ("Cannot open device(%s)!!!\n", DEV_NAME) );
		goto ERROR_EXIT;
	}

	switch( eCodecType )
	{
		case NX_MP4_ENC:
			openArg.codecStd = CODEC_STD_MPEG4;
			break;
		case NX_AVC_ENC:
			openArg.codecStd = CODEC_STD_AVC;
			break;
		case NX_JPEG_ENC:
			openArg.codecStd = CODEC_STD_MJPG;
			break;
		case NX_H263_ENC:
			openArg.codecStd = CODEC_STD_H263;
			break;
		default:
			NX_ErrMsg( ("Invalid codec type (%d)!!!\n", eCodecType) );
			goto ERROR_EXIT;
	}

	//	Allocate Instance Memory & Stream Buffer
	hEnc->hInstanceBuf =  NX_AllocateMemory( WORK_BUF_SIZE, 4096 );		//	x16 aligned
	if( 0 == hEnc->hInstanceBuf ){
		NX_ErrMsg(("hInstanceBuf allocation failed.\n"));
		goto ERROR_EXIT;
	}

	openArg.instIndex = -1;
	openArg.isEncoder = 1;
	openArg.instanceBuf = *hEnc->hInstanceBuf;

	ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_OPEN_INSTANCE, &openArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("NX_VidEncOpen() : IOCTL_VPU_OPEN_INSTANCE ioctl failed!!!\n") );
		goto ERROR_EXIT;
	}

	hEnc->instIndex = openArg.instIndex;
	hEnc->codecMode = eCodecType;
	hEnc->refChromaInterleave = RECON_CHROMA_INTERLEAVED;

	if ( piInstanceIdx )
		*piInstanceIdx = hEnc->instIndex;

	FUNC_OUT();
	return hEnc;

ERROR_EXIT:
	if( hEnc )
	{
		if( hEnc->hEncDrv > 0 )
		{
			close( hEnc->hEncDrv );
		}
		free( hEnc );
	}
	return NULL;
}

VID_ERROR_E NX_VidEncClose( NX_VID_ENC_HANDLE hEnc )
{
	int32_t ret;
	FUNC_IN();

	if( !hEnc )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	if ( hEnc->hRC )
	{
		free( hEnc->hRC );
		hEnc->hRC = NULL;
	}

	if( hEnc->hEncDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		free( hEnc );
		return VID_ERR_PARAM;
	}

	ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_CLOSE_INSTANCE, 0 );
	if( ret < 0 )
	{
		ret = VID_ERR_FAIL;
		NX_ErrMsg( ("NX_VidEncClose() : IOCTL_VPU_CLOSE_INSTANCE ioctl failed!!!\n") );
	}

	FreeEncoderMemory( hEnc );
	close( hEnc->hEncDrv );
	free( hEnc );

	FUNC_OUT();
	return VID_ERR_NONE;
}

VID_ERROR_E NX_VidEncInit( NX_VID_ENC_HANDLE hEnc, NX_VID_ENC_INIT_PARAM *pstParam )
{
	int32_t ret = VID_ERR_NONE;
	VPU_ENC_SEQ_ARG seqArg;
	VPU_ENC_SET_FRAME_ARG frameArg;
	VPU_ENC_GET_HEADER_ARG *pHdrArg = &hEnc->seqInfo;

	FUNC_IN();

	if( !hEnc )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	if( hEnc->hEncDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	memset( &seqArg, 0, sizeof( seqArg ) );
	memset( &frameArg, 0, sizeof( frameArg ) );
	memset( pHdrArg, 0, sizeof(VPU_ENC_GET_HEADER_ARG) );

	//	Initialize Encoder
	if( hEnc->isInitialized  )
	{
		NX_ErrMsg( ("Already initialized\n") );
		return VID_ERR_FAIL;
	}

	hEnc->width = pstParam->width;
	hEnc->height = pstParam->height;
	hEnc->gopSize = pstParam->gopSize;
	hEnc->frameRateNum = pstParam->fpsNum;
	hEnc->frameRateDen = pstParam->fpsDen;
	hEnc->bitRate = pstParam->bitrate;
	hEnc->enableSkip = !pstParam->disableSkip;

	if( 0 != AllocateEncoderMemory( hEnc ) )
	{
		NX_ErrMsg( ("AllocateEncoderMemory() failed!!\n") );
		return VID_ERR_NOT_ALLOC_BUFF;
	}

	seqArg.srcWidth = pstParam->width;
	seqArg.srcHeight = pstParam->height;

	seqArg.chromaInterleave = pstParam->chromaInterleave;
	seqArg.refChromaInterleave = hEnc->refChromaInterleave;

	seqArg.rotAngle = pstParam->rotAngle;
	seqArg.mirDirection = pstParam->mirDirection;

	seqArg.strmBufPhyAddr = hEnc->hBitStreamBuf->phyAddr;
	seqArg.strmBufVirAddr = hEnc->hBitStreamBuf->virAddr;
	seqArg.strmBufSize = hEnc->hBitStreamBuf->size;

	if( hEnc->codecMode != NX_JPEG_ENC )
	{
		seqArg.gopSize = pstParam->gopSize;
		seqArg.frameRateNum = pstParam->fpsNum;
		seqArg.frameRateDen = pstParam->fpsDen;

		//	Rate Control
		if ( pstParam->enableRC )
		{
			if (pstParam->RCAlgorithm == 1)
			{
				seqArg.RCModule = 2;
	    		seqArg.bitrate = 0;
	    		seqArg.disableSkip = 0;
				seqArg.initialDelay = 0;
				seqArg.vbvBufferSize = 0;
				seqArg.gammaFactor = 0;

				hEnc->hRC = NX_VidRateCtrlInit( hEnc->codecMode, pstParam );
				if( hEnc->hRC == NULL ) goto ERROR_EXIT;
			}
			else
			{
				hEnc->hRC = NULL;
				seqArg.RCModule = 1;
	    		seqArg.bitrate = pstParam->bitrate;
	    		seqArg.disableSkip = !pstParam->disableSkip;
				seqArg.initialDelay = pstParam->RCDelay;
				seqArg.vbvBufferSize = pstParam->rcVbvSize;
				seqArg.gammaFactor = ( pstParam->gammaFactor ) ? ( pstParam->gammaFactor ) : ((int)(0.75 * 32768));
			}
		}
		else
		{
			seqArg.RCModule = 0;
			hEnc->userQScale = (pstParam->initialQp == 0) ? (23) : (pstParam->initialQp);
		}

		seqArg.searchRange = (hEnc->codecMode != NX_H263_ENC) ? (pstParam->searchRange) : (3);       // ME Search Range
		seqArg.intraRefreshMbs = pstParam->numIntraRefreshMbs;

		if( hEnc->codecMode == NX_AVC_ENC )
		{
			if ( pstParam->enableAUDelimiter != 0 )
				seqArg.enableAUDelimiter = 1;
			seqArg.maxQP = ( pstParam->maximumQp > 0 ) ? ( pstParam->maximumQp ) : (51);
		}
		else
		{
			seqArg.maxQP = ( pstParam->maximumQp > 0 ) ? ( pstParam->maximumQp ) : (31);
		}
	}
	else
	{
		seqArg.frameRateNum = 1;
		seqArg.frameRateDen = 1;
		seqArg.gopSize = 1;
		seqArg.quality = pstParam->jpgQuality;
	}

	ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_ENC_SET_SEQ_PARAM, &seqArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_ENC_SET_SEQ_PARAM ioctl failed!!!\n") );
		goto ERROR_EXIT;
	}

	if( hEnc->codecMode != NX_JPEG_ENC )
	{
		frameArg.numFrameBuffer = 2;		//	We use always 2 frame
		frameArg.frameBuffer[0] = *hEnc->hRefReconBuf[0];
		frameArg.frameBuffer[1] = *hEnc->hRefReconBuf[1];
		frameArg.subSampleBuffer[0] = *hEnc->hSubSampleBuf[0];
		frameArg.subSampleBuffer[1] = *hEnc->hSubSampleBuf[1];

		//	data partition mode always disabled ( for MPEG4 )
		frameArg.dataPartitionBuffer.phyAddr = 0;
		frameArg.dataPartitionBuffer.virAddr = 0;

		ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_ENC_SET_FRAME_BUF, &frameArg );
		if( ret < 0 )
		{
			ret = VID_ERR_INIT;
			NX_ErrMsg( ("IOCTL_VPU_ENC_SET_FRAME_BUF ioctl failed!!!\n") );
			goto ERROR_EXIT;
		}

		ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_ENC_GET_HEADER, pHdrArg );
		if( ret < 0 )
		{
			NX_ErrMsg( ("IOCTL_VPU_ENC_GET_HEADER ioctl failed!!!\n") );
			goto ERROR_EXIT;
		}
	}
	else
	{
		frameArg.numFrameBuffer = 0;
	}

	hEnc->isInitialized = 1;
	ret = VID_ERR_NONE;
	FUNC_OUT();

ERROR_EXIT:
	return ret;
}

VID_ERROR_E NX_VidEncGetSeqInfo( NX_VID_ENC_HANDLE hEnc, uint8_t *pbySeqBuf, int32_t *piSeqBufSize )
{
	FUNC_IN();
	if( !hEnc )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	if( hEnc->hEncDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	if( !hEnc->isInitialized )
	{
		NX_ErrMsg( ("Invalid encoder operation initialize first!!!\n") );
		return VID_ERR_INIT;
	}

	if( hEnc->codecMode == NX_AVC_ENC )
	{
		memcpy( pbySeqBuf, hEnc->seqInfo.avcHeader.spsData, hEnc->seqInfo.avcHeader.spsSize );
		memcpy( pbySeqBuf+hEnc->seqInfo.avcHeader.spsSize, hEnc->seqInfo.avcHeader.ppsData, hEnc->seqInfo.avcHeader.ppsSize );
		*piSeqBufSize = hEnc->seqInfo.avcHeader.spsSize + hEnc->seqInfo.avcHeader.ppsSize;
	}
	else if ( hEnc->codecMode == NX_MP4_ENC )
	{
		memcpy( pbySeqBuf, hEnc->seqInfo.mp4Header.vosData, hEnc->seqInfo.mp4Header.vosSize );
		memcpy( pbySeqBuf+hEnc->seqInfo.mp4Header.vosSize, hEnc->seqInfo.mp4Header.volData, hEnc->seqInfo.mp4Header.volSize );
		*piSeqBufSize = hEnc->seqInfo.mp4Header.vosSize + hEnc->seqInfo.mp4Header.volSize;
	}
	else
	{
		*piSeqBufSize = 0;
		return VID_ERR_NONE;
	}

	FUNC_OUT();
	return VID_ERR_NONE;
}

VID_ERROR_E NX_VidEncEncodeFrame( NX_VID_ENC_HANDLE hEnc, NX_VID_ENC_IN *pstEncIn, NX_VID_ENC_OUT *pstEncOut )
{
	int32_t ret;
	int32_t iFrmType = PIC_TYPE_UNKNOWN;
	VPU_ENC_RUN_FRAME_ARG runArg;

	FUNC_IN();
	if( !hEnc )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	if( hEnc->hEncDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	memset( &runArg, 0, sizeof(runArg) );

	runArg.inImgBuffer = *(pstEncIn->pImage);
	//runArg.changeFlag = 0;
	//runArg.enableRc = 1;					//	N/A
	runArg.quantParam = pstEncIn->quantParam;
	runArg.skipPicture = pstEncIn->forcedSkipFrame;
	//pstEncIn->timeStamp;

	//printf("forcedIFrame = %d, GopFrmCnt = %d, gopSize = %d \n", pstEncIn->forcedIFrame, hEnc->GopFrmCnt, hEnc->gopSize);

	if( (pstEncIn->forcedIFrame) || (hEnc->GopFrmCnt >= hEnc->gopSize) || (hEnc->GopFrmCnt == 0) )
	{
		runArg.forceIPicture = 1;
		hEnc->GopFrmCnt = 0;
	}
	hEnc->GopFrmCnt += 1;

	if ( hEnc->hRC )
	{
#if 1
		if ( runArg.forceIPicture == 1 )
			iFrmType = PIC_TYPE_I;
		else if ( runArg.skipPicture == 1 )
			iFrmType = PIC_TYPE_SKIP;
		else
			iFrmType = PIC_TYPE_P;
#else
		iFrmType = PIC_TYPE_UNKNOWN;
#endif
		NX_VidRateCtrlGetFrameQp( hEnc->hRC, &runArg.quantParam, &iFrmType );

		if ( iFrmType == PIC_TYPE_SKIP )
			runArg.skipPicture = 1;

#if 1
		pstEncIn->quantParam = runArg.quantParam;
#endif
	}

	ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_ENC_RUN_FRAME, &runArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_ENC_RUN_FRAME ioctl failed!!!\n") );
		return ret;
	}

	if ( hEnc->hRC )
	{
		NX_VidRateCtrlUpdate( hEnc->hRC, runArg.outStreamSize );
	}

	pstEncOut->width = hEnc->width;
	pstEncOut->height = hEnc->height;
	pstEncOut->frameType = ( iFrmType != PIC_TYPE_SKIP ) ? (runArg.frameType) : (PIC_TYPE_SKIP);
	pstEncOut->bufSize = runArg.outStreamSize;
	pstEncOut->outBuf = runArg.outStreamAddr;
	pstEncOut->ReconImg = *hEnc->hRefReconBuf[runArg.reconImgIdx];

//	NX_DbgMsg( DBG_ENC_OUT, ("Encoder Output : Success(outputSize = %d, isKey=%d)\n", pEncOut->bufSize, pEncOut->isKey) );
	FUNC_OUT();
	return 0;
}

VID_ERROR_E NX_VidEncChangeParameter( NX_VID_ENC_HANDLE hEnc, NX_VID_ENC_CHG_PARAM *pstChgParam )
{
	int32_t ret;
	VPU_ENC_CHG_PARA_ARG chgArg;
	FUNC_IN();
	if( !hEnc )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	if( hEnc->hEncDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return VID_ERR_PARAM;
	}

	memset( &chgArg, 0, sizeof(chgArg) );

	//printf("chgFlg = %x, gopSize = %d, bitrate = %d, fps = %d/%d, maxQp = %d, skip = %d, vbv = %d, mb = %d \n",
	//	pstChgParam->chgFlg, pstChgParam->gopSize, pstChgParam->bitrate, pstChgParam->fpsNum, pstChgParam->fpsDen, pstChgParam->maximumQp, pstChgParam->disableSkip, pstChgParam->rcVbvSize, pstChgParam->numIntraRefreshMbs);

	chgArg.chgFlg = pstChgParam->chgFlg;
	chgArg.gopSize = pstChgParam->gopSize;
	chgArg.bitrate = pstChgParam->bitrate;
	chgArg.frameRateNum = pstChgParam->fpsNum;
	chgArg.frameRateDen = pstChgParam->fpsDen;
	chgArg.intraRefreshMbs = pstChgParam->numIntraRefreshMbs;

	if ( chgArg.chgFlg & VID_CHG_GOP )
		hEnc->gopSize = chgArg.gopSize;

	if ( hEnc->hRC )
	{
		if ( NX_VidRateCtrlChangePara( hEnc->hRC, pstChgParam ) != VID_ERR_NONE )
			return VID_ERR_CHG_PARAM;

		chgArg.chgFlg = pstChgParam->chgFlg & 0x18;
		chgArg.gopSize = 0;
		chgArg.bitrate = 0;
		chgArg.frameRateNum = pstChgParam->fpsNum;
		chgArg.frameRateDen = pstChgParam->fpsDen;
		chgArg.intraRefreshMbs = pstChgParam->numIntraRefreshMbs;
	}

	ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_ENC_CHG_PARAM, &chgArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_ENC_CHG_PARAM ioctl failed!!!\n") );
		return ret;
	}

	FUNC_OUT();
	return VID_ERR_NONE;
}

//
//		End of Encoder APIs
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//
//	Jpeg Encoder APIs
//
VID_ERROR_E NX_VidEncJpegGetHeader( NX_VID_ENC_HANDLE hEnc, uint8_t *pbyJpgHeader, int32_t *piHeaderSize )
{
	int32_t ret;
	VPU_ENC_GET_HEADER_ARG *pHdrArg = (VPU_ENC_GET_HEADER_ARG *)calloc(sizeof(VPU_ENC_GET_HEADER_ARG), 1);
	FUNC_IN();
	ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_JPG_GET_HEADER, pHdrArg );

	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_JPG_GET_HEADER ioctl failed!!!\n") );
	}
	else
	{
		memcpy( pbyJpgHeader, pHdrArg->jpgHeader.jpegHeader, pHdrArg->jpgHeader.headerSize );
		*piHeaderSize = pHdrArg->jpgHeader.headerSize;
	}
	FUNC_OUT();
	return ret;
}

VID_ERROR_E NX_VidEncJpegRunFrame( NX_VID_ENC_HANDLE hEnc, NX_VID_MEMORY_HANDLE hInImage, NX_VID_ENC_OUT *pstEncOut )
{
	int32_t ret;
	VPU_ENC_RUN_FRAME_ARG runArg;
	FUNC_IN();
	if( !hEnc )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return -1;
	}

	if( hEnc->hEncDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		return -1;
	}

	memset( &runArg, 0, sizeof(runArg) );
	runArg.inImgBuffer = *hInImage;

	ret = ioctl( hEnc->hEncDrv, IOCTL_VPU_JPG_RUN_FRAME, &runArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_JPG_RUN_FRAME ioctl failed!!!\n") );
		return -1;
	}

	pstEncOut->width = hEnc->width;
	pstEncOut->height = hEnc->height;
	pstEncOut->bufSize = runArg.outStreamSize;
	pstEncOut->outBuf = runArg.outStreamAddr;
	FUNC_OUT();
	return 0;
}

//
//	Jpeg Encoder APIs
//
//////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////
//
//	Video Decoder APIs
//

#define	PIC_FLAG_KEY		0x0001
#define	PIC_FLAG_INTERLACE	0x0010

struct NX_VIDEO_DEC_INFO
{
	// open information
	int32_t hDecDrv;                                       // Device Driver Handle
	int32_t codecStd;                                      // NX_VPU_CODEC_MODE 	( AVC_DEC = 0, MP2_DEC = 2, MP4_DEC = 3, DV3_DEC = 3, RV_DEC = 4  )
	int32_t instIndex;                                     // Instance Index

	int32_t width;
	int32_t height;

	// Frame Buffer Information ( for Initialization )
	int32_t numFrameBuffers;
	NX_MEMORY_HANDLE hInstanceBuf;								// Decoder Instance Memory Buffer
	NX_MEMORY_HANDLE hBitStreamBuf;								// Bit Stream Buffer
	NX_VID_MEMORY_HANDLE hFrameBuffer[MAX_DEC_FRAME_BUFFERS];	// Reconstruct & Referernce Buffer Information
	NX_MEMORY_HANDLE hColMvBuffer;								// All Codecs
	NX_MEMORY_HANDLE hSliceBuffer;								// AVC codec
	NX_MEMORY_HANDLE hPvbSliceBuffer;							// PVX codec

	int32_t enableUserData;										// User Data Mode Enable/Disable
	NX_MEMORY_HANDLE hUserDataBuffer;							// User Data ( MPEG2 Only )

	int32_t isInitialized;

	int32_t useExternalFrameBuffer;
	int32_t numBufferableBuffers;

	// Initialize Output Information
	uint8_t	pSeqData[2048];										// SPS PPS (H.264) or Decoder Specific Information(for MPEG4)
	int32_t seqDataSize;

	uint64_t timeStamp[MAX_DEC_FRAME_BUFFERS][2];
	int32_t picType[MAX_DEC_FRAME_BUFFERS];
	int32_t picFlag[MAX_DEC_FRAME_BUFFERS];

	int32_t multiResolution[MAX_DEC_FRAME_BUFFERS];

	// For Display Frame Information
	int32_t isInterlace[MAX_DEC_FRAME_BUFFERS];
	int32_t topFieldFirst[MAX_DEC_FRAME_BUFFERS];
	int32_t FrmReliable_0_100[MAX_DEC_FRAME_BUFFERS];
	int32_t upSampledWidth[MAX_DEC_FRAME_BUFFERS];
	int32_t upSampledHeight[MAX_DEC_FRAME_BUFFERS];

	// For MPEG4
	int32_t vopTimeBits;
};

NX_VID_DEC_HANDLE NX_VidDecOpen( VID_TYPE_E eCodecType, uint32_t uMp4Class, int32_t iOptions, int32_t *piInstanceIdx  )
{
	int32_t ret;
	VPU_OPEN_ARG openArg;
	int32_t workBufSize = WORK_BUF_SIZE;
	FUNC_IN();
	//	Create Context
	NX_VID_DEC_HANDLE hDec = (NX_VID_DEC_HANDLE)malloc( sizeof(struct NX_VIDEO_DEC_INFO) );

	memset( hDec, 0, sizeof(struct NX_VIDEO_DEC_INFO) );
	memset( &openArg, 0, sizeof(openArg) );

	//	Open Device Driver
	hDec->hDecDrv = open(DEV_NAME, O_RDWR);
	if( hDec->hDecDrv < 0 )
	{
		NX_ErrMsg(("Cannot open device(%s)!!!\n", DEV_NAME));
		goto ERROR_EXIT;
	}

	if( eCodecType == NX_AVC_DEC || eCodecType == NX_AVC_ENC )
	{
		workBufSize += PS_SAVE_SIZE;
	}
	hDec->hBitStreamBuf = NX_AllocateMemory( STREAM_BUF_SIZE, 4096 );	//	x16 aligned
	if( 0 == hDec->hBitStreamBuf ){
		NX_ErrMsg(("hBitStreamBuf allocation failed.\n"));
		goto ERROR_EXIT;
	}

	//	Allocate Instance Memory & Stream Buffer
	hDec->hInstanceBuf =  NX_AllocateMemory( workBufSize, 4096 );		//	x16 aligned
	if( 0 == hDec->hInstanceBuf ){
		NX_ErrMsg(("hInstanceBuf allocation failed.\n"));
		goto ERROR_EXIT;
	}

	switch( eCodecType )
	{
		case NX_AVC_DEC:
			openArg.codecStd = CODEC_STD_AVC;
			break;
		case NX_MP2_DEC:
			openArg.codecStd = CODEC_STD_MPEG2;
			break;
		case NX_MP4_DEC:
			openArg.codecStd = CODEC_STD_MPEG4;
			openArg.mp4Class = uMp4Class;
			break;
		case NX_H263_DEC:	//
			openArg.codecStd = CODEC_STD_H263;
			break;
		case NX_DIV3_DEC:	//
			openArg.codecStd = CODEC_STD_DIV3;
			break;
		case NX_RV_DEC:		// Real Video
			openArg.codecStd = CODEC_STD_RV;
			break;
		case NX_VC1_DEC:	//	WMV
			openArg.codecStd = CODEC_STD_VC1;
			break;
		case NX_THEORA_DEC:	//	Theora
			openArg.codecStd = CODEC_STD_THO;
			break;
		case NX_VP8_DEC:	//	VP8
			openArg.codecStd = CODEC_STD_VP8;
			break;
		default:
			NX_ErrMsg( ("IOCTL_VPU_OPEN_INSTANCE codec Type\n") );
			goto ERROR_EXIT;
	}

	openArg.instIndex = -1;
	openArg.instanceBuf = *hDec->hInstanceBuf;
	openArg.streamBuf = *hDec->hBitStreamBuf;

	if( iOptions && DEC_OPT_CHROMA_INTERLEAVE )
	{
		openArg.chromaInterleave = 1;
	}

	ret = ioctl( hDec->hDecDrv, IOCTL_VPU_OPEN_INSTANCE, &openArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_OPEN_INSTANCE ioctl failed!!!\n") );
		goto ERROR_EXIT;
	}
	hDec->instIndex = openArg.instIndex;
	hDec->codecStd = openArg.codecStd;

	if ( piInstanceIdx )
		*piInstanceIdx = hDec->instIndex;

	DecoderFlushDispInfo(hDec);

	FUNC_OUT();
	return hDec;

ERROR_EXIT:
	if( hDec->hDecDrv > 0 )
	{
		if( hDec->hInstanceBuf )
		{
			NX_FreeMemory(hDec->hInstanceBuf);
		}
		if( hDec->hBitStreamBuf )
		{
			NX_FreeMemory(hDec->hBitStreamBuf);
		}
		close( hDec->hDecDrv );
		free( hDec );
	}
	return 0;
}

VID_ERROR_E NX_VidDecClose( NX_VID_DEC_HANDLE hDec )
{
	int32_t ret;
	FUNC_IN();
	if( !hDec )
	{
		NX_ErrMsg( ("Invalid decoder handle or driver handle!!!\n") );
		return -1;
	}

	if( hDec->hDecDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid decoder handle or driver handle!!!\n") );
		return -1;
	}

	ret = ioctl( hDec->hDecDrv, IOCTL_VPU_CLOSE_INSTANCE, 0 );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_CLOSE_INSTANCE ioctl failed!!!\n") );
	}

	FreeDecoderMemory( hDec );

	close( hDec->hDecDrv );
	free( hDec );

	FUNC_OUT();
	return 0;
}

VID_ERROR_E NX_VidDecParseVideoCfg(NX_VID_DEC_HANDLE hDec, NX_VID_SEQ_IN *pstSeqIn, NX_VID_SEQ_OUT *pstSeqOut)
{
	int32_t ret = -1;
	VPU_DEC_SEQ_INIT_ARG seqArg;

	FUNC_IN();
	memset( &seqArg, 0, sizeof(seqArg) );
	memset( pstSeqOut, 0, sizeof(NX_VID_SEQ_OUT) );

	if( !hDec )
	{
		NX_ErrMsg( ("Invalid decoder handle or driver handle!!!\n") );
		goto ERROR_EXIT;
	}
	if( hDec->hDecDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid decoder handle or driver handle!!!\n") );
		goto ERROR_EXIT;
	}

	//	Initialize Decoder
	if( hDec->isInitialized  )
	{
		int32_t i = 0;
		int32_t iSeqSize = ( pstSeqIn->seqSize < 2048 ) ? ( pstSeqIn->seqSize ) : ( 2048 );

		if ( iSeqSize == hDec->seqDataSize )
		{
			uint8_t *pbySrc = hDec->pSeqData;
			uint8_t *pbyDst = pstSeqIn->seqInfo;
			for (i=0 ; i<iSeqSize ; i++)
			{
				if ( *pbySrc++ != *pbyDst++ )	break;
			}
		}

		if ( (iSeqSize == 0) || (i == iSeqSize) )
		{
			NX_ErrMsg( ("Already initialized\n") );
			goto ERROR_EXIT;
		}
		hDec->isInitialized = 0;
	}

	seqArg.seqData        	= pstSeqIn->seqInfo;
	seqArg.seqDataSize    	= pstSeqIn->seqSize;
	seqArg.outWidth 		= pstSeqIn->width;
	seqArg.outHeight 		= pstSeqIn->height;

	if( pstSeqIn->disableOutReorder )
	{
		NX_DbgMsg( DBG_WARNING, ("Diable Reordering!!!!\n") );
		seqArg.disableOutReorder = 1;
	}

	seqArg.enablePostFilter = pstSeqIn->enablePostFilter;
	seqArg.enableUserData   = (pstSeqIn->enableUserData) && (hDec->codecStd == CODEC_STD_MPEG2);
	if( seqArg.enableUserData )
	{
		NX_DbgMsg(DBG_USER_DATA, ("Enabled user data\n"));
		hDec->enableUserData = 1;
		hDec->hUserDataBuffer = NX_AllocateMemory( 0x10000, 4096 );		//	x16 aligned
		if( 0 == hDec->hUserDataBuffer ){
			NX_ErrMsg(("hUserDataBuffer allocation failed.(size=%d,align=%d)\n", 0x10000, 4096));
			goto ERROR_EXIT;
		}
		seqArg.userDataBuffer = *hDec->hUserDataBuffer;
	}

	ret = ioctl( hDec->hDecDrv, IOCTL_VPU_DEC_SET_SEQ_INFO, &seqArg );
	if( ret == VID_NEED_STREAM )
		goto ERROR_EXIT;
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_DEC_SET_SEQ_INFO ioctl failed!!!\n") );
		goto ERROR_EXIT;
	}

	if( seqArg.minFrameBufCnt < 1 || seqArg.minFrameBufCnt > MAX_DEC_FRAME_BUFFERS )
	{
		NX_ErrMsg( ("IOCTL_VPU_DEC_SET_SEQ_INFO ioctl failed(nimFrameBufCnt = %d)!!!\n", seqArg.minFrameBufCnt) );
		goto ERROR_EXIT;
	}

	hDec->seqDataSize = ( pstSeqIn->seqSize < 2048 ) ? ( pstSeqIn->seqSize ) : ( 2048 );
	memcpy (hDec->pSeqData, pstSeqIn->seqInfo, hDec->seqDataSize);

	if ( (hDec->codecStd == CODEC_STD_MPEG4) && ( hDec->pSeqData != NULL) && (hDec->seqDataSize > 0) )
	{
		uint8_t *pbyStrm = hDec->pSeqData;
		uint32_t uPreFourByte = (uint32_t)-1;

		hDec->vopTimeBits = 0;

		do
		{
			if ( pbyStrm >= (hDec->pSeqData + hDec->seqDataSize) )
			{
				//goto ERROR_EXIT;
				break;
			}
			uPreFourByte = (uPreFourByte << 8) + *pbyStrm++;

			if ( uPreFourByte >= 0x00000120 && uPreFourByte <= 0x0000012F )
			{
				VLD_STREAM stStrm = { 0, pbyStrm, hDec->seqDataSize };
				int32_t    i;

				vld_flush_bits( &stStrm, 1+8 );									// random_accessible_vol, video_object_type_indication
				if (vld_get_bits( &stStrm, 1 ))									// is_object_layer_identifier
					vld_flush_bits( &stStrm, 4 + 3 );							// video_object_layer_verid, video_object_layer_priority

				if (vld_get_bits( &stStrm, 4 ) == 0xF )							// aspect_ratio_info
					vld_flush_bits( &stStrm, 8+8 );								// par_width, par_height

				if (vld_get_bits( &stStrm, 1)) 									// vol_control_parameters
				{
					if (vld_get_bits( &stStrm, 2+1+1 ) & 1) 					// chroma_format, low_delay, vbv_parameters
					{
		                vld_flush_bits( &stStrm, 15+1 );						// first_half_bit_rate, marker_bit
		                vld_flush_bits( &stStrm, 15+1 );						// latter_half_bit_rate, marker_bit
		                vld_flush_bits( &stStrm, 15+1 );						// first_half_vbv_buffer_size, marker_bit
		                vld_flush_bits( &stStrm, 3+11+1 );					// latter_half_vbv_buffer_size, first_half_vbv_occupancy, marker_bit
		                vld_flush_bits( &stStrm, 15+1 );						// latter_half_vbv_occupancy, marker_bit
		            }
		        }

				vld_flush_bits( &stStrm, 2+1);									// video_object_layer_shape, marker_bit

				for (i=0 ; i<16 ; i++)												// vop_time_increment_resolution
					if ( vld_get_bits( &stStrm, 1) )
						break;
				hDec->vopTimeBits = 16 - i;
				break;
			}
		} while(1);
	}

	if ( pstSeqIn->seqSize == 0 )
	{
		seqArg.cropRight = pstSeqIn->width;
		seqArg.cropBottom = pstSeqIn->height;
	}

	hDec->width = seqArg.cropRight;
	hDec->height = seqArg.cropBottom;
	hDec->numFrameBuffers = seqArg.minFrameBufCnt;

	pstSeqOut->minBuffers 		= seqArg.minFrameBufCnt;
	pstSeqOut->numBuffers 		= seqArg.minFrameBufCnt + pstSeqIn->addNumBuffers;
	pstSeqOut->width        	= seqArg.cropRight;
	pstSeqOut->height       	= seqArg.cropBottom;
	pstSeqOut->frameBufDelay	= seqArg.frameBufDelay;
	pstSeqOut->isInterlace 	= seqArg.interlace;
	pstSeqOut->frameRateNum 	= seqArg.frameRateNum;
	pstSeqOut->frameRateDen 	= seqArg.frameRateDen;

	if (seqArg.vp8HScaleFactor == 0)		pstSeqOut->vp8ScaleWidth  = 0;
	else if (seqArg.vp8HScaleFactor == 1) 	pstSeqOut->vp8ScaleWidth  = seqArg.vp8ScaleWidth * 5 / 4;
	else if (seqArg.vp8HScaleFactor == 2) 	pstSeqOut->vp8ScaleWidth  = seqArg.vp8ScaleWidth * 5 / 3;
	else if (seqArg.vp8HScaleFactor == 3) 	pstSeqOut->vp8ScaleWidth  = seqArg.vp8ScaleWidth * 2;

	if (seqArg.vp8VScaleFactor == 0)		pstSeqOut->vp8ScaleHeight  = 0;
	else if (seqArg.vp8VScaleFactor == 1) 	pstSeqOut->vp8ScaleHeight  = seqArg.vp8ScaleHeight * 5 / 4;
	else if (seqArg.vp8VScaleFactor == 2) 	pstSeqOut->vp8ScaleHeight  = seqArg.vp8ScaleHeight * 5 / 3;
	else if (seqArg.vp8VScaleFactor == 3) 	pstSeqOut->vp8ScaleHeight  = seqArg.vp8ScaleHeight * 2;

	// TBD.
	pstSeqOut->userDataNum = 0;
	pstSeqOut->userDataSize = 0;
	pstSeqOut->userDataBufFull = 0;
	pstSeqOut->unsupportedFeature = 0;

ERROR_EXIT:
	FUNC_OUT();
	return ret;
}

VID_ERROR_E NX_VidDecInit(NX_VID_DEC_HANDLE hDec, NX_VID_SEQ_IN *pstSeqIn)
{
	int32_t i, ret=-1;
	VPU_DEC_REG_FRAME_ARG frameArg;

	FUNC_IN();
	memset( &frameArg, 0, sizeof(frameArg) );

	if( !hDec )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		goto ERROR_EXIT;
	}
	if( hDec->hDecDrv <= 0 )
	{
		NX_ErrMsg( ("Invalid encoder handle or driver handle!!!\n") );
		goto ERROR_EXIT;
	}
	//	Initialize Encoder
	if( hDec->isInitialized  )
	{
		NX_ErrMsg( ("Already initialized\n") );
		goto ERROR_EXIT;
	}

	if( pstSeqIn->numBuffers > 0 )
	{
		hDec->useExternalFrameBuffer = 1;

		if ( pstSeqIn->numBuffers - hDec->numFrameBuffers < 2 )
		{
			NX_DbgMsg( DBG_WARNING, ("[Warning] External Buffer too small.(min=%d, buffers=%d)\n", hDec->numFrameBuffers, pstSeqIn->numBuffers) );
		}

		hDec->numFrameBuffers = pstSeqIn->numBuffers;
	}
	else
	{
		hDec->numFrameBuffers += pstSeqIn->addNumBuffers;
	}

	//	Allocation & Save Parameter in the decoder handle.
	if( 0 != AllocateDecoderMemory( hDec ) )
	{
		ret = VID_ERR_NOT_ALLOC_BUFF;
		NX_ErrMsg(("AllocateDecoderMemory() Failed!!!\n"));
		goto ERROR_EXIT;
	}

	//	Set Frame Argement Valiable
	frameArg.numFrameBuffer = hDec->numFrameBuffers;
	for( i=0 ; i< hDec->numFrameBuffers ; i++ )
	{
		if( hDec->useExternalFrameBuffer )
			hDec->hFrameBuffer[i] = pstSeqIn->pMemHandle[i];
		frameArg.frameBuffer[i] = *hDec->hFrameBuffer[i];
	}
	if( hDec->hSliceBuffer )
		frameArg.sliceBuffer = *hDec->hSliceBuffer;
	if( hDec->hColMvBuffer)
		frameArg.colMvBuffer = *hDec->hColMvBuffer;
	if( hDec->hPvbSliceBuffer )
		frameArg.pvbSliceBuffer = *hDec->hPvbSliceBuffer;

	ret = ioctl( hDec->hDecDrv, IOCTL_VPU_DEC_REG_FRAME_BUF, &frameArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_DEC_REG_FRAME_BUF ioctl failed!!!\n") );
		goto ERROR_EXIT;
	}

	hDec->isInitialized = 1;

ERROR_EXIT:
	FUNC_OUT();
	return ret;
}

VID_ERROR_E NX_VidDecDecodeFrame( NX_VID_DEC_HANDLE hDec, NX_VID_DEC_IN *pstDecIn, NX_VID_DEC_OUT *pstDecOut )
{
	int32_t ret;
	VPU_DEC_DEC_FRAME_ARG decArg;

	FUNC_IN();

	//	Initialize Decoder
	if( !hDec->isInitialized  )
	{
		NX_ErrMsg( ("%s Line(%d) : Not initialized!!!\n", __func__, __LINE__));
		return -1;
	}

	memset( pstDecOut, 0, sizeof(NX_VID_DEC_OUT) );
	memset( &decArg, 0, sizeof(decArg) );
	decArg.strmData = pstDecIn->strmBuf;
	decArg.strmDataSize = pstDecIn->strmSize;
	decArg.eos = pstDecIn->eos;
	decArg.iFrameSearchEnable = 0;
	decArg.skipFrameMode = 0;
	decArg.decSkipFrameNum = 0;
	pstDecOut->outImgIdx = -1;

	if ( (hDec->codecStd == CODEC_STD_MPEG4) && (hDec->vopTimeBits > 0) && (decArg.strmDataSize > 0) )
	{
		uint8_t *pbyStrm = decArg.strmData;
		VLD_STREAM stStrm = { 0, pbyStrm, decArg.strmDataSize };

		if (vld_get_bits( &stStrm, 32 ) == 0x000001B6)
		{
			vld_flush_bits( &stStrm, 2 );								// vop_coding_type
			do
			{
				if ( vld_get_bits( &stStrm, 1 ) == 0 )	break;
			} while( stStrm.dwUsedBits < ((unsigned long)decArg.strmDataSize<<3) );
			vld_flush_bits( &stStrm, 1+hDec->vopTimeBits+1 );			// marker_bits, vop_time_increment, marker_bits

			if ( vld_get_bits( &stStrm, 1 ) == 0 )						// vop_coded
				decArg.strmDataSize = 0;
		}
	}

	ret = ioctl( hDec->hDecDrv, IOCTL_VPU_DEC_RUN_FRAME, &decArg );
	if( ret != VID_ERR_NONE )
	{
		NX_ErrMsg( ("IOCTL_VPU_DEC_RUN_FRAME ioctl failed!!!(%d) \n", decArg.iRet ) );
		return (decArg.iRet);
	}

	pstDecOut->outImgIdx = decArg.indexFrameDisplay;
	pstDecOut->outDecIdx = decArg.indexFrameDecoded;
	pstDecOut->width     = decArg.outRect.right;
	pstDecOut->height    = decArg.outRect.bottom;
	pstDecOut->picType[DECODED_FRAME] = ( decArg.picType != 7 ) ? ( decArg.picType ) : ( PIC_TYPE_UNKNOWN );

	pstDecOut->strmReadPos  = decArg.strmReadPos;
	pstDecOut->strmWritePos = decArg.strmWritePos;

	if ( pstDecOut->outDecIdx >= 0 )
	{
		if ( decArg.numOfErrMBs == 0 )
			pstDecOut->outFrmReliable_0_100[DECODED_FRAME] = ( pstDecOut->outDecIdx < 0 ) ? ( 0 ) : ( 100 );
		else
		{
			int32_t TotalMbNum = ( (decArg.outWidth + 15) >> 4 ) * ( (decArg.outHeight + 15) >> 4 );
			pstDecOut->outFrmReliable_0_100[DECODED_FRAME] = (TotalMbNum - decArg.numOfErrMBs) * 100 / TotalMbNum;
		}
	}

	DecoderPutDispInfo( hDec, pstDecOut->outDecIdx, &decArg, pstDecIn->timeStamp, pstDecOut->outFrmReliable_0_100[DECODED_FRAME] );

	if( (pstDecOut->outImgIdx >= 0) && (pstDecOut->outImgIdx < hDec->numFrameBuffers) )
	{
		int32_t iIdx = pstDecOut->outImgIdx;
		pstDecOut->outImg = *hDec->hFrameBuffer[ iIdx ];
		pstDecOut->timeStamp[FIRST_FIELD] = hDec->timeStamp[ iIdx ][ FIRST_FIELD ];
		pstDecOut->timeStamp[SECOND_FIELD] = ( hDec->timeStamp[ iIdx ][ SECOND_FIELD ] != (uint64_t)(-10) ) ? ( hDec->timeStamp[ iIdx ][ SECOND_FIELD ] ) : ( (uint64_t)(-1) );
		pstDecOut->picType[DISPLAY_FRAME] = hDec->picType[ iIdx ];
		pstDecOut->outFrmReliable_0_100[DISPLAY_FRAME] = hDec->FrmReliable_0_100[ iIdx ];
		pstDecOut->isInterlace = hDec->isInterlace[ iIdx ];
		pstDecOut->topFieldFirst = hDec->topFieldFirst[ iIdx ];
		pstDecOut->multiResolution = hDec->multiResolution[ iIdx ];
		pstDecOut->upSampledWidth = hDec->upSampledWidth[ iIdx ];
		pstDecOut->upSampledHeight = hDec->upSampledHeight[ iIdx ];

		hDec->timeStamp[ iIdx ][ FIRST_FIELD] = -10;
		hDec->timeStamp[ iIdx ][SECOND_FIELD] = -10;
		hDec->FrmReliable_0_100[ iIdx ] = 0;

#if DBG_BUF_INFO
		// {
		// 	int32_t j=0;
		// 	NX_MEMORY_INFO *memInfo;
		// 	for( j=0 ; j<3 ; j++ )
		// 	{
		// 		memInfo = (NX_MEMORY_INFO *)pDecOut->outImg.privateDesc[j];
		// 		NX_DbgMsg( DBG_BUF_INFO, ("privateDesc = 0x%.8x\n", memInfo->privateDesc ) );
		// 		NX_DbgMsg( DBG_BUF_INFO, ("align       = 0x%.8x\n", memInfo->align       ) );
		// 		NX_DbgMsg( DBG_BUF_INFO, ("size        = 0x%.8x\n", memInfo->size        ) );
		// 		NX_DbgMsg( DBG_BUF_INFO, ("virAddr     = 0x%.8x\n", memInfo->virAddr     ) );
		// 		NX_DbgMsg( DBG_BUF_INFO, ("phyAddr     = 0x%.8x\n", memInfo->phyAddr     ) );
		// 	}
		// }
		{
			NX_VID_MEMORY_INFO *memInfo = &pDecOut->outImg;
			NX_DbgMsg( DBG_BUF_INFO, ("Phy(0x%08x,0x%08x,0x%08x), Vir(0x%08x,0x%08x,0x%08x), Stride(0x%08x,0x%08x,0x%08x)\n",
				memInfo->luPhyAddr, memInfo->cbPhyAddr, memInfo->crPhyAddr,
				memInfo->luVirAddr, memInfo->cbVirAddr, memInfo->crVirAddr,
				memInfo->luStride , memInfo->cbStride , memInfo->crStride) );
		}
#endif
	}
	else
	{
		pstDecOut->timeStamp[FIRST_FIELD] = -1;
		pstDecOut->timeStamp[SECOND_FIELD] = -1;
	}

	NX_RelMsg( 0, ("NX_VidDecDecodeFrame() Resol:%dx%d, picType=%d, %d, imgIdx = %d\n", pstDecOut->width, pstDecOut->height, pstDecOut->picType[DECODED_FRAME], pstDecOut->picType[DISPLAY_FRAME], pstDecOut->outImgIdx) );
	FUNC_OUT();
	return VID_ERR_NONE;
}

VID_ERROR_E NX_VidDecFlush( NX_VID_DEC_HANDLE hDec )
{
	int32_t ret;
	FUNC_IN();

	if( !hDec->isInitialized  )
	{
		NX_ErrMsg( ("%s Line(%d) : Not initialized!!!\n", __func__, __LINE__));
		return -1;
	}

	ret = ioctl( hDec->hDecDrv, IOCTL_VPU_DEC_FLUSH, NULL );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_DEC_FLUSH ioctl failed!!!\n") );
		return -1;
	}

	DecoderFlushDispInfo( hDec );

	FUNC_OUT();
	return VID_ERR_NONE;
}

VID_ERROR_E NX_VidDecClrDspFlag( NX_VID_DEC_HANDLE hDec, NX_VID_MEMORY_HANDLE hFrameBuf, int32_t iFrameIdx )
{
	int32_t ret;
	FUNC_IN();
	VPU_DEC_CLR_DSP_FLAG_ARG clrFlagArg;

	if( !hDec->isInitialized  )
	{
		return -1;
	}

	clrFlagArg.indexFrameDisplay = iFrameIdx;
	if( NULL != hFrameBuf )
	{
		//	Optional
		clrFlagArg.frameBuffer = *hFrameBuf;
	}
	else
	{
		memset( &clrFlagArg.frameBuffer, 0, sizeof(clrFlagArg.frameBuffer) );
	}

	ret = ioctl( hDec->hDecDrv, IOCTL_VPU_DEC_CLR_DSP_FLAG, &clrFlagArg );
	if( ret < 0 )
	{
		NX_ErrMsg( ("IOCTL_VPU_DEC_CLR_DSP_FLAG ioctl failed!!!\n") );
		return -1;
	}

	FUNC_OUT();
	return VID_ERR_NONE;
}

// Optional Function
VID_ERROR_E NX_VidDecGetFrameType( VID_TYPE_E eCodecType, NX_VID_DEC_IN *pstDecIn, int32_t *piFrameType )
{
	uint8_t *pbyStrm = pstDecIn->strmBuf;
	uint32_t uPreFourByte = (uint32_t)-1;
	int32_t  iFrmType = PIC_TYPE_UNKNOWN;

	if ( (pbyStrm == NULL) || (piFrameType == NULL) )
		return VID_ERR_PARAM;

	if ( eCodecType == NX_AVC_DEC )
	{
		do
		{
			if ( pbyStrm >= (pstDecIn->strmBuf + pstDecIn->strmSize) )		return VID_ERR_NOT_ENOUGH_STREAM;
			uPreFourByte = (uPreFourByte << 8) + *pbyStrm++;

			if ( uPreFourByte == 0x00000001 || uPreFourByte<<8 == 0x00000100 )
			{
				int32_t iNaluType = pbyStrm[0] & 0x1F;

				// Slice start code
				if ( iNaluType == 5 )
				{
					//vld_get_uev(&stStrm);                 // First_mb_in_slice
					iFrmType = PIC_TYPE_IDR;
					break;
				}
				else if ( iNaluType == 1 )
				{
					VLD_STREAM stStrm = { 8, pbyStrm, pstDecIn->strmSize };
					vld_get_uev(&stStrm);                   // First_mb_in_slice
					iFrmType = vld_get_uev(&stStrm);        // Slice type

					if ( iFrmType == 0 || iFrmType == 5 ) 		iFrmType = PIC_TYPE_P;
					else if ( iFrmType == 1 || iFrmType == 6 ) iFrmType = PIC_TYPE_B;
					else if ( iFrmType == 2 || iFrmType == 7 ) iFrmType = PIC_TYPE_I;
					break;
				}
			}
		} while(1);
	}
	else if ( eCodecType == NX_MP2_DEC )
	{
		do
		{
			if ( pbyStrm >= (pstDecIn->strmBuf + pstDecIn->strmSize) )		return VID_ERR_NOT_ENOUGH_STREAM;
			uPreFourByte = (uPreFourByte << 8) + *pbyStrm++;

			// Picture start code
			if ( uPreFourByte == 0x00000100 )
			{
				VLD_STREAM stStrm = { 0, pbyStrm, pstDecIn->strmSize };

				vld_flush_bits( &stStrm, 10 );				// tmoporal_reference
				iFrmType = vld_get_bits( &stStrm, 3 );		// picture_coding_type

				if ( iFrmType == 1 ) 		iFrmType = PIC_TYPE_I;
				else if ( iFrmType == 2 ) 	iFrmType = PIC_TYPE_P;
				else if ( iFrmType == 3 ) 	iFrmType = PIC_TYPE_B;
				break;
			}
		} while(1);
	}
	else
	{
		return VID_ERR_NOT_SUPPORT;
	}

	*piFrameType = iFrmType;
	return VID_ERR_NONE;
}


//
//	Video Decoder APIs
//
//////////////////////////////////////////////////////////////////////////////

VID_ERROR_E NX_VidGetVersion( NX_VID_VERSION *pstVersion )
{
	pstVersion->iMajor = NX_VID_VER_MAJOR;
	pstVersion->iMinor = NX_VID_VER_MINOR;
	pstVersion->iPatch = NX_VID_VER_PATCH;
	pstVersion->iReserved = (int32_t)NULL;
	return VID_ERR_NONE;
}


//////////////////////////////////////////////////////////////////////////////
//
//	Static Internal Functions
//

static int32_t AllocateEncoderMemory( NX_VID_ENC_HANDLE hEnc )
{
	int32_t width, height;

	if( !hEnc || hEnc->hEncDrv<=0 )
	{
		NX_ErrMsg(("invalid encoder handle or driver handle!!!\n"));
		return -1;
	}

	//	Make alligned x16
	width  = ((hEnc->width  + 15) >> 4)<<4;
	height = ((hEnc->height + 15) >> 4)<<4;

	if( hEnc->codecMode == NX_JPEG_ENC )
	{
		int32_t jpegStreamBufSize = width * height * 1.5;
		hEnc->hRefReconBuf[0] = NULL;
		hEnc->hRefReconBuf[1] = NULL;
		hEnc->hSubSampleBuf[0] = NULL;
		hEnc->hSubSampleBuf[1] = NULL;
		hEnc->hBitStreamBuf = NX_AllocateMemory( jpegStreamBufSize, 16 );		//	x16 aligned
		if( 0 == hEnc->hBitStreamBuf ){
			NX_ErrMsg(("hBitStreamBuf allocation failed.(size=%d,align=%d)\n", ENC_BITSTREAM_BUFFER, 16));
			goto ERROR_EXIT;
		}
	}
	else
	{
		int32_t fourcc = FOURCC_MVS0;
		if( hEnc->refChromaInterleave ){
			fourcc = FOURCC_NV12;	//	2 Planar 420( Luminunce Plane + Cb/Cr Interleaved Plane )
		}
		else
		{
			fourcc = FOURCC_MVS0;	//	3 Planar 420( Luminunce plane + Cb Plane + Cr Plane )
		}

		hEnc->hRefReconBuf[0] = NX_VideoAllocateMemory( 64, width, height, NX_MEM_MAP_LINEAR, fourcc );
		if( 0 == hEnc->hRefReconBuf[0] ){
			NX_ErrMsg(("NX_VideoAllocateMemory(64,%d,%d,..) failed.(recon0)\n", width, height));
			goto ERROR_EXIT;
		}

		hEnc->hRefReconBuf[1] = NX_VideoAllocateMemory( 64, width, height, NX_MEM_MAP_LINEAR, fourcc );
		if( 0 == hEnc->hRefReconBuf[1] ){
			NX_ErrMsg(("NX_VideoAllocateMemory(64,%d,%d,..) failed.(recon1)\n", width, height));
			goto ERROR_EXIT;
		}

		hEnc->hSubSampleBuf[0] = NX_AllocateMemory( width*height/4, 16 );	//	x16 aligned
		if( 0 == hEnc->hSubSampleBuf[0] ){
			NX_ErrMsg(("hSubSampleBuf allocation failed.(size=%d,align=%d)\n", width*height, 16));
			goto ERROR_EXIT;
		}

		hEnc->hSubSampleBuf[1] = NX_AllocateMemory( width*height/4, 16 );	//	x16 aligned
		if( 0 == hEnc->hSubSampleBuf[1] ){
			NX_ErrMsg(("hSubSampleBuf allocation failed.(size=%d,align=%d)\n", width*height, 16));
			goto ERROR_EXIT;
		}

		hEnc->hBitStreamBuf = NX_AllocateMemory( ENC_BITSTREAM_BUFFER, 16 );		//	x16 aligned
		if( 0 == hEnc->hBitStreamBuf ){
			NX_ErrMsg(("hBitStreamBuf allocation failed.(size=%d,align=%d)\n", ENC_BITSTREAM_BUFFER, 16));
			goto ERROR_EXIT;
		}
	}

#if (DBG_BUF_ALLOC)
	NX_DbgMsg( DBG_BUF_ALLOC, ("Allocate Encoder Memory\n") );
	NX_DbgMsg( DBG_BUF_ALLOC, ("    hRefReconBuf[0]  : LuPhy(0x%08x), CbPhy(0x%08x), CrPhy(0x%08x)\n", hEnc->hRefReconBuf[0]->luPhyAddr, hEnc->hRefReconBuf[0]->cbPhyAddr, hEnc->hRefReconBuf[0]->crPhyAddr) );
	NX_DbgMsg( DBG_BUF_ALLOC, ("    hRefReconBuf[1]  : LuPhy(0x%08x), CbPhy(0x%08x), CrPhy(0x%08x)\n", hEnc->hRefReconBuf[1]->luPhyAddr, hEnc->hRefReconBuf[1]->cbPhyAddr, hEnc->hRefReconBuf[1]->crPhyAddr) );
	NX_DbgMsg( DBG_BUF_ALLOC, ("    hSubSampleBuf[0] : PhyAddr(0x%08x), VirAddr(0x%08x)\n", hEnc->hSubSampleBuf[0]->phyAddr, hEnc->hSubSampleBuf[0]->virAddr) );
	NX_DbgMsg( DBG_BUF_ALLOC, ("    hSubSampleBuf[1] : PhyAddr(0x%08x), VirAddr(0x%08x)\n", hEnc->hSubSampleBuf[1]->phyAddr, hEnc->hSubSampleBuf[1]->virAddr) );
	NX_DbgMsg( DBG_BUF_ALLOC, ("    hBitStreamBuf    : PhyAddr(0x%08x), VirAddr(0x%08x)\n", hEnc->hBitStreamBuf->phyAddr, hEnc->hBitStreamBuf->virAddr) );
#endif	//	DBG_BUF_ALLOC

	return 0;

ERROR_EXIT:
	FreeEncoderMemory( hEnc );
	return -1;
}

static int32_t FreeEncoderMemory( NX_VID_ENC_HANDLE hEnc )
{
	if( !hEnc )
	{
		NX_ErrMsg(("invalid encoder handle!!!\n"));
		return -1;
	}

	//	Free Reconstruct Buffer & Reference Buffer
	if( hEnc->hRefReconBuf[0] )
	{
		NX_FreeVideoMemory( hEnc->hRefReconBuf[0] );
		hEnc->hRefReconBuf[0] = 0;
	}
	if( hEnc->hRefReconBuf[1] )
	{
		NX_FreeVideoMemory( hEnc->hRefReconBuf[1] );
		hEnc->hRefReconBuf[1] = 0;
	}

	//	Free SubSampleb Buffer
	if( hEnc->hSubSampleBuf[0] )
	{
		NX_FreeMemory( hEnc->hSubSampleBuf[0] );
		hEnc->hSubSampleBuf[0] = 0;
	}
	if( hEnc->hSubSampleBuf[1] )
	{
		NX_FreeMemory( hEnc->hSubSampleBuf[1] );
		hEnc->hSubSampleBuf[1] = 0;
	}

	//	Free Bitstream Buffer
	if( hEnc->hBitStreamBuf )
	{
		NX_FreeMemory( hEnc->hBitStreamBuf );
		hEnc->hBitStreamBuf = 0;
	}

	if( hEnc->hInstanceBuf )
	{
		NX_FreeMemory(hEnc->hInstanceBuf);
		hEnc->hInstanceBuf = 0;
	}

	return 0;
}

static int32_t AllocateDecoderMemory( NX_VID_DEC_HANDLE hDec )
{
	int32_t i, width, height, mvSize;

	if( !hDec || !hDec->hDecDrv )
	{
		NX_ErrMsg(("invalid encoder handle or driver handle!!!\n"));
		return -1;
	}

	//	Make alligned x16
	width  = ((hDec->width  + 15) >> 4)<<4;
	height = ((hDec->height + 15) >> 4)<<4;

	//
	mvSize = ((hDec->width+31)&~31)*((hDec->height+31)&~31);
	mvSize = (mvSize*3)/2;
	mvSize = (mvSize+4) / 5;
	mvSize = ((mvSize+7)/ 8) * 8;
	mvSize = ((mvSize + 4096-1)/4096) * 4096;

	if( width==0 || height==0 || mvSize==0 )
	{
		NX_ErrMsg(("Invalid memory parameters!!!(width=%d, height=%d, mvSize=%d)\n", width, height, mvSize));
		return -1;
	}

	if( !hDec->useExternalFrameBuffer )
	{
		NX_RelMsg( 1, ( "resole : %dx%d, numFrameBuffers=%d\n", width, height, hDec->numFrameBuffers ));
		for( i=0 ; i<hDec->numFrameBuffers ; i++ )
		{
			hDec->hFrameBuffer[i] = NX_VideoAllocateMemory( 4096, width, height, NX_MEM_MAP_LINEAR, FOURCC_MVS0 );	//	Planar Lu/Cb/Cr
			if( 0 == hDec->hFrameBuffer[i] ){
				NX_ErrMsg(("NX_VideoAllocateMemory(64,%d,%d,..) failed.(i=%d)\n", width, height, i));
				goto ERROR_EXIT;
			}
		}
	}

	hDec->hColMvBuffer = NX_AllocateMemory( mvSize*hDec->numFrameBuffers, 4096 );	//	Planar Lu/Cb/Cr
	if( 0 == hDec->hColMvBuffer ){
		NX_ErrMsg(("hColMvBuffer allocation failed(size=%d, align=%d)\n", mvSize*hDec->numFrameBuffers, 4096));
		goto ERROR_EXIT;
	}

	if( hDec->codecStd == CODEC_STD_AVC )
	{
		hDec->hSliceBuffer = NX_AllocateMemory( 2048*2048*3/4, 4096 );		//	x16 aligned
		if( 0 == hDec->hSliceBuffer ){
			NX_ErrMsg(("hSliceBuffer allocation failed.(size=%d,align=%d)\n",  2048*2048*3/4, 4096));
			goto ERROR_EXIT;
		}
	}

	if( hDec->codecStd == CODEC_STD_THO || hDec->codecStd == CODEC_STD_VP3 || hDec->codecStd == CODEC_STD_VP8 )
	{
		hDec->hPvbSliceBuffer = NX_AllocateMemory( 17*4*(2048*2048/256), 4096 );		//	x16 aligned
		if( 0 == hDec->hPvbSliceBuffer ){
			NX_ErrMsg(("hPvbSliceBuffer allocation failed.(size=%d,align=%d)\n", 17*4*(2048*2048/256), 4096));
			goto ERROR_EXIT;
		}
	}

#if DBG_BUF_ALLOC
	NX_DbgMsg( DBG_BUF_ALLOC, ("Allocate Decoder Memory\n") );
	for( i=0 ; i<hDec->numFrameBuffers ; i++ )
	{
		NX_DbgMsg( DBG_BUF_ALLOC, ("    hFrameBuffer[%d]  : LuPhy(0x%08x), CbPhy(0x%08x), CrPhy(0x%08x)\n", i, hDec->hFrameBuffer[i]->luPhyAddr, hDec->hFrameBuffer[i]->cbPhyAddr, hDec->hFrameBuffer[i]->crPhyAddr) );
		NX_DbgMsg( DBG_BUF_ALLOC, ("    hFrameBuffer[%d]  : LuVir(0x%08x), CbVir(0x%08x), CrVir(0x%08x)\n", i, hDec->hFrameBuffer[i]->luVirAddr, hDec->hFrameBuffer[i]->cbVirAddr, hDec->hFrameBuffer[i]->crVirAddr) );
	}
	NX_DbgMsg( DBG_BUF_ALLOC, ("    hBitStreamBuf    : PhyAddr(0x%08x), VirAddr(0x%08x)\n", hDec->hBitStreamBuf->phyAddr, hDec->hBitStreamBuf->virAddr) );
#endif	//	DBG_BUF_ALLOC

	return 0;

ERROR_EXIT:
	FreeDecoderMemory( hDec );
	return -1;
}

static int32_t FreeDecoderMemory( NX_VID_DEC_HANDLE hDec )
{
	int32_t i;
	if( !hDec )
	{
		NX_ErrMsg(("invalid encoder handle!!!\n"));
		return -1;
	}

	if( !hDec->useExternalFrameBuffer )
	{
		//	Free Frame Buffer
		for( i=0 ; i<hDec->numFrameBuffers ; i++ )
		{
			if( hDec->hFrameBuffer[i] )
			{
				NX_FreeVideoMemory( hDec->hFrameBuffer[i] );
				hDec->hFrameBuffer[i] = 0;
			}
		}
	}

	if( hDec->hColMvBuffer )
	{
		NX_FreeMemory( hDec->hColMvBuffer );
		hDec->hColMvBuffer = 0;
	}

	if( hDec->hSliceBuffer )
	{
		NX_FreeMemory( hDec->hSliceBuffer );
		hDec->hSliceBuffer = 0;
	}

	if( hDec->hPvbSliceBuffer )
	{
		NX_FreeMemory( hDec->hPvbSliceBuffer );
		hDec->hPvbSliceBuffer = 0;
	}

	//	Allocate Instance Memory & Stream Buffer
	if( hDec->hInstanceBuf )
	{
		NX_FreeMemory( hDec->hInstanceBuf );
		hDec->hInstanceBuf = 0;
	}

	//	Free Bitstream Buffer
	if( hDec->hBitStreamBuf )
	{
		NX_FreeMemory( hDec->hBitStreamBuf );
		hDec->hBitStreamBuf = 0;
	}

	//	Free USerdata Buffer
	if( hDec->hUserDataBuffer )
	{
		NX_FreeMemory( hDec->hUserDataBuffer );
		hDec->hUserDataBuffer = 0;
	}

	return 0;
}

static void DecoderFlushDispInfo( NX_VID_DEC_HANDLE hDec )
{
	int32_t i;
	for( i=0 ; i<MAX_DEC_FRAME_BUFFERS ; i++ )
	{
		hDec->timeStamp[i][FIRST_FIELD] = -10;
		hDec->timeStamp[i][SECOND_FIELD] = -10;
		hDec->picType[i] = -1;
		hDec->picFlag[i] = -1;
		hDec->multiResolution[i] = 0;
		hDec->isInterlace[i] = -1;
		hDec->topFieldFirst[i] = -1;
		hDec->FrmReliable_0_100[i] = 0;
		hDec->upSampledWidth[i] = 0;
		hDec->upSampledHeight[i] = 0;
	}
}

static void DecoderPutDispInfo( NX_VID_DEC_HANDLE hDec, int32_t iIndex, VPU_DEC_DEC_FRAME_ARG *pDecArg, uint64_t lTimeStamp, int32_t FrmReliable_0_100 )
{
	int32_t iPicStructure = ( hDec->codecStd != CODEC_STD_MPEG2 ) ? ( 0 ) : ( pDecArg->picStructure );

	hDec->picType[ iIndex ] = pDecArg->picType;
	hDec->isInterlace[ iIndex ] = pDecArg->isInterace;

	if( (pDecArg->isInterace == 0) || (iPicStructure == 3) )
	{
		hDec->topFieldFirst[ iIndex ] = pDecArg->topFieldFirst;
		hDec->timeStamp[ iIndex ][ NONE_FIELD ] = lTimeStamp;
	}
	else
	{

		hDec->picFlag[ iIndex ] |= PIC_FLAG_INTERLACE;

		if ( hDec->timeStamp[iIndex][FIRST_FIELD] == (uint32_t)(-10) )
		{
			hDec->topFieldFirst[ iIndex ] = pDecArg->topFieldFirst;
			hDec->timeStamp[ iIndex ][ FIRST_FIELD ] = lTimeStamp;
		}
		else
		{
			hDec->timeStamp[ iIndex ][ SECOND_FIELD ] = lTimeStamp;
		}
	}

	if ( hDec->FrmReliable_0_100[ iIndex ] == 0 )
	{
		hDec->FrmReliable_0_100[ iIndex ] = ( (pDecArg->isInterace == 0) || (pDecArg->npf) || (iPicStructure == 3) || (iIndex >= 0) ) ? ( FrmReliable_0_100 ) : ( FrmReliable_0_100 >> 1 );
	}
	else
	{
		hDec->FrmReliable_0_100[ iIndex ] += ( FrmReliable_0_100 >> 1 );
	}

	if( hDec->codecStd == CODEC_STD_AVC )
	{
		if( pDecArg->picTypeFirst == 6 || pDecArg->picType == 0 || pDecArg->picType == 6 )
		{
			hDec->picFlag[ iIndex ] |= PIC_FLAG_KEY;
		}
	}
	else if( hDec->codecStd == CODEC_STD_VC1 )
	{
		hDec->multiResolution[ iIndex ] = pDecArg->multiRes;
	}
	else if (hDec->codecStd == CODEC_STD_VP8)
	{
		if (pDecArg->vp8ScaleInfo.hScaleFactor == 0)       hDec->upSampledWidth[ iIndex ] = 0;
		else if (pDecArg->vp8ScaleInfo.hScaleFactor == 1) hDec->upSampledWidth[ iIndex ] = pDecArg->vp8ScaleInfo.picWidth * 5 / 4;
		else if (pDecArg->vp8ScaleInfo.hScaleFactor == 2) hDec->upSampledWidth[ iIndex ] = pDecArg->vp8ScaleInfo.picWidth * 5 / 3;
		else if (pDecArg->vp8ScaleInfo.hScaleFactor == 3) hDec->upSampledWidth[ iIndex ] = pDecArg->vp8ScaleInfo.picWidth * 2;

		if (pDecArg->vp8ScaleInfo.vScaleFactor == 0)       hDec->upSampledHeight[ iIndex ] = 0;
		else if (pDecArg->vp8ScaleInfo.vScaleFactor == 1) hDec->upSampledHeight[ iIndex ] = pDecArg->vp8ScaleInfo.picHeight * 5 / 4;
		else if (pDecArg->vp8ScaleInfo.vScaleFactor == 2) hDec->upSampledHeight[ iIndex ] = pDecArg->vp8ScaleInfo.picHeight * 5 / 3;
		else if (pDecArg->vp8ScaleInfo.vScaleFactor == 3) hDec->upSampledHeight[ iIndex ] = pDecArg->vp8ScaleInfo.picHeight * 2;
	}
}

//
//	End of Static Functions
//
//////////////////////////////////////////////////////////////////////////////
