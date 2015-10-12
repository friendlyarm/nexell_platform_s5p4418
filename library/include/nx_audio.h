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

#ifndef	__NX_AUDIO_H__
#define	__NX_AUDIO_H__

#include <stdbool.h>
#include <stdint.h>

typedef void*	NX_AUDIO_HANDLE;

typedef enum {
	AUDIO_TYPE_PLAYBACK,	// play
	AUDIO_TYPE_CAPTURE,		// record
} NX_AUDIO_TYPE;

#ifdef __cplusplus
extern "C" {
#endif

void* PlayAsyncWaveFile( const char *fileName, int isMix, int *error );
void StopWaveFile( void *handle, int bDrop );

NX_AUDIO_HANDLE		NX_AudioInit( void );
void				NX_AudioDeinit( NX_AUDIO_HANDLE hAudio );
int32_t				NX_AudioPlay( NX_AUDIO_HANDLE hAudio, const char *filename );
int32_t				NX_AudioStop( NX_AUDIO_HANDLE hAudio, bool bImmediate );

int32_t				NX_AudioSetVolume( NX_AUDIO_HANDLE hAudio, NX_AUDIO_TYPE nAudioType, int32_t percent );
int32_t				NX_AudioGetVolume( NX_AUDIO_HANDLE hAudio, NX_AUDIO_TYPE nAudioType );

#ifdef __cplusplus
}
#endif

#endif	//	__NX_AUDIO_H__
