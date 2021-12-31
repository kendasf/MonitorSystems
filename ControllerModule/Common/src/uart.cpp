#include "uart.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>			// O_RDWR, O_NOCTTY, O_NONBLOCK...
#include <termios.h>
#include <errno.h>


const char* uart::get_uart_path(uart_type ut) {
   switch(ut) {
      case xbee:
         return "/dev/ttyO1";
      case radar:
         return "/dev/ttyO4";
      case aux:
         return "/dev/ttyO5";
   }

   return NULL;
}

uart::uart(const char* path, int baudrate)
{
   strcpy(_path, path);
   if (strstr(path, "ttyO") != NULL)
      open_tty(path, baudrate); // Open the port
   else {
      _fd = open(path, O_RDONLY); // Open a file
      _file = true;
   }
   if (_fd <= 0)
      printf("uart: open errno: %s\n", strerror(errno));
   // printf("uart #%s:  %d %d\r\n", path, _fd, baudrate);
}

uart::~uart() {
   if (_fd > 0)
      close(_fd);
}

inline int get_rate(int baudrate)
{
   switch(baudrate) {
      case 0: return B0;
      case 50: return B50;
      case 75: return B75;
      case 110: return B110;
      case 134: return B134;
      case 150: return B150;
      case 200: return B200;
      case 300: return B300;
      case 600: return B600;
      case 1200: return B1200;
      case 1800: return B1800;
      case 2400: return B2400;
      case 4800: return B4800;
      case 9600: return B9600;
      case 19200: return B19200;
      case 38400: return B38400;
      case 57600: return B57600;
      case 115200: return B115200;
      case 230400: return B230400;
      case 460800: return B460800;
      case 500000: return B500000;
      case 576000: return B576000;
      case 921600: return B921600;
      case 1000000: return B1000000;
      case 1152000: return B1152000;
      case 1500000: return B1500000;
      case 2000000: return B2000000;
      case 2500000: return B2500000;
      case 3000000: return B3000000;
      case 3500000: return B3500000;
      case 4000000: return B4000000;
      default: return B115200;
   }
   return B0;
}

void uart::open_tty(const char* path, int baudrate) {
   int rate = get_rate(baudrate);
   
   _fd = open(path, O_RDWR | O_NOCTTY | O_NONBLOCK); //set the user console port up
   if (_fd < 0) {
      printf("uart %s: error opening device\n", path);
      return;
   }

   struct termios newtio;       //place for old and new port settings for serial port
   memset(&newtio, 0, sizeof(newtio));

   newtio.c_cflag = rate | CS8 | CLOCAL | CREAD;
   newtio.c_iflag = IGNPAR;
   newtio.c_oflag = 0;
   newtio.c_lflag = 0;       //ICANON;
   newtio.c_cc[VMIN]=1;
   newtio.c_cc[VTIME]=0;
   tcflush(_fd, TCIFLUSH);
   tcsetattr(_fd,TCSANOW,&newtio);
}

void uart::send(const char *buf, int len) {
   if (_fd < 0)
      return;
   int err = ::write(_fd, buf, len);
   if (err < 0)
      printf("uart: %s - %d %s@%d\r\n", strerror(errno), errno, __FUNCTION__, __LINE__);
}

void uart::write(const char* text) {
   send(text, strlen(text));
}

int uart::read(char* chr) {
   if (_fd <= 0)
      return 0;

   int err = ::read(_fd, chr, 1);
   if (err <= 0) {
      if (_file) {
         close(_fd);
         _fd = open(_path, O_RDONLY);
      }
      return 0;
   }
   return 1;
}

void uart::purge_in_buffers() {
   char ch;
   while (read(&ch)); // Wait until it stops
}
