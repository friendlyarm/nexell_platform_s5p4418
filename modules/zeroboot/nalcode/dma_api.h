#ifndef __DMA_API_H__
#define	__DMA_API_H__

#ifdef ARCH_V7
extern void v7_dma_clean_range(unsigned int start, unsigned int end, int a, int b);
extern void v7_dma_inv_range(unsigned int start, unsigned int end, int a, int b);
extern void v7_inv_range(unsigned int start, unsigned int end, int a, int b);
extern void v7_dma_flush_range(unsigned int start, unsigned int end, int a, int b);
extern void l2x0_inv_range(unsigned long start, unsigned long end, int a, int b);
extern void l2x0_flush_range(unsigned long start, unsigned long end, int a, int b);

extern void v7_flush_icache_all(int a);

#endif

#endif // __DMA_API_H__


