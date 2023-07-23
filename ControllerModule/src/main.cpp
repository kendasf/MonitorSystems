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
#include <sys/resource.h>



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
// #define CAMERA_ENABLED

static FILE *fpErrLog = NULL;

volatile int PWMDutyCycle = 10;

bool HaveExternalFlash = false;

int fileDirty = 1;
int CurrentlyDisplayedSpeed = -1;
unsigned int lastSpeedToDisplay = -1;
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

void *readLuxThread( void *argPtr );
char readLuxThreadRunning = 0;
pthread_t readLuxThreadID;
int LuxmeterAvgPos = 0;
float LuxMeterLPF = 100.0;
float lastLuxMeterLPF = 100.0;

int LastRefreshTime = 0;


pwmHandle thePWMHandle = NULL;                                 // Control PWM

void* DoAutoDimming( void *argPtr );
pthread_t autoDimThreadID;
char autoDimThreadRunning = 0;
char disablePWM = 0;

void *readVoltageThread(  void *argPtr );
char readVoltageThreadRunning = 0;
pthread_t readVoltageThreadID;
int voltageLockOut = 0;


unsigned short UDPCallback(unsigned char socket, unsigned char *remip, unsigned short remport, unsigned char *buf, unsigned short len) { return 0; }

//
// timing globals
//
unsigned long long SecondsTimer = 0;
float SupplyVoltageLevel = 12.0f;

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

unsigned long calcPwmDuty(int dutyCyclePercent)                /* For Linux this converts Duty % to ns */
{
   unsigned long calcValue = 0;
   float newValue = 0.0;
   float decimalPct = 0.0;

   if( dutyCyclePercent > 0)
   {
      //printf("Trying %d duty cycle\t\t", dutyCyclePercent);
      // PWM_PERIOD is 100% duty cycle and is in ns
      decimalPct = (float)PWMMAX / 100.0;                      // 0.3
      newValue = decimalPct * (float)PWM_PERIOD;      // 13 888 888 * 0.3 =  4 166 666
      //printf("Max PWM %2.2f\t", newValue);

      decimalPct = (float)dutyCyclePercent / 100.0;            // 50 = 0.5
      newValue = newValue * decimalPct;               // 4 166 666 * 0.5 = 2 083 333
      //printf("Decimal PWM %2.2f\t", newValue);

      calcValue = (unsigned long)(newValue);          // 1 440 000 ns
      //printf("Result PWM %lu ns\n\n", calcValue);
   }
   else
   {
      calcValue = 1;
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
         printf("<7>UDP mesage rx'ed\n");
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
   adc::inst().delete_channel(4);
   adc::inst().delete_channel(2);
   VMSDriver_shutdown();
   sleep(2);
}

// Catching Traps.  Clearing display before Exit
// Also could trap differently for each type of trap
void TrapCtlC(int sigval)
{
   printf("<2>TrapCtlC\nClearing Display\n");
   doShutdown();
   exit(sigval);
}

void TrapKill6(int sigval)
{
   printf("<2>TrapKill6\nClearing Display\n");
   doShutdown();
   exit(sigval);
}

void TrapKill9(int sigval)
{
   printf("<2>TrapKill9\nClearing Display\n");
   doShutdown();
   exit(sigval);
}

void TrapKill15(int sigval)
{
   printf("<2>TrapKill15\nClearing Display\n");
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
   unsigned int speedToDisplay = 0;
   int bStandby = 0;
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
#ifdef CAMERA_ENABLED
   int cameraBusy = 0;
#endif
   int res = 0;
   char *inMsg = NULL;
   char pinSet = 0;

   long loopMax = 0;
   long loopDur = 0;

   // int which = PRIO_PROCESS;
   // id_t pid;
   // int priority = 0;

    
   // pid = getpid();
   // setpriority(which, pid, priority);

   pthread_setname_np( pthread_self() , "Main Thread");

   printf("<4>Starting Main Application\n\n");

#ifdef USE_SHUTDOWN
   int shutdownPinFd;
   int shutdownEnabled = 0;
#endif

   setvbuf(stdout, NULL, _IONBF, 0); 
   
   signal(SIGINT, TrapCtlC);   // Setup signal trap for child exit
   signal(SIGABRT, TrapKill6); // Setup signal trap for child exit
   signal(SIGTERM, TrapKill15);
   signal(SIGKILL, TrapKill9); // Setup signal trap for child exit
   signal(SIGSEGV, TrapKill9); // Setup signal trap for child exit

   //SysCmd("/root/init_peripherals.sh");  // Handle by systemd now

   if (FileRoutines_autoconfFromCard("/root/update.vac") > 0)
   {
      printf("<5>Configuration Updated\r\n");
      unlink("/root/update.vac");
   }

   FileRoutines_readDeviceInfo(&deviceInfo);

   printf(JOURNALD_LEVEL "Setting ADC 4 for Lux Meter\n");
   adc::inst().enable_channel(4); // luxmeter

   printf(JOURNALD_LEVEL "Setting ADC 2 for Vin Monitor\n");
   adc::inst().enable_channel(2); // Vin

   printf(JOURNALD_LEVEL "Setting Pin 47 for external power fet\n");
   camPwrPin = pinctl::inst().export_pin(CAMERA_POWER, 0);
   pinctl::inst().set(camPwrPin, 0);

   initPWMDriver(); /* This sets up the structures to drive the PWM */

   // init PWM
   thePWMHandle = createPWM(0, pwmPeriod, 1);   // Pin is assigned by Device Tree
   setPwmDuty(thePWMHandle, 0ul);
   startPwm(thePWMHandle);
   
  // VMS Driver Init here to clear screen before PWM enabled
   VMSDriver_Initialize();

   CommProt_init();
#define luxThread
#ifdef luxThread
   /**************************  Read LUX A2D ********** */
   pthread_attr_t threadAttrs;
   pthread_attr_init(&threadAttrs);
   int stackSize = (PTHREAD_STACK_MIN + 0x4000);
   pthread_attr_setstacksize(&threadAttrs, stackSize);

   printf("<2>Starting Lux Meter thread\n\n");
   res = pthread_create(&readLuxThreadID, &threadAttrs, readLuxThread, NULL);
   if( res != 0 )
   {
      printf("<2>%d - %s", res, strerror(res));
      return -1;
   }
   pthread_attr_destroy(&threadAttrs);

   res = pthread_setname_np(readLuxThreadID, "Lux Thread");

   sleep(5);  // Allow Lux to saturate ~333 samples
#endif

#define voltmeter
#ifdef voltmeter
   /**************************  Read Voltage A2D ********** */
   pthread_attr_init(&threadAttrs);
   pthread_attr_setstacksize(&threadAttrs, stackSize);

   printf("<2>Starting Voltmeter Thread\n\n");
   res = pthread_create(&readVoltageThreadID, &threadAttrs, readVoltageThread, NULL);
   if( res != 0 )
   {
      printf("<2>%d - %s", res, strerror(res));
      return -1;
   }
   pthread_attr_destroy(&threadAttrs);

   res = pthread_setname_np(readVoltageThreadID, "DMM Thread");
#endif

#define autodim
#ifdef autodim
   /**************************  Autodimming Thread ********** */
   pthread_attr_init(&threadAttrs);
   pthread_attr_setstacksize(&threadAttrs, stackSize);

   printf("<2>Starting Autodimmer thread\n\n");
   res = pthread_create(&autoDimThreadID, &threadAttrs, DoAutoDimming, NULL);
   if( res != 0 )
   {
      printf("<2>%d - %s", res, strerror(res));
      return -1;
   }
   pthread_attr_destroy(&threadAttrs);

   res = pthread_setname_np(autoDimThreadID, "AutoDim Thread");
#endif 

   if (argc > 1)  // May augment update.vac to have camera IP stuff
   {
      CAMERA_IP_ADDR = argv[1];
   }

   if (sem_init(&displaySem, 0, 1) == -1)
   {
      printf("<3>sem_init(&displaySem): %s\n", strerror(errno));
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
      printf("<3>Socket allocation failure\n");
      return -1;
   }

// Config for shutdown ping usage
#ifdef USE_SHUTDOWN
   shutdownPinFd = pinctl::inst().export_pin(SHUTDOWN, 1);
   shutdownEnabled = pinctl::inst().get(shutdownPinFd);
   if (shutdownEnabled)
      printf("<5>Shutdown Monitor Enabled\n");
   else
      printf("<5>Shutdown Monitor not enabled\n");
#endif

   FileRoutines_readDeviceInfo(&deviceInfo);

   

   tcpListenerFd = getTcpListen(SERVER_PORT_NUM, 1);
   
   Directory("/store/System");

   CreateMemoryTask(5801);

   // CreateCameraTask(5800, mainPort);
   Radar_CreateTask(5802, mainPort);

   sched_param param;  
   pthread_attr_init(&threadAttrs);

   param.sched_priority = 1;
   res = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
   //res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
   if( 0 != res )
   {
      strerror(errno);
   }

   //
   // log system start event
   //
   printf("<6>Before add log\n");
   FileRoutines_addLog(LOG_SYSTEM_START, NULL);

   printf("<6>Before read info\n");
   FileRoutines_readDeviceInfo(&deviceInfo);
   fileDirty = 1;

   // loopTime = GetTickCount();

   printf("<6>Before start web services\n");
   http::inst().start_web_services("/root/html", "/store", "/tmp", 80);
   printf("<6>Before program loop\n");

   VMSDriver_RunStartSequence();

   struct timespec delay, timeLeft;

	delay.tv_nsec = (2) * (1000) * (1000);
	delay.tv_sec = 0;

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

      //pthread_yield();

      // loopTime = GetTickCount();

      //
      // update device info
      //
      if (fileDirty)
      {
         fileDirty = 0;
         VMSDriver_Clear(true);
         FileRoutines_readDeviceInfo(&deviceInfo);    /* Do this a lot since this is where our memory is  */
      }
      

      //
      // handle inbound data
      //
#ifdef CAMERA_ENABLED
      inMsg = RecvMessages(hSock, 5); // short wait
      if (inMsg != NULL)
      {
         if (strstr(inMsg, "SNAP") != NULL)
         {
            printf("<6>Camera Ready\n");
            cameraBusy = 0;
         }
         free(inMsg);
         inMsg = NULL;
      }
#endif
      // if TestModeDuration active then speed is ignored
      if (TestModeDuration == 0)
      {
         int speedBefore = speedToDisplay;

         
         speedToDisplay = Radar_DetermineSpeedForDisplay(&deviceInfo);
         //speedToDisplay = Radar_GetLastSpeed();
         //printf("<3> Checking SPeed - %d\n", speedToDisplay);
         
         if (speedToDisplay < 0)
            speedToDisplay = 0;

         if (lastSpeedToDisplay != speedToDisplay)
         {
            int blinkLimit = deviceInfo.blinkLimit;
            int displaySpeed = speedToDisplay;

            lastSpeedToDisplay = speedToDisplay;
            printf("<3>\t\tNew Speed: %d mph\r\n", speedToDisplay);
            // Send Command to Camera
            if (deviceInfo.unitType == 1)                         /* Convert KPH to MPH */ 
            {                                                     /* TODO - this gets truncated due to type */
               int addKph = 0;
               blinkLimit = blinkLimit * 10000 / 16093;           // Get BlinkLimit in MPH
               displaySpeed = speedToDisplay * 16093;             // make sure to display KPH
               if ((displaySpeed % 10000) >= 5000)
                  addKph = 1;
               displaySpeed /= 10000; // make sure to display KPH
               displaySpeed += addKph;
               printf("<6>Speed in KPH: %d\r\n", displaySpeed);
            }

#ifdef CAMERA_ENABLED
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

                  printf("<6>Send Speed: %d %s\n", speedToDisplay, requestSnap);
                  udpSendLocal(hSock, 5800, requestSnap, strlen(requestSnap));

                  char debug[64];
                  sprintf(debug, "snap: %d\n", displaySpeed);
                  FileRoutines_SaveDebugInfo(debug, strlen(debug));
               }
               else // Ask for speed adjust
               {
                  char requestSnap[128];
                  sprintf(requestSnap, "SPEEDADJ,%s,%d", CAMERA_IP_ADDR, displaySpeed);
                  printf("<6>Adjust Speed: %d, %s\n", speedToDisplay, requestSnap);
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
               printf("<6>Adjust Speed: %d, %s\n", speedToDisplay, adjustSpeed);
               udpSendLocal(hSock, 5800, adjustSpeed, strlen(adjustSpeed));
            }
#endif // CAMERA_ENABLEDs
         }

         if ((!bStandby) || DisplayFlashingCorners)
         {  
            DisplaySpeed(speedToDisplay, &deviceInfo);
         }
         else
            VMSDriver_Clear(true);

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
      }
      else
      {
         CurrentlyDisplayedSpeed = 0;
      }

      if ((UpdateLuxmeterCnt > 30) )  // Update auto dim every 30 seconds
      {

         static int lastDutyCycleLevel = -1;
         UpdateLuxmeterCnt = 0;

         // Change only if there is a zero crossing - But why!!!
         if (((lastDutyCycleLevel > 30) && (PWMDutyCycle <= 30)) || 
             ((lastDutyCycleLevel <= 30) && (PWMDutyCycle > 30)) ||
             (lastDutyCycleLevel == -1) ||
             (SetCameraModeCnt > 10 * 60))
         {
            printf("<5>PWM Duty Change: Last = %d Current = %d\n", lastDutyCycleLevel, PWMDutyCycle);
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

         if ((RTC_SEC % 2) == 0)
         {
            // flash status LED to indicate that the software is running properly
            if (TestModeDuration > 0)
            {
               pinSet = ~pinSet;
               pinctl::inst().set(camPwrPin, pinSet);
            }
               
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
      if (tcpSocketFd < 0) // If no active socke then check Listening sock
      {
         int newFd = getTcpActiveSocket(tcpListenerFd);
         if (newFd > 0)
         {
            CommProt_resetChannel(CHANNEL_SERIAL_A); // Init channel info
            tcpSocketFd = newFd;
         }
      }
      else
      {
         int status = ExchangeTcpData(tcpSocketFd);
         if ((status < 0) && (errno == 104))
         {
            printf("<6>Closing tcp Socket\r\n");
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
         printf("<2>Halting vms\n");
         usleep(1000 * 60000);
      }
#endif

   }

   autoDimThreadRunning = 0;
   readLuxThreadRunning = 0;

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


unsigned int toKPH( unsigned int mph )
{
   unsigned int retval = 0;
   unsigned int addKph = 0; 

   retval = mph * 16093;
   if( (retval % 10000) > 5000 )
   {
      addKph = 1;
   }
   retval /= 10000;
   retval += addKph;

   return retval;
}

unsigned int toMPH( unsigned int kph )
{
   unsigned int retval = 0;

   retval = kph * 10000;
   retval = retval / 16093;

   return retval;
}


void frameUpdate()
{
   if ( 1 == VMSDriver_UpdateFrameFast() )
   {
      printf("<7>Refresh Seconds %llu\n\n", SecondsTimer);
      LastRefreshTime = SecondsTimer;
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
   unsigned long long now;

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
      printf("<6>Updating Speed - %d\n", speedToDisplay);
      int forceResetAnim = (CurrentlyDisplayedSpeed == -1);
      CurrentlyDisplayedSpeed = speedToDisplay;

      if (forceResetAnim || (PrevBcIdx != bcIdx) || (pDeviceInfo->bitmapsConfig[bcIdx].speedDisplayMode == 1) || (pDeviceInfo->bitmapsConfig[bcIdx].speedDisplayMode == 2))
      {
         now = GetTickCount();
      CurrentlyDisplayedFrameIdx = 0;
         CurrentlyDisplayedFrameStart = now;
      refreshRequired = 1;
      AnimationFrameIdx = 0;
         AnimationFrameStart = now;
      }

      if (pDeviceInfo->radarProtocol != Protocol_NMEA)
         FileRoutines_addVehicleLog(speedToDisplay);
   }
   else
   {
      now = GetTickCount();
      if (CurrentlyDisplayedFrameIdx == 0)
      {
         if (((now - CurrentlyDisplayedFrameStart) >= pDeviceInfo->blinkOnDurationMs) || (now < CurrentlyDisplayedFrameStart))
         {
            CurrentlyDisplayedFrameIdx = 1;
            if (now < CurrentlyDisplayedFrameStart)
               CurrentlyDisplayedFrameStart = now;
            else
               CurrentlyDisplayedFrameStart = CurrentlyDisplayedFrameStart + pDeviceInfo->blinkOnDurationMs;
            
            refreshRequired = 1;
         }
      }
      else
      {
         if (((now - CurrentlyDisplayedFrameStart) >= pDeviceInfo->blinkOffDurationMs) || (now < CurrentlyDisplayedFrameStart))
      {
               CurrentlyDisplayedFrameIdx = 0;
            if (now < CurrentlyDisplayedFrameStart)
               CurrentlyDisplayedFrameStart = now;
            else
               CurrentlyDisplayedFrameStart = CurrentlyDisplayedFrameStart + pDeviceInfo->blinkOffDurationMs;
            
            refreshRequired = 1;
            }
      }     
      
      if (TicksElapsed(AnimationFrameStart) >= AnimationFrameLengthMs)
      {
         
         if (NumAnimationFrames > 1)
         {
            now = GetTickCount();
            printf("<6>Do animation %llu, %llu, \t\t", now, AnimationFrameStart);  
            printf("<6>Animation Frame idx - %d, %dms\n", AnimationFrameIdx, AnimationFrameLengthMs);
            AnimationFrameIdx = (AnimationFrameIdx + 1) % NumAnimationFrames;             // Odd way to increment frames, but it auto rolls

            if (now < AnimationFrameStart)
               AnimationFrameStart = now;
            else
            {
               if ((now - AnimationFrameStart) > AnimationFrameLengthMs )      // Have I gone tooo long
               {
                  AnimationFrameStart = now;
                  CurrentlyDisplayedFrameStart = now;
         }
               else
                  AnimationFrameStart = AnimationFrameStart + AnimationFrameLengthMs;
            }
            refreshRequired = 1;
            printf("<6>\n");
         }
         
      }
   }

   if (LastRefreshTime + 10 < SecondsTimer)
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
      printf("<7>Do refresh\n\n");
      VMSDriver_Clear(false);  // Empty buffer and clean display

   // draw bitmap
      if( 1 == CurrentlyDisplayedFrameIdx)
   {
         switch ( pDeviceInfo->bitmapsConfig[bcIdx].numFrames )
         {
            case 0:
               // No bitmap
               
            break;

            case 1:
         VMSDriver_RenderBitmap((pDeviceInfo->bitmapsConfig[bcIdx].frames[0]) - 1, NULL);
            break;

            default:
         int imageID = ((pDeviceInfo->bitmapsConfig[bcIdx].frames[AnimationFrameIdx]) - 1);
               printf("<6>\t\t\tLoading image ---- %d\n\n", imageID);
         VMSDriver_RenderBitmap(imageID, NULL);
            break;
      }
      }
      else
      {
      // draw speed
         switch( pDeviceInfo->bitmapsConfig[bcIdx].speedDisplayMode )
               {
            case 1:
            case 2:
            {
               printf("<6>\t\t\tPrinting Speed of %d\n\n", CurrentlyDisplayedSpeed);
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
            break;

            default:
            break;
         }
      }

      if (DisplayFlashingCorners)
            {
         printf("<6>Flashing Corners displayed \n\n");
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

      frameUpdate();

      // Update locals
      PrevBcIdx = bcIdx;

      NumAnimationFrames = pDeviceInfo->bitmapsConfig[bcIdx].numFrames;
      AnimationFrameLengthMs = pDeviceInfo->bitmapsConfig[bcIdx].frameLength;
}
}



//
// int GetLuxmeterValue()
//
// Returns the average luminance.
//
unsigned int GetLuxmeterValue()
{
   // printf("Reading Lux %2.3f\n", LuxMeterLPF);
   return (unsigned int)LuxMeterLPF;
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
   unsigned int luminance = 0;
   char useAutoDim = 1;
   unsigned long pwmSet = 0;
   unsigned long pwmMax = PWMMAX;
   //unsigned long pwmDutyLast  = 0;
   unsigned long pwmDuty = 0;
   unsigned int maxIndex  = (sizeof(deviceInfo.autoDimming) / sizeof(AutoDimmingEntryS)) - 1;

   sched_param param;
   int res;
   pthread_attr_t threadAttrs;
   
   pthread_attr_init(&threadAttrs);

   param.sched_priority = 0;
   res = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
   //res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
   if( 0 != res )
   {
      strerror(errno);
   }

   autoDimThreadRunning = 1;

   sleepDelay.tv_nsec = (125) * (1000) * (1000);                       // 100ms sleep cycle
   sleepDelay.tv_sec = 0;

   nanosleep(&sleepDelay, &timeLeft);

   printf("<3>Absolute PWM max is %lu percent of input voltage\n", calcPwmDuty(100));

   while(1 == autoDimThreadRunning )
   {
      FileRoutines_readDeviceInfo(&deviceInfo);                      // Update our config
      if (1 == deviceInfo.autoDim)                                           // Are we doing auto dim?
      {
         luminance = GetLuxmeterValue();
         PWMDutyCycle = deviceInfo.autoDimming[maxIndex].brightness;

         for (i = 0; i < maxIndex; i++)   // This is a table lookup 
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
         getPwmDuty(thePWMHandle);
         PWMDutyCycle = (deviceInfo.displayBrightness & 0x7F) ;
      }

      // printf("<7>Setting duty cycle to %d\n\n", PWMDutyCycle);
      pwmDuty = calcPwmDuty(PWMDutyCycle);

      setPwmDuty(thePWMHandle, pwmDuty);

      nanosleep(&sleepDelay, &timeLeft);
 
   }

   return 0;
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



int maxF = 0;        // 1928 = ~25 lux
int minF = 4096;     // 1765 = ~574 lux
typedef struct luxSlope {
   float analogV;
   int a2d_Value;
   int  lux;
} luxSlope;

luxSlope m_point[2] = {{1.566, 4096, 2000},{0.07, 0, 0}};
int luxPrint = 0;
void *readLuxThread(  void *argPtr )
{
   int rawLvl;
   float conv;
   int lux = 0;
   float x1 = m_point[0].a2d_Value; // 3559;    // A2D in dark
   float x2 = m_point[1].a2d_Value; //160;      // A2D in bright
   float y1 = 0;                                // Lux in dark
   float y2 = 2000;                             // Lux in bright
   float m = (y2 - y1 ) / ( x2 - x1 );
   //float m = (y1 - y2 ) / ( x1 - x2 );
   float b = (y1) - ( m * x1 );

   struct timespec sleepDelay, timeLeft;
   sched_param param;
   int res;
   pthread_attr_t threadAttrs;
   
   pthread_attr_init(&threadAttrs);

   param.sched_priority = 6;
   //res = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
   res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
   if( 0 != res )
   {
      strerror(errno);
   }

	sleepDelay.tv_nsec = (125) * (1000) * (1000);                       // 1ms sleep cycle
   sleepDelay.tv_sec = 0;

   readLuxThreadRunning = 1;

   nanosleep(&sleepDelay, &timeLeft);

   while (1 == readLuxThreadRunning )
   {
      // waitPwmOff();

      rawLvl = adc::inst().readVal(4);             // Low number is max LUX  - diode grounds resistor and thus takes voltage down
      if (rawLvl > 0)
      {
         rawLvl = rawLvl + 350;
         conv = ((float)rawLvl * (1.8 / 4096.0));      // Raw Voltage

         lux = (int)( (m * rawLvl) + b);
         // printf("Conv %f\t, Raw %d\t M val %0.3f\t B %03f\t Result Lux %d\n\r", conv, rawLvl, m, b, lux);

         // if (lux > 2000)   // Cap at full bright
         //     lux = 2000;

         // if (lux < 0)      // Cap at full black
         //     lux = 0;

         
         if( lux < minF )
         {
            m_point[0].analogV = conv;
            //m_point[0].a2d_Value = rawLvl;
            m_point[0].lux = lux;
            minF = lux;
         }

         if( lux > maxF )
         {
            m_point[1].analogV = conv;
            //m_point[1].a2d_Value = rawLvl;
            m_point[1].lux = lux;
            maxF = lux;
         }
         
         if( LuxMeterLPF > lux )  // Down slope
         {
            LuxMeterLPF = (LuxMeterLPF * 0.99) + (lux * 0.01);
         }
         else
         {
            LuxMeterLPF = (LuxMeterLPF * 0.99) + (lux * 0.01);
         }

         luxPrint++;
         if( luxPrint > 100 )
         {
            printf("<5>Raw ADC Read %d\t LPF Lux result = %0.3f\n", rawLvl, LuxMeterLPF);
            printf("<6>Min found %d\t Max Found %d\n", minF, maxF);
            luxPrint = 0;
         }
      }

      nanosleep(&sleepDelay, &timeLeft);
   }

   return 0;
}

#define voltageSleep 100
void *readVoltageThread(  void *argPtr )
{
   int lowVoltCounter = 0;
   int voltPrint = 0;
   int doPrint = 0;
   unsigned long pwmDuty = 0;
   unsigned long holdDuty = 0;
   struct timespec sleepDelay, timeLeft;
   int adcr = 0;
   float calcV = 0.0;
   sched_param param;
   int res;
   pthread_attr_t threadAttrs;
   
   pthread_attr_init(&threadAttrs);

   param.sched_priority = 2;
   res = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
   //res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
   if( 0 != res )
   {
      strerror(errno);
   }

	sleepDelay.tv_nsec = (voltageSleep) * (1000) * (1000);                       // 1ms sleep cycle
   sleepDelay.tv_sec = 0;

   readVoltageThreadRunning = 1;
   disablePWM = 0;

   nanosleep(&sleepDelay, &timeLeft);

   while (1 == readVoltageThreadRunning )
   {
      adcr = adc::inst().readVal(2);
      calcV = (adcr * (1.8 / 4096) * 8.5) + 0.318; // 0.225 is the measured line loss fudge factor
      
      // r1 = 1K r2 =7.5K VREF = 1.8
      // Vmax In = 15.3V cause 4096 counts  = 1.8V = 8.5 * 1.8 = 15.3V
      if (SupplyVoltageLevel == 0.0)
      {
         SupplyVoltageLevel = calcV;
      }
      else
      {
         // Poor mans LPF - will cause ramp up over about 20 Seconds
         SupplyVoltageLevel = (SupplyVoltageLevel * 0.99) + (calcV * 0.01);
      }


      if (SupplyVoltageLevel <= 8.5f )
      {
         lowVoltCounter++;
      }
      
      if (SupplyVoltageLevel <= 8.5f && lowVoltCounter >= 30 )
      {
         //FileRoutines_addLog(LOG_VOLTAGE_TOO_LOW, NULL);
         if(0 == disablePWM )
         {
            printf("<1>PWM is being disabled due to low voltage detection\n");
            VMSDriver_LockDisplay();
            stopPwm(thePWMHandle);
            
            disablePWM = 1;
         }
      }
      else
      {
         if(1 == disablePWM )
         {
            sleepDelay.tv_nsec = (voltageSleep) * (1000) * (1000);                       // 1ms sleep cycle
            sleepDelay.tv_sec = 1;

            nanosleep(&sleepDelay, &timeLeft);
            printf("<1>PWM is being enabled due to low voltage recovery\n");
            startPwm(thePWMHandle);
            VMSDriver_UnlockDisplay();
            disablePWM = 0;
            lowVoltCounter = 0;

            sleepDelay.tv_nsec = (voltageSleep) * (1000) * (1000);                       // 3ms sleep cycle
            sleepDelay.tv_sec = 0;
         }
      }

      voltPrint++;
      if( voltPrint > 100 )
      {
         printf("<6>A2D = %d -> Calculate Supply Voltage %0.2f \t\t", adcr, calcV);
         printf("<5>Supply Voltage estimate %0.2f\n\n", SupplyVoltageLevel);
         voltPrint = 0;
      }

      nanosleep(&sleepDelay, &timeLeft);
   }

   return 0;
}