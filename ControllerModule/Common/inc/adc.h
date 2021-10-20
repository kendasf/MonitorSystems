#ifndef __ADC_H_INCLUDED
#define __ADC_H_INCLUDED

#include <stdio.h>

class adc {
public:
  static adc& inst() {
    static adc i;
    return i;
  }

  void enable_channel(int ch);
  int read(int ch);

private:
  adc() {}

  struct adc_type {
    char* adc_fname;
    FILE* fd;
  }; 

  adc_type adc_info[8];
};

#endif //__ADC_H_INCLUDED