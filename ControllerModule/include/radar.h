
/*****************************************************************************
 *   radar.h:  Radar parsing routines.
 *
 *   Copyright(C) 2010, Geolux d.o.o.
 *   All rights reserved.
 *
 *   History
 *   2008.06.24  ver 1.00    Prelimnary version, first Release
 *
******************************************************************************/
#include "FileRoutines.h"
#include "../Common/inc/uart.h"

typedef enum RadarProtocolE_
{
	Radar_ProtoSpecial28 = 0,
	Radar_ProtoASCII64,
	Radar_ProtoNone,
	Radar_ProtoNMEA

} RadarProtocolE;

void Radar_CreateTask(int localPort, int reportPort);

uart_ptr Radar_Init(int baudRate, RadarProtocolE protocol, int sensitivity);
int Radar_AutoConf(void);

void Radar_ReceiveData(DeviceInfoS *pDeviceInfo);
int Radar_GetLastSpeed(void);
int Radar_GetLastSpeedTime(void);
int Radar_DetermineSpeedForDisplay(DeviceInfoS *pDeviceInfo);
int Radar_CurrentIntensity(void);

// private:
void Radar_ReceiveByte(unsigned char byte, DeviceInfoS *pDeviceInfo);
void Radar_ReceiveByte_Special28(unsigned char byte, DeviceInfoS *pDeviceInfo);
void Radar_ReceiveByte_ASCII64(unsigned char byte, DeviceInfoS *pDeviceInfo);
void Radar_ReceiveByte_NMEA(unsigned char byte, DeviceInfoS *pDeviceInfo);


