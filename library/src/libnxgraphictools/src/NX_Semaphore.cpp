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
#include "NX_Semaphore.h"

NX_SEM_HANDLE	NX_SemaporeInit( int32_t maxValue, int32_t initValue )
{
	NX_SEM_HANDLE hSem = (NX_SEM_HANDLE)malloc( sizeof(NX_SEM_INFO) );
	
	if( !hSem ) return NULL;

	memset( hSem, 0x00, sizeof(NX_SEM_INFO) );
	hSem->curValue	= initValue;
	hSem->maxValue	= maxValue;
	pthread_cond_init( &hSem->hCond, NULL );
	pthread_mutex_init( &hSem->hMutex, NULL );

	return hSem;
}

void	NX_SemaporeDeinit( NX_SEM_HANDLE hSem )
{
	int32_t i = 0;
	
	pthread_mutex_lock( &hSem->hMutex );
	for( i = 0; i < hSem->maxValue; i++ )
		pthread_cond_signal( &hSem->hCond );
	pthread_mutex_unlock( &hSem->hMutex );

	pthread_cond_destroy( &hSem->hCond );
	pthread_mutex_destroy( &hSem->hMutex );

	if( hSem )
		free( hSem );
}

int32_t NX_SemaporePost( NX_SEM_HANDLE hSem )
{
	int32_t ret = 0;
	pthread_mutex_lock( &hSem->hMutex );
	hSem->curValue++;
	pthread_cond_signal( &hSem->hCond );
	if( hSem->curValue > hSem->maxValue )
		ret = -1;
	pthread_mutex_unlock( &hSem->hMutex );

	return ret;
}

int32_t NX_SemaporePend( NX_SEM_HANDLE hSem )
{
	int32_t ret = 0;
	pthread_mutex_lock( &hSem->hMutex );
	if( hSem->curValue == 0 ) {
		//pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime);
		ret = pthread_cond_wait( &hSem->hCond, &hSem->hMutex );
		hSem->curValue--;
	}
	else if( hSem->curValue < 0 ) {
		ret = -1;
	}
	else {
		hSem->curValue--;
		ret = 0;
	}
	pthread_mutex_unlock( &hSem->hMutex );

	return ret;
}

