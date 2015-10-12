#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>		//	open, O_RDWR
#include <unistd.h>		//	close
#include "nx_pwm.h"

struct NX_PWM_INFO {
//	int			handle;				//	Device Driver Handle
	int32_t		port;				//
	uint32_t	frequency;			//
	uint32_t	duty;				//
	char		sysIfStr[64];
};

NX_PWM_HANDLE NX_PwmInit ( int32_t port )
{
	char buf[64];
	NX_PWM_HANDLE pInfo;
	if( port >= PWM_MAX || port < 0 )
	{
		printf("Error : Invalid port Number Max 0 ~ %d\n", PWM_MAX-1);
		return NULL;
	}

	snprintf( buf, sizeof(buf), "%s%d", PWM_SYS_IF_PREFIX, port );
	if( access(buf, F_OK) ) {
		printf("Error : %s%d file is not exist.\n", PWM_SYS_IF_PREFIX, port);
		return NULL;
	}

	pInfo = (NX_PWM_HANDLE)malloc(sizeof(struct NX_PWM_INFO));
	pInfo->port = port;
	pInfo->frequency = 0;
	pInfo->duty = 0;
	strncpy( pInfo->sysIfStr, strdup(buf), strlen(buf) );

	return pInfo;
}

//
//	Parameters
//		int32_t off : forced off PWM signal.
//
void NX_PwmDeinit ( NX_PWM_HANDLE hPwm, int32_t off )
{
	if( hPwm )
	{
		//	PWM Power Off
		if( off )
		{
			NX_PwmSetFreqDuty( hPwm, 0, 0 );
		}
		free( hPwm );
	}
}

//
//	Parameters
//		int32_t freq : frequency
//		int32_t duty : duty cycle
//
int32_t NX_PwmSetFreqDuty ( NX_PWM_HANDLE hPwm, uint32_t freq, uint32_t duty )
{
	int fd;
	char buf[64];
	size_t len;
	int32_t ret = PWM_ERR_NONE;
	if( !hPwm )
	{
		return PWM_ERR_INVALID_HANDLE;
	}
	if( 0 > (fd = open( hPwm->sysIfStr, O_RDWR )) )
	{
		printf("Error : Failed open( %s%d ).\n", PWM_SYS_IF_PREFIX, hPwm->port);
		return PWM_ERR_ACCESS_FAILED;
	}

	memset( buf, 0, sizeof(buf));
	snprintf( buf, sizeof(buf), "%d,%d", freq, duty );
	len = write( fd, buf, strlen(buf) );

	if( len != strlen(buf) )
	{
		ret = PWM_ERR_WRITE_FAILED;
	}
	close( fd );
	return ret;
}

int32_t NX_PwmGetInfo ( NX_PWM_HANDLE hPwm, uint32_t *freq, uint32_t *duty )
{
	int fd;
	char buf[64];
	int rdSize;
	if( !hPwm )
	{
		return PWM_ERR_INVALID_HANDLE;
	}

	if( 0 > (fd = open( hPwm->sysIfStr, O_RDWR )) )
	{
		printf("Error : Failed open( %s%d ).\n", PWM_SYS_IF_PREFIX, hPwm->port);
		return PWM_ERR_ACCESS_FAILED;
	}

	rdSize = read( fd, buf, sizeof(buf) );

	if( rdSize <= 0 )
	{
		printf("Error : %s() Read failed\n", __func__);
	}

	sscanf( buf, "%d,%d", freq, duty );
	close(fd);

	return PWM_ERR_NONE;
}
