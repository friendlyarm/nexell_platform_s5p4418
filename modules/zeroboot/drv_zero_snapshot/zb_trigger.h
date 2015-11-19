#ifndef __ZB_TRIGGER_H__
#define	__ZB_TRIGGER_H__

int		zb_trigger_init			( void );
void	zb_trigger_exit			( void );

void zb_event_trigger			( void );
void zb_timer_remove			( void );
void zb_event_trigger_set_time	(int sec);

#endif	//__ZB_TRIGGER_H__
