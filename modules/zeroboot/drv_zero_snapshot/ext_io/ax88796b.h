/*=========================================================================
* Generic AX88796B register definitions. 
* This file is part of AX88796B drivers, and is distributed under the same license.
*
*	Note:
*
*	AX88796B has 16K buffer for packet transmit and receive.
*	this buffer act as ring buffer. and so need some index and pointer.
*	follows definition's was related that.
*
*	--------+-------------------------------+	0x4000
*			|					 (256 bytes)|
*	TxBuffer+-------------------------------+
*			|								|
*	--------+-------------------------------+	0x4c00
*			|PageStart						|
*			+-------------------------------+
*			|								|
*	RxBuffer+-------------------------------+
*			|								|
*			+-------------------------------+
*			|PageStop						|
*	--------+-------------------------------+	0x8000
*
*	* Current Page acts as write pointer and
*	  Boundary(Nex) Page acts as read pointer
=========================================================================*/

#ifndef _ax88796_h
#define _ax88796_h


// xmit space
#define TX_PAGES            	12
#define Tx_PAGES_SIZE        	256

#define NE_IO_EXTENT        	0xFFF

// receive space
#define NESM_START_PG       	0x40	// First page of TX buffer
#define NESM_RX_START_PG	    (NESM_START_PG + TX_PAGES)	// 0x4C First page of RX buffer
#define NESM_STOP_PG		    0x80	// Last page +1 of RX ring

#define ETHER_ADDR_LEN      	6

#define AX88796_WATCHDOG_PERIOD (2*HZ)

// Some generic ethernet register configurations.

#define E8390_RXCONFIG			0x4		// EN0_RXCR: broadcasts, no multicast,errors
#define E8390_RXOFF				0x20	// EN0_RXCR: Accept no packets
#define E8390_TXCONFIG			0x80	// EN0_TXCR: Normal transmit mode
#define E8390_TXOFF				0x02	// EN0_TXCR: Transmitter off

//  Register accessed at EN_CMD, the 8390 base addr.
#define E8390_STOP				0x01	// Stop and reset the chip
#define E8390_START				0x02	// Start the chip, clear reset
#define E8390_TRANS				0x04	// Transmit a frame
#define E8390_RREAD				0x08	// Remote read
#define E8390_RWRITE			0x10	// Remote write
#define E8390_NODMA				0x20	// Remote DMA
#define E8390_PAGE0				0x00	// Select page chip registers
#define E8390_PAGE1				0x40	// using the two high-order bits
#define E8390_PAGE2				0x80	// Page 2 is invalid.
#define E8390_PAGE3				0xc0	// Page 3 for AX88796B

#define EI_SHIFT(x)	((x) << AX_ADDR_SHIFT )

#define E8390_CMD				EI_SHIFT(0x00)		// The command register (for all pages)
// Page 0 register offsets.
#define EN0_CLDALO				EI_SHIFT(0x01)		// Low byte of current local dma addr  RD
#define EN0_STARTPG				EI_SHIFT(0x01)		// Starting page of ring bfr WR
#define EN0_CLDAHI				EI_SHIFT(0x02)		// High byte of current local dma addr  RD
#define EN0_STOPPG				EI_SHIFT(0x02)		// Ending page +1 of ring bfr WR
#define EN0_BOUNDARY        	EI_SHIFT(0x03)		// Boundary page of ring bfr RD WR
#define EN0_TSR             	EI_SHIFT(0x04)		// Transmit status reg RD
#define EN0_TPSR				EI_SHIFT(0x04)		// Transmit starting page WR
#define EN0_NCR             	EI_SHIFT(0x05)		// Number of collision reg RD
#define EN0_TCNTLO				EI_SHIFT(0x05)		// Low  byte of tx byte count WR
#define EN0_FIFO				EI_SHIFT(0x06)		// FIFO RD
#define EN0_TCNTHI				EI_SHIFT(0x06)		// High byte of tx byte count WR
#define EN0_ISR             	EI_SHIFT(0x07)		// Interrupt status reg RD WR
#define EN0_CRDALO				EI_SHIFT(0x08)		// low byte of current remote dma address RD
#define EN0_RSARLO				EI_SHIFT(0x08)		// Remote start address reg 0
#define EN0_CRDAHI				EI_SHIFT(0x09)		// high byte, current remote dma address RD
#define EN0_RSARHI				EI_SHIFT(0x09)		// Remote start address reg 1
#define EN0_RCNTLO				EI_SHIFT(0x0a)		// Remote byte count reg WR
#define EN0_RCNTHI				EI_SHIFT(0x0b)		// Remote byte count reg WR
#define EN0_RSR             	EI_SHIFT(0x0c)		// rx status reg RD
#define EN0_RXCR				EI_SHIFT(0x0c)		// RX configuration reg WR
#define EN0_TXCR				EI_SHIFT(0x0d)		// TX configuration reg WR
#define EN0_COUNTER0        	EI_SHIFT(0x0d)		// Rcv alignment error counter RD
#define EN0_DCFG				EI_SHIFT(0x0e)		// Data configuration reg WR
#define EN0_COUNTER1        	EI_SHIFT(0x0e)		// Rcv CRC error counter RD
#define EN0_IMR             	EI_SHIFT(0x0f)		// Interrupt mask reg WR
#define EN0_COUNTER2        	EI_SHIFT(0x0f)		// Rcv missed frame error counter RD
#define EN0_DATAPORT        	EI_SHIFT(0x10)		
#define EN0_PHYID				EI_SHIFT(0x10)		
#define AX88796_MII_EEPROM  	EI_SHIFT(0x14)		
#define EN0_BTCR				EI_SHIFT(0x15)		
#define EN0_SR              	EI_SHIFT(0X17)		// Louis add for AX88796B Status Register
#define EN0_FLOW				EI_SHIFT(0x1a)		// AX88796B Flow control register
#define EN0_MCR             	EI_SHIFT(0X1b)		// Mac configure register
#define EN0_CTEPR				EI_SHIFT(0x1c)		// Current TX End Page
#define EN0_VID0				EI_SHIFT(0x1c)		// VLAN ID 0
#define EN0_VID1				EI_SHIFT(0x1d)		// VLAN ID 1
#define EN0_RESET				EI_SHIFT(0X1f)		// Issue a read to reset, a write to clear.

// [FALINUX]
#define EN0_DATA_ADDR	    	0x40        // A6
//#define EN0_DATA_ADDR	    	0x0800      // A11
                            	
                            	
#define ENVLAN_ENABLE   		0x08

// Bits in EN0_ISR - Interrupt status register
#define ENISR_RX				0x01		// Receiver, no error
#define ENISR_TX				0x02		// Transmitter, no error
#define ENISR_RX_ERR    		0x04		// Receiver, with error
#define ENISR_TX_ERR    		0x08		// Transmitter, with error
#define ENISR_OVER				0x10		// Receiver overwrote the ring
#define ENISR_COUNTERS			0x20		// Counters need emptying
#define ENISR_RDC				0x40		// remote dma complete
#define ENISR_RESET				0x80		// Reset completed
#define ENISR_ALL				(ENISR_RX | ENISR_TX | ENISR_RX_ERR | ENISR_TX_ERR | ENISR_OVER | ENISR_COUNTERS)	// Interrupts we will enable


// Bits in EN0_DCFG - Data config register
#define ENDCFG_WTS				0x01			// word transfer mode selection
#define ENDCFG_BOS				0x02			// byte order selection
                        		            	
#define ENFLOW_ENABLE			0xc7			// Flow Control Control Register
#define ENTQC_ENABLE    		0x20			// Enable TXQ
                        		
#define EN3_TBR         		EI_SHIFT(0x0d)	// Transmit Buffer Ring Control Register
#define ENTBR_ENABLE    		0x01			// Enable Transmit Buffer Ring

// Page 1 register offsets.
#define EN1_PHYS            	EI_SHIFT(0x01)  // This board's physical enet addr RD WR
#define EN1_PHYS_SHIFT(i)   	EI_SHIFT(i+1)   // Get and set mac address
#define EN1_CURPAG          	EI_SHIFT(0x07)  // Current memory page RD WR
#define EN1_MULT            	EI_SHIFT(0x08)  // Multicast filter mask array (8 bytes) RD WR
#define EN1_MULT_SHIFT(i)   	EI_SHIFT(8+i)   // Get and set multicast filter


// Constants for the CSR_IMR register.
// Configure which ISR settings actually cause interrupts.
//
#define ENIMR_RX				0x01		// packet received with no errors
#define ENIMR_TRANS				0x02		// packet transmitted with no errors
#define ENIMR_RX_ERR			0x04		// error on packet reception
#define ENIMR_TRANS_ERR			0x08		// error on packet transmission
#define ENIMR_OVER				0x10		// receive buffer overflow
#define ENIMR_COUNTER			0x20		// MSB set on tally counter
                            	
#define ENINT_MASK				(ENIMR_RX | ENIMR_TRANS | ENIMR_RX_ERR | ENIMR_TRANS_ERR | ENIMR_OVER | ENIMR_COUNTER)


/* Constants for the CSR_DSR register. */
#define ENDSR_LINK				0x01	// link status
#define ENDSR_DUPLEX			0x02	// PHY duplex mode
#define ENDSR_SPEED				0x04	// PHY link speed
#define ENDSR_DEV_RDY			0x10	// device ready
#define ENDSR_RD_RDY			0x20	// read data port ready
#define ENDSR_RDMA_RDY			0x40	// remote DMA complete
#define ENDSR_ENDIAN			0x80	// big/little endian

/* Bits in received packet status byte and EN0_RSR*/
#define ENRSR_RXOK      		0x01	/* Received a good packet */
#define ENRSR_CRC       		0x02	/* CRC error */
#define ENRSR_FAE       		0x04	/* frame alignment error */
#define ENRSR_FO        		0x08	/* FIFO overrun */
#define ENRSR_MPA       		0x10	/* missed pkt */
#define ENRSR_PHY       		0x20	/* physical/multicast address */
#define ENRSR_DIS       		0x40	/* receiver disable. set in monitor mode */
#define ENRSR_DEF       		0x80	/* deferring */

/* Constants for the ENO_RCR register. */
#define ENRCR_RUN_ERROR			0x01	// Enable the receiver to accept and save packets with error
#define ENRCR_RUN				0x02	// Enable the receiver to accept runt packet
#define ENRCR_BROADCAST			0x04	// receive broadcast packets
#define ENRCR_MULTICAST			0x08	// receive multicast packets
#define ENRCR_ALL_PHYS			0x10	// receive ALL directed packets
#define ENRCR_MONITOR			0x20	// don't collect packets
#define ENRCR_INT_ACT			0x40	// ??
                        		
#define ENRCR_CONFIG			(RCR_BROADCAST | RCR_INT_ACT)

/* Transmitted packet status, EN0_TSR. */
#define ENTSR_PTX       		0x01   /* Packet transmitted without error */
#define ENTSR_ND        		0x02   /* The transmit wasn't deferred. */
#define ENTSR_COL       		0x04   /* The transmit collided at least once. */
#define ENTSR_ABT       		0x08   /* The transmit collided 16 times, and was deferred. */
#define ENTSR_CRS       		0x10   /* The carrier sense was lost. */
#define ENTSR_FU        		0x20   /* A "FIFO underrun" occurred during transmit. */
#define ENTSR_CDH       		0x40   /* The collision detect "heartbeat" signal was lost. */
#define ENTSR_OWC       		0x80   /* There was an out-of-window collision. */

/* Transmitter control register,  ENO_TCR. */
#define ENTCR_NOCRC     		0x01	/* 1=inhibit CRC */
#define ENTCR_NORMAL    		0x00	/* Normal transmitter operation */
#define ENTCR_LOCAL     		0x02	/* Internal NIC loopback */
#define ENTCR_INLOOP    		0x04	/* Full internal loopback */
#define ENTCR_OUTLOOP   		0x08	/* External loopback */
#define ENTCR_ATD       		0x10	/* Auto transmit disable */
#define ENTCR_RLO				0x20	/* retry of late collision */ 
#define ENTCR_PD				0x40	/* pad disable */
#define ENTCR_FDU				0x80	/* full duplex */

/* Power Management register offsets. */
#define EN3_BM0         		EI_SHIFT(0x01)
#define EN3_BM1         		EI_SHIFT(0x02)
#define EN3_BM2         		EI_SHIFT(0x03)
#define EN3_BM3         		EI_SH2IFT(0x04)
#define EN3_BM10CRC     		EI_SHIFT(0x05)
#define EN3_BM32CRC     		EI_SHIFT(0x06)
#define EN3_BMOFST      		EI_SHIFT(0x07)
#define EN3_LSTBYT      		EI_SHIFT(0x08)
#define EN3_BMCD        		EI_SHIFT(0x09)
#define EN3_WUCS        		EI_SHIFT(0x0a)
#define EN3_PMR         		EI_SHIFT(0x0b)

/* Bits in Wake up Control */
#define ENWUCS_MPEN				0x01
#define ENWUCS_WUEN				0x02
#define ENWUCS_LINK				0x04

/* Bits in PM Control */
#define ENPMR_D1				0x01
#define ENPMR_D2				0x02


/* SMDK2440 Registers Definition */
/* SMDK2440 default clocks: FCLK=400MHZ, HCLK=125MHZ, PCLK=62.5MHZ */
#define CLKDIVN_125MHZ			0x0000000F 	/* Set HCLK=FCLK/3, PCLK=HCLK/2 when CAMDIVN[8]=0 */
#define CAMDIVN_125MHZ			0x00000000	/* Set HCLK=FCLK/3, PCLK=HCLK/2 when CAMDIVN[8]=0 */
#define UBRDIV0_125MHZ			0x00000023	/* Set UART Baud Rate divisor for 125MHZ HCLK */
                            	
#define CLKDIVN_100MHZ			0x0000000D	/* Set HCLK=FCLK/4, PCLK=HCLK/2 when CAMDIVN[9]=0 */
#define CAMDIVN_100MHZ			0x00000000	/* Set HCLK=FCLK/4, PCLK=HCLK/2 when CAMDIVN[9]=0 */
#define UBRDIV0_100MHZ			0x0000001B	/* Set UART Baud Rate divisor for 100MHZ HCLK */
                            	
#define CLKDIVN_50MHZ			0x0000000D	/* Set HCLK=FCLK/8, PCLK=HCLK/2 when CAMDIVN[9]=1 */
#define CAMDIVN_50MHZ			0x00000200	/* Set HCLK=FCLK/8, PCLK=HCLK/2 when CAMDIVN[9]=1 */
#define UBRDIV0_50MHZ			0x0000000D	/* Set UART Baud Rate divisor for 50MHZ HCLK */

#define DEFAULT_100MHZ_BANKCON1	0x00000400
#define DEFAULT_125MHZ_BANKCON1	0x00000510
#define BURST_BANKCON1			0x0000040f

/* EINTMASK Register Bit Definition */
#define EINT11_MASK				0x00000800		/* Clear this bit to enable EINT11 interrupt */

/* EXTINT1 Register Bit Definition */
#define FLTEN11_HIGHLEVEL		0x00009000
#define FLTEN11_LOWLEVEL		0x00008000		/* Enable EINT11 signal with noise filter */
/* End of SMDK2440 Registers Definition */


#define MEDIA_AUTO      		0
#define MEDIA_100FULL   		1
#define MEDIA_100HALF   		2
#define MEDIA_10FULL    		3
#define MEDIA_10HALF    		4

/* Debug Message Display Level Definition */
#define DRIVER_MSG      		0x0001
#define INIT_MSG        		0x0002
#define TX_MSG          		0x0004
#define RX_MSG          		0x0008
#define INT_MSG         		0x0010
#define ERROR_MSG       		0x0020
#define WARNING_MSG     		0x0040
#define DEBUG_MSG       		0x0080
#define OTHERS_MSG      		0x0100
#define ALL_MSG         		0x01FF
#define NO_MSG          		0x0000
#define DEFAULT_MSG     		(DRIVER_MSG | ERROR_MSG)
#define DEBUG_FLAGS     		DEFAULT_MSG

#define IEEE_8023_MIN_FRAME     64		/* Smallest possible ethernet frame */
#define IEEE_8023_MAX_FRAME     1518	/* Largest possible ethernet frame */


#define AX88696B_S3C6410_PA_BASE	0x18000000

extern void ax88796b_save( void __iomem *base );
extern void ax88796b_restore( void __iomem *base );


#endif /* _8390_h */
