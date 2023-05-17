#ifndef __PWM_H__
#define __PWM_H__

#include <semaphore.h>

#define PWM_PERIOD 13888888    // Max 3.3MHz for the display shifters - this is period in ns so 8 000 000 ns = 125Hz
#define PWMMAX 3             // Max duty cycle that we allow in this system - on time of 2 880 000 ns


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
