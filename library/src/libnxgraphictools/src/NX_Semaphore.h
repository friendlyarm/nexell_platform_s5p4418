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

#ifndef __NX_SEMAPORE_H__
#define __NX_SEMAPORE_H__

#include <stdint.h>
#include <pthread.h>

typedef struct tag_NX_SEM_INFO {
	int32_t			curValue;
	int32_t			maxValue;
	pthread_cond_t	hCond;
	pthread_mutex_t	hMutex;
} NX_SEM_INFO;

typedef NX_SEM_INFO		*NX_SEM_HANDLE;

NX_SEM_HANDLE	NX_SemaporeInit		( int32_t maxValue, int32_t initValue );
void			NX_SemaporeDeinit	( NX_SEM_HANDLE hSem );
int32_t			NX_SemaporePost		( NX_SEM_HANDLE hSem );
int32_t			NX_SemaporePend		( NX_SEM_HANDLE hSem );

#endif	// __NX_SEMAPORE_H__