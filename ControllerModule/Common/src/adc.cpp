#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "../../include/SystemUtilities.h"

#include "../../gnode/fs.h"
#include "../inc/adc.h"


void adc::enable_channel(int ch) {
   char chnl_name[51];
   bool channelFound = 0;

   for (int i = 0; i < 4; i++) {
      sprintf(chnl_name, "/sys/bus/iio/devices/iio:device%d/in_voltage%d_raw", i, ch - 1);
      if (gnode::fs::exists_sync(chnl_name))
      {
         channelFound = 1;
         break;
      }
   }

   if( 0 == channelFound ) // Invalid channel 
   {
      return;  
   }   
   else
   {
      if (adc_info[ch - 1].adc_fname != NULL)
         free(adc_info[ch - 1].adc_fname);

      adc_info[ch - 1].adc_fname = (char*) malloc(strlen(chnl_name) + 1);
      memset(adc_info[ch - 1].adc_fname, 0, strlen(chnl_name) + 1);
      strcpy(adc_info[ch - 1].adc_fname, chnl_name);
      
      adc_info[ch - 1].fd = fopen(chnl_name, "r");
      if (adc_info[ch - 1].fd == NULL)
      {
         printf("ADC channel fd error: %d - %s\r\n", errno, strerror(errno));
         adc_info[ch - 1].channelEnabled = 0;
      }
      else
      {
         //printf("ADC channel %d %d %s\r\n", ch, adc_info[ch - 1].fd, chnl_name);
         adc_info[ch - 1].channelEnabled = 1;
         fclose(adc_info[ch - 1].fd);
      }
         
   }
}

int c_last = 0;
int adc::readVal(int ch) {
   int val = -1;
   ssize_t retval = 0;
   int buffSize = 0;
   char adcbuff[5] = {0};

   if(0 == adc_info[ch - 1].channelEnabled )
   {
      printf("ADC channel not enabled yet\n");
      enable_channel(ch); // Try to do setup

      return -1;
   }
   else
   {
      adc_info[ch - 1].fd = fopen(adc_info[ch - 1].adc_fname, "r");
   }

   if( adc_info[ch - 1].fd != NULL )
   {
      retval = fread(adcbuff, sizeof(adcbuff), 1, adc_info[ch - 1].fd );
      if( 1 != retval )
      {
         printf(JOURNALD_LEVEL "ADC channel %d read error: %d - %s\r\n", ch, errno, strerror(errno));
      }
      else
      {
         val = atoi(adcbuff);
         if( c_last != ch )
         {
            //printf(JOURNALD_LEVEL "ADC %d read %s -> %d\n", ch, adcbuff, val );
            c_last = ch;
         }
      }
      fclose(adc_info[ch - 1].fd);
   }

   return val;
}

