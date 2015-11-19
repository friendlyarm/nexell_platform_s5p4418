#ifndef __NX_IOS7816_H__
#define __NX_IOS7816_H__

#include <stdint.h>

#include <nx_gpio.h>
#include <nx_pwm.h>

//
//	Supported Data Frequency :
//		9600, 19200, 38400, 57600,
//		115200 ,230400 ,460800 ,500000 ,
//		576000 ,921600 ,1000000, 1152000,
//		1500000, 2000000, 2500000, 3000000,
//		3500000, 4000000,
//
typedef struct NX_ISO7816_INFO *NX_ISO7816_HANDLE;

#ifdef __cplusplus
extern "C" {
#endif

NX_ISO7816_HANDLE NX_InitISO7816( void );
void NX_DeinitISO7816( NX_ISO7816_HANDLE handle );
int32_t NX_WriteISO7816( NX_ISO7816_HANDLE handle, uint8_t *buf, int32_t size );
int32_t NX_ReadISO7816( NX_ISO7816_HANDLE handle, uint8_t *buf, int32_t size, int32_t timeout );
int32_t NX_SetClockFreqISO7816( NX_ISO7816_HANDLE handle, int32_t frequency );
int32_t NX_SetDataFreqIOS7816( NX_ISO7816_HANDLE handle, int32_t frequency );
int32_t NX_SetResetISO7816( NX_ISO7816_HANDLE handle, int32_t value );

#ifdef __cplusplus
}
#endif

#endif	//	__NX_IOS7816_H__
