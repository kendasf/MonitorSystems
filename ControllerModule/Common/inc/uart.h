#ifndef __UART_H_INCLUDED 
#define __UART_H_INCLUDED

#include <memory>

class uart {
public:
  enum uart_type {xbee, radar, aux};
  static const char* get_uart_path(uart_type ut);
  uart(const char* path, int baudrate);
  ~uart();

  void send(const char *buf, int len);
  void write(const char* text);
  int read(char* chr);
  void purge_in_buffers();

  int fd() const { return _fd; }

private:
  void open_tty(const char* path, int baudrate);

  int _fd = 0;
  bool _file = false;
  char _path[128];
};

typedef std::shared_ptr<uart> uart_ptr;

#endif //__UART_H_INCLUDED
