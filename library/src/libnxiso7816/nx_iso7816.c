#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>		//	open, O_RDWR
#include <unistd.h>		//	close
#include <stdint.h>
#include <termios.h>
#include <errno.h> 			/* error */
#include <sys/mman.h> 		/* for mmap */

#include <poll.h>
#include <pthread.h>

#include <nx_iso7816.h>
#include "iso7816_cfg.h"

#define	MMAP_ALIGN		4096	// 0x1000
#define	MMAP_DEVICE		"/dev/mem"
static uint32_t iomem_map(uint32_t phys, uint32_t len)
{
	uint32_t virt = 0;
	int32_t fd;

	fd = open(MMAP_DEVICE, O_RDWR|O_SYNC);
	if (0 > fd) {
		printf("Fail, open %s, %s\n", MMAP_DEVICE, strerror(errno));
		return 0;
	}

	if (len & (MMAP_ALIGN-1))
		len = (len & ~(MMAP_ALIGN-1)) + MMAP_ALIGN;

	virt = (uint32_t)mmap((void*)0, len,
				PROT_READ|PROT_WRITE, MAP_SHARED, fd, (off_t)phys);
	if (-1 == (int32_t)virt) {
		printf("Fail: map phys=0x%08x, len=%d, %s \n", phys, len, strerror(errno));
		goto _err;
	}

_err:
	close(fd);
	return virt;
}

static void iomem_free(uint32_t virt, uint32_t len)
{
	if (virt && len)
		munmap((void*)virt, len);
}


//////////////////////////////////////////////////////////////////////////////
//																			//
//					ISO7816 UART APIs( static )								//
//																			//
//////////////////////////////////////////////////////////////////////////////

#define	UART_DEVNAME_PREFIX	"/dev/ttyAMA"

typedef struct ISO7816_INFO *ISO7816_HANDLE;

#define	ISO7816_TIEOFF_REG_PHY		0xC0011000
#define	ISO7816_TIEOFF_REG_SIZE		0x00001000

#define TIEOFFREG04					0xC0011010
#define TIEOFFREG05					0xC0011014

struct ISO7816_INFO {
	int			uartHandle;
	char		ifStr[64];

	//	ISO7816 Stadard
	struct termios oldTio;
	uint32_t	freq;
	int32_t		startBits;
	int32_t		dataBits;
	int32_t		stopBits;
	int32_t		eanbleParity;
	int32_t		parity;			//	0(even), 1(odd)
};

static ISO7816_HANDLE _OpenISO7816( int port )
{
	int fd;
	char buf[64];
	struct termios newTio, oldTio;

	ISO7816_HANDLE handle = (ISO7816_HANDLE)calloc( sizeof(struct ISO7816_INFO), 1 );

	snprintf( buf, sizeof(buf), "%s%d", UART_DEVNAME_PREFIX, DATA_UART_PORT );
//	if( 0 > (fd = open( buf, O_RDWR | O_NOCTTY | O_NDELAY )  ) )
	if( 0 > (fd = open( buf, O_RDWR | O_NOCTTY )  ) )
	{
		printf("Cannot open UART Device(%s%d)", UART_DEVNAME_PREFIX, port);
		goto ErrorExit;
	}
	tcgetattr( fd, &oldTio );
	//	Flush
	tcflush( fd, TCIFLUSH );

	memset( &newTio, 0, sizeof(newTio) );
	//	Input Modes
	newTio.c_iflag = IGNBRK | INPCK | IGNCR;	//	Ignore Break, Enable Parity Checking, Ignore Carrigae Return
	//	Output Modes
	newTio.c_oflag = 0;
	//	Control Modes
	//newTio.c_cflag = B9600 | CS8 | CSTOPB | PARENB;	//	Baud rate(9600), Data 8 bits, 2 Stop bits, enable parity(even)
	newTio.c_cflag = B9600 | CS8 | PARENB;	//	Baud rate(9600), Data 8 bits, 2 Stop bits, enable parity(even)
	//	Local Flags
	newTio.c_lflag = ISIG;	//	noncanonical mode
	newTio.c_cc[VMIN] = 1;
	newTio.c_cc[VTIME] = 30;

	//
	//	Set TIE-OFF Register for ISO7816
	//
	{
		uint32_t *tieoff;
		uint32_t virAddr = iomem_map(ISO7816_TIEOFF_REG_PHY, ISO7816_TIEOFF_REG_SIZE);
#if (HW_UART_PORT == 0)
		tieoff = (uint32_t*)( virAddr + (TIEOFFREG04 - ISO7816_TIEOFF_REG_PHY) );
		*tieoff |= 0x700000;
#elif(HW_UART_PORT == 1)
		tieoff = (uint32_t*)( virAddr + (TIEOFFREG04 - ISO7816_TIEOFF_REG_PHY) );
		*tieoff |= 0x3800000;
#elif(HW_UART_PORT == 2)
		tieoff = (uint32_t*)( virAddr + (TIEOFFREG04 - ISO7816_TIEOFF_REG_PHY) );
		*tieoff |= 0x1c000000;
#elif(HW_UART_PORT == 3)
		tieoff = (uint32_t*)( virAddr + (TIEOFFREG04 - ISO7816_TIEOFF_REG_PHY) );
		*tieoff |= 0xe0000000;
#elif(HW_UART_PORT == 4)
		tieoff = (uint32_t*)( virAddr + (TIEOFFREG05 - ISO7816_TIEOFF_REG_PHY) );
		*tieoff |= 0x38;
#elif(HW_UART_PORT == 5)
		tieoff = (uint32_t*)( virAddr + (TIEOFFREG05 - ISO7816_TIEOFF_REG_PHY) );
		*tieoff |= 0x7;
#endif
		iomem_free( virAddr, ISO7816_TIEOFF_REG_SIZE );
	}

	tcsetattr( fd, TCSANOW, &newTio );
	handle->uartHandle = fd;
	handle->oldTio = oldTio;
	handle->freq = 9600;
	handle->startBits = 1;
	handle->dataBits = 8;
	handle->stopBits = 2;
	handle->eanbleParity = 1;
	handle->parity = 0;	//	even parity

	return handle;
ErrorExit:
	if( handle )
	{
		free( handle );
	}
	return NULL;
}

static void _CloseIOS7816( ISO7816_HANDLE handle )
{
	if( handle )
	{
		tcsetattr(handle->uartHandle, TCSANOW, &handle->oldTio );
		if(handle->uartHandle)
		{
			close(handle->uartHandle);
		}
		free( handle );
	}
}

//
//	Parameters:
//		uint8_t buf     : destination buffer's pointer
//		int32_t size    : read size
//		int32_t	timeout : timeout in milliseconds ( 0(no timeout), positive value(milli-seconds), min(100~) )
//	Return Value :
//		> 0  : receive bytes
//		== 0 : timeout
//		< 0  : error
//

static int32_t _ReadISO7816( ISO7816_HANDLE handle, uint8_t *buf, int32_t size, int32_t timeout )
{
	int32_t len, readSize = 0, res;
	struct pollfd fds;
	if( !handle && handle->uartHandle<0 )
	{
		return -1;
	}
	fds.fd		= handle->uartHandle;
	fds.events	= POLLIN | POLLERR;
	fds.revents	= 0;
	while( readSize<size && timeout>0 )
	{
		res = poll( (struct pollfd*)&fds, 1, 30);
		if( res < 0 )
		{
			return -1;
		}
		else if( res == 0 )
		{
			timeout -= 30;
		}
		else
		{
			len = read( handle->uartHandle, buf, size );
			buf += len;
			readSize += len;
		}
	}
	return readSize;
}


//
//	Return Value:
//		> 0 : no error
//		< 0 : error
//
static int32_t _WriteISO7816( ISO7816_HANDLE handle, uint8_t *buf, int32_t size )
{
	if( !handle && handle->uartHandle<0 )
	{
		return -1;
	}
	return write(handle->uartHandle, buf, size);
}

static int32_t _SetFreqIOS7816( ISO7816_HANDLE handle, int32_t frequency )
{
	int32_t ret;
	struct termios tio;
	speed_t speed;
	if( !handle && handle->uartHandle<0 )
	{
		return -1;
	}

	if     ( frequency == 9600    )	speed = B9600   ;
	else if( frequency == 19200   )	speed = B19200  ;
	else if( frequency == 38400   )	speed = B38400  ;
	else if( frequency == 57600   )	speed = B57600  ;
	else if( frequency == 115200  )	speed = B115200 ;
	else if( frequency == 230400  )	speed = B230400 ;
	else if( frequency == 460800  )	speed = B460800 ;
	else if( frequency == 500000  )	speed = B500000 ;
	else if( frequency == 576000  )	speed = B576000 ;
	else if( frequency == 921600  )	speed = B921600 ;
	else if( frequency == 1000000 )	speed = B1000000;
	else if( frequency == 1152000 )	speed = B1152000;
	else if( frequency == 1500000 )	speed = B1500000;
	else if( frequency == 2000000 )	speed = B2000000;
	else if( frequency == 2500000 )	speed = B2500000;
	else if( frequency == 3000000 )	speed = B3000000;
	else if( frequency == 3500000 )	speed = B3500000;
	else if( frequency == 4000000 )	speed = B4000000;
	else{
		printf("unknown frequency = %d\n", frequency);
		return -1;
	}

	tcgetattr( handle->uartHandle, &tio );
	tio.c_cflag &= ~CBAUD;
	tio.c_cflag |= speed;
	tcsetattr( handle->uartHandle, TCSANOW, &tio );
	return ret;
}



//////////////////////////////////////////////////////////////////////////////
//																			//
//					External API for ISO7816								//
//																			//
//////////////////////////////////////////////////////////////////////////////

struct NX_ISO7816_INFO
{
	ISO7816_HANDLE hData;			//	ISO7816 Data Handler
	NX_GPIO_HANDLE hReset;			//	IOS7816 Reset Handler
#ifndef USE_EXTERNAL_CLK
	NX_PWM_HANDLE hClock;			//	ISO7816 Clock Handler
#endif

	//	Flags
	int32_t		bInit;
};

//
//	dataPort  : UART Data Port (0~5)
//	clockPort : PWM Port (0~3)
//	resetPort : Reset GPIO Port Number
//
NX_ISO7816_HANDLE NX_InitISO7816( void )
{
	NX_ISO7816_HANDLE handle;
#ifndef USE_EXTERNAL_CLK
	NX_PWM_HANDLE hClock = NX_PwmInit( CLK_PWM_PORT );
#endif
	NX_GPIO_HANDLE hReset = NX_GpioInit( RST_GPIO_PORT );
	ISO7816_HANDLE hData = _OpenISO7816( DATA_UART_PORT );

	if( !hReset ||
#ifndef USE_EXTERNAL_CLK
		!hClock ||
#endif
		!hData )
	{
		goto ErrorExit;
	}

	//	GPIO Setting
	NX_GpioDirection( hReset, GPIO_DIRECTION_OUT );
	NX_GpioSetValue( hReset, 0 );

#ifndef USE_EXTERNAL_CLK
	//	Clock Enable
	NX_PwmSetFreqDuty( hClock, 9600*372, 50 );
#endif

	handle = (NX_ISO7816_HANDLE)calloc( sizeof(struct NX_ISO7816_INFO), 1 );

	handle->hData = hData;
#ifndef USE_EXTERNAL_CLK
	handle->hClock = hClock;
#endif
	handle->hReset = hReset;
	handle->bInit = 1;

	return handle;
ErrorExit:

	return NULL;
}

void NX_DeinitISO7816( NX_ISO7816_HANDLE handle )
{
	if( handle )
	{
		if( handle->hData  )
		{
			_CloseIOS7816( handle->hData );
		}
		if( handle->hReset )
		{
			NX_GpioDeinit( handle->hReset );
		}
#ifndef USE_EXTERNAL_CLK
		if( handle->hClock )
		{
			NX_PwmDeinit( handle->hClock, 1 );
		}
#endif
	}
}

int32_t NX_WriteISO7816( NX_ISO7816_HANDLE handle, uint8_t *buf, int32_t size )
{
	if( !handle )
	{
		return -1;
	}
	return _WriteISO7816( handle->hData, buf, size );
}

int32_t NX_ReadISO7816( NX_ISO7816_HANDLE handle, uint8_t *buf, int32_t size, int32_t timeout )
{
	if( !handle )
	{
		return -1;
	}
	return _ReadISO7816( handle->hData, buf, size, timeout );
}

int32_t NX_SetClockFreqISO7816( NX_ISO7816_HANDLE handle, int32_t frequency )
{
#ifndef USE_EXTERNAL_CLK
	if( !handle && !handle->hClock )
	{
		return -1;
	}
#if 0	//	For Debugging
	{
		uint32_t freq, duty;
		NX_PwmSetFreqDuty(handle->hClock, frequency, 50);
		NX_PwmGetInfo(handle->hClock, &freq, &duty);
		printf("Frequncy = %d, duty = %d\n", freq, duty);
		return ret;
	}
#endif
	return NX_PwmSetFreqDuty(handle->hClock, frequency, 50);
#else
	return 0;
#endif
}

int32_t NX_SetDataFreqIOS7816( NX_ISO7816_HANDLE handle, int32_t frequency )
{
	if( !handle && !handle->hData )
	{
		return -1;
	}
	return _SetFreqIOS7816( handle->hData, frequency );
}

int32_t NX_SetResetISO7816( NX_ISO7816_HANDLE handle, int32_t value )
{
	if( !handle && !handle->hReset )
	{
		return -1;
	}
	return NX_GpioSetValue( handle->hReset, !!value );
}
