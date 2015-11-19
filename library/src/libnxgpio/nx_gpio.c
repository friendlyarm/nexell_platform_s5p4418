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
#include <stdlib.h>		// malloc
#include <unistd.h>		// write
#include <fcntl.h>		// open
#include <assert.h>		// assert
#include <pthread.h>
#include <sys/poll.h>

#include "nx_gpio.h"

// create node / remove node
// node : /sys/calss/gpio/gpio[num]
// echo [gpio_num] > /sys/class/gpio/export
// echo [gpio_num] > /sys/class/gpio/unexport

// direction handling
// node : /sys/class/gpio/gpio[num]/direction
// echo "in" > direction
// echo "out" > direction

// write value / read value
// node : /sys/class/gpio/gpio[num]/value
// echo [valie] > value
// cat value

typedef struct {
	int32_t			port;			// gpio number
	int32_t 		direction;		// gpio direction
	int32_t			bPost;
	pthread_mutex_t hLock;
} NX_GPIO_HANDLE_INFO;

NX_GPIO_HANDLE NX_GpioInit( int32_t nGpio )
{
	int32_t fd = 0, len = 0;
	NX_GPIO_HANDLE_INFO	*pInfo;
	char buf[64];

	if( nGpio <= GPIO_ERROR || nGpio >= GPIO_MAX ) {
		printf("over range gpio number! ( gpio%d )\n", nGpio);
		return NULL;
	}

	// Check gpio node.
	len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d", nGpio);

	if( !access(buf, F_OK) ) {
		printf("gpio%d already initialize.\n", nGpio);
	}
	else {

		if( 0 > (fd = open("/sys/class/gpio/export", O_WRONLY)) ){
			printf("cannot export gpio%d\n", nGpio);
			return NULL;
		}

		len = snprintf(buf, sizeof(buf), "%d", nGpio);
	
		if( 0 > write(fd, buf, len) ) {
			printf("cannot write gpio%d export file!\n", nGpio);
			close(fd);
			return NULL;
		}
	}

	// create handle.
	pInfo = (NX_GPIO_HANDLE_INFO*)malloc( sizeof(NX_GPIO_HANDLE_INFO) );

	pInfo->port			= nGpio;
	pInfo->direction	= GPIO_DIRECTION_IN;	// default input
	pInfo->bPost		= false;

	close(fd);

	pthread_mutex_init( &pInfo->hLock, NULL );

	return (NX_GPIO_HANDLE_INFO*)pInfo;
}

void	NX_GpioDeinit( NX_GPIO_HANDLE hGpio )
{
	int32_t fd = 0, len = 0;
	NX_GPIO_HANDLE_INFO	*pInfo;
	char buf[64];

	assert(hGpio);
	pInfo = (NX_GPIO_HANDLE_INFO*)hGpio;

	if( 0 > (fd = open("/sys/class/gpio/unexport", O_WRONLY)) ){
		printf("cannot unexport gpio%d\n", pInfo->port);
	}
	
	len = snprintf(buf, sizeof(buf), "%d", pInfo->port);

	if( 0 > write(fd, buf, len) ) {
		printf("cannot write gpio%d unexport file!\n", pInfo->port);
		close(fd);
	}
	
	close(fd);

	pthread_mutex_destroy( &pInfo->hLock );

	if(pInfo)
		free(pInfo);
}

int32_t	NX_GpioDirection( NX_GPIO_HANDLE hGpio, int32_t direction )
{
	int32_t fd = 0, len = 0;
	NX_GPIO_HANDLE_INFO	*pInfo;
	char buf[64];

	assert(hGpio);
	pInfo = (NX_GPIO_HANDLE_INFO*)hGpio;

	if( direction < GPIO_DIRECTION_IN || direction > GPIO_DIRECTION_OUT ) {
		printf("unknown gpio direction! ( %d )\n", direction);
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/direction", pInfo->port);
	if( 0 > (fd = open(buf, O_WRONLY)) ){
		printf("cannot open gpio%d direction!\n", pInfo->port);
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "%s", (direction == GPIO_DIRECTION_IN) ? "in" : "out");

	if( 0 > write(fd, buf, len) ) {
		printf("cannot write gpio%d direction!\n", pInfo->port);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

int32_t	NX_GpioSetValue( NX_GPIO_HANDLE hGpio, int32_t value )
{
	int32_t fd = 0, len = 0;
	NX_GPIO_HANDLE_INFO	*pInfo;
	char buf[64];

	assert(hGpio);
	pInfo = (NX_GPIO_HANDLE_INFO*)hGpio;

	if( value < 0 || value > 1 ) {
		printf("unknown gpio value! ( %d )\n", value);
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", pInfo->port);
	if( 0 > (fd = open(buf, O_RDWR)) ){
		printf("cannot open gpio%d value!\n", pInfo->port);
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "%d", value);
	if( 0 > write(fd, buf, len) ) {
		printf("cannot write gpio%d value!\n", pInfo->port);
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

int32_t	NX_GpioGetValue( NX_GPIO_HANDLE hGpio )
{
	int32_t fd = 0, len = 0;
	NX_GPIO_HANDLE_INFO	*pInfo;
	char buf[64];

	assert(hGpio);
	pInfo = (NX_GPIO_HANDLE_INFO*)hGpio;

	len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", pInfo->port);
	if( 0 > (fd = open(buf, O_RDWR)) ){
		printf("cannot open gpio%d value!\n", pInfo->port);
		return -1;
	}

	if( 0 > (len = read(fd, buf, sizeof(buf))) ) {
		printf("cannot read gpio%d value!\n", pInfo->port);
		close(fd);
		return -1;
	}

	close(fd);
	
	return atoi(buf);
}

int32_t NX_GPioSetEdge( NX_GPIO_HANDLE hGpio, int32_t edge )
{
	int32_t fd = 0, len = 0;
	NX_GPIO_HANDLE_INFO	*pInfo;
	char buf[64];

	assert(hGpio);
	pInfo = (NX_GPIO_HANDLE_INFO*)hGpio;

	if( edge < GPIO_EDGE_NONE || edge > GPIO_EDGE_BOTH ) {
		printf("unknown gpio edge! ( %d )\n", edge);
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/edge", pInfo->port);
	if( 0 > (fd = open(buf, O_RDWR)) ){
		printf("cannot open gpio%d edge!\n", pInfo->port);
		return -1;
	}

	if( edge == GPIO_EDGE_NONE ) 	len = snprintf(buf, sizeof(buf), "none");
	if( edge == GPIO_EDGE_FALLING ) len = snprintf(buf, sizeof(buf), "falling");
	if( edge == GPIO_EDGE_RIGING )	len = snprintf(buf, sizeof(buf), "rising");
	if( edge == GPIO_EDGE_BOTH )	len = snprintf(buf, sizeof(buf), "both");
	
	if( 0 > write(fd, buf, len) ) {
		printf("cannot write gpio%d edge!\n", pInfo->port);
		close(fd);
		return -1;
	}
	
	close(fd);

	return 0;	
}

int32_t NX_GpioGetInterrupt( NX_GPIO_HANDLE hGpio )
{
	NX_GPIO_HANDLE_INFO	*pInfo;

	int32_t fd = 0, len = 0;
	int32_t hPoll = 0;
	struct pollfd   pollEvent;
	char buf[64];
	int32_t bRun = true;

	assert(hGpio);
	pInfo = (NX_GPIO_HANDLE_INFO*)hGpio;

	pthread_mutex_lock( &pInfo->hLock );
	pInfo->bPost = false;
	pthread_mutex_unlock( &pInfo->hLock );

	if( pInfo->direction != GPIO_DIRECTION_IN ) {
		printf("direction is not input.\n");
		return -1;
	}

	len = snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", pInfo->port);
	if( 0 > (fd = open(buf, O_RDWR)) ){
		printf("cannot open gpio%d value!\n", pInfo->port);
		return -1;
	}

	// dummy read.
	if( 0 > (len = read(fd, buf, sizeof(buf))) ) {
		printf("cannot read gpio%d value!\n", pInfo->port);
		if( fd ) close(fd);
		return -1;
	}

	memset( &pollEvent, 0x00, sizeof(pollEvent) );
	pollEvent.fd = fd;
	pollEvent.events = POLLPRI;

	while( bRun )
	{
		hPoll = poll( (struct pollfd*)&pollEvent, 1, 1000 );

		if( hPoll < 0 ) {
			printf("error poller.\n");
			if( fd ) close(fd);
			return -1;
		}
		else if( hPoll > 0 ) {
			if( pollEvent.revents & POLLPRI ) {
				if( fd ) close(fd);
				return 0;
			}
		}

		pthread_mutex_lock( &pInfo->hLock );
		if( pInfo->bPost ) bRun = false;
		pthread_mutex_unlock( &pInfo->hLock );
	}

	if( fd ) close(fd);
	
	return -1;
}

int32_t NX_GpioPostInterrupt( NX_GPIO_HANDLE hGpio )
{
	NX_GPIO_HANDLE_INFO	*pInfo;

	assert(hGpio);
	pInfo = (NX_GPIO_HANDLE_INFO*)hGpio;

	pthread_mutex_lock( &pInfo->hLock );
	pInfo->bPost = true;
	pthread_mutex_unlock( &pInfo->hLock );

	return 0;
}