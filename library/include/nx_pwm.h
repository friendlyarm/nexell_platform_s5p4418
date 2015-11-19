#ifndef __NX_PWM_H__
#define __NX_PWM_H__

#include <stdint.h>

#define	PWM_ERR_NONE			0
#define	PWM_ERR_INVALID_HANDLE	-1
#define	PWM_ERR_ACCESS_FAILED	-2
#define	PWM_ERR_NOT_EXIST		-3
#define	PWM_ERR_INVALID_PARAM	-4
#define	PWM_ERR_WRITE_FAILED	-5


#define	PWM_SYS_IF_PREFIX	"/sys/devices/platform/pwm/pwm."

typedef struct NX_PWM_INFO *NX_PWM_HANDLE;

enum NX_PWM_PORT{
	PWM_0,
	PWM_1,
	PWM_2,
	PWM_3,
	PWM_MAX,
};

#ifdef __cplusplus
extern "C" {
#endif

NX_PWM_HANDLE	NX_PwmInit			( int32_t port );
void			NX_PwmDeinit		( NX_PWM_HANDLE hPwm, int32_t off );
int32_t 		NX_PwmSetFreqDuty	( NX_PWM_HANDLE hPwm, uint32_t freq, uint32_t duty );
int32_t			NX_PwmGetInfo		( NX_PWM_HANDLE hPwm, uint32_t *freq, uint32_t *duty );

#ifdef __cplusplus
}
#endif

#endif	//	__NX_GPIO_H__
