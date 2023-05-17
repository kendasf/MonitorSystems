#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <errno.h>				// errno
#include <string.h> 			// memset, strlen, strstr, ... etc
#include <stdlib.h> 			// atioi
#include <termios.h>			// B115200, CS8, CLOCAL, CREAD, IGNPAR, VMIN, VTIME, TCIFLUSH, TCSANOW
#include <stdarg.h> 			// va_list, va_start, va_end

#include "spi.h"

spi::spi(char *device, uint8_t mode, uint8_t bits, uint32_t speed) : _speed(speed), _bits(bits) {
   int ret = 0;
   int ret2 = 0;
   _fd = -1;
   int count = 0;

   while( _fd < 0 && count < 10)
   {
      _fd = open(device, O_RDWR);
      if (_fd < 0)
      {
         printf("spi: can't open device %s\n", device);
         printf("Waiting 15 seconds ...\n");
         sleep(15);
      }
      count++;
   }

   if(_fd < 0)
   {
      printf("Failed to connect with SPI bus\n");
      return;  /* Failed to allocated SPI */
   }
   
   
   printf("spi: starting \n");
   
   switch(mode)
   {
      default:
      case 0:
         ret = ioctl(_fd, SPI_IOC_WR_MODE, SPI_MODE_0);
         ret2 = ioctl(_fd, SPI_IOC_RD_MODE, SPI_MODE_0);
         break;

      case 1:
         ret = ioctl(_fd, SPI_IOC_WR_MODE, SPI_MODE_1);
         ret2 = ioctl(_fd, SPI_IOC_RD_MODE, SPI_MODE_1);
      break;

      case 2:
         ret = ioctl(_fd, SPI_IOC_WR_MODE, SPI_MODE_2);
         ret2 = ioctl(_fd, SPI_IOC_RD_MODE, SPI_MODE_2);
      break;

      case 3:
         ret = ioctl(_fd, SPI_IOC_WR_MODE, SPI_MODE_3);
         ret2 = ioctl(_fd, SPI_IOC_RD_MODE, SPI_MODE_3);
      break;
   }
   // spi mode
   
   if (ret == -1)
      printf("spi: can't set spi mode\n");

   if (ret2 == -1)
      printf("spi: can't get spi mode\n");

   // bits per word
   ret = ioctl(_fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
   if (ret == -1)
      printf("spi: can't set bits per word\n");

   ret = ioctl(_fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
   if (ret == -1)
      printf("spi: can't get bits per word\n");

   // max speed hz
   ret = ioctl(_fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
   if (ret == -1)
      printf("spi: can't set max speed hz\n");


   ret = ioctl(_fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
   if (ret == -1)
      printf("spi: can't get max speed hz\n");

      printf("SPI speed %d Hz\n", speed);
}

void spi::write(unsigned char *msg, int len) {
   memset(_rx, 0xFF, sizeof(_rx));
   memset(_tx, 0, sizeof(_tx));  
   memcpy(_tx, msg, len);
   transfer(len);
}

void spi::flush(void) {
   memset(_rx, 0xFF, sizeof(_rx));
   memset(_tx, 0, sizeof(_tx));
   transfer(sizeof(_tx));
}

void spi::transfer(int len) {
   struct spi_ioc_transfer tr = { 0 };

   tr.tx_buf = (unsigned long)_tx;
   tr.rx_buf = (unsigned long)_rx;
   tr.len = len;
   tr.delay_usecs = _delay;
   tr.speed_hz = _speed;
   tr.bits_per_word = _bits;

   if (_fd < 0) 
   {
      printf("spi: file not open\n");
      return;
   }

   int ret = ioctl(_fd, SPI_IOC_MESSAGE(1), &tr);
   if (ret < 1)
   {
      printf("spi: can't send spi message\n");
   }
}






