
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // read, write
#include <stdio.h>      // printf, fprintf
#include <sys/types.h>  // open, close
#include <sys/stat.h>  // open, close
#include <fcntl.h>    // O_RDONLY, O_WRONLY
#include <string.h>   // strlen
#include <errno.h>
#include "pinctl.h"

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

int pinctl::get(int fd) {
	int retval = -1;
	char data = 'x';
	if (fd > 0) {
    int status;
    lseek(fd, 0, SEEK_SET);	
    status = read(fd, &data, 1);
    if (status == 1) {
      if (data == '1')
       retval = 1;
      else
       retval = 0;
    }
	}
	return retval;
}

void pinctl::set(int fd, int val)
{
	char str[10];
	if (fd > 0) {
		int err;
		sprintf(str, "%d", val);
	   	err = write(fd, str, strlen(str));
		if (err < 0)
			printf("ERR: %s\r\n", strerror(errno));
	}
}

int pinctl::open_gpio_pin(int pin, int dir) {
	int fd = -1;
	char path[50];

  sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	if (dir == 0)
		fd = open(path, O_WRONLY);
	else
		fd = open(path, O_RDONLY);

	return fd;
}

int pinctl::export_pin(int pin, int dir) {
  FILE *fd = NULL;
  char cmd[80];
	char* resp;
  
  int direct = dir & 1; // 0 is out or 1 is in 
  int up = (dir & 2) >> 1; 
  int down = (dir & 4) >> 2;
  // char path[75] = "/sys/class/gpio/export";
  // fd = fopen(path, "w");
  // if (fd != NULL)
  // {
  //   fprintf(fd, "%d", pin);
  //   fclose(fd);
  // }
  // else
  // {
  //   printf("%s ERROR: %s - %d\r\n", path, strerror(errno), errno);
  // }

  sprintf(cmd, "echo %d > /sys/class/gpio/export", pin);
  resp = SysCmd(cmd);
  if (resp != NULL) 
  {
    printf("%s", resp);
    free(resp);	
  }

  if (up)
  {
    sprintf(cmd, "echo %s+ > /sys/class/gpio/gpio%d/direction", (direct == 0) ? "out" : "in", pin);
  }
  else if (down)
  {
    sprintf(cmd, "echo %s- > /sys/class/gpio/gpio%d/direction", (direct == 0) ? "out" : "in", pin);
  }
  else
  {
    sprintf(cmd, "echo %s > /sys/class/gpio/gpio%d/direction", (direct == 0) ? "out" : "in", pin);
  }
  resp = SysCmd(cmd);
  if (resp != NULL) 
  {
    printf("%s", resp);
    free(resp);	
  }

  // sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
  // fd = fopen(path, "w+");
  // if (fd != NULL) {
  //   if (up)
  //     fprintf(fd, "%s+", (direct == 0) ? "out" : "in");
  //   else if (down)
  //     fprintf(fd, "%s-", (direct == 0) ? "out" : "in");
  //   else
  //     fprintf(fd, "%s", (direct == 0) ? "out" : "in");
  //   fclose(fd);
  // }
  // else {
  //   printf("%s ERROR: %s - %d\r\n", path, strerror(errno), errno);
  // }

  sprintf(cmd, "chmod %d /sys/class/gpio/gpio%d/value", (direct == 0) ? S_IWUSR | S_IWOTH | S_IWGRP : S_IRUSR | S_IRGRP | S_IROTH, pin);
  resp = SysCmd(cmd);
  if (resp != NULL) 
  {
    printf("%s", resp);
    free(resp);	
  }

  // sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
  // chmod(path, (direct == 0) ? S_IWUSR | S_IWOTH | S_IWGRP : S_IRUSR | S_IRGRP | S_IROTH);

  return open_gpio_pin(pin, direct);
}


int pinctl::unexport_pin(int pin)
{
  char cmd[80];
	char* resp;

  sprintf(cmd, "echo 0 > /sys/class/gpio/gpio%d/value", pin);
  resp = SysCmd(cmd);
  if (resp != NULL) 
  {
    printf("%s", resp);
    free(resp);	
  }

  sprintf(cmd, "echo %d > /sys/class/gpio/unexport", pin);
  resp = SysCmd(cmd);
  if (resp != NULL) 
  {
    printf("%s", resp);
    free(resp);	
  }
}

