//================================================================================================
/**
@file :
@project: 
@data :
@author : 
@brief : 
@version :
@todo :
@remark :
@warning :
@modifylnfo :
*///==================================================================================================

#include "typedef.h"

#include <uart.h>

void mmu_delay(unsigned int time)
{
    while(time--);
}

void uart_init( void )
{
}

void uart_putc( char c )
{
#ifdef NAL_RELEASE
       return;
#endif
	REG_UART0_UTXH = c;
#ifdef UART_SET_WAIT
	while ((REG_UART0_UTRSTAT & TX_EMPTY));
#else
	while (!(REG_UART0_UTRSTAT & TX_EMPTY));
#endif
}

void uart_puts( char *s )
{
#ifdef NAL_RELEASE
       return;
#endif
		while( *s )
	{
		if( *s == '\n' ) uart_putc( '\r' );	
		uart_putc( *s++ );
	}	
}

int	uart_read_ok( void )
{
	if( REG_UART0_UTRSTAT & RX_READY ) return 1;
	return 0;
}

char uart_getc( void )
{
	return REG_UART0_URXH;
}
//------------------------------------------------------------------------------
/**
@brief :
*///------------------------------------------------------------------------------
void uart_put_hex_nibble( u8 b )
{
	u8 nibble;

	if( b < 10 ) uart_putc( '0' + b );
	else	     uart_putc( 'A' + b - 10 );
}
//------------------------------------------------------------------------------
/**
@brief :
*///------------------------------------------------------------------------------
void uart_put_hex_byte( u8 b )
{
	uart_put_hex_nibble( ( b >> 4 ) & 0xF );
	uart_put_hex_nibble( ( b >> 0 ) & 0xF );
}
//------------------------------------------------------------------------------
/**
@brief :
*///------------------------------------------------------------------------------
void uart_put_hex( u32  data )
{
	uart_put_hex_byte( ( data >> 24 ) & 0xFF  );
	uart_put_hex_byte( ( data >> 16 ) & 0xFF  );
	uart_put_hex_byte( ( data >>  8 ) & 0xFF  );
	uart_put_hex_byte( ( data >>  0 ) & 0xFF  );
	
	uart_puts("\n");
}

void uart_put_hex_raw( u32  data )
{
	uart_put_hex_byte( ( data >> 24 ) & 0xFF  );
	uart_put_hex_byte( ( data >> 16 ) & 0xFF  );
	uart_put_hex_byte( ( data >>  8 ) & 0xFF  );
	uart_put_hex_byte( ( data >>  0 ) & 0xFF  );
}

