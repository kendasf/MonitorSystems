#include "../Common/inc/uart.h"
#include "main.h"
#include "VMSDriver.h"
#include "radar.h"
#include "CommProt.h"
#include "FileRoutines.h"
#include "camera.h"
#include "cameraFuncts.h"
#include "../Common/inc/adc.h"
#include <signal.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/time.h>  // gettimeofday
#include <sys/times.h> // gettimeofday
#include <stdarg.h>
#include <chrono>
#include "../Ethernet/UdpInterface.h"
#include "SystemUtilities.h"
#include "../Ethernet/TcpInterface.h"
// #include "xbeeInterface.h"
#include "../Common/inc/timer.h"
#include "../Common/inc/pinctl.h"
#include "../Common/inc/pwm.h"
#include "FlashDriveTask.h"
#include "cameraFuncts.h"
// #define ASIO_STANDALONE
#include <boost/asio.hpp>
#include "std_fix.h"
#include "../gnode/fs.h"
#include "../gnode/net.h"
#include "../gnode/json.h"
#include "../gnode/path.h"
#include "../gnode/dgram.h"
#include "../gnode/timers.h"
#include "../gnode/executor.h"

#include "../http/efgy_http.h"
#include "http.h"

#define PWMMAX 36

static const int DIM[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

const int LookupLux[541] = {
    3077, 3069, 3060, 3052, 3043, 3035, 3026, 3018, 3009, 3001, 2993, 2984, 2976, 2967, 2959, 2951, 2942, 2934, 2926, 2917, 2909, 2901, 2893, 2884, 2876, 2868, 2860, 2851,
    2843, 2835, 2827, 2819, 2811, 2802, 2794, 2786, 2778, 2770, 2762, 2754, 2746, 2738, 2730, 2722, 2714, 2706, 2698, 2690, 2682, 2674, 2666, 2658, 2650, 2642, 2634, 2626,
    2618, 2611, 2603, 2595, 2587, 2579, 2572, 2564, 2556, 2548, 2540, 2533, 2525, 2517, 2510, 2502, 2494, 2487, 2479, 2471, 2464, 2456, 2449, 2441, 2433, 2426, 2418, 2411,
    2403, 2396, 2388, 2381, 2373, 2366, 2358, 2351, 2343, 2336, 2329, 2321, 2314, 2306, 2299, 2292, 2284, 2277, 2270, 2262, 2255, 2248, 2241, 2233, 2226, 2219, 2212, 2204,
    2197, 2190, 2183, 2176, 2169, 2161, 2154, 2147, 2140, 2133, 2126, 2119, 2112, 2105, 2098, 2091, 2084, 2077, 2070, 2063, 2056, 2049, 2042, 2035, 2028, 2021, 2014, 2008,
    2001, 1994, 1987, 1980, 1973, 1967, 1960, 1953, 1946, 1940, 1933, 1926, 1919, 1913, 1906, 1899, 1893, 1886, 1879, 1873, 1866, 1860, 1853, 1846, 1840, 1833, 1827, 1820,
    1814, 1807, 1801, 1794, 1788, 1781, 1775, 1768, 1762, 1756, 1749, 1743, 1736, 1730, 1724, 1717, 1711, 1705, 1698, 1692, 1686, 1679, 1673, 1667, 1661, 1655, 1648, 1642,
    1636, 1630, 1624, 1617, 1611, 1605, 1599, 1593, 1587, 1581, 1575, 1569, 1563, 1557, 1551, 1545, 1539, 1533, 1527, 1521, 1515, 1509, 1503, 1497, 1491, 1485, 1479, 1473,
    1468, 1462, 1456, 1450, 1444, 1439, 1433, 1427, 1421, 1416, 1410, 1404, 1398, 1393, 1387, 1381, 1376, 1370, 1364, 1359, 1353, 1348, 1342, 1336, 1331, 1325, 1320, 1314,
    1309, 1303, 1298, 1292, 1287, 1281, 1276, 1270, 1265, 1260, 1254, 1249, 1244, 1238, 1233, 1227, 1222, 1217, 1212, 1206, 1201, 1196, 1190, 1185, 1180, 1175, 1170, 1164,
    1159, 1154, 1149, 1144, 1139, 1134, 1128, 1123, 1118, 1113, 1108, 1103, 1098, 1093, 1088, 1083, 1078, 1073, 1068, 1063, 1058, 1053, 1048, 1043, 1039, 1034, 1029, 1024,
    1019, 1014, 1009, 1005, 1000, 995, 990, 986, 981, 976, 971, 967, 962, 957, 953, 948, 943, 939, 934, 929, 925, 920, 916, 911, 907, 902, 897, 893, 888, 884, 879, 875, 871,
    866, 862, 857, 853, 848, 844, 840, 835, 831, 827, 822, 818, 814, 809, 805, 801, 797, 792, 788, 784, 780, 775, 771, 767, 763, 759, 755, 751, 746, 742, 738, 734, 730, 726,
    722, 718, 714, 710, 706, 702, 698, 694, 690, 686, 682, 678, 674, 671, 667, 663, 659, 655, 651, 648, 644, 640, 636, 632, 629, 625, 621, 618, 614, 610, 606, 603, 599, 595,
    592, 588, 585, 581, 577, 574, 570, 567, 563, 560, 556, 553, 549, 546, 542, 539, 535, 532, 529, 525, 522, 518, 515, 512, 508, 505, 502, 498, 495, 492, 488, 485, 482, 479,
    476, 472, 469, 466, 463, 460, 456, 453, 450, 447, 444, 441, 438, 435, 432, 429, 426, 423, 420, 417, 414, 411, 408, 405, 402, 399, 396, 393, 390, 387, 384, 382, 379, 376,
    373, 370, 367, 365, 362, 359, 356, 354, 351, 348, 346, 343, 340, 338, 335, 332, 330, 327, 324, 322, 319, 317, 314, 312, 309, 307, 304, 302, 299, 297, 294, 292, 289, 287,
    284, 282, 280, 277, 275, 273, 270, 268, 266, 263, 261, 259, 257, 254, 252, 250, 248, 245, 243, 241, 239, 237, 235, 232, 230, 228, 226, 224, 222, 220};

#define NEW_IF_BOARD
#define EXTERNAL_DRIVEN_SIREN

static FILE *fpErrLog = NULL;

volatile int PWMDutyCycle = 10;

bool HaveExternalFlash = false;

int fileDirty = 1;
int CurrentlyDisplayedSpeed = -1;
int lastSpeedToDisplay = -1;
int CurrentlyDisplayedFrameIdx = 0;
int AnimationFrameIdx = 0;
unsigned long long CurrentlyDisplayedFrameStart = 0;
unsigned long long AnimationFrameStart = 0;
unsigned long long AnimationFrameLengthMs = 1;
int NumAnimationFrames = 1;
volatile int UpdateLuxmeterCnt = 0;
volatile int SetCameraModeCnt = 0;
int PrevBcIdx = -1;

int TestModeBitmap = 0;
int LastTestModeBitmap = -1;
int TestModeDuration = 0;
int ActiveEthSock = 0;

void readLux(void);
int LuxmeterAvgPos = 0;
float LuxMeterLPF = 100.0;
float lastLuxMeterLPF = 100.0;

int LastRefreshTime = 0;
int pwmMonitorFd = -1;

unsigned long pwmDuty = 0; // 95%;
pwmHandle thePWMHandle = NULL;                                 // Control PWM

void* DoAutoDimming( void *argPtr );
pthread_t autoDimThreadID;
char autoDimThreadRunning = 0;
char disablePWM = 0;

unsigned short UDPCallback(unsigned char socket, unsigned char *remip, unsigned short remport, unsigned char *buf, unsigned short len) { return 0; }

//
// timing globals
//
unsigned long long SecondsTimer = 0;
float SupplyVoltageLevel = 0.0f;

#define MAX_COMM_SENTENCE_LENGTH 820

char inSerialABuffer[MAX_COMM_SENTENCE_LENGTH];
int inSerialALength = 0;
char outSerialABuffer[MAX_COMM_SENTENCE_LENGTH];
int outSerialALength = 0;

char inSerialBtBuffer[MAX_COMM_SENTENCE_LENGTH];
int inSerialBtLength = 0;
char outSerialBtBuffer[MAX_COMM_SENTENCE_LENGTH];
int outSerialBtLength = 0;
char inSerialEthBuffer[MAX_COMM_SENTENCE_LENGTH];
int inSerialEthLength = 0;
char outSerialEthBuffer[MAX_COMM_SENTENCE_LENGTH];
int outSerialEthLength = 0;

int DisplayFlashingCorners = 0;

DeviceInfoS deviceInfo;

extern int LastSpeed;

static sem_t displaySem;

#define XB_RESET GPIO(1, 1)     // P8.24 Active Low XBreset
#define XB_SLEEP GPIO(1, 29)    // P8.26 Active Low XB Sleep
#define XB_POWER GPIO(2, 24)    // P8.28 Active High XB Power
#define PANEL_FLASH GPIO(2, 25) // P8.30 Active High Panel Flasher
#define CAMERA_POWER GPIO(1, 15) // P8.15 Active High Camera Power - GPIO 47

std::vector<gnode::fs::pending_info> gnode::fs::_pending_reads;
std::vector<gnode::fs::pending_info> gnode::fs::_pending_writes;
std::vector<gnode::timer_desc_ptr> gnode::timers::_active_timers;
std::deque<gnode::executor::running_process> gnode::executor::_running_processes;
std::mutex gnode::executor::_lock;
boost::asio::io_service gnode::net::_io_service;
boost::asio::io_service gnode::dgram::_io_service;
std::vector<gnode::connection_ptr> gnode::connection::_connections;

// Note: 10 = 10%
//        9 =  8%
//        8 =  6%
//        7 =  4%
//        6 =  2%
//        5 =  1%
//        4 = 1/2% 0.50%
//        3 = 1/3% 0.33%
//        2 = 1/4% 0.25%
//        1 = 1/5% 0.20%


unsigned long scalePercentage(unsigned long maxValue, unsigned long percentage)
{
   /*  Ex max is 30 and value put in is 100%  
   PWMMAX * 100% = 31, PWMMAX * 50% = 16
   */
   unsigned long retval = 0;
   float calc = (float)maxValue * ((float)percentage / 100.0);
   
   retval = (unsigned long)calc;
   if( retval < 1 )
   {
      retval = 1;
   }
   // printf("Max = %lu, Perc = %lu, Res %lu\n", maxValue, percentage, retval);
   return retval;
}

unsigned long calcPwmDuty(int dutyCyclePercent)
{
   unsigned long calcValue = 0;
   unsigned long setVal = 0;
   float newValue = 0.0;

#if 0 // This could give a better resolution for low light
if (dutyCyclePercent <= 5)
calcValue = PWM_PERIOD - (PWM_PERIOD/500)/(6 - dutyCyclePercent);
else if (dutyCyclePercent <= 10)
calcValue = PWM_PERIOD - ((dutyCyclePercent - 5) * 2) * (PWM_PERIOD/500);
else
calcValue = PWM_PERIOD - (dutyCyclePercent * (PWM_PERIOD/500));
#endif
   if( dutyCyclePercent > 0)
   {
      setVal = 100 - dutyCyclePercent;
      newValue = PWM_PERIOD * (setVal / 100.0);
      calcValue = (unsigned long)newValue;
   }
   else
   {
      calcValue = 0;
   }
   return calcValue;
}

void displayWait()
{
   sem_wait(&displaySem);
}

void displayRelease()
{
   sem_post(&displaySem);
}

int displayWaitDelay(long lTmoMs)
{
   int retval;
   struct timeval tv;
   struct timespec ts;

   gettimeofday(&tv, NULL);
   ts.tv_sec = tv.tv_sec + (lTmoMs / 1000);
   ts.tv_nsec = (tv.tv_usec + (lTmoMs % 1000) * 1000); // add half usecs
   if (ts.tv_nsec > 1000000)
   {
      ts.tv_sec++;
      ts.tv_nsec -= 1000000;
   }
   ts.tv_nsec *= 1000; /* convert to nanosecs */

   retval = sem_timedwait(&displaySem, &ts); // zero success; -1 failure

   return retval;
}

char *RecvMessages(int hSock, long lTmoMs)
{
   fd_set fdr, fde;
   struct timeval delay;
   int iSel;
   int len;
   char *retval = NULL;

   delay.tv_sec = (long)(lTmoMs / 1000);         // How many seconds
   delay.tv_usec = (long)(lTmoMs % 1000) * 1000; // milliseconds expressed in uS

   FD_ZERO(&fdr);
   FD_ZERO(&fde); // error fd

   FD_SET(hSock, &fdr);
   FD_SET(hSock, &fde);

   iSel = select(hSock + 1, &fdr, 0, &fde, &delay); // Select for both Cmd Pipe and For IO

   if ((iSel > 0) && (FD_ISSET(hSock, &fdr)))
   {
      int len;
      char *postMsg = udpRead(hSock, &len, 50);
      if (postMsg != NULL)
      {
         retval = postMsg;
      }
   }

   return retval;
}

void doShutdown()
{
   VMSDriver_Clear(true);
   autoDimThreadRunning = 0;
   stopPwm(thePWMHandle);
   deletePwm(thePWMHandle);
   VMSDriver_shutdown();
   sleep(2);
}

// Catching Traps.  Clearing display before Exit
// Also could trap differently for each type of trap
void TrapCtlC(int sigval)
{
   printf("TrapCtlC\nClearing Display\n");
   doShutdown();
   exit(sigval);
}

void TrapKill6(int sigval)
{
   printf("TrapKill6\nClearing Display\n");
   doShutdown();
   exit(sigval);
}

void TrapKill9(int sigval)
{
   printf("TrapKill9\nClearing Display\n");
   doShutdown();
   exit(sigval);
}

void TrapKill15(int sigval)
{
   printf("TrapKill15\nClearing Display\n");
   doShutdown();
   exit(sigval);
}

void SetTime(void) /* Dont need this anymore.  Handled by external OS script */
{
   // old-BB:: char* resp = SysCmd("i2cset -f -y 1 0x68 0x0E 0x1c b"); // Restarts the osc if not running
   char *resp = SysCmd("i2cset -f -y 2 0x68 0x00 0x1c b"); // Restarts the osc if not running
   if (resp != NULL)
      free(resp);

   // old-BB:: resp = SysCmd("echo ds1307 0x68 > /sys/class/i2c-adapter/i2c-1/new_device"); // Configures rtc1
   resp = SysCmd("echo pcf8523 0x68 > /sys/class/i2c-adapter/i2c-2/new_device"); // Configures rtc1
   if (resp != NULL)
      free(resp);

   resp = SysCmd("hwclock -s -f /dev/rtc1"); // Sets System Time
   if (resp != NULL)
      free(resp);

   resp = SysCmd("hwclock -s -f /dev/rtc1"); // Sets System Time
   if (resp != NULL)
      free(resp);

   resp = SysCmd("hwclock -w -f /dev/rtc0");
   if (resp != NULL)
      free(resp);
}

void ChangeWifiName(const char *newName)
{
   char cmd[128];
   sprintf(cmd, "sed -i s/ssid=.*/ssid=%s/ /etc/hostapd/hostapd.conf", newName);
   char *resp = SysCmd(cmd);
   if (resp != NULL)
      free(resp);

   sprintf(cmd, "systemctl restart hostapd.service"); /* Force the name change */
   resp = SysCmd(cmd);
   if (resp != NULL)
      free(resp);
}

int GetVideoDelayMSecs(int speed, DeviceInfoS *pDeviceInfo)
{
   // Calculate number of milliseconds to delay video capture based on vehicle speed
   int VideoDelayMSecs;
   int VehicleMPH;
   int MaxDelayMSecs = 4500;

   //Speed is in MPH - use as is
   VehicleMPH = speed;

   VideoDelayMSecs = MaxDelayMSecs - (VehicleMPH * 100);
   if (VideoDelayMSecs < 0)
      VideoDelayMSecs = 0;

   return VideoDelayMSecs;
}

// int main (void)
//
// Main program loop.
//
int camPwrPin = -1;
int main(int argc, char *argv[])
{
   int i;
   // unsigned long long loopTime = 0;
   int secondsTimer = 0;
   int speedToDisplay = 0;
   int bStandby = 0;
   int voltageTooLow = 0;
   int hSock = -1;
   char *pStatus;
   int initval = 0;
   int xbeeSleepFd = -1;
   int xbeeResetFd = -1;
   int xbeePowerFd = -1;
   uart_ptr uart_xbee;
   unsigned long pwmPeriod = PWM_PERIOD; // 10 000 000 =  100Hz and  8 000 000 =  125Hz
   int tcpListenerFd = -1;
   int tcpSocketFd = -1;
   int mainPort = 6800;
   int cameraBusy = 0;
   int res = 0;
   char *inMsg = NULL;
   char pinSet = 0;

   long loopMax = 0;
   long loopDur = 0;

#ifdef USE_SHUTDOWN
   int shutdownPinFd;
   int shutdownEnabled = 0;
#endif

   signal(SIGINT, TrapCtlC);   // Setup signal trap for child exit
   signal(SIGABRT, TrapKill6); // Setup signal trap for child exit
   signal(SIGTERM, TrapKill15);
   signal(SIGKILL, TrapKill9); // Setup signal trap for child exit
   signal(SIGSEGV, TrapKill9); // Setup signal trap for child exit

   //SysCmd("/root/init_peripherals.sh");  // Handle by systemd now

   // VMS Driver Init here to clear screen before PWM enabled
   VMSDriver_Initialize();
   
   initPWMDriver(); /* This sets up the structures to drive the PWM */

   // init PWM
   thePWMHandle = createPWM(0, pwmPeriod, 0);
   setPwmDuty(thePWMHandle, 0ul);
   startPwm(thePWMHandle);

   

   printf(JOURNALD_LEVEL "Setting ADC 4 for Lux Meter\n");
   adc::inst().enable_channel(4); // luxmeter

   printf(JOURNALD_LEVEL "Setting ADC 2 for Vin Monitor\n");
   adc::inst().enable_channel(2); // Vin

   camPwrPin = pinctl::inst().export_pin(CAMERA_POWER, 0);
   pinctl::inst().set(camPwrPin, 0);

   for (i = 0; i < 500; i++)  // Preload lux LPF
   {
      readLux();
      usleep(500);
   }

   res = pthread_create(&autoDimThreadID, NULL, DoAutoDimming, NULL);
   if( res != 0 )
   {
      printf("%d - %s", res, strerror(res));
      return -1;
   }

   CommProt_init();

   if (FileRoutines_autoconfFromCard("/root/update.vac") > 0)
   {
      printf("Configuration Updated\r\n");
      unlink("/root/update.vac");
   }

   if (argc > 1)  // May augment update.vac to have camera IP stuff
   {
      CAMERA_IP_ADDR = argv[1];
   }

   if (sem_init(&displaySem, 0, 1) == -1)
   {
      printf("sem_init(&displaySem): %s\n", strerror(errno));
      return -1;
   }

   /* Mount the external Flash */
   uSD_flash_monitor_init();


   /* The following line is a dirty hack. Why, you ask? Because some idiot who wrote vmsd which starts vms process did not give
   // vms process proper stdin, so its filedescription "0" is left unused. And when the first socket is created, it gets filedesc of zero.
   // For some reason, the code does not work when filedesc for socket is zero. So lets create dummy UDP server socket that will 
   // have filedesc zero and then everything else magically works.
   // - Niksa, 2016-11-11

   hSock = GetUdpServerHandle(12345);		No longer needed - systemd handles stdin correctly
   */

   hSock = GetUdpServerHandle(mainPort);

   if (hSock < 0)
   {
      printf("Socket allocation failure\n");
      return -1;
   }

// Config for shutdown ping usage
#ifdef USE_SHUTDOWN
   shutdownPinFd = pinctl::inst().export_pin(GPIO(2, 13), 1);
   shutdownEnabled = pinctl::inst().get(shutdownPinFd);
   if (shutdownEnabled)
      printf("Shutdown Monitor Enabled\n");
   else
      printf("Shutdown Monitor not enabled\n");
#endif

   FileRoutines_readDeviceInfo(&deviceInfo);

   

   tcpListenerFd = getTcpListen(SERVER_PORT_NUM, 1);
   
   Directory("/store/System");

   CreateMemoryTask(5801);

   // CreateCameraTask(5800, mainPort);
   Radar_CreateTask(5802, mainPort);

   //
   // log system start event
   //
   printf("Before add log\n");
   FileRoutines_addLog(LOG_SYSTEM_START, NULL);

   printf("Before read info\n");
   FileRoutines_readDeviceInfo(&deviceInfo);
   fileDirty = 1;

   // loopTime = GetTickCount();

   printf("Before start web services\n");
   http::inst().start_web_services("/root/html", "/store", "/tmp", 80);
   printf("Before program loop\n");

   VMSDriver_RunStartSequence();

   while (1)
   {
      for (int i = 0; i < 10; i++)
      {
         if (!gnode::fs::proc_events())
            break;
      }

      gnode::timers::proc_events();
      gnode::net::proc_events();
      gnode::dgram::proc_events();
      gnode::executor::proc_events();
      http::inst().proc_events();

      pthread_yield();

      // loopTime = GetTickCount();

      //
      // update device info
      //
      if (fileDirty)
      {
         fileDirty = 0;
         VMSDriver_Clear(true);
         FileRoutines_readDeviceInfo(&deviceInfo);
      }

      //
      // handle inbound data
      //
      inMsg = RecvMessages(hSock, 5); // short wait
      if (inMsg != NULL)
      {
         if (strstr(inMsg, "SNAP") != NULL)
         {
            printf("Camera Ready\n");
            cameraBusy = 0;
         }
         free(inMsg);
         inMsg = NULL;
      }

      // if TestModeDuration active then speed is ignored
      if (TestModeDuration == 0)
      {
         int speedBefore = speedToDisplay;

         speedToDisplay = Radar_DetermineSpeedForDisplay(&deviceInfo);

         if (speedToDisplay < 0)
            speedToDisplay = 0;

         if (lastSpeedToDisplay != speedToDisplay)
         {
            int blinkLimit = deviceInfo.blinkLimit;
            int displaySpeed = speedToDisplay;

            lastSpeedToDisplay = speedToDisplay;
            printf("New Speed: %d mph\r\n", speedToDisplay);
            // Send Command to Camera
            if (deviceInfo.unitType == 1)
            {
               int addKph = 0;
               blinkLimit = blinkLimit * 10000 / 16093; // Get BlinkLimit in MPH
               displaySpeed = speedToDisplay * 16093;   // make sure to display KPH
               if ((displaySpeed % 10000) >= 5000)
                  addKph = 1;
               displaySpeed /= 10000; // make sure to display KPH
               displaySpeed += addKph;
               printf("Speed in KPH: %d\r\n", displaySpeed);
            }

            if (speedToDisplay >= blinkLimit)
            {
               if (cameraBusy == 0)
               {
                  static int snap = 0;
                  int image = 1;
                  char requestSnap[128];
                  int slowSpeed = 45;

                  cameraBusy = 1;
                  if (speedToDisplay > slowSpeed)
                     image = 1;
                  else
                     image = 2;

                  sprintf(requestSnap, "SNAP,%s,%d,%d,%d", CAMERA_IP_ADDR, displaySpeed,
                          GetVideoDelayMSecs(speedToDisplay, &deviceInfo), image);

                  printf("Send Speed: %d %s\n", speedToDisplay, requestSnap);
                  udpSendLocal(hSock, 5800, requestSnap, strlen(requestSnap));

                  char debug[64];
                  sprintf(debug, "snap: %d\n", displaySpeed);
                  FileRoutines_SaveDebugInfo(debug, strlen(debug));
               }
               else // Ask for speed adjust
               {
                  char requestSnap[128];
                  sprintf(requestSnap, "SPEEDADJ,%s,%d", CAMERA_IP_ADDR, displaySpeed);
                  printf("Adjust Speed: %d, %s\n", speedToDisplay, requestSnap);
                  udpSendLocal(hSock, 5800, requestSnap, strlen(requestSnap));

                  char debug[64];
                  sprintf(debug, "snap-skip: %d\n", displaySpeed);
                  FileRoutines_SaveDebugInfo(debug, strlen(debug));

                  lastSpeedToDisplay = 0;
               }
            }
            else
            {
               char adjustSpeed[128];
               sprintf(adjustSpeed, "SPEEDADJ,%s,%d", CAMERA_IP_ADDR, displaySpeed);
               printf("Adjust Speed: %d, %s\n", speedToDisplay, adjustSpeed);
               udpSendLocal(hSock, 5800, adjustSpeed, strlen(adjustSpeed));
            }
         }

         if ((!bStandby) || DisplayFlashingCorners)
            DisplaySpeed(speedToDisplay, &deviceInfo);
         else
            VMSDriver_Off();

#ifdef EXTERNAL_DRIVEN_SIREN
         if( speedToDisplay > deviceInfo.blinkLimit )   /* Blink speed is the speed limit according to the webservices.cpp  */
         {
            pinctl::inst().set(camPwrPin, 1);
         }
         else
         {
            pinctl::inst().set(camPwrPin, 0);            
         }
#endif

         displayRelease();
      }
      else
      {
         CurrentlyDisplayedSpeed = 0;
      }

      if ((UpdateLuxmeterCnt > 30) && (!voltageTooLow))  // Update auto dim every 30 seconds
      {

         static int lastDutyCycleLevel = -1;
         UpdateLuxmeterCnt = 0;

         // Change only if there is a zero crossing - But why!!!
         if (((lastDutyCycleLevel > 30) && (PWMDutyCycle <= 30)) || 
             ((lastDutyCycleLevel <= 30) && (PWMDutyCycle > 30)) ||
             (lastDutyCycleLevel == -1) ||
             (SetCameraModeCnt > 10 * 60))
         {
            printf("PWM Duty Change: Last = %d Current = %d\n", lastDutyCycleLevel, PWMDutyCycle);
            if (PWMDutyCycle > 30)
            {
               char request[50] = "DAYTIME,0,0,0,0";
               udpSendLocal(hSock, 5800, request, strlen(request));
            }
            else
            {
               char request[50] = "NIGHTTIME,0,0,0,0";
               udpSendLocal(hSock, 5800, request, strlen(request));
            }
            SetCameraModeCnt = 0;
         }
         lastDutyCycleLevel = PWMDutyCycle;
      }

      if (secondsTimer != RTC_SEC)
      {
         // increment global second counter
         SecondsTimer++;
         secondsTimer = RTC_SEC;

         UpdateLuxmeterCnt++;
         SetCameraModeCnt++;

         CommProt_KeepAlive();

         bStandby = UpdateStandbyMode(&deviceInfo);

         // handle test mode
         if (TestModeDuration > 0)
         {
            if ((TestModeDuration > 1) && (LastTestModeBitmap != TestModeBitmap))
               VMSDriver_TestMode(TestModeBitmap);

            LastTestModeBitmap = TestModeBitmap;

            TestModeDuration--;
            if (TestModeDuration == 0)
            {
               VMSDriver_Off();
               LastTestModeBitmap = -1;
               // Reset Animation Sequences
               CurrentlyDisplayedFrameIdx = 0;
               CurrentlyDisplayedFrameStart = GetTickCount();
               AnimationFrameIdx = 0;
               AnimationFrameStart = GetTickCount();
               pinctl::inst().set(camPwrPin, 0);
            }
            else
            {
               pinctl::inst().set(camPwrPin, 1);
            }
         }


         int adcr = adc::inst().readVal(2);
         float calcV = (adcr * (3.3 / 4096) * 8.5) + 0.225; // 0.225 is the measured line loss fudge factor

         if ((RTC_SEC % 2) == 0)
         {
#ifdef NEW_IF_BOARD
            // r1 = 1K r2 =7.5K VREF = 3.3
            // Vmax In = 15.3V cause 4096 counts  = 3.3V = 8.5 * 1.8 = 15.3V
            if (SupplyVoltageLevel == 0.0)
               SupplyVoltageLevel = calcV;
            else
               // Poor mans LPF - will cause ramp up over about 20 Seconds
               SupplyVoltageLevel = (SupplyVoltageLevel * 0.75) + (calcV * 0.25);
#else
            if (SupplyVoltageLevel == 0.0)
               SupplyVoltageLevel = adcr / 254.0f;
            else
               SupplyVoltageLevel = (SupplyVoltageLevel + (2 * adcr / 266.1f)) / 3.0f;
#endif

            // flash status LED to indicate that the software is running properly
            if (TestModeDuration > 0)
            {
               pinSet = ~pinSet;
               pinctl::inst().set(camPwrPin, pinSet);
            }
            
         }

         /*
if (!voltageTooLow && (SupplyVoltageLevel < 8.5))
{
FileRoutines_addLog(LOG_VOLTAGE_TOO_LOW, NULL);
voltageTooLow = 1;
}

if (voltageTooLow && (SupplyVoltageLevel > 9.5))
{
voltageTooLow = 0;
}
*/

         if (SupplyVoltageLevel < 8.5)
         {
            FileRoutines_addLog(LOG_VOLTAGE_TOO_LOW, NULL);
            voltageTooLow = 1;
            disablePWM = 1;
         }
         else
         {
            voltageTooLow = 0;
            disablePWM = 0;
         }

         /*
         if ((RTC_SEC % 30) == 0)
         {
            char *resp = SysCmd("rm /var/log/vms.log");
            if (resp)
               free(resp);
            resp = SysCmd("rm /var/log/vmsd.log");
            if (resp)
               free(resp);
         }
         */
      }

      // TCP Communication
      // if (tcpSocketFd < 0) // If no active socke then check Listening sock
      {
         int newFd = getTcpActiveSocket(tcpListenerFd);
         if (newFd > 0)
         {
            if (tcpSocketFd > 0) // Close old socket if there is an open one
            {
               close(tcpSocketFd);
            }
            CommProt_resetChannel(CHANNEL_SERIAL_A); // Init channel info
            tcpSocketFd = newFd;
         }
      }

      // If active socket check for activity
      if (tcpSocketFd > 0)
      {
         int status = ExchangeTcpData(tcpSocketFd);
         if ((status < 0) && (errno == 104))
         {
            printf("Closing tcp Socket\r\n");
            close(tcpSocketFd);
            tcpSocketFd = -1;
         }
      }

      if (uart_xbee)
         ExchangeUARTData(uart_xbee);

#ifdef USE_SHUTDOWN
      if ((shutdownEnabled) && (pinctl::inst().get(shutdownPinFd) == 0))
      {
         char *resp = NULL;
         if (fpErrLog != NULL)
         {
            fclose(fpErrLog);
            fpErrLog = NULL;
         }
         VMSDriver_Clear(true);
         // VMSDriver_UpdateFrame();
         resp = SysCmd("shutdown -h now");
         if (resp != NULL)
         {
            free(resp);
         }
         printf("Halting vms\n");
         usleep(1000 * 60000);
      }
#endif
   }

   autoDimThreadRunning = 0;

   return 0;
}

//
// unsigned int GetSecondsTimer()
//
// Returns the relative seconds counter value.
//
unsigned long long GetSecondsTimer()
{
   return SecondsTimer;
}

//#define DEBUG_TCP
//
// void ExchangeTcpData()
//
// Exchanges UARTn data between the TCP and the CommProt module.
//
int ExchangeTcpData(int sockFd)
{
   int status = -1;

   if ((status = tcpDataWait(sockFd, 10)) > 0)
   {
      // Read Data from Tcp Socket
      int len = 0;
      unsigned char *data = (unsigned char *)tcpRecv(sockFd, &len, 50);
      //
      // copy received data to incoming buffer
      //
      if (len > 0)
      {
         int i;

         //
         // copy received UART data to incoming buffer
         //
         for (i = 0; i < len; i++)
         {
            // wait for the sentence start symbol
            if ((inSerialEthLength == 0) && (data[i] != 0x19))
               continue;

            // append the received data
            inSerialEthBuffer[inSerialEthLength] = data[i];
            inSerialEthLength++;

            // check if incoming buffer is full
            if (inSerialEthLength > MAX_COMM_SENTENCE_LENGTH)
               inSerialEthLength = 0;

            // check if sentence end has been found; process the sentence
            if (data[i] == 0x18)
            {
               int i;
               ActiveEthSock = sockFd;
               CommProt_processStream(inSerialEthBuffer, inSerialEthLength, CHANNEL_ETHERNET);
               fileDirty = 1;
               inSerialEthLength = 0;
               break;
            }
         }
         free(data);
      }
   }

   if (status != -1)
   {
      //
      // write UART responses
      //
      outSerialEthLength = CommProt_getOutputBuffer(outSerialEthBuffer, MAX_COMM_SENTENCE_LENGTH - 1, CHANNEL_ETHERNET);
      if (outSerialEthLength > 0)
      {
         status = tcpWrite(sockFd, (unsigned char *)outSerialEthBuffer, outSerialEthLength);
      }
   }
   return status;
}

//
// void ExchangeUARTData()
//
// Exchanges UARTn data between the UART controller and the CommProt module.
//
void ExchangeUARTData(uart_ptr dev)
{
   char incomingChar;

   //
   // copy received UART data to incoming buffer
   //
   while (dev->read(&incomingChar))
   {
      // wait for the sentence start symbol
      if ((inSerialALength == 0) && (incomingChar != 0x19))
         continue;

      // append the received data
      inSerialABuffer[inSerialALength] = incomingChar;
      inSerialALength++;

      // check if incoming buffer is full
      if (inSerialALength > MAX_COMM_SENTENCE_LENGTH)
         inSerialALength = 0;

      // check if sentence end has been found; process the sentence
      if (incomingChar == 0x18)
      {
         CommProt_processStream(inSerialABuffer, inSerialALength, CHANNEL_SERIAL_A);
         fileDirty = 1;
         inSerialALength = 0;
         break;
      }
   }

   //
   // write UART responses
   //
   outSerialALength = CommProt_getOutputBuffer(outSerialABuffer, MAX_COMM_SENTENCE_LENGTH - 1, CHANNEL_SERIAL_A);
   if (outSerialALength > 0)
   {
      dev->send(outSerialABuffer, outSerialALength);
   }
}

void SetBitmapPixel(BitmapS *bmp, int x, int y)
{
   int bmpPixelPos = y * MAX_BMP_WIDTH + x;
   bmp->bitmapData[bmpPixelPos >> 3] |= 1 << (bmpPixelPos & 0x07);
}

void MakeCornerBitmap(BitmapS *bmp, int width, int height)
{
   memset(bmp->bitmapData, 0, sizeof(bmp->bitmapData));
   for (int i = 0; i < 3; i++)
   {
      for (int j = 0; j < 3; j++)
      {
         SetBitmapPixel(bmp, i, j);
         SetBitmapPixel(bmp, i + width - 3, j);
         SetBitmapPixel(bmp, i, j + height - 3);
         SetBitmapPixel(bmp, i + width - 3, j + height - 3);
      }
   }
}

//
// void DisplaySpeed(int speedToDisplay, DeviceInfoS* pDeviceInfo)
//
// Updates the VMS if neccessary.
//
void DisplaySpeed(int speedToDisplay, DeviceInfoS *pDeviceInfo)
{
   int minSpeed, blinkSpeed, maxSpeed;
   int refreshRequired = 0;
   int bcIdx = 0;
   std::chrono::duration<float> durationCast;

   //
   // determine min, blink and maximum speed limits
   //
   minSpeed = pDeviceInfo->minDisplaySpeed;
   blinkSpeed = pDeviceInfo->blinkLimit;
   maxSpeed = pDeviceInfo->maxDisplaySpeed;

   if (pDeviceInfo->unitType == 1)
   {
      minSpeed = minSpeed * 10000 / 16093;
      blinkSpeed = blinkSpeed * 10000 / 16093;
      maxSpeed = maxSpeed * 10000 / 16093;
   }

   if (speedToDisplay <= minSpeed)
      bcIdx = 0;
   else if (speedToDisplay <= blinkSpeed)
      bcIdx = 1;
   else if (speedToDisplay <= maxSpeed)
      bcIdx = 2;
   else
      bcIdx = 3;

   if ((bcIdx == 1) && (speedToDisplay == CurrentlyDisplayedSpeed) && (!DisplayFlashingCorners))
      return;

   //
   // if new speed is set, update the new speed; otherwise update frame count
   //
   if (speedToDisplay != CurrentlyDisplayedSpeed)
   {
      int forceResetAnim = (CurrentlyDisplayedSpeed == -1);
      CurrentlyDisplayedSpeed = speedToDisplay;

      if (forceResetAnim || (PrevBcIdx != bcIdx) || (pDeviceInfo->bitmapsConfig[bcIdx].speedDisplayMode == 1) || (pDeviceInfo->bitmapsConfig[bcIdx].speedDisplayMode == 2))
      {
         CurrentlyDisplayedFrameIdx = 0;
         CurrentlyDisplayedFrameStart = GetTickCount();
         refreshRequired = 1;
         AnimationFrameIdx = 0;
         AnimationFrameStart = GetTickCount();
      }

      if (pDeviceInfo->radarProtocol != Protocol_NMEA)
         FileRoutines_addVehicleLog(speedToDisplay);
   }
   else
   {
      if (CurrentlyDisplayedFrameIdx == 0)
      {
         if (((GetTickCount() - CurrentlyDisplayedFrameStart) >= pDeviceInfo->blinkOnDurationMs) ||
             (GetTickCount() < CurrentlyDisplayedFrameStart))
         {
            CurrentlyDisplayedFrameIdx = 1;
            if (GetTickCount() < CurrentlyDisplayedFrameStart)
               CurrentlyDisplayedFrameStart = GetTickCount();
            else
               CurrentlyDisplayedFrameStart = CurrentlyDisplayedFrameStart + pDeviceInfo->blinkOnDurationMs;
            refreshRequired = 1;
         }
      }
      else
      {
         if (((GetTickCount() - CurrentlyDisplayedFrameStart) >= pDeviceInfo->blinkOffDurationMs) ||
             (GetTickCount() < CurrentlyDisplayedFrameStart))
         {
            CurrentlyDisplayedFrameIdx = 0;
            if (GetTickCount() < CurrentlyDisplayedFrameStart)
               CurrentlyDisplayedFrameStart = GetTickCount();
            else
               CurrentlyDisplayedFrameStart = CurrentlyDisplayedFrameStart + pDeviceInfo->blinkOffDurationMs;
            refreshRequired = 1;
         }
      }

      if (TicksElapsed(AnimationFrameStart) >= AnimationFrameLengthMs)
      {
         if (NumAnimationFrames > 1)
         {

            AnimationFrameIdx = (AnimationFrameIdx + 1) % NumAnimationFrames;
            if (GetTickCount() < AnimationFrameStart)
               AnimationFrameStart = GetTickCount();
            else
            {
               if ((GetTickCount() - AnimationFrameStart) > AnimationFrameLengthMs * 4)
               {
                  AnimationFrameStart = GetTickCount();
                  CurrentlyDisplayedFrameStart = GetTickCount();
               }
               else
                  AnimationFrameStart = AnimationFrameStart + AnimationFrameLengthMs;
            }
            refreshRequired = 1;
         }
      }
   }

   if (LastRefreshTime + 30 < SecondsTimer)
   {
      VMSDriver_Invalidate();
      refreshRequired = 1;
   }

   if (DisplayFlashingCorners)
      refreshRequired = 1;

   //
   // if the display needs to be refreshed, refresh it
   //
   if (refreshRequired)
   {
      VMSDriver_Clear(false);

      PrevBcIdx = bcIdx;

      //
      // draw bitmap
      //
      if (pDeviceInfo->bitmapsConfig[bcIdx].numFrames == 0)
      {
         // no bitmap to display
      }
      else if (pDeviceInfo->bitmapsConfig[bcIdx].numFrames == 1)
      {
         VMSDriver_RenderBitmap((pDeviceInfo->bitmapsConfig[bcIdx].frames[0]) - 1, NULL);
      }
      else
      {
         int imageID = ((pDeviceInfo->bitmapsConfig[bcIdx].frames[AnimationFrameIdx]) - 1);

         VMSDriver_RenderBitmap(imageID, NULL);
      }

      //
      // draw speed
      //
      if ((pDeviceInfo->bitmapsConfig[bcIdx].speedDisplayMode == 1) ||
          ((pDeviceInfo->bitmapsConfig[bcIdx].speedDisplayMode == 2) && (CurrentlyDisplayedFrameIdx == 0)))
      {
         int speed = CurrentlyDisplayedSpeed;
         if (pDeviceInfo->unitType == 1)
         {
            int addKph = 0;
            speed = speed * 16093;
            if ((speed % 10000) >= 5000)
               addKph = 1;
            speed /= 10000;
            speed += addKph;
         }

         VMSDriver_WriteSpeed(pDeviceInfo->bitmapsConfig[bcIdx].x, pDeviceInfo->bitmapsConfig[bcIdx].y,
                              pDeviceInfo->panelsConfiguration, speed, pDeviceInfo->bitmapsConfig[bcIdx].font);
      }

      NumAnimationFrames = pDeviceInfo->bitmapsConfig[bcIdx].numFrames;
      AnimationFrameLengthMs = pDeviceInfo->bitmapsConfig[bcIdx].frameLength;

      if (DisplayFlashingCorners)
      {
         BitmapS corners;
         int width, height;
         VMSDriver_GetDimensions(pDeviceInfo->panelsConfiguration, width, height);
         MakeCornerBitmap(&corners, width, height);

         VMSDriver_Clear(false);

         if ((GetTickCount() % 1000) < 500)
            VMSDriver_RenderBitmap(1, &corners);
         else
            VMSDriver_RenderBitmap(1, &corners, 0);
      }

      if (VMSDriver_UpdateFrameFast())
      {
         LastRefreshTime = SecondsTimer;
         // set LE signal
         // SetDisplayLE();
      }
   }
}

//
// int GetLuxmeterValue()
//
// Returns the average luminance.
//
int GetLuxmeterValue()
{
   return (int)LuxMeterLPF;
}

//
// void DoAutoDimming()
//
// Changes brightness if auto dimming is selected.
//
void* DoAutoDimming( void *argPtr )
{
   int targetBrightness;
   DeviceInfoS deviceInfo;
   struct timespec sleepDelay, timeLeft;
   int i, j;
   int luminance = 0;
   char useAutoDim = 0;
   unsigned long pwmSet = 0;
   unsigned long pwmMax = PWMMAX;

   autoDimThreadRunning = 1;

   sleepDelay.tv_nsec = (150) * (1000) * (1000);                       // 100ms sleep cycle
   sleepDelay.tv_sec = 0;


   printf("Absolute PWM max is %lu percent of input voltage\n", pwmMax);

   while(1 == autoDimThreadRunning )
   {
      FileRoutines_readDeviceInfo(&deviceInfo);                      // Update our config

      if (1 == deviceInfo.autoDim)                                           // Are we doing auto dim?
      {
         luminance = GetLuxmeterValue();
         PWMDutyCycle = deviceInfo.autoDimming[15].brightness;       // Start at max

         for (i = 0; i < 16; i++)   // This is a table lookup 
         {
            if (luminance <= deviceInfo.autoDimming[i].luminance)
            {
               PWMDutyCycle = deviceInfo.autoDimming[i].brightness;  // 0 - 100
               break;                                                // Found the array value
            }
         }
      }
      else
      {
         PWMDutyCycle = (deviceInfo.displayBrightness & 0x7F) ; 
      }

      pwmSet = scalePercentage(pwmMax, PWMDutyCycle);
      
      if(1 == disablePWM)
      {
         pwmDuty = 0;
      }
      else
      {
         pwmDuty = calcPwmDuty(pwmSet);
      }

      setPwmDuty(thePWMHandle, pwmDuty);
      //printf("\t\tPWM Duty %lu, Set %lu\t\n", PWMDutyCycle, pwmSet);

      nanosleep(&sleepDelay, &timeLeft);
      readLux();
   }
   
   return NULL;
}

int IsWithinInterval(int start, int duration)
{
   int crntTime = RTC_HOUR * 60 + RTC_MIN;

   if ((start <= crntTime) && (start + duration > crntTime))
      return 1;

   if ((start > crntTime) && (start + duration > crntTime + 24 * 60))
      return 2;

   return 0;
}

int IsWithinDateInterval(unsigned char periodStartDay, unsigned char periodStartMonth, unsigned char periodEndDay, unsigned char periodEndMonth)
{
   int reversed = 0;
   if (periodStartMonth > periodEndMonth)
      reversed = 1;
   if ((periodStartMonth == periodEndMonth) && (periodStartDay > periodEndDay))
      reversed = 1;

   if (!reversed)
   {
      if (RTC_MONTH < periodStartMonth)
         return 0;

      if (RTC_MONTH > periodEndMonth)
         return 0;

      if ((RTC_MONTH == periodStartMonth) && (RTC_DOM < periodStartDay))
         return 0;

      if ((RTC_MONTH == periodEndMonth) && (RTC_DOM > periodEndDay))
         return 0;
   }
   else
   {
      if ((RTC_MONTH < periodStartMonth) && (RTC_MONTH > periodEndMonth))
         return 0;

      if ((RTC_MONTH == periodStartMonth) && (RTC_MONTH == periodEndMonth))
      {
         if ((RTC_DOM < periodStartDay) && (RTC_DOM > periodEndDay))
            return 0;
         return 1;
      }

      if ((RTC_MONTH == periodStartMonth) && (RTC_DOM < periodStartDay))
         return 0;

      if ((RTC_MONTH == periodEndMonth) && (RTC_DOM > periodEndDay))
         return 0;
   }

   return 1;
}

int UpdateStandbyMode(DeviceInfoS *pDeviceInfo)
{
   int i;
   if (pDeviceInfo->scheduleType == 0)
      return 0;

   for (i = 0; i < pDeviceInfo->schedulesCount; i++)
   {
      int within = IsWithinInterval(pDeviceInfo->schedules[i].start, pDeviceInfo->schedules[i].duration);
      int withinDate = IsWithinDateInterval(pDeviceInfo->schedules[i].periodStartDay, pDeviceInfo->schedules[i].periodStartMonth,
                                            pDeviceInfo->schedules[i].periodEndDay, pDeviceInfo->schedules[i].periodEndMonth);

      if ((within == 0) || (withinDate == 0))
         continue;

      if (pDeviceInfo->schedules[i].entryType == 0) // every day
         return 0;

      if (pDeviceInfo->schedules[i].entryType == 1) // weekday
      {
         struct tm tmStruct;
         time_t localTime;
         int wd;
         tmStruct.tm_year = RTC_YEAR - 1900;
         tmStruct.tm_mon = RTC_MONTH - 1;
         tmStruct.tm_mday = RTC_DOM;
         tmStruct.tm_hour = RTC_HOUR;
         tmStruct.tm_min = RTC_MIN;
         tmStruct.tm_sec = RTC_SEC;

         localTime = mktime(&tmStruct);
         tmStruct = *localtime(&localTime);
         wd = tmStruct.tm_wday;

         if ((within == 1) && (wd == pDeviceInfo->schedules[i].dow))
            return 0;

         if ((within == 2) && (wd == ((pDeviceInfo->schedules[i].dow + 1) % 7)))
            return 0;
      }

      if (pDeviceInfo->schedules[i].entryType == 2) // year day
      {
         if (within == 1)
         {
            if ((RTC_YEAR == pDeviceInfo->schedules[i].year) &&
                (RTC_MONTH == pDeviceInfo->schedules[i].month) &&
                (RTC_DOM == pDeviceInfo->schedules[i].day))
               return 0;
         }

         if (within == 2)
         {
            int y, m, d;
            y = RTC_YEAR;
            m = RTC_MONTH;
            d = RTC_DOM;

            d--;
            if (d <= 0)
            {
               m--;
               if (m <= 0)
               {
                  m = 12;
                  y--;
               }
               d = DIM[m - 1];
            }

            if ((y == pDeviceInfo->schedules[i].year) &&
                (m == pDeviceInfo->schedules[i].month) &&
                (d == pDeviceInfo->schedules[i].day))
               return 0;
         }
      }
   }

   return 1;
}

//
// void SetupEthernetParams()
//
// Reads the settings and setups the TCP/IP stack.
//
void SetupEthernetParams()
{
   DeviceInfoS deviceInfo;

   //
   // read the settings
   //
   FileRoutines_readDeviceInfo(&deviceInfo);
}

void waitPwmOff(void)
{
   if (getPwmDuty(thePWMHandle) != PWM_PERIOD) // IN the off chance PWM is off skip check
   {
      while (pinctl::inst().get(pwmMonitorFd) == 1)
         ; // Off wait for beginning of next off
      while (pinctl::inst().get(pwmMonitorFd) == 0)
         ; // Wait until its off
   }
}

int maxF = 0;        // 1928 = ~25 lux
int minF = 4096;     // 1765 = ~574 lux
int m_point[2] = {1778, 1989};
int luxPrint = 0;
void readLux(void)
{
   int rawLvl;
   float conv;
   int lux = 0;
   float x1 = 2081.0;
   float x2 = 2940.0;
   float y1 = 632;
   float y2 = 1;
   float m = (y2 - y1 ) / ( x2 - x1 );
   //float m = (y1 - y2 ) / ( x1 - x2 );
   float b = (y1) - ( m * x1 );

   waitPwmOff();

   rawLvl = adc::inst().readVal(4);             // Low number is max LUX  - diode grounds resistor and thus takes voltage down
   if (rawLvl > 0)
   {
      rawLvl = rawLvl + 1050 ;                        // Use offset to adjust result
      //conv = ((float)rawLvl * (3.3 / 4096.0));      // Raw Voltage
      lux = (int)( (m * rawLvl) + b + 100.0);
      //printf("Raw %d\t M val %0.3f\t B %03f\t Result Lux %d\n\r", rawLvl, m, b, lux);

      // if (lux > 2000)   // Cap at full bright
      //    lux = 2000;

      // if (lux < 0)      // Cap at full black
      //    lux = 0;

      
      if( lux < minF )
      {
         m_point[0] = lux;
         minF = lux;
      }

      if( lux > maxF )
      {
         m_point[1] = lux;
         maxF = lux;
      }
      
      if( LuxMeterLPF > lux )  // Down slope
      {
         LuxMeterLPF = (LuxMeterLPF * 0.9) + (lux * 0.1);
      }
      else
      {
         LuxMeterLPF = (LuxMeterLPF * 0.97) + (lux * 0.03);
      }

      luxPrint++;
      if( luxPrint > 25 )
      {
         printf("Raw ADC Read %d\t LPF Lux result = %0.3f\n", rawLvl, LuxMeterLPF);
         // printf("Min found %d\t Max Found %d\t", minF, maxF);
         luxPrint = 0;
      }
   }
}
