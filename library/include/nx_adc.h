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

#include <stdint.h>

#ifndef __NX_ADC_H__
#define __NX_ADC_H__

typedef void *NX_ADC_HANDLE;

enum NX_ADC {
	NX_ADC_ERROR = -1,
	NX_ADC0,
	NX_ADC1,
	NX_ADC2,
	NX_ADC3,
	NX_ADC4,
	NX_ADC5,
	NX_ADC6,
	NX_ADC7,
	NX_ADC_MAX,
};

#ifdef __cplusplus
extern "C" {
#endif

NX_ADC_HANDLE	NX_AdcInit	( int32_t nAdc );
void			NX_AdcDeinit( NX_ADC_HANDLE hAdc );
int32_t			NX_AdcRead	( NX_ADC_HANDLE hAdc );

#ifdef __cplusplus
}
#endif

#endif	// __NX_ADC_H__
