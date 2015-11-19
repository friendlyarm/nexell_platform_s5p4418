#ifndef __NX_GRAPHICTOOLS_H__
#define	__NX_GRAPHICTOOLS_H__

#include <nx_alloc_mem.h>

typedef struct NX_GT_DEINT_INFO  *NX_GT_DEINT_HANDLE;
typedef struct NX_GT_SCALER_INFO *NX_GT_SCALER_HANDLE;
typedef struct NX_GT_RGB2YUV_INFO *NX_GT_RGB2YUV_HANDLE;
typedef struct NX_GT_YUV2RGB_INFO *NX_GT_YUV2RGB_HANDLE;


#ifdef __cplusplus
extern "C" {
#endif	//	__cplusplus

#define	MAX_GRAPHIC_BUF_SIZE		22

//	Error Message
#define	ERR_NONE			( 0)
#define ERR_DEINTERLACE		(-2)
#define	ERR_SCALE			(-3)
#define ERR_NOT_INIT		(-14)
#define	ERR_SURFACE_CREATE	(-15)
#define ERR_BUF_SIZE		(-16)


//
//				Deinterlace Using 3D Accelerator
//			( ex) 1080i 30 picuter --> 1080p 30 picture )
//
//		Usage : Open --> Deinterlace --> Close
//
NX_GT_DEINT_HANDLE NX_GTDeintOpen( int srcWidth, int srcHeight, int numOutBuf );
int32_t NX_GTDeintDoDeinterlace( NX_GT_DEINT_HANDLE handle, NX_VID_MEMORY_INFO *pInBuf, NX_VID_MEMORY_INFO *pOutBuf );
void NX_GTDeintClose( NX_GT_DEINT_HANDLE handle );


//
//				Scaler Using 3D Accelerator
//					( Bi-Linear Filter )
//
//		Usage : Open --> Scale --> Close
//
NX_GT_SCALER_HANDLE NX_GTSclOpen(  int srcWidth, int srcHeight, int dstWidth, int dstHeight, int numOutBuf );
int32_t NX_GTSclDoScale(NX_GT_SCALER_HANDLE handle, NX_VID_MEMORY_INFO *pInBuf, NX_VID_MEMORY_INFO *pOutBuf );
void NX_GTSclClose( NX_GT_SCALER_HANDLE handle );

//
//		Color Space Converter Using 3D Accelerator
//				( RGB32 to YUV(NV12) )
//
//		Usage : Open --> CSC --> Close
//
NX_GT_RGB2YUV_HANDLE NX_GTRgb2YuvOpen( int srcWidth, int srcHeight, int dstWidth, int dstHeight, int numOutBuf );
int32_t NX_GTRgb2YuvDoConvert( NX_GT_RGB2YUV_HANDLE handle, int inMemIonFd, NX_VID_MEMORY_INFO *pOutBuf );
void NX_GTRgb2YuvClose( NX_GT_RGB2YUV_HANDLE handle );

//
//		Color Space Converter Using 3D Accelerator
//				( YUV to RGB32 )
//
//		Usage : Open --> CSC --> Close
//
NX_GT_YUV2RGB_HANDLE NX_GTYuv2RgbOpen( int srcWidth, int srcHeight, int dstWidth, int dstHeight, int numOutBuf );
int32_t NX_GTYuv2RgbDoConvert( NX_GT_YUV2RGB_HANDLE handle, NX_VID_MEMORY_INFO *pOutBuf, int inMemIonFd );
void NX_GTYuv2RgbClose( NX_GT_YUV2RGB_HANDLE handle );

#ifdef __cplusplus
};
#endif	//	__cplusplus

#endif	// __NX_GRAPHICTOOLS_H__