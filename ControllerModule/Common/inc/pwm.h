#ifndef __PWM_H__
#define __PWM_H__


struct pwm_control_block;
typedef struct pwm_control_block *pwmHandle;

void initPWMDriver( void );

unsigned long  getPwmDuty(pwmHandle pwmHdl);
pwmHandle createPWM(int pwmPin, unsigned long period, int polarity);
void startPwm(pwmHandle pwmHdl);
void stopPwm(pwmHandle pwmHdl);
int runStatus(pwmHandle pwmHdl);
void setPwmDuty(pwmHandle pwmHdl, unsigned long duty);

#endif
