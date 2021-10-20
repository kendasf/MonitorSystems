#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "../../include/SystemUtilities.h"

#include "../../gnode/fs.h"
#include "../inc/adc.h"


void adc::enable_channel(int ch) {
#ifndef SIMULATOR_MODE	
	char chnl_name[51];

	for (int i = 0; i < 4; i++) {
		sprintf(chnl_name, "/sys/bus/iio/devices/iio:device%d/in_voltage%d_raw", i, ch - 1);
		if (gnode::fs::exists_sync(chnl_name))
			break;	
	}
	

	if (adc_info[ch - 1].adc_fname != NULL)
		free(adc_info[ch - 1].adc_fname);

	adc_info[ch - 1].adc_fname = (char*) malloc(strlen(chnl_name) + 1);
	memset(adc_info[ch - 1].adc_fname, 0, strlen(chnl_name) + 1);
	strcpy(adc_info[ch - 1].adc_fname, chnl_name);
	
	adc_info[ch - 1].fd = fopen(chnl_name, "r");
	printf("ADC channel %d %p %s\r\n", ch, adc_info[ch - 1].adc_fname, chnl_name);
	if (adc_info[ch - 1].fd == NULL)
		printf("adc::enable_channel error: %d - %s\r\n", errno, strerror(errno));
	else
		fclose(adc_info[ch - 1].fd);
#endif	
}

int adc::read(int ch) {
#ifndef SIMULATOR_MODE	
	int val = -1;
	adc_info[ch - 1].fd = fopen(adc_info[ch - 1].adc_fname, "r");

	if(adc_info[ch - 1].fd == NULL)	{
		printf("Open Failure on ADC: %d\r\n", ch);
		return -1;
	}
	
	char adcbuff[20];
	memset(adcbuff, 0, sizeof(adcbuff));
	if (fgets(adcbuff, sizeof(adcbuff), adc_info[ch - 1].fd) != NULL) {
		val = atoi(adcbuff);
	}
	else {
		printf("ADC channel read %d not failed\r\n", ch);
		val = -1;
	}
	fclose(adc_info[ch - 1].fd);
	
	return val;
#else
	return 0;
#endif
}

