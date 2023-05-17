#ifndef __ADC_H_INCLUDED
#define __ADC_H_INCLUDED

#include <stdio.h>

#define ADC_CHANNELS 8

class adc {
public:
  static adc& inst() {
    static adc i;
    return i;
  }

  void enable_channel(int ch);
  void delete_channel(int ch);
  int readVal(int ch);

private:
  adc() {}

  struct adc_type {
    char* adc_fname;
    FILE* fd;
    bool channelEnabled;
  }; 

  adc_type adc_info[ADC_CHANNELS];
};

#endif //__ADC_H_INCLUDED