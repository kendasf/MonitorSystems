#include <string.h>
#include <sys/times.h>

#include "../Common/inc/spi.h"
#include "../Common/inc/pwm.h"
#include "../Common/inc/pinctl.h"
#include "fonts.h"
#include "main.h"
#include "VMSDriver.h"
#include "FileRoutines.h"
#include "SystemUtilities.h"
#include <semaphore.h>
#include <limits.h>

#include <chrono>
#include <cstdlib>

#define bitmapSize ( (VMS_PANELS + 3) * VMS_WIDTH * VMS_HEIGHT / 8 )		// (36 + 3) * 8 * 10 / 8 = 39 * 8 * 10 / 8 = 39 * 10 = 390
#define displaySize ( VMS_PANELS * VMS_WIDTH * VMS_HEIGHT / 8 )			// 36 * 8 * 10 * 8 = 36 * 10 = 360 


unsigned char VMSBitmap[bitmapSize] = {0};				// = 390 * 8 = 39 panels
unsigned char PrevVMSBitmap[bitmapSize] = {0xF};			

static spi_ptr spi_dev;
static int bufleFd = -1;

extern pwmHandle thePWMHandle;
extern unsigned long pwmDuty;

sem_t displayLock;
sem_t displayUpdate;
char start_seq_running = 0;

unsigned char *spi_buff = NULL;
unsigned int spi_buffLen = 0;
unsigned int trigger_Send = 0;
pthread_t displayThreadID;
char displayThreadRunning = 0;
void *displayThread( void *argPtr );

int LogicalPanelToPhysical(DeviceInfoS *pDeviceInfo, int panelID);

inline void busy_usleep(int usecs) {
	std::chrono::time_point<std::chrono::steady_clock> start_time = std::chrono::steady_clock::now();
	std::chrono::duration<float> duration;
	do {
		duration = std::chrono::steady_clock::now() - start_time;
	} while (duration.count() * 1000000 < usecs);
}

//
// void VMSDriver_Initialize()
//
// Initializes the VMS display interface.
//
void VMSDriver_Initialize()
{
	int res = 0;
	printf("Setup PWM_MONITOR - GPIO 60 - Port 1 Pin 28\n");
	pwmMonitorFd = pinctl::inst().export_pin(PWM_MONITOR, 1);  //60 - This is the GPIO tied to OE or the PWM output

	printf("Setup BUFLE - GPIO 112 - Port 3 Pin 16\n");
	bufleFd = pinctl::inst().export_pin(BUFLE, 0);  // 112
	pinctl::inst().set(bufleFd, 0);

	spi_dev = spi_ptr(new spi("/dev/spidev0.1", 0, 8, SPI_CLK_4M)); 	// Clock idle low, Transmit leading edge 

	sem_init(&displayLock, 0, 1);
	sem_init(&displayUpdate, 0, 1);

	pthread_attr_t threadAttrs;
   pthread_attr_init(&threadAttrs);
   int stackSize = (PTHREAD_STACK_MIN + 0x4000);
   pthread_attr_setstacksize(&threadAttrs, stackSize);

   printf("<2>Starting Display thread\n\n");
   res = pthread_create(&displayThreadID, &threadAttrs, displayThread, NULL);
   if( res != 0 )
   {
      printf("<2>%d - %s", res, strerror(res));
      return;
   }
   pthread_attr_destroy(&threadAttrs);

   res = pthread_setname_np(displayThreadID, "Display Thread");

	pinctl::inst().set(bufleFd, 0);
	VMSDriver_Clear(true);
	pinctl::inst().set(bufleFd, 1);
	pinctl::inst().set(bufleFd, 0);
	pinctl::inst().set(bufleFd, 1);
} 

void VMSDriver_LockDisplay()
{
	sem_wait(&displayLock);
}

void VMSDriver_UnlockDisplay()
{
	sem_post(&displayLock);
}

void VMSDriver_shutdown()
{
	displayThreadRunning = 0;
	pinctl::inst().unexport_pin(PWM_MONITOR);
	pinctl::inst().unexport_pin(BUFLE);
}

//
// void SetDisplayLE()
//
// Latches display output.
//
void SetDisplayLE(void)
{
	// struct timespec delay, timeLeft;

	// delay.tv_nsec = (5) * (1000) * (1000);
	// delay.tv_sec = 0;

	pinctl::inst().set(bufleFd, 1);

	// nanosleep(&delay, &timeLeft);
}

void ClearDisplayLE(void)
{
	pinctl::inst().set(bufleFd, 0);
}

void SpiSendPanel(unsigned char * data, int len)
{
	if (spi_dev)
	{	
		waitPwmOff(thePWMHandle);			// Wait for PWM to go off - then you have 13.477ms to transmit with no PWM noise
		spi_dev->write(data, len);			// 8Mhz - 360bytes =  360us tx time
		SetDisplayLE();
		ClearDisplayLE();
	}
}

void *displayThread( void *argPtr )
{
	sched_param param;
   int res;
   pthread_attr_t threadAttrs;
	struct timespec delay, timeLeft;

	// delay.tv_nsec = (5) * (1000) * (1000);
	// delay.tv_sec = 0;
   
   pthread_attr_init(&threadAttrs);

	param.sched_priority = 5;
   res = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
   //res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
   if( 0 != res )
   {
      strerror(errno);
   }
	displayThreadRunning = 1;

	while( 1 == displayThreadRunning )
	{
		int i, j;
		DeviceInfoS deviceInfo;
		unsigned char spi_stream[sizeof(VMSBitmap)];

		sem_wait(&displayUpdate);
		if( spi_buffLen  > 0)
		{
			FileRoutines_readDeviceInfo(&deviceInfo);	

			for( j = 0; j < VMS_PANELS; j++)
			{
				int panelOffset = LogicalPanelToPhysical(&deviceInfo, j)*(VMS_WIDTH * VMS_HEIGHT / 8);

				memcpy(spi_stream + VMS_HEIGHT * (VMS_PANELS - 1 - j), &spi_buff[panelOffset], VMS_HEIGHT);
			}

			// printf("Sending Map - - - %d\n", sizeof(VMSBitmap));
			// int lastLine = 0;
			// int line = 0;
			// printf("Panel %02d\t", line);
			// for(int i = 0; i < sizeof(VMSBitmap); i++)
			// {
			// 	if(lastLine != line)
			// 	{
			// 		printf("Panel %02d\t", line);
			// 		lastLine = line;
			// 	}
			// 	printf("0x%02X\t", spi_stream[i]);
			// 	if( (0 == i % 10) && (i != 0)  )
			// 	{
			// 		printf("\n");
			// 		line++;
			// 	}
			// }
			// printf("\n- - - - \n\n\n\n");

			SpiSendPanel(spi_stream, VMS_PANELS * VMS_HEIGHT);

			spi_buffLen = 0;
			free(spi_buff);

			sem_post(&displayLock);
		}
	}

	return 0;
}

//
// int LogicalPanelToPhysical(DeviceInfoS *pDeviceInfo, int panelID)
//
// Maps the logical panel (going from left to right, in each row), to physical (the panels connection goes "around"). 
//
int LogicalPanelToPhysical(DeviceInfoS *pDeviceInfo, int panelID)
{
	int cols = 1;
	int row, col;
	int physical;
	
	switch (pDeviceInfo->panelsConfiguration)
	{
	case 0:
	case 1:
		//rows = 1; 
		cols = 2;
		break;

	case 2:
	case 3:
	case 4:
	case 5:
		//rows = 2; 
		cols = 2;
		break;

	case 6:
	case 7:
	case 8:
	case 9:
		//rows = 3;
		cols = 2;
		break;
	case 10:
	case 11:
		//rows = 2;
		cols = 3;
		break;

	case 13:
	case 14:
		//rows = 2;
		cols = 4;
		break;

	case 15:
	case 16:
		//rows = 3;
		cols = 3;
		break;

	case 17:
	case 18:
		//rows = 3;
		cols = 4;
		break;
	case 19:
		//rows = 3;
		cols = 5;
		break;
	case 20:
		//rows = 4;
		cols = 5;
		break;
	case 21:
		//rows = 4;
		cols = 4;
		break;
	}
	
	row = panelID / cols;
	col = panelID % cols;
	
	physical =  row * cols;
	if (row % 2 == 0)
		physical += col;
	else
		physical += cols - col - 1;

	return physical;
}

//
// int VMSDriver_UpdateFrame()
//
// Sends the next frame to the display modules.
//
int VMSDriver_UpdateFrame()
{
	int i, j;
	DeviceInfoS deviceInfo;

	FileRoutines_readDeviceInfo(&deviceInfo);

	sem_wait(&displayLock);
	if (memcmp(PrevVMSBitmap, VMSBitmap, sizeof(VMSBitmap)) == 0)
	{
		sem_post(&displayLock);
		return 0;
	}
	else
	{
		memcpy(PrevVMSBitmap, VMSBitmap, sizeof(VMSBitmap));
	}

	// busy_usleep(PWM_PERIOD * 3 / 2000);

	// Send a panel at a time for all panels	
	for(j = VMS_PANELS - 1; j >= 0; j--)
	{
		int panelOffset = LogicalPanelToPhysical(&deviceInfo, j)*(VMS_WIDTH * VMS_HEIGHT / 8);
		
		// Send data for a panel
		SpiSendPanel(&VMSBitmap[panelOffset], VMS_HEIGHT);
	}

	sem_post(&displayLock);

	return 1;
}

//
// int VMSDriver_UpdateFrameFast()
//
// Sends the next frame to the display modules.
//
int VMSDriver_UpdateFrameFast()
{
	
	sem_wait(&displayLock);
		spi_buffLen = bitmapSize;
		spi_buff = (unsigned char *)malloc(bitmapSize * sizeof(unsigned char));
		memcpy(spi_buff, &VMSBitmap, bitmapSize);
	sem_post(&displayUpdate);
	
	return 1;
}

void VMSDriver_Invalidate()
{
	memset(PrevVMSBitmap, 0xFF, VMS_PANELS * VMS_WIDTH * VMS_HEIGHT / 8);
}

//
// void VMSDriver_Off(int speed)
//
// Turns off the display
//
void VMSDriver_Off()
{
	VMSDriver_Clear(true);
		
	// VMSDriver_UpdateFrameFast();

	// set LE signal
	// SetDisplayLE();

	busy_usleep(10);	
}

void VMSDriver_GetDimensions(int panelsConfig, int& width, int& height) {
#ifdef LARGER_DOTS
	switch (panelsConfig)
	{
	case 0:
	case 1:
		width = 16; 
		height = 12;
		break;

	case 4:
		width = 24; 
		height = 16;
		break;
	case 5:
		width = 16; 
		height = 24;
		break;

	case 8:
		width = 16; 
		height = 36;
		break;
	case 9:
		width = 24; 
		height = 24;
		break;
	case 10:
		width = 36; 
		height = 16;
		break;
	case 11:
		width = 24; 
		height = 24;
		break;

	case 13:
        width = 32;
        height = 24;
        break;
    case 14:
        width = 48;
        height = 16;
        break;
    case 15:
        width = 24;
        height = 36;
        break;
    case 16:
        width = 36;
        height = 24;
        break;
    case 17:
        width = 32;
        height = 36;
        break;
    case 18:
        width = 48;
        height = 24;
        break;
	case 19:
		width = 60;
		height = 24;
		break;
	case 20:
		width = 60;
		height = 32;
		break;
	case 21:
		width = 48;
		height = 32;
		break;
	}
#else	
	switch (panelsConfig)
	{
	case 0:
	case 1:
		width = 16; 
		height = 10;
		break;

	case 4:
		width = 20; 
		height = 16;
		break;
	case 5:
		width = 16; 
		height = 20;
		break;

	case 8:
		width = 16; 
		height = 30;
		break;
	case 9:
		width = 20; 
		height = 24;
		break;
	case 10:
		width = 30; 
		height = 16;
		break;
	case 11:
		width = 24; 
		height = 20;
		break;

	case 13:
        width = 32;
        height = 20;
        break;
    case 14:
        width = 40;
        height = 16;
        break;
    case 15:
        width = 24;
        height = 30;
        break;
    case 16:
        width = 30;
        height = 24;
        break;
    case 17:
        width = 32;
        height = 30;
        break;
    case 18:
        width = 40;
        height = 24;
        break;
	case 19:
		width = 50;
		height = 24;
		break;
	case 20:
		width = 50;
		height = 32;
		break;
	case 21:
		width = 40;
		height = 32;
		break;
	}
#endif
}

//
// void VMSDriver_WriteSpeed(int x, int y, int panelsConfig, int speed, int font)
//
// Writes the speed on the display
//
void VMSDriver_WriteSpeed(int x, int y, int panelsConfig, int speed, int font)
{
	int i;
	int width = 0, height = 0;
	BitmapS bitmap;

	if (speed == 0)
		return;

	VMSDriver_GetDimensions(panelsConfig, width, height);

	memset(bitmap.bitmapData, 0, sizeof(bitmap.bitmapData));

	bool use3digits = false;
	int dig3offset = 0;
	if (speed >= 100) {
		use3digits = true;
		dig3offset = (Digit1_XRange[font][1] - Digit1_XRange[font][0] + 2) / 2;
		switch(font) {
			case 0:
				VMSDriver_WriteSmallChar(x - dig3offset - 1, y, width, height, &bitmap, Font0_Digit1);
			break;
			case 1:
				VMSDriver_WriteChar(x - dig3offset - 1, y, width, height, &bitmap, Font1_Digit1);
			break;
			case 2:
				VMSDriver_WriteChar(x - dig3offset - 3, y, width, height, &bitmap, Font2_Digit1);
			break;
			case 3:
				VMSDriver_WriteSemiLargeChar(x - dig3offset - 1, y, width, height, &bitmap, Font5_Digit1);
			break;
			case 4:
				VMSDriver_WriteLargeChar(x - dig3offset - 3, y, width, height, &bitmap, Font4_Digit1);
			break;
			case 5:
				VMSDriver_WriteXLargeChar(x - dig3offset - 3, y, width, height, &bitmap, Font6_Digit1);
			break;
		}
	}

	// write second and third digits
	for(i = 0; i < 3; i++)
	{
		int digit = speed % 10;
		speed = speed / 10;
		
		if (font == 0)
		{
			int ax = (i == 0) ? 1 : 0;
			switch (digit)
			{
			case 0:
				if ((i == 0) || (use3digits)) // do not display first digit as zero
					VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset , y, width, height, &bitmap, Font0_Digit0);
				break;
			case 1:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit1);
				break;
			case 2:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit2);
				break;
			case 3:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit3);
				break;
			case 4:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit4);
				break;
			case 5:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit5);
				break;
			case 6:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit6);
				break;
			case 7:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit7);
				break;
			case 8:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit8);
				break;
			case 9:
				VMSDriver_WriteSmallChar(x + 5 - i*5 + ax + dig3offset, y, width, height, &bitmap, Font0_Digit9);
				break;
			}
		}
		else if (font == 1)
		{
			switch (digit)
			{
			case 0:
				if ((i == 0) || (use3digits)) // do not display first digit as zero
					VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit0);
				break;
			case 1:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit1);
				break;
			case 2:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit2);
				break;
			case 3:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit3);
				break;
			case 4:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit4);
				break;
			case 5:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit5);
				break;
			case 6:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit6);
				break;
			case 7:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit7);
				break;
			case 8:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit8);
				break;
			case 9:
				VMSDriver_WriteChar(x + 8 - i*8 + dig3offset, y, width, height, &bitmap, Font1_Digit9);
				break;
			}
		} 
		else if (font == 2)
		{
			int ax = (i == 0) ? 1 : 0;
			switch (digit)
			{
			case 0:
				if ((i == 0) || (use3digits))// do not display first digit as zero
					VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit0);
				break;
			case 1:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit1);
				break;
			case 2:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit2);
				break;
			case 3:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit3);
				break;
			case 4:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit4);
				break;
			case 5:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit5);
				break;
			case 6:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit6);
				break;
			case 7:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit7);
				break;
			case 8:
				VMSDriver_WriteChar(x + 8 + ax - i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit8);
				break;
			case 9:
				VMSDriver_WriteChar(x + 8 + ax- i*8 + dig3offset, y, width, height, &bitmap, Font2_Digit9);
				break;
			}
		}
		else if (font == 3)
		{
			int ax = (i == 0) ? 1 : 0;
			switch (digit)
			{
			case 0:
				if ((i == 0) || (use3digits)) // do not display first digit as zero
					VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit0);
				break;
			case 1:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit1);
				break;
			case 2:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit2);
				break;
			case 3:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit3);
				break;
			case 4:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit4);
				break;
			case 5:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit5);
				break;
			case 6:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit6);
				break;
			case 7:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit7);
				break;
			case 8:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit8);
				break;
			case 9:
				VMSDriver_WriteSemiLargeChar(x + 8 - i*8 + ax + dig3offset, y, width, height, &bitmap, Font5_Digit9);
				break;
			}			
		}
		else if (font == 4)
		{
			int ax = (i == 0) ? 1 : 0;
			switch (digit)
			{
			case 0:
				if ((i == 0) || (use3digits)) // do not display first digit as zero
					VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit0);
				break;
			case 1:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit1);
				break;
			case 2:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit2);
				break;
			case 3:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit3);
				break;
			case 4:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit4);
				break;
			case 5:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit5);
				break;
			case 6:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit6);
				break;
			case 7:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit7);
				break;
			case 8:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit8);
				break;
			case 9:
				VMSDriver_WriteLargeChar(x + 12 - i*12 + ax + dig3offset, y, width, height, &bitmap, Font4_Digit9);
				break;
			}			
		}
		else if (font == 5)
		{
			int ax = (i == 0) ? 1 : 0;
			switch (digit)
			{
			case 0:
				if ((i == 0) || (use3digits)) // do not display first digit as zero
					VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit0);
				break;
			case 1:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit1);
				break;
			case 2:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit2);
				break;
			case 3:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit3);
				break;
			case 4:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit4);
				break;
			case 5:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit5);
				break;
			case 6:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit6);
				break;
			case 7:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit7);
				break;
			case 8:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit8);
				break;
			case 9:
				VMSDriver_WriteXLargeChar(x + 16 - i*16 + ax + dig3offset, y, width, height, &bitmap, Font6_Digit9);
				break;
			}			
		}
	}

	VMSDriver_RenderBitmap(0, &bitmap);
}

//
// void VMSDriver_SetPixel(int x, int y)
//
// Turns pixel at given position ON.
//
void VMSDriver_SetPixel(unsigned int x, unsigned int y)
{
	int panelOffset = (x / VMS_WIDTH) * (VMS_WIDTH * VMS_HEIGHT / 8);								// x / 8 * (8 * 10 / 8) = x / 8 * 10	, x can be 0 -> 720
	if ((x < 0) || (x >= VMS_WIDTH * VMS_PANELS) || (y < 0) || (y >= VMS_HEIGHT))
		return;

	if ((VMS_HEIGHT - y - 1 + panelOffset) >= (displaySize))		// 10 - y - 1 + ()
		return;

	VMSBitmap[VMS_HEIGHT - y - 1 + panelOffset] |= (1 << (x % VMS_WIDTH));
}

//
// void VMSDriver_ClearPixel(int x, int y)
//
// Turns pixel off at given position ON.
//
void VMSDriver_ClearPixel(unsigned int x, unsigned int y)
{
	int panelOffset = (x / VMS_WIDTH) * (VMS_WIDTH * VMS_HEIGHT / 8);
	if ((x < 0) || (x >= VMS_WIDTH * VMS_PANELS) || (y < 0) || (y >= VMS_HEIGHT))
		return;

	VMSBitmap[VMS_HEIGHT - y - 1 + panelOffset] &= ~(1 << (x % VMS_WIDTH));
}

//
// void VMSDriver_Clear()
//
// Clears the display.
//
void VMSDriver_Clear(bool doUpdate)
{
	memset(VMSBitmap, 0, displaySize);

	if(true == doUpdate)
	{

		SpiSendPanel(VMSBitmap, displaySize );

		// SetDisplayLE(); /* Load into latch */ 
	}
}

//
// void VMSDriver_Test()
//
// VMS test sequence state machine.
//
int VMSDriver_Test()
{
	VMSDriver_Clear(false);

	return 1;
}

//
// void VMSDriver_TestMode(int imageID)
//
// Display selected test mode bitmap.
//
void VMSDriver_TestMode(int imageID)
{
	if (imageID == 0)
	{
		memset(VMSBitmap, 0xFF, displaySize);
	}
	else
	{
		VMSDriver_Clear(false);
		VMSDriver_RenderBitmap(imageID - 1, NULL);
	}

	VMSDriver_UpdateFrameFast();
}

//
// void VMSDriver_WriteChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, unsigned char Digit[10][8])
//
// Displays the given character.
//	   Note: xPos is horizontal offset in pixels for the Digit pattern
//           yPos is vertical offset in pixels for the Digit pattern
//
void VMSDriver_WriteChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[10][8])
{
	int x, y;
	for(x = 0; x < 8; x++)
	{
		for(y = 0; y < 10; y++)
		{
			if ((Digit[y][x]) && (yPos + y < height) && (xPos + x < width))
			{
				int pixelPos = xPos + x + (yPos + y)* MAX_BMP_WIDTH;
				pBitmap->bitmapData[pixelPos / 8] |= (1 << (pixelPos % 8));
			}
		}
	}
}


//
// void VMSDriver_WriteXLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, unsigned char Digit[24][16])
//
// Displays the given character.
//	   Note: xPos is horizontal offset in pixels for the Digit pattern
//           yPos is vertical offset in pixels for the Digit pattern
//
void VMSDriver_WriteXLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[24][16])
{
	int x, y;
	for(x = 0; x < 16; x++)
	{
		for(y = 0; y < 24; y++)
		{
			if ((Digit[y][x]) && (yPos + y < height) && (xPos + x < width))
			{
				int pixelPos = xPos + x + (yPos + y)* MAX_BMP_WIDTH;
				pBitmap->bitmapData[pixelPos / 8] |= (1 << (pixelPos % 8));
			}
		}
	}
}

//
// void VMSDriver_WriteLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, unsigned char Digit[17][12])
//
// Displays the given character.
//	   Note: xPos is horizontal offset in pixels for the Digit pattern
//           yPos is vertical offset in pixels for the Digit pattern
//
void VMSDriver_WriteLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[18][12])
{
	int x, y;
	for(x = 0; x < 12; x++)
	{
		for(y = 0; y < 18; y++)
		{
			if ((Digit[y][x]) && (yPos + y < height) && (xPos + x < width))
			{
				int pixelPos = xPos + x + (yPos + y)* MAX_BMP_WIDTH;
				pBitmap->bitmapData[pixelPos / 8] |= (1 << (pixelPos % 8));
			}
		}
	}
}

void VMSDriver_WriteSemiLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[13][8])
{
	int x, y;
	for(x = 0; x < 8; x++)
	{
		for(y = 0; y < 13; y++)
		{
			if ((Digit[y][x]) && (yPos + y < height) && (xPos + x < width))
			{
				int pixelPos = xPos + x + (yPos + y)* MAX_BMP_WIDTH;
				pBitmap->bitmapData[pixelPos / 8] |= (1 << (pixelPos % 8));
			}
		}
	}
}

//
// void VMSDriver_WriteSmallChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, unsigned char Digit[7][5])
//
// Displays the given character.
//	   Note: xPos is horizontal offset in pixels for the Digit pattern
//           yPos is vertical offset in pixels for the Digit pattern
//
void VMSDriver_WriteSmallChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[7][5])
{
	int x, y;
	for(x = 0; x < 5; x++)
	{
		for(y = 0; y < 7; y++)
		{
			if ((Digit[y][x]) && (yPos + y < height) && (xPos + x < width))
			{
				int pixelPos = xPos + x + (yPos + y)* MAX_BMP_WIDTH;
				pBitmap->bitmapData[pixelPos / 8] |= (1 << (pixelPos % 8));
			}
		}
	}
}



//
// void VMSDriver_RenderBitmap(int imageID, BitmapS *pBitmap)
//
// Renders the selected bitmap to the VMS display buffer.
//
void VMSDriver_RenderBitmap(int imageID, BitmapS *pBitmap, int color)
{
	BitmapS bitmap;
	DeviceInfoS deviceInfo;

	if ((imageID < 0) || (imageID >= MAX_BITMAPS))
		return;

	if (pBitmap == NULL)
	{
		FileRoutines_readBitmap(imageID, &bitmap);
		pBitmap = &bitmap;
	}

	FileRoutines_readDeviceInfo(&deviceInfo);	

#ifdef LARGER_DOTS
	switch (deviceInfo.panelsConfiguration)
	{
	case 0:
		break;
	case 1:
		VMSDriver_RenderBitmapToPanels(16, 12, 0, 1, 2, pBitmap, color);
		break;

	case 2:
		VMSDriver_RenderBitmapToPanels(24, 8, 2, 1, 2, pBitmap, color); 
		break;
	case 3:
		VMSDriver_RenderBitmapToPanels(16, 12, 2, 1, 2, pBitmap, color);
		break;
	case 4:
		VMSDriver_RenderBitmapToPanels(24, 16, 0, 2, 2, pBitmap, color);
		break;
	case 5:
		VMSDriver_RenderBitmapToPanels(16, 24, 0, 2, 2, pBitmap, color);
		break;

	case 6:
		VMSDriver_RenderBitmapToPanels(24, 16, 2, 2, 2, pBitmap, color);
		break;
	case 7:
		VMSDriver_RenderBitmapToPanels(16, 24, 2, 2, 2, pBitmap, color);
		break;
	case 8:
		VMSDriver_RenderBitmapToPanels(16, 36, 0, 3, 2, pBitmap, color);
		break;
	case 9:
		VMSDriver_RenderBitmapToPanels(24, 24, 0, 3, 2, pBitmap, color);
		break;
	case 10:
		VMSDriver_RenderBitmapToPanels(36, 16, 0, 2, 3, pBitmap, color);
		break;
	case 11:
		VMSDriver_RenderBitmapToPanels(24, 24, 0, 2, 3, pBitmap, color);
		break;

	case 13:
		VMSDriver_RenderBitmapToPanels(32, 24, 0, 2, 4, pBitmap, color);
		break;
	case 14:
		VMSDriver_RenderBitmapToPanels(48, 16, 0, 2, 4, pBitmap, color);
		break;
	case 15:
		VMSDriver_RenderBitmapToPanels(24, 36, 0, 3, 3, pBitmap, color);
		break;
	case 16:
		VMSDriver_RenderBitmapToPanels(36, 24, 0, 3, 3, pBitmap, color);
		break;
	case 17:
		VMSDriver_RenderBitmapToPanels(32, 36, 0, 3, 4, pBitmap, color);
		break;
	case 18:
		VMSDriver_RenderBitmapToPanels(48, 24, 0, 3, 4, pBitmap, color);
		break;
	case 19:
		VMSDriver_RenderBitmapToPanels(60, 24, 0, 3, 5, pBitmap, color);
		break;
	case 20:
		VMSDriver_RenderBitmapToPanels(60, 32, 0, 4, 5, pBitmap, color);
		break;
	case 21:
		VMSDriver_RenderBitmapToPanels(48, 32, 0, 4, 4, pBitmap, color);
		break;
	}
#else
	switch (deviceInfo.panelsConfiguration)
	{
	case 0:
		break;
	case 1:
		VMSDriver_RenderBitmapToPanels(16, 10, 0, 1, 2, pBitmap, color);
		break;

	case 2:
		VMSDriver_RenderBitmapToPanels(20, 8, 2, 1, 2, pBitmap, color); 
		break;
	case 3:
		VMSDriver_RenderBitmapToPanels(16, 10, 2, 1, 2, pBitmap, color);
		break;
	case 4:
		VMSDriver_RenderBitmapToPanels(20, 16, 0, 2, 2, pBitmap, color);
		break;
	case 5:
		VMSDriver_RenderBitmapToPanels(16, 20, 0, 2, 2, pBitmap, color);
		break;

	case 6:
		VMSDriver_RenderBitmapToPanels(20, 16, 2, 2, 2, pBitmap, color);
		break;
	case 7:
		VMSDriver_RenderBitmapToPanels(16, 20, 2, 2, 2, pBitmap, color);
		break;
	case 8:
		VMSDriver_RenderBitmapToPanels(16, 30, 0, 3, 2, pBitmap, color);
		break;
	case 9:
		VMSDriver_RenderBitmapToPanels(20, 24, 0, 3, 2, pBitmap, color);
		break;
	case 10:
		VMSDriver_RenderBitmapToPanels(30, 16, 0, 2, 3, pBitmap, color);
		break;
	case 11:
		VMSDriver_RenderBitmapToPanels(24, 20, 0, 2, 3, pBitmap, color);
		break;

	case 13:
		VMSDriver_RenderBitmapToPanels(32, 20, 0, 2, 4, pBitmap, color);
		break;
	case 14:
		VMSDriver_RenderBitmapToPanels(40, 16, 0, 2, 4, pBitmap, color);
		break;
	case 15:
		VMSDriver_RenderBitmapToPanels(24, 30, 0, 3, 3, pBitmap, color);
		break;
	case 16:
		VMSDriver_RenderBitmapToPanels(30, 24, 0, 3, 3, pBitmap, color);
		break;
	case 17:
		VMSDriver_RenderBitmapToPanels(32, 30, 0, 3, 4, pBitmap, color);
		break;
	case 18:
		VMSDriver_RenderBitmapToPanels(40, 24, 0, 3, 4, pBitmap, color);
		break;
	case 19:
		VMSDriver_RenderBitmapToPanels(50, 24, 0, 3, 5, pBitmap, color);
		break;
	case 20:
		VMSDriver_RenderBitmapToPanels(50, 32, 0, 4, 5, pBitmap, color);
		break;
	case 21:
		VMSDriver_RenderBitmapToPanels(40, 32, 0, 4, 4, pBitmap, color);
		break;
	}
#endif	
}

//
// void VMSDriver_RenderBitmapFile(char* bMapFileName)
//
// Renders the selected bitmap to the VMS display buffer.
//
void VMSDriver_RenderBitmapFile(char* bMapFileName)
{
	BitmapS bitmap;
	BitmapS* pBitmap = &bitmap;
	DeviceInfoS deviceInfo;

	if (bMapFileName == NULL)
	{
		printf("No Bitmap File Specified\r\n");
		return;
	}

	FileRoutines_readBitmapFile(bMapFileName, &bitmap);

	FileRoutines_readDeviceInfo(&deviceInfo);	

#ifdef LARGER_DOTS
	switch (deviceInfo.panelsConfiguration)
	{
	case 0:
		break;
	case 1:
		VMSDriver_RenderBitmapToPanels(16, 12, 0, 1, 2, pBitmap);
		break;

	case 2:
		VMSDriver_RenderBitmapToPanels(24, 8, 2, 1, 2, pBitmap); 
		break;
	case 3:
		VMSDriver_RenderBitmapToPanels(16, 12, 2, 1, 2, pBitmap);
		break;
	case 4:
		VMSDriver_RenderBitmapToPanels(24, 16, 0, 2, 2, pBitmap);
		break;
	case 5:
		VMSDriver_RenderBitmapToPanels(16, 24, 0, 2, 2, pBitmap);
		break;

	case 6:
		VMSDriver_RenderBitmapToPanels(24, 16, 2, 2, 2, pBitmap);
		break;
	case 7:
		VMSDriver_RenderBitmapToPanels(16, 24, 2, 2, 2, pBitmap);
		break;
	case 8:
		VMSDriver_RenderBitmapToPanels(16, 36, 0, 3, 2, pBitmap);
		break;
	case 9:
		VMSDriver_RenderBitmapToPanels(24, 24, 0, 3, 2, pBitmap);
		break;
	case 10:
		VMSDriver_RenderBitmapToPanels(36, 16, 0, 2, 3, pBitmap);
		break;
	case 11:
		VMSDriver_RenderBitmapToPanels(24, 24, 0, 2, 3, pBitmap);
		break;

	case 13:
		VMSDriver_RenderBitmapToPanels(32, 24, 0, 2, 4, pBitmap);
		break;
	case 14:
		VMSDriver_RenderBitmapToPanels(48, 16, 0, 2, 4, pBitmap);
		break;
	case 15:
		VMSDriver_RenderBitmapToPanels(24, 36, 0, 3, 3, pBitmap);
		break;
	case 16:
		VMSDriver_RenderBitmapToPanels(36, 24, 0, 3, 3, pBitmap);
		break;
	case 17:
		VMSDriver_RenderBitmapToPanels(32, 36, 0, 3, 4, pBitmap);
		break;
	case 18:
		VMSDriver_RenderBitmapToPanels(48, 24, 0, 3, 4, pBitmap);
		break;
	case 19:
		VMSDriver_RenderBitmapToPanels(60, 24, 0, 3, 5, pBitmap);
		break;
	case 20:
		VMSDriver_RenderBitmapToPanels(60, 32, 0, 4, 5, pBitmap);
		break;
	case 21:
		VMSDriver_RenderBitmapToPanels(48, 32, 0, 4, 4, pBitmap);
		break;
	}
#else
	switch (deviceInfo.panelsConfiguration)
	{
	case 0:
		break;
	case 1:
		VMSDriver_RenderBitmapToPanels(16, 10, 0, 1, 2, pBitmap);
		break;

	case 2:
		VMSDriver_RenderBitmapToPanels(20, 8, 2, 1, 2, pBitmap); 
		break;
	case 3:
		VMSDriver_RenderBitmapToPanels(16, 10, 2, 1, 2, pBitmap);
		break;
	case 4:
		VMSDriver_RenderBitmapToPanels(20, 16, 0, 2, 2, pBitmap);
		break;
	case 5:
		VMSDriver_RenderBitmapToPanels(16, 20, 0, 2, 2, pBitmap);
		break;

	case 6:
		VMSDriver_RenderBitmapToPanels(20, 16, 2, 2, 2, pBitmap);
		break;
	case 7:
		VMSDriver_RenderBitmapToPanels(16, 20, 2, 2, 2, pBitmap);
		break;
	case 8:
		VMSDriver_RenderBitmapToPanels(16, 30, 0, 3, 2, pBitmap);
		break;
	case 9:
		VMSDriver_RenderBitmapToPanels(20, 24, 0, 3, 2, pBitmap);
		break;
	case 10:
		VMSDriver_RenderBitmapToPanels(30, 16, 0, 2, 3, pBitmap);
		break;
	case 11:
		VMSDriver_RenderBitmapToPanels(24, 20, 0, 2, 3, pBitmap);
		break;

	case 13:
		VMSDriver_RenderBitmapToPanels(32, 20, 0, 2, 4, pBitmap);
		break;
	case 14:
		VMSDriver_RenderBitmapToPanels(40, 16, 0, 2, 4, pBitmap);
		break;
	case 15:
		VMSDriver_RenderBitmapToPanels(24, 30, 0, 3, 3, pBitmap);
		break;
	case 16:
		VMSDriver_RenderBitmapToPanels(30, 24, 0, 3, 3, pBitmap);
		break;
	case 17:
		VMSDriver_RenderBitmapToPanels(32, 30, 0, 3, 4, pBitmap);
		break;
	case 18:
		VMSDriver_RenderBitmapToPanels(40, 24, 0, 3, 4, pBitmap);
		break;
	case 19:
		VMSDriver_RenderBitmapToPanels(50, 24, 0, 3, 5, pBitmap);
		break;
	case 20:
		VMSDriver_RenderBitmapToPanels(50, 32, 0, 4, 5, pBitmap);
		break;
	case 21:
		VMSDriver_RenderBitmapToPanels(40, 32, 0, 4, 4, pBitmap);
		break;
	}
#endif	
}

//
// void VMSDriver_RenderBitmapToPanels(int bmpWidth, int bmpHeight, int firstPanel, int panelsRows, int panelsCols, BitmapS *pBitmap)
//
// Renders the bitmap to panels; takes into account panel orientation and configuration.
//
void VMSDriver_RenderBitmapToPanels(int bmpWidth, int bmpHeight, int firstPanel, int panelsRows, int panelsCols, BitmapS *pBitmap, int color)
{
	int x, y;
	int bLandscape = 0;
	int panelWidth = VMS_WIDTH;
	int panelHeight = VMS_HEIGHT;

	if ((bmpWidth / panelsCols) == VMS_HEIGHT)
	{
		bLandscape = 1;
		panelWidth = VMS_HEIGHT;
		panelHeight = VMS_WIDTH;
	}

	//memset(VMSBitmap, 0, sizeof(VMSBitmap));
	for(y = 0; y < bmpHeight; y++)
	{
		for(x = 0; x < bmpWidth; x++)
		{
			int bmpPixelPos = y * MAX_BMP_WIDTH + x;
			if (pBitmap->bitmapData[bmpPixelPos >> 3] & (1 << (bmpPixelPos & 0x07)))
			{
				int xPos = x % panelWidth;
				int yPos = y % panelHeight;
				int logicalPanelOffset = panelsCols * (y / panelHeight) + (x / panelWidth);	// 3 * (24 / 8) + (30 / 10) = 3 * 4 * 3 = 36
				int panelOffset = (firstPanel + logicalPanelOffset)* VMS_WIDTH;
				

				if (bLandscape) {
					if (color)
						VMSDriver_SetPixel(panelHeight - yPos - 1 + panelOffset, xPos);
					else
						VMSDriver_ClearPixel(panelHeight - yPos - 1 + panelOffset, xPos);
				}
				else {
					if (color)
						VMSDriver_SetPixel(xPos + panelOffset, yPos);
					else
						VMSDriver_ClearPixel(xPos + panelOffset, yPos);
				}
			}				
		}
	}
}

void RunChristmasSequence() {
	VMSDriver_Clear(false);
	BitmapS bmp;
	memset(bmp.bitmapData, 0, sizeof(bmp.bitmapData));

	int seq = 0;
	int pos = 0;

	while(true) {
		busy_usleep(75000);
		VMSDriver_Clear(false);

		for(int y = 15; y > 0 ; y--) {
			for(int x = 0; x < 20; x++) {
				int bmpPixelPos = y * MAX_BMP_WIDTH + x;
				if (bmp.bitmapData[(bmpPixelPos - MAX_BMP_WIDTH) >> 3] & (1 << ((bmpPixelPos - MAX_BMP_WIDTH) & 0x07)))
					bmp.bitmapData[(bmpPixelPos) >> 3] |= (1 << ((bmpPixelPos) & 0x07));
				else
					bmp.bitmapData[(bmpPixelPos) >> 3] &= ~(1 << ((bmpPixelPos) & 0x07));
			}
		}

		bmp.bitmapData[0] = 0;
		bmp.bitmapData[1] = 0;
		bmp.bitmapData[2] = 0;

		switch(seq) {
			case 0:
				pos = rand() % 16 + 2;
				break;
			case 1:
			case 4:
			case 7:
				bmp.bitmapData[pos >> 3] |= (1 << (pos & 0x07));
				break;
			case 2:
			case 6:
				bmp.bitmapData[pos >> 3] |= (1 << (pos & 0x07));
				bmp.bitmapData[(pos-2) >> 3] |= (1 << ((pos-2) & 0x07));
				bmp.bitmapData[(pos+2) >> 3] |= (1 << ((pos+2) & 0x07));
				break;
			case 3:
			case 5:
				bmp.bitmapData[pos >> 3] |= (1 << (pos & 0x07));
				bmp.bitmapData[(pos-1) >> 3] |= (1 << ((pos-1) & 0x07));
				bmp.bitmapData[(pos+1) >> 3] |= (1 << ((pos+1) & 0x07));
				break;

		}

		seq = seq + 1;
		if (seq > 8 + (rand() % 3))
			seq = 0;

		VMSDriver_RenderBitmapToPanels(20, 16, 0, 2, 2, &bmp, 1);
		VMSDriver_UpdateFrameFast();
		//SetDisplayLE();
	}

}

//
// void VMSDriver_RunStartSequence()
//
// Runs the test sequence at startup.
//
void VMSDriver_RunStartSequence()
{
	int i, j, k;
	struct timespec delay, timeLeft;
	DeviceInfoS deviceInfo;

	start_seq_running = 1;

	delay.tv_sec = 0;
	delay.tv_nsec	= (66) * (1000) * (1000); 

	VMSDriver_Clear(true);

	for(i = 0; i < VMS_WIDTH + VMS_HEIGHT; i++)
	{
		if (i < (VMS_WIDTH + VMS_HEIGHT - 1))
		{
			for(j = 0; j < VMS_PANELS; j++)
			{
				if (i < VMS_HEIGHT)
				{
					for(k = 0; k <= i; k++)
					{
						VMSDriver_SetPixel(k + j * VMS_WIDTH, i-k);
					}
				}
				else
				{
					for(k = (i - VMS_HEIGHT) + 1; k < VMS_WIDTH; k++)
					{
						VMSDriver_SetPixel(k + j * VMS_WIDTH, VMS_HEIGHT - k + (i - VMS_HEIGHT));
					}
				}
			}
		}

		VMSDriver_UpdateFrameFast();

		nanosleep(&delay, &timeLeft);

	}

	// for(i = 0; i < VMS_PANELS; i++ )			
	// {
	// 	int offset = (i * (VMS_HEIGHT * VMS_WIDTH) );
	// 	printf("<7> Panel %d\n", i);
	// 	for(j = 0; j < VMS_HEIGHT; j++)
	// 	{
	// 		printf("<7>Column %d\n", j);
	// 		for(k = 0; k < VMS_WIDTH; k++)
	// 		{
	// 			//printf("<7>Row %d\t", k);
	// 			VMSDriver_SetPixel(k + offset, j);

	// 			VMSDriver_UpdateFrameFast();

	// 			nanosleep(&delay, &timeLeft);
	// 		}
	// 	}
	// }

	

	//RunChristmasSequence();
}
