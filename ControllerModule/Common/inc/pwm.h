#ifndef __PWM_H__
#define __PWM_H__

#include <semaphore.h>

// Frequencies are in ns so 1/f * 1e-9 = ns
#define HZ_8k  	 125000
#define HZ_4k  	 250000
#define HZ_2k  	 500000
#define HZ_1k 		1000000
#define HZ_500  	2000000
#define HZ_250  	4000000
#define HZ_125  	8000000
#define HZ_90   	1111111
#define HZ_72   	1388888
#define HZ_60   	1666666
#define PWM_PERIOD HZ_4k
#define PWMMAX 40


struct pwm_control_block
{
	int fd;
	unsigned long period;
	unsigned long duty;
	int run;
	char pwmStatus;
	char *polarity;
	int pin;
	sem_t pwmUpdateLock;
};
typedef struct pwm_control_block *pwmHandle;

void initPWMDriver( void );

unsigned long  getPwmDuty(pwmHandle pwmHdl);
pwmHandle createPWM(int pwmPin, unsigned long period, int polarity);
void startPwm(pwmHandle pwmHdl);
void stopPwm(pwmHandle pwmHdl);
void deletePwm(pwmHandle pwmHdl);
int runStatus(pwmHandle pwmHdl);
void setPwmDuty(pwmHandle pwmHdl, unsigned long duty);

void haltPWMOutput(pwmHandle pwmHdl);
void resumePWMOutput(pwmHandle pwmHdl);
void waitPwmOff(pwmHandle pwmHdl);

extern int pwmMonitorFd;

#endif
