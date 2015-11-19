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
#include <stdlib.h>		// malloc
#include <unistd.h>		// close
#include <fcntl.h>		// open
#include <assert.h>		// assert

#include "nx_adc.h"

// read value
// node : /sys/devices/platform/nxp-adc/iio:device0/in_voltage[num]_raw

typedef struct {
	int32_t port;		// adc port
} NX_ADC_HANDLE_INFO;

NX_ADC_HANDLE NX_AdcInit( int32_t nAdc )
{
	NX_ADC_HANDLE_INFO *pInfo;
	
	if( nAdc <= NX_ADC_ERROR || nAdc >= NX_ADC_MAX ) {
		printf("over range adc number! ( ADC%d )\n", nAdc);
		return NULL;
	}

	pInfo = (NX_ADC_HANDLE_INFO*)malloc( sizeof(NX_ADC_HANDLE_INFO) );
	pInfo->port	= nAdc;

	return (NX_ADC_HANDLE_INFO*)pInfo;
}

void NX_AdcDeinit( NX_ADC_HANDLE hAdc)
{
	NX_ADC_HANDLE_INFO *pInfo;
	
	assert(hAdc);
	pInfo = (NX_ADC_HANDLE_INFO*)hAdc;
	
	if( pInfo )		free( pInfo );
}

int32_t NX_AdcRead( NX_ADC_HANDLE hAdc)
{
	NX_ADC_HANDLE_INFO *pInfo;
	int32_t fd, len;
	char buf[64];

	assert( hAdc );
	pInfo = (NX_ADC_HANDLE_INFO*)hAdc;
	
	len = snprintf(buf, sizeof(buf), "/sys/devices/platform/nxp-adc/iio:device0/in_voltage%d_raw", pInfo->port);
	if( 0 > (fd = open(buf, O_RDONLY)) || len <= 0 ) {
		printf("cannot open adc%d\n", pInfo->port);
		goto ERROR;
	}

	if( 0 > (len = read(fd, buf, sizeof(buf))) ) {
		printf("cannot read adc%d value!\n", pInfo->port);
		goto ERROR;
	}

	if( fd ) close(fd);

	return atoi(buf);

ERROR :
	if( fd ) close(fd);
	return NX_ADC_ERROR;
}
