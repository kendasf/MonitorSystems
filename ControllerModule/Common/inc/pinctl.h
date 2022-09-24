#ifndef __PINCTL_H_INCLUDED
#define __PINCTL_H_INCLUDED

class pinctl {
public:
  static pinctl& inst() {
    static pinctl i;
    return i;
  }

  int get(int fd);
  void set(int fd, int val);
  int export_pin(int pin, int dir);
  int unexport_pin(int pin);

private:
  pinctl() {}

  int open_gpio_pin(int pin, int dir);
};

#endif //__PINCTL_H_INCLUDED
