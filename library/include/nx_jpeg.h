//------------------------------------------------------------------------------
//
//  Copyright (C) 2014 Nexell Co. All Rights Reserved
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

#include <stdint.h>

#ifndef	__NX_JPEG_H__
#define	__NX_JPEG_H__

typedef enum {
	NX_JPEG_FORMAT_YUV444 = 0,
	NX_JPEG_FORMAT_YUV422,
	NX_JPEG_FORMAT_YUV420,
	NX_JPEG_FORMAT_YUVGRAY,
	NX_JPEG_FORMAT_YUV440,
} NX_JPEG_FORMAT;

typedef enum {
	NX_JPEG_DECODE_FORMAT_RGB = 0,
	NX_JPEG_DECODE_FORMAT_BGR,
	NX_JPEG_DECODE_FORMAT_RGBX,
	NX_JPEG_DECODE_FORMAT_BGRX,
	NX_JPEG_DECODE_FORMAT_XBGR,
	NX_JPEG_DECODE_FORMAT_XRGB,
	NX_JPEG_DECODE_FORMAT_GRAY,
	NX_JPEG_DECODE_FORMAT_RGBA,
	NX_JPEG_DECODE_FORMAT_BGRA,
	NX_JPEG_DECODE_FORMAT_ABGR,
	NX_JPEG_DECODE_FORMAT_ARGB,
	NX_JPEG_DECODE_FORMAT_YUV,
} NX_JPEG_DECODE_FORMAT;

typedef struct {
	int32_t			luWidth;
	int32_t			luHeight;
	int32_t			cbWidth;
	int32_t			cbHeight;
	int32_t			crWidth;
	int32_t			crHeight;
} NX_JPEG_BUFFER_ALIGN;

typedef void *NX_JPEG_HANDLE;

typedef struct {
	int32_t 		width;		// Image width
	int32_t 		height;		// Image height
	NX_JPEG_FORMAT	format;		// Level of chrominance subsampling used when compressing the JPEG image
} NX_JPEG_HEADER_INFO;

#ifdef __cplusplus
extern "C" {
#endif

NX_JPEG_HANDLE 	NX_JpegOpen( uint8_t *path );
void			NX_JpegClose( NX_JPEG_HANDLE hJpeg );

int32_t			NX_JpegGetHeaderInfo( NX_JPEG_HANDLE hJpeg, NX_JPEG_HEADER_INFO *pHeaderInfo );
int32_t			NX_JpegDecode( NX_JPEG_HANDLE hJpeg, uint8_t *pOutBuf, NX_JPEG_DECODE_FORMAT decFormat );

int64_t			NX_JpegRequireBufSize( NX_JPEG_HANDLE hJpeg, NX_JPEG_DECODE_FORMAT decFormat );
int32_t			NX_JpegGetYuvBufferAlign( NX_JPEG_HANDLE hJpeg, NX_JPEG_BUFFER_ALIGN *pAlignInfo );

#ifdef __cplusplus
}
#endif

#endif	//	__NX_JPEG_H__
