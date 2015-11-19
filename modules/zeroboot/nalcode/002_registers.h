
#ifndef _REGISTER_HEADER_
#define _REGISTER_HEADER_

#ifndef __ASSEMBLY__
	#define __REG(x)       	(*((volatile unsigned long *)(x)))
	#define __REG_B(x)		(*((volatile u8 *)(x))) 

#else
	#define __REG(x)       (x)
	#define __REG_B(x)		(x) 

#endif

// from top make-arch-board.sh
#define	REG_UART_VIRT			ARG_REG_UART_VIRT

#if defined(ARCH_MX6)
#define	TX_FULL  		(1<<23)
#define TX_EMPTY  		(1<<3)
#define RX_READY  		(1<<0)
#define	RX_FULL  		(1<<14)

#define REG_UART0_UTXH			__REG(REG_UART_VIRT + 0x40)
#define REG_UART0_URXH			__REG(REG_UART_VIRT + 0x00)
#define REG_UART0_UTRSTAT		__REG(REG_UART_VIRT + 0x98)

#elif defined(ARCH_OMAP4) || defined(ARCH_AM3874)
#define TX_EMPTY  		(1<<6)
#define RX_READY  		(1<<0)

#define REG_UART0_UTXH			__REG(REG_UART_VIRT + 0x00)
#define REG_UART0_URXH			__REG(REG_UART_VIRT + 0x00)
#define REG_UART0_UTRSTAT		__REG(REG_UART_VIRT + 0x14)

#elif defined(ARCH_S5P4118)
// amba-pl011
#define	UART_SET_WAIT	1
#define UART01x_FR_TXFF     0x020
#define TX_EMPTY  		UART01x_FR_TXFF
#define RX_READY  		(1<<0)

#define REG_UART0_UTXH			__REG(REG_UART_VIRT + 0x00)
#define REG_UART0_URXH			__REG(REG_UART_VIRT + 0x00)
#define REG_UART0_UTRSTAT		__REG(REG_UART_VIRT + 0x18)

#else
#define TX_EMPTY  		(1<<1)
#define RX_READY  		(1<<0)

#define REG_UART0_BASE			__REG(REG_UART_VIRT + 0x00)	
#define REG_UART0_ULCON			__REG(REG_UART_VIRT + 0x00)	
#define REG_UART0_UCON			__REG(REG_UART_VIRT + 0x04)	
#define REG_UART0_UFCON			__REG(REG_UART_VIRT + 0x08)	
#define REG_UART0_UMCON			__REG(REG_UART_VIRT + 0x0C)	
#define REG_UART0_UTRSTAT		__REG(REG_UART_VIRT + 0x10)	
#define REG_UART0_UERSTAT		__REG(REG_UART_VIRT + 0x14)	
#define REG_UART0_UFSTAT		__REG(REG_UART_VIRT + 0x18)	
#define REG_UART0_UMSTAT		__REG(REG_UART_VIRT + 0x1C)	
#define REG_UART0_UTXH			__REG(REG_UART_VIRT + 0x20)	
#define REG_UART0_URXH			__REG(REG_UART_VIRT + 0x24)	
#endif

#endif // _REGISTER_HEADER_
