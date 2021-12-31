/*****************************************************************************
 *   main.h:  Main program defines.
 *
 *   Copyright(C) 2008, Geolux d.o.o. (www.geolux.hr)
 *   All rights reserved.
 *
 *   History
 *   2008.11.11  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#ifndef __MAIN_H__
#define __MAIN_H__

#include <pthread.h>
#include "../Common/inc/uart.h"

#define SERVER_PORT_NUM 1530

// comment the following line for smaller DOTs
//#define LARGER_DOTS

#define PWM_PERIOD 2000000

#include "FileRoutines.h"

#define BASE_DATA "/store"

extern int TestModeBitmap;
extern int TestModeDuration;
extern int flashDrivePresent;
extern int DisplayFlashingCorners;

extern char *CAMERA_IP_ADDR;

void RTC_Start(void);
void UpdateMemoryCardStatus(void);
void CheckMemoryCard(void);
int CanWriteMemoryCard(void);
void CheckMemoryCardCapacity(void);
unsigned long long GetSecondsTimer(void);
int ExchangeTcpData(int sockFd);
void ExchangeUARTData(uart_ptr dev);
void DisplaySpeed(int speedToDisplay, DeviceInfoS *pDeviceInfo);
void UpdateLuxmeter(void);
int GetLuxmeterValue(void);
int DoAutoDimming(int force);
void SetTime(void);

int UpdateStandbyMode(DeviceInfoS *pDeviceInfo);
void SetupEthernetParams();
void waitPwmOff(void);

void displayWait(void);
void displayRelease(void);
int displayWaitDelay(long lTmoMs);

void ChangeWifiName(const char *newName);

typedef struct taskControlBlock
{
   pthread_t threadID;
   int hostPort;
   char hostIPAddr[50];
   int localPort;
   int arg1;
   int arg2;
   int arg3;
} TCB;

void CreateMemoryTask(int localPort);
void Directory(char *dir);

#define TIMECHECK_START                     \
   {                                        \
      static long maxDurTIMECHECK = 0;      \
      long startTIMECHECK = GetTickCount(); \
      long currentDurTIMECHECK;             \
      int lineTIMECHECK = __LINE__;         \
      char *fnctTIMECHECK = __FUNCTION__;
#define TIMECHECK_END                                                                           \
   currentDurTIMECHECK = GetTickCount() - startTIMECHECK;                                       \
   if (maxDurTIMECHECK < currentDurTIMECHECK)                                                   \
   {                                                                                            \
      printf("CHECK %s@%d: %ld\r\n", (const)fnctTIMECHECK, lineTIMECHECK, currentDurTIMECHECK); \
      maxDurTIMECHECK = currentDurTIMECHECK;                                                    \
   }                                                                                            \
   }

#undef LOGFILE_ENABLED

#endif // __MAIN_H__
