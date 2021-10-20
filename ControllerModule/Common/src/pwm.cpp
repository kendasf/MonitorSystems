#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include "pinctl.h"
#include "pwm.h"

enum
{
	pwm_Unknown,
	pwm_Error,
	pwm_Busy,
	pwm_OK
};

struct pwm_control_block
{
	int fd;
	int polarity;
	unsigned long period;
	unsigned long duty;
	int run;
	char pwmStatus;
	int pin;
};

pwmHandle pwmCtl;
int pwmCount = 0;
int pwmAvailable = 0;

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
		pwmCtl[index].polarity = polarity;
		pwmCtl[index].pwmStatus = pwm_Unknown;
	}	

	return index;
}

void initPWMDriver( void )
{
	char* response = SysCmd("cat /sys/class/pwm/pwmchip0/npwm");
	pwmCount = atoi(response);

	if( pwmCount > 0 )
	{
		pwmCtl = (pwmHandle)malloc(sizeof(struct pwm_control_block) * pwmCount);
		pwmAvailable = 1;
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
		
		sprintf(cmd, "echo %d > /sys/class/pwm/pwmchip0/unexport", pwmPin);						/* Disable Access */
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);

		sprintf(cmd, "echo %d > /sys/class/pwm/pwmchip0/export", pwmPin);						/* Request Access */
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);

		if( 0 != polarity )
		{
			snprintf(polarityType, 20, "inversed");
		}
		sprintf(cmd, "echo %s > /sys/class/pwm/pwmchip0/pwm%d/polarity", polarityType, pwmPin);		/* Set polarity */
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);	

		sprintf(cmd, "echo %ld > /sys/class/pwm/pwmchip0/pwm%d/period", period, pwmPin);		/* Set period of PWM*/
		resp = SysCmd(cmd);
		if (resp != NULL) free(resp);	

		sprintf(fname, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmPin);						/* Open PWM at duty cycle*/
		pwmCtl[index].fd = open(fname, O_RDWR);		/* Set file descriptor for PWM */
	}
	else
	{
		return NULL;	/* Invalid parameter for PWM pin */
	}

	pwmCtl[index].pwmStatus = pwm_OK;

	return &pwmCtl[index];	
}

void startPwm(pwmHandle pwmHdl)
{
	char cmd[80];
	char* resp;
	if(NULL != pwmHdl )
	{
		sprintf(cmd, "echo 1 > /sys/class/pwm/pwmchip0/pwm%d/enable", pwmHdl->pin);
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
	char cmd[80];
	char* resp;
	if(NULL != pwmHdl )
	{
		sprintf(cmd, "echo 0 > /sys/class/pwm/pwmchip0/pwm%d/enable", pwmHdl->pin);
		resp = SysCmd(cmd);
		pwmHdl->run = 0;
		if (resp != NULL) free(resp);	
	}
	else {
		printf("PWM not allocated\n");
	}
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
	if(NULL != pwmHdl )
	{
		
		/*
		int err;
		memset(dutyStr, 0, sizeof(dutyStr));
		lseek(pwmHdl->fd, 0, SEEK_SET);
		err = read(pwmHdl->fd, dutyStr, sizeof(dutyStr));
		if (err < 0)
			printf("getPwmDuty ERR: %d - %s\r\n", errno, strerror(errno));
		else
			retval = atoi(dutyStr);
		*/
		return pwmHdl->duty;
	}
	else {
		printf("PWM not allocated\n");
	}

	return (unsigned long)-1;
}

void setPwmDuty(pwmHandle pwmHdl, unsigned long duty)
{
	char dutyStr[20];
	if(NULL != pwmHdl )
	{
		int err;
		sprintf(dutyStr, "%lu", duty);
		err = write(pwmHdl->fd, dutyStr, strlen(dutyStr));
		if (err < 0)
		{
			printf("setPwmDuty ERR: %d - %s\r\n", errno, strerror(errno));
			pwmHdl->duty = (unsigned int)-1;
		}
		else
		{
			pwmHdl->duty = duty;
		}
	}
	else {
		printf("PWM not allocated\n");
	}
}
