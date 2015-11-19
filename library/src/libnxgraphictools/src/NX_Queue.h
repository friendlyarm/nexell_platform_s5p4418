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

#ifndef __NX_QUEUE_H__
#define __NX_QUEUE_H__

#include <stdint.h>
#include <pthread.h>

#define NX_MAX_QUEUE_ELEMENT	128

typedef struct tag_NX_QUEUE_INFO {
	uint32_t		head;
	uint32_t		tail;
	uint32_t		maxElement;
	uint32_t		curElement;
	void			*pElements[NX_MAX_QUEUE_ELEMENT];
	pthread_mutex_t	hMutex;
} NX_QUEUE_INFO;

typedef NX_QUEUE_INFO		*NX_QUEUE_HANDLE;

NX_QUEUE_HANDLE	NX_QueueInit		( uint32_t maxNumElement );
void			NX_QueueDeinit		( NX_QUEUE_HANDLE hQueue );
int32_t			NX_QueuePush		( NX_QUEUE_HANDLE hQueue, void *pElement );
int32_t			NX_QueuePop			( NX_QUEUE_HANDLE hQueue, void **pElement );
uint32_t		NX_QueueGetCount	( NX_QUEUE_HANDLE hQueue );

#endif	// __NX_QUEUE_H__