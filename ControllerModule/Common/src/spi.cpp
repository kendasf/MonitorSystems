#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#ifndef SIMULATOR_MODE
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#endif
#include <errno.h>				// errno
#include <string.h> 			// memset, strlen, strstr, ... etc
#include <stdlib.h> 			// atioi
#include <termios.h>			// B115200, CS8, CLOCAL, CREAD, IGNPAR, VMIN, VTIME, TCIFLUSH, TCSANOW
#include <stdarg.h> 			// va_list, va_start, va_end

#include "spi.h"

spi::spi(char *device, uint8_t mode, uint8_t bits, uint32_t speed) : _speed(speed), _bits(bits) {
#ifndef SIMULATOR_MODE	
	int ret = 0;

	_fd = open(device, O_RDWR);
	if (_fd < 0)
		printf("spi: can't open device %s\n", device);
	
	printf("spi: starting \n");
	
	// spi mode
	ret = ioctl(_fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1)
		printf("spi: can't set spi mode\n");

	ret = ioctl(_fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1)
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
#endif
}

void spi::write(unsigned char *msg, int len) {
#ifndef SIMULATOR_MODE
	memcpy(_tx, msg, len);
	memset(_rx, 0xFF, sizeof(_rx));
	transfer(len);
#endif
}

void spi::transfer(int len) {
#ifndef SIMULATOR_MODE
	struct spi_ioc_transfer tr = { 0 };

	tr.tx_buf = (unsigned long)_tx;
	tr.rx_buf = (unsigned long)_rx;
	tr.len = len;
	tr.delay_usecs = _delay;
	tr.speed_hz = _speed;
	tr.bits_per_word = _bits;

	if (_fd < 0) {
		printf("spi: file not open\n");
		return;
	}

	int ret = ioctl(_fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1)
		printf("spi: can't send spi message\n");
#endif
}






