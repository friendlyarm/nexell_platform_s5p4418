//================================================================================================
/**
  @file : 110_uart.h
  @project: 
  @data : 2011. 7. 8.
  @author : Administrator
  @brief : 
  @version :
  @todo :
  @remark :
  @warning :
  @modifylnfo :
  *///==================================================================================================

#ifndef __UART_H__
#define __UART_H__

// from top make-arch-board.sh
#define	REG_UART_PHYS			ARG_REG_UART_PHYS

#if defined(ARCH_MX6)
#define	TX_FULL  		(1<<23)
#define TX_EMPTY  		(1<<3)
#define RX_READY  		(1<<0)
#define	RX_FULL  		(1<<14)

#define REG_UART0_UTXH			__REG(REG_UART_PHYS + 0x40)
#define REG_UART0_URXH			__REG(REG_UART_PHYS + 0x00)
#define REG_UART0_UTRSTAT		__REG(REG_UART_PHYS + 0x98)

#elif defined(ARCH_OMAP4) || defined(ARCH_AM3874)
 
#define TX_EMPTY  		(1<<6)
#define RX_READY  		(1<<0)

#define REG_UART0_UTXH			__REG(REG_UART_PHYS + 0x00)
#define REG_UART0_URXH			__REG(REG_UART_PHYS + 0x00)
#define REG_UART0_UTRSTAT		__REG(REG_UART_PHYS + 0x14)

#elif defined(ARCH_S5P4118)
// amba-pl011
#define	UART_SET_WAIT	1
#define UART01x_FR_TXFF     0x020
#define TX_EMPTY  		UART01x_FR_TXFF
#define RX_READY  		(1<<0)

#define REG_UART0_UTXH			__REG(REG_UART_PHYS + 0x00)
#define REG_UART0_URXH			__REG(REG_UART_PHYS + 0x00)
#define REG_UART0_UTRSTAT		__REG(REG_UART_PHYS + 0x18)

#else
#define TX_EMPTY  		(1<<1)
#define RX_READY  		(1<<0)

#define REG_UART0_BASE			__REG(REG_UART_PHYS + 0x00)
#define REG_UART0_ULCON			__REG(REG_UART_PHYS + 0x00)
#define REG_UART0_UCON			__REG(REG_UART_PHYS + 0x04)
#define REG_UART0_UFCON			__REG(REG_UART_PHYS + 0x08)
#define REG_UART0_UMCON			__REG(REG_UART_PHYS + 0x0C)
#define REG_UART0_UTRSTAT		__REG(REG_UART_PHYS + 0x10)
#define REG_UART0_UERSTAT		__REG(REG_UART_PHYS + 0x14)
#define REG_UART0_UFSTAT		__REG(REG_UART_PHYS + 0x18)
#define REG_UART0_UMSTAT		__REG(REG_UART_PHYS + 0x1C)
#define REG_UART0_UTXH			__REG(REG_UART_PHYS + 0x20)
#define REG_UART0_URXH			__REG(REG_UART_PHYS + 0x24)
#endif

extern void 	uart_init			( void 		);
extern void 	uart_putc			( char c	);
extern void 	uart_puts			( char *str	);
extern void 	uart_put_hex_nibble	( u8 b 		);
extern void 	uart_put_hex_byte	( u8 b 		);
extern void 	uart_put_hex		( u32  data );
extern int		uart_read_ok		( void 		);
extern char		uart_getc			( void 		);

#endif /* __UART_H__ */
