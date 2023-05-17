#ifndef __SPI_H_INCLUDED
#define __SPI_H_INCLUDED

#include <stdint.h>
#include <memory>

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

