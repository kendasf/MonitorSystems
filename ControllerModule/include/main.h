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
#include "FileRoutines.h"
#include "../Common/inc/pwm.h"


#define SERVER_PORT_NUM 1530

// comment the following line for smaller DOTs
//#define LARGER_DOTS

#define BASE_DATA "/store"

// GPIO 
#define GPIO(A, B) (A*32 + B)
#define BUFLE GPIO(3, 16)			// GPIO_112, P9.14		Shift Register Latch
#define PWM_MONITOR GPIO(1,28)	// GPIO 60,  P9.12		PWM Edge Monitor
#define XB_RESET GPIO(1, 1)     // P8.24 Active Low XBreset
#define XB_SLEEP GPIO(1, 29)    // P8.26 Active Low XB Sleep
#define XB_POWER GPIO(2, 24)    // P8.28 Active High XB Power
#define PANEL_FLASH GPIO(2, 25) // P8.30 Active High Panel Flasher
#define CAMERA_POWER GPIO(1, 15) // P8.15 Active High Camera Power - GPIO 47
#define SHUTDOWN GPIO(2, 13)
#define BUFOE GPIO(3, 19)       // 115
#define RADAR_POWER  GPIO(2,1)  // P8.18 Active High Radar Power


extern int TestModeBitmap;
extern int TestModeDuration;
extern int flashDrivePresent;
extern int DisplayFlashingCorners;
extern pwmHandle thePWMHandle;

extern char *CAMERA_IP_ADDR;

void RTC_Start(void);
void UpdateMemoryCardStatus(void);
void CheckMemoryCard(void);
int CanWriteMemoryCard(void);
void CheckMemoryCardCapacity(void);
unsigned long long GetSecondsTimer(void);
int ExchangeTcpData(int sockFd);
void ExchangeUARTData(uart_ptr dev);
void DisplaySpeedToScreen(int speedToDisplay, DeviceInfoS *pDeviceInfo);
void UpdateLuxmeter(void);
unsigned int GetLuxmeterValue(void);
int GetPWMDuty(void);
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
