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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>

#include <turbojpeg.h>
#include <nx_jpeg.h>

typedef struct {
	uint8_t 			*inBuf;
	uint32_t			inSize;

	int32_t				width;
	int32_t				height;
	int32_t				format;

	tjhandle			hTurboJpeg;
	pthread_mutex_t 	hLock;
} NX_JPEG_HANDLE_INFO;

NX_JPEG_HANDLE NX_JpegOpen( uint8_t *path )
{
	NX_JPEG_HANDLE_INFO *pInfo = NULL;
	tjhandle hTurboJpeg = NULL;
	FILE *inFile = NULL;

	int32_t width, height, subsampling;

	inFile = fopen( (char*)path, "rb" );
	if( NULL == inFile ) {
		printf("%s(): Error, cannot open file. (%s)\n", __func__, path);
		return NULL;
	}

	hTurboJpeg = tjInitDecompress();
	if( NULL == hTurboJpeg ) {
		printf("%s(): Error, cannot create jpeg decoder instance.\n", __func__);
		if( inFile ) fclose( inFile );
		return NULL;
	}

	pInfo = (NX_JPEG_HANDLE_INFO*)malloc( sizeof(NX_JPEG_HANDLE_INFO) );

	fseek( inFile, 0, SEEK_END);
	pInfo->inSize = ftell( inFile );
	fseek( inFile, 0, SEEK_SET);

	pInfo->inBuf = (uint8_t*)malloc(pInfo->inSize);
	fread( pInfo->inBuf, 1, pInfo->inSize, inFile );

	tjDecompressHeader2( hTurboJpeg, pInfo->inBuf, pInfo->inSize, &width, &height, &subsampling );

	pInfo->width		= width;
	pInfo->height		= height;
	pInfo->format		= subsampling;
	pInfo->hTurboJpeg	= hTurboJpeg;

	pthread_mutex_init( &pInfo->hLock, NULL );

	if( inFile ) fclose( inFile );
	
	return (NX_JPEG_HANDLE_INFO*)pInfo;
}

void NX_JpegClose( NX_JPEG_HANDLE hJpeg )
{
	NX_JPEG_HANDLE_INFO	*pInfo = (NX_JPEG_HANDLE_INFO*)hJpeg;
	assert( pInfo );

	pthread_mutex_destroy( &pInfo->hLock );

	if( pInfo->hTurboJpeg ) tjDestroy( pInfo->hTurboJpeg );
	if( pInfo->inBuf ) free( pInfo->inBuf );
	if( pInfo ) free( pInfo );
}

int32_t NX_JpegGetHeaderInfo( NX_JPEG_HANDLE hJpeg, NX_JPEG_HEADER_INFO *pHeaderInfo )
{
	NX_JPEG_HANDLE_INFO	*pInfo = (NX_JPEG_HANDLE_INFO*)hJpeg;
	assert( pInfo );

	pHeaderInfo->width		= pInfo->width;
	pHeaderInfo->height 	= pInfo->height;
	pHeaderInfo->format		= pInfo->format;

	return 0;
}

int32_t NX_JpegDecode( NX_JPEG_HANDLE hJpeg, uint8_t *pOutBuf, NX_JPEG_DECODE_FORMAT decFormat )
{
	NX_JPEG_HANDLE_INFO	*pInfo = (NX_JPEG_HANDLE_INFO*)hJpeg;
	assert( pInfo );

	if( NX_JPEG_DECODE_FORMAT_YUV == decFormat ) {
		if( 0 > tjDecompressToYUV( pInfo->hTurboJpeg, pInfo->inBuf, pInfo->inSize, pOutBuf, 0) ) {
			printf("%s(): Error, cannot decoding jpeg.\n", __func__);
			return -1;
		}
	}
	else { 
		if( 0 > tjDecompress2( pInfo->hTurboJpeg, pInfo->inBuf, pInfo->inSize, pOutBuf, pInfo->width, 0, pInfo->height, decFormat, TJFLAG_FASTDCT) ) {
			printf("%s(): Error, cannot decoding jpeg.\n", __func__);
			return -1;
		}
	}
	
	return 0;
}

int64_t NX_JpegRequireBufSize( NX_JPEG_HANDLE hJpeg, NX_JPEG_DECODE_FORMAT decFormat )
{
	NX_JPEG_HANDLE_INFO	*pInfo = (NX_JPEG_HANDLE_INFO*)hJpeg;
	int64_t bufSize = 0;
	assert( pInfo );

	if( NX_JPEG_DECODE_FORMAT_YUV == decFormat ) {
		bufSize = tjBufSizeYUV( pInfo->width, pInfo->height, pInfo->format );
	}
	else {
		if( (NX_JPEG_DECODE_FORMAT_RGB == decFormat) || (NX_JPEG_DECODE_FORMAT_BGR == decFormat) ) {
			bufSize = pInfo->width * pInfo->height * 3;
		}
		else {
			bufSize = pInfo->width * pInfo->height * 4;	
		}
	}

	return bufSize;
}

#define PAD(v, p) ((v+(p)-1)&(~((p)-1)))

int32_t NX_JpegGetYuvBufferAlign( NX_JPEG_HANDLE hJpeg, NX_JPEG_BUFFER_ALIGN *pAlignInfo )
{
	NX_JPEG_HANDLE_INFO	*pInfo = (NX_JPEG_HANDLE_INFO*)hJpeg;
	int pw, ph, cw, ch;

	assert( pInfo );

	pw = PAD( pInfo->width, tjMCUWidth[pInfo->format] / 8 );
	ph = PAD( pInfo->height, tjMCUHeight[pInfo->format] / 8 );

	cw = pw * 8 / tjMCUWidth[pInfo->format];
	ch = ph * 8 / tjMCUHeight[pInfo->format];

	pAlignInfo->luWidth 	= PAD( pw, 4 );
	pAlignInfo->luHeight 	= ph;
	pAlignInfo->cbWidth		= (pInfo->format != NX_JPEG_FORMAT_YUVGRAY) ? PAD( cw, 4 ) : 0;
	pAlignInfo->cbHeight	= (pInfo->format != NX_JPEG_FORMAT_YUVGRAY) ? ch : 0;
	pAlignInfo->crWidth		= (pInfo->format != NX_JPEG_FORMAT_YUVGRAY) ? PAD( cw, 4 ) : 0;
	pAlignInfo->crHeight	= (pInfo->format != NX_JPEG_FORMAT_YUVGRAY) ? ch : 0;

	return 0;
}