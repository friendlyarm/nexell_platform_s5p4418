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
#include <assert.h>

#include "NX_Queue.h"

NX_QUEUE_HANDLE	NX_QueueInit( uint32_t maxNumElement )
{
	NX_QUEUE_HANDLE hQueue = (NX_QUEUE_HANDLE)malloc( sizeof(NX_QUEUE_INFO) );

	memset( hQueue, 0x00, sizeof(NX_QUEUE_INFO) );
	if( maxNumElement > NX_MAX_QUEUE_ELEMENT ){
		return NULL;
	}
	if( 0 != pthread_mutex_init( &hQueue->hMutex, NULL ) ){
		return NULL;
	}
	hQueue->maxElement = maxNumElement;
	
	return hQueue;
}

void NX_QueueDeinit( NX_QUEUE_HANDLE hQueue )
{
	assert( NULL != hQueue );
	pthread_mutex_destroy( &hQueue->hMutex );
	if( hQueue )
		free( hQueue );
}

int32_t NX_QueuePush( NX_QUEUE_HANDLE hQueue, void *pElement )
{
	assert( NULL != hQueue );
	pthread_mutex_lock( &hQueue->hMutex );
	//	Check Buffer Full
	if( hQueue->curElement >= hQueue->maxElement ){
		pthread_mutex_unlock( &hQueue->hMutex );
		return -1;
	} else    {
		hQueue->pElements[hQueue->tail] = pElement;
		hQueue->tail = (hQueue->tail+1) % hQueue->maxElement;
		hQueue->curElement++;
	}
	pthread_mutex_unlock( &hQueue->hMutex );
	return 0;
}

int32_t NX_QueuePop( NX_QUEUE_HANDLE hQueue, void **pElement )
{
	assert( NULL != hQueue );
	pthread_mutex_lock( &hQueue->hMutex );
	//	Check Buffer Full
	if( hQueue->curElement == 0 ){
		pthread_mutex_unlock( &hQueue->hMutex );
		return -1;
	}else{
		*pElement = hQueue->pElements[hQueue->head];
		hQueue->head = (hQueue->head + 1) % hQueue->maxElement;
		hQueue->curElement--;
	}
	pthread_mutex_unlock( &hQueue->hMutex );
	return 0;
}

uint32_t NX_QueueGetCount( NX_QUEUE_HANDLE hQueue )
{
	assert( NULL != hQueue );
	return hQueue->curElement;
}


