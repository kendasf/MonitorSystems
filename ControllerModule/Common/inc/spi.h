#ifndef __SPI_H_INCLUDED
#define __SPI_H_INCLUDED

#include <stdint.h>
#include <memory>

#define SPI_CLK_500K 500000
#define SPI_CLK_1M   1000000
#define SPI_CLK_2M   2000000
#define SPI_CLK_4M   4000000
#define SPI_CLK_8M   8000000
#define SPI_CLK_16M  16000000 // Max for BeagleBone Enhanced - look in device tree file

class spi {
public:
  spi(char *device, uint8_t mode, uint8_t bits, uint32_t speed);
  void write(unsigned char *msgbuff, int len);
  void flush(void);


private:
  void transfer(int len);

  int _fd;
  uint32_t _speed;
  uint8_t _bits;


  static const int buffer_size = 4092;  
  unsigned char _tx[buffer_size + 4];
  unsigned char _rx[buffer_size + 4]; 
  

  static const uint16_t _delay = 1;
};

typedef std::shared_ptr<spi> spi_ptr;


#endif // __SPI_H_INCLUDED

