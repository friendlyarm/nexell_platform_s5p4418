#ifndef __ZB_PROC_H__
#define	__ZB_PROC_H__

int		zb_proc_init			( void );
void	zb_proc_exit			( void );

void	zb_nalcode_init			( void );
void	zb_nalcode_deactive		( void );
u32		zb_nalcode_start_paddr  ( void );
u32		zb_nalcode_size         ( void );
u32 zb_nalcode_spare_size(void);

#endif	//__ZB_PROC_H__
