#include <unistd.h>     // read, write
#include <stdio.h>      // printf, fprintf
#include <sys/types.h>  // open, close
#include <sys/stat.h>  // open, close
#include <fcntl.h>    // O_RDONLY, O_WRONLY
#include <string.h>   // strlen
#include <errno.h>

#include "pinctl.h"

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

  bool old_path = (::access("/sys/devices/virtual/gpio/", F_OK) == 0);

  if (old_path)
	 sprintf(path, "/sys/devices/virtual/gpio/gpio%d/value", pin);
  else
    sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
	if (dir == 0)
		fd = open(path, O_WRONLY);
	else
		fd = open(path, O_RDONLY);

	return fd;
}

int pinctl::export_pin(int pin, int dir) {
  FILE *fd = NULL;
  int direct = dir & 1; // 0 or 1 
  int up = (dir & 2) >> 1; 
  int down = (dir & 4) >> 2;
  char path[75] = "/sys/class/gpio/export";
  fd = fopen(path, "w");
  if (fd != NULL)
  {
    fprintf(fd, "%d", pin);
    fclose(fd);
  }
  else
  {
    printf("%s ERROR: %s - %d\r\n", path, strerror(errno), errno);
  }

  bool old_path = (::access("/sys/devices/virtual/gpio/", F_OK) == 0);

  if (old_path)
    sprintf(path, "/sys/devices/virtual/gpio/gpio%d/direction", pin);
  else
    sprintf(path, "/sys/class/gpio/gpio%d/direction", pin);
  fd = fopen(path, "w+");
  if (fd != NULL) {
    if (up)
      fprintf(fd, "%s+", (direct == 0) ? "out" : "in");
    else if (down)
      fprintf(fd, "%s-", (direct == 0) ? "out" : "in");
    else
      fprintf(fd, "%s", (direct == 0) ? "out" : "in");
    fclose(fd);
  }
  else {
    printf("%s ERROR: %s - %d\r\n", path, strerror(errno), errno);
  }


  if (old_path)
    sprintf(path, "/sys/devices/virtual/gpio/gpio%d/value", pin);
  else
    sprintf(path, "/sys/class/gpio/gpio%d/value", pin);
  chmod(path, (direct == 0) ? S_IWUSR | S_IWOTH | S_IWGRP : S_IRUSR | S_IRGRP | S_IROTH);

  return open_gpio_pin(pin, direct);
}


