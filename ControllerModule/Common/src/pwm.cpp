#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>

#include <time.h>
#include "pinctl.h"
#include "pwm.h"

enum
{
	pwm_Unknown,
	pwm_Error,
	pwm_Busy,
	pwm_OK
};



pwmHandle pwmCtl;
int pwmCount = 0;
int pwmAvailable = 0;
int pwmMonitorFd = -1;

static char* SysCmd(char *Command)
{
  char* retval = NULL;
  int dataSize = 0;
  FILE *fp;
  char buff[256];

  fp = popen(Command, "re");
  if (fp != NULL)
  {
    while (fgets(buff, sizeof(buff), fp) != NULL)
    {
      int newSize = dataSize + strlen(buff) + 1;
      char* data = (char*) malloc(newSize);
      if (data != NULL)
      {
        memset(data, 0, newSize);

        if (retval != NULL)
        {
          strcat(data, retval);
          free(retval);
          retval = NULL;
        }

        strcat(data, buff);
        retval = data;
        dataSize = strlen(retval);
      }
    }
    pclose(fp);
  }
  return retval;
}

int isOverlayLoaded(char *overlay)
{
	int retval = 0;
	char* data = SysCmd("cat /sys/devices/bone_capemgr.9/slots");
	if (strstr(data, overlay) != NULL)
		retval = 1;
	free(data);
	return retval;
}

void loadOverlay(char *overlay)
{
	char cmd[70];
	char* response;
	sprintf(cmd, "echo %s > /sys/devices/bone_capemgr.9/slots", overlay);
	response = SysCmd(cmd);
	if (response != NULL)
		free(response);
}

static int getPwmEntry( int pwmPin )
{
	int index = -1;
	int i;

	for( int i = 0; i < pwmCount; i++)
	{
		if( pwmCtl != NULL )
		{
			if( pwmPin == pwmCtl[i].pin )	
			{
				index = i;
				break;
			}
		}
	}

	return index;	
}

static int createPwmEntry(int pwmPin, unsigned long period, int polarity)
{
	int index = getPwmEntry(pwmPin);
	if (index < 0)
	{
		index++;
		pwmCtl[index].pin = pwmPin;
		pwmCtl[index].period = period;
		pwmCtl[index].pwmStatus = pwm_Unknown;
	}	

	return index;
}

void initPWMDriver( void )
{
	char cmd[80] = {0};
	char* resp;

	// resp = SysCmd("echo BB-PWM1 > /sys/devices/platform/bone_capemgr/slots");
	// if (resp != NULL)
	// {
	// 	printf("%s", resp);
	// 	free(resp);
	// }

	resp = SysCmd("cat /sys/class/pwm/pwmchip0/npwm");
	if( resp != NULL)
	{
		pwmCount = atoi(resp);
		free(resp);
	}

	if( pwmCount > 0 )
	{
		pwmCtl = (pwmHandle)malloc(sizeof(struct pwm_control_block) * pwmCount);
		pwmAvailable = 1;

		// for( int i = 0; i < pwmCount; i++)
		// {
		// 	sprintf(cmd, "echo %d > /sys/class/pwm/pwmchip0/export", i);					/* Add PWM channel */
		// 	resp = SysCmd(cmd);
		// 	if( resp != NULL) free(resp);
		// }
	}
	else
	{
		pwmCount  = 0;
	}
}

pwmHandle createPWM(int pwmPin, unsigned long period, int polarity)
{
	char cmd[80] = {0};
	char polarityType[20] = "normal";
	char fname[80];
	char* resp;
	int index = -1;	
	char found = 0;

	if( !pwmAvailable )
	{
		return NULL;
	}

	index = getPwmEntry(pwmPin);

	if( index > 0 )
	{
		return &pwmCtl[index];	/* This PWM has already been allocated */
	}
	else if( pwmPin < pwmCount )
	{
		index = createPwmEntry(pwmPin, period, polarity);

		sprintf(cmd, "echo %d > /sys/class/pwm/pwmchip0/unexport", pwmPin);				/* Remove PWM 0  */
		resp = SysCmd(cmd);
		if (resp != NULL) 
		{	
			printf("%s", resp);
			free(resp);
		}

		sprintf(cmd, "echo %d > /sys/class/pwm/pwmchip0/export", pwmPin);					/* Add PWM 0 */
		resp = SysCmd(cmd);
		if (resp != NULL) 
		{	
			printf("%s", resp);
			free(resp);
		}

		sleep(2);  // Allow time for OS to allocate

		pwmCtl[index].polarity = (char *)malloc(20 * sizeof(char) );
		if( 0 != polarity )
		{
			snprintf(pwmCtl[index].polarity, 20, "inversed");
		}
		else
		{
			snprintf(pwmCtl[index].polarity, 20, "normal");
		}
		sprintf(cmd, "echo %s > /sys/class/pwm/pwmchip0/pwm-0:%d/polarity", pwmCtl[index].polarity, pwmPin);		/* Set polarity */
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);	

		sprintf(cmd, "echo %ld > /sys/class/pwm/pwmchip0/pwm-0:%d/period", period, pwmPin);		/* Set period of PWM*/
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);	

		sprintf(cmd, "echo %ld > /sys/class/pwm/pwmchip0/pwm-0:%d/duty_cycle", 0ul, pwmPin);		/* Set duty of PWM*/
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);
		
		sem_init (&pwmCtl[index].pwmUpdateLock, 0, 1);
	}
	else
	{
		return NULL;	/* Invalid parameter for PWM pin */
	}

	pwmCtl[index].pwmStatus = pwm_OK;

	return &pwmCtl[index];	
}

void deletePwm(pwmHandle pwmHdl)
{	
	char cmd[80] = {0};
	char* resp = NULL;
	if(NULL != pwmHdl )
	{
		pwmHdl->run = 1;
		sprintf(cmd, "echo %d > /sys/class/pwm/pwmchip0/unexport", pwmHdl->pin);				/* Remove PWM 0  */
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);
	}
	else {
		printf("PWM not allocated\n");
	}
	sem_destroy(&pwmHdl->pwmUpdateLock);
	free(pwmHdl->polarity);
}

void startPwm(pwmHandle pwmHdl)
{
	char cmd[80] = {0};
	char* resp = NULL;
	if(NULL != pwmHdl )
	{
		sprintf(cmd, "echo 1 > /sys/class/pwm/pwmchip0/pwm-0:%d/enable", pwmHdl->pin);
		resp = SysCmd(cmd);
		pwmHdl->run = 1;
		if (resp != NULL) free(resp);	
	}
	else {
		printf("PWM not allocated\n");
	}
}

void stopPwm(pwmHandle pwmHdl)
{
	char cmd[80] = {0};
	char* resp = NULL;
	if(NULL != pwmHdl )
	{
		pwmHdl->run = 1;
		sprintf(cmd, "echo 0 > /sys/class/pwm/pwmchip0/pwm-0:%d/enable", pwmHdl->pin);
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);	
		
	}
	else {
		printf("PWM not allocated\n");
	}
}

unsigned long pwmDutyLast = -1ul;
unsigned long tempPWM = 0;
void haltPWMOutput(pwmHandle pwmHdl)
{
	char cmd[80];
	char *resp;

	sem_wait(&pwmHdl->pwmUpdateLock);
	tempPWM = pwmDutyLast;
	sprintf(cmd, "echo %ld > /sys/class/pwm/pwmchip0/pwm-0:%d/duty_cycle", 0ul, pwmHdl->pin);		/* Set duty of PWM*/
	resp = SysCmd(cmd);
	if (resp != NULL) free(resp);
}

void resumePWMOutput(pwmHandle pwmHdl)
{
	char cmd[80];
	char *resp;

	sprintf(cmd, "echo %ld > /sys/class/pwm/pwmchip0/pwm-0:%d/duty_cycle", tempPWM, pwmHdl->pin);		/* Set duty of PWM*/
	resp = SysCmd(cmd);
	if (resp != NULL) free(resp);
	sem_post(&pwmHdl->pwmUpdateLock);
}


void waitPwmOff(pwmHandle pwmHdl)
{
	do{
		; // Off wait for beginning of next off
	}
   while (1 == pinctl::inst().get(pwmMonitorFd));
}


int runStatus(pwmHandle pwmHdl)
{
	if(NULL != pwmHdl )
	{
		return pwmHdl->run;
	}
	else {
		printf("PWM not allocated\n");
	}

	return -1;
}

unsigned long  getPwmDuty(pwmHandle pwmHdl)
{
	unsigned long retval = 0;
	if(NULL != pwmHdl )
	{
		retval = pwmHdl->duty;
	}
	else {
		printf("PWM not allocated\n");
	}

	return retval;
}


void setPwmDuty(pwmHandle pwmHdl, unsigned long duty)
{
	char dutyStr[20] = {0};
	int err;
	char cmd[80];
	char *resp;
	struct timespec sleepDelay, timeLeft;

	sleepDelay.tv_nsec = (1) * (1000) * (1000);                       // 1ms sleep cycle
   sleepDelay.tv_sec = 0;

	if(NULL != pwmHdl )
	{
		if( 1 == pwmHdl->run)
		{
			sem_wait(&pwmHdl->pwmUpdateLock);
			if( pwmDutyLast != duty )
			{
				sprintf(cmd, "echo %s > /sys/class/pwm/pwmchip0/pwm-0:%d/polarity", pwmHdl->polarity, pwmHdl->pin);		/* Set polarity */
				resp = SysCmd(cmd);
				if (resp != NULL) free(resp);

				pwmHdl->duty = 0U;
				sprintf(cmd, "echo %ld > /sys/class/pwm/pwmchip0/pwm-0:%d/duty_cycle", duty, pwmHdl->pin);		/* Set duty of PWM*/
				resp = SysCmd(cmd);
				if (resp != NULL) 
				{
					printf("setPwmDuty ERR: - %s\r\n", resp);
					free(resp);
				}
				else
				{
					printf("<6>PWM set - %lu\n", duty);
					pwmHdl->duty = duty;
					pwmDutyLast = duty;
				}
			}
			sem_post(&pwmHdl->pwmUpdateLock);
		}
	}
	else {
		printf("PWM not allocated\n");
	}
	nanosleep(&sleepDelay, &timeLeft);
}

