#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "main.h"
#include "../Common/inc/uart.h"
#include "CommProt.h"
#include "FileRoutines.h"
#include "SystemUtilities.h"

//
// Default device settings
//
DeviceInfoS DefaultDeviceInfo = 
{
	0x00000000,
	"User2",
	"User1",
	"Admin",
	"VMSController",
	57600,
	3,
	Protocol_NMEA,
	2,					/* minDisplaySpeed - normally 10*/
	35,					/* blinkLimit */ 
	0,					/* blinkLimitFromRotary */
	80,					/* maxDisplaySpeed */ 
	0,					/* maxSpeedFromRotary */
	850,				/* blinkOnDurationMs */
	850,				/* blinkOffDurationMs */ 
	1000,
	1,
	2000,
	227,
	1,
	{
		{1, 5}, {2, 7}, {4, 10}, {7, 15}, {10, 20}, {15, 35}, {20, 50}, {30, 125},
		{40, 200}, {65, 350}, {70, 500}, {75, 750}, {80, 1000}, {90, 1500}, {100, 2000}, {100, 2000}
		
	},
	{{0}, {1, 0, 0, 1, 0, 0, 0}, {2, 0, 0, 1, 0, 0, 0}, {0}},
	0, 0, {0},
	0,				// useDHCP
	0x6400A8C0,		// ipAddress	 192.168.0.100
	0x00FFFFFF,		// subnetMask	  255.255.255.0
	0x0100A8C0,		// gatewayAddress 192.168.0.1
	0				// km/h
};

DeviceInfoS DeviceInfo;
BitmapS Bitmap;

//
// extern globals
//
extern float SupplyVoltageLevel;
extern int MemoryCardStatus;

//
// void FileRoutines_init()
//
// Initializes the module.
//
void FileRoutines_init()
{
	int i;
	BitmapS bitmap;

	//
	// check opcode
	//
	unsigned int *pOpcode = (unsigned int*)OP_CODE_ADDR;

	if (*pOpcode == OP_CODE)
	{
		return;
	}

	printf("FileRoutines:: write default settings\n");

	FileRoutines_writeDeviceInfo(&DefaultDeviceInfo);

	memset(bitmap.bitmapData, 0, sizeof(bitmap.bitmapData));

	for(i = 0; i < MAX_BITMAPS; i++)
	{
		FileRoutines_writeBitmap(i, &bitmap);
	}
}

//
// void FileRoutines_readDeviceInfo(DeviceInfoS* pDeviceInfo)
//
// Reads device info.
//
void FileRoutines_readDeviceInfo(DeviceInfoS* pDeviceInfo)
{
	int fd = open("/store/Data/DeviceInfoS.dat", O_RDONLY);
	if (fd >= 0)
	{
		read(fd, pDeviceInfo, sizeof(DeviceInfoS)); 
		close(fd);
	}
	else
	{
		memcpy(pDeviceInfo, &DefaultDeviceInfo, sizeof(DeviceInfoS));
	}
}

//
// void FileRoutines_writeDeviceInfo(DeviceInfoS* pDeviceInfo)
//
// Writes device info.
//
void FileRoutines_writeDeviceInfo(DeviceInfoS* pDeviceInfo)
{
	int i;
	int fd = open("/store/Data/DeviceInfoS.dat", O_WRONLY | O_CREAT, 0666);
	if (fd < 0)
	{
		printf("/store/Data/DeviceInfoS.dat ERR-0: %d - %s\r\n", errno, strerror(errno));
	}
	else
	{
		unsigned char *pData = (unsigned char*)pDeviceInfo;
		write(fd, pDeviceInfo, sizeof(DeviceInfoS));
		close(fd);
		Sync();
	}
}

//
// void FileRoutines_readBitmap(int idx, BitmapS* pBitmap)
//
// Reads bitmap from Flash.
//
void FileRoutines_readBitmap(int idx, BitmapS* pBitmap)
{
	FILE* fd;
	char bmapFname[25];
	sprintf(bmapFname, "/store/Data/bitmap%d.dat", idx);
	fd = fopen(bmapFname, "r+");
	if (fd != NULL)
	{
		fread(pBitmap, sizeof(BitmapS), 1, fd); 
		fclose(fd);
	}
	else
	{
		memset(pBitmap, 0, sizeof(BitmapS));
	}
}

//
// void FileRoutines_readBitmapFile(char* fname, BitmapS* pBitmap)
//
// Reads bitmap from Flash.
//
void FileRoutines_readBitmapFile(char* bmapFname, BitmapS* pBitmap)
{
	FILE* fd;
	fd = fopen(bmapFname, "r+");
	if (fd != NULL)
	{
		int size = fread(pBitmap, 1, sizeof(BitmapS), fd);
		printf("Size: %d\r\n", size);
		if (size < 0)
			printf("ERROR on %s: %s\r\n", bmapFname, strerror(errno));
		fclose(fd);
	}
	else
	{
		printf("Failed to open Bitmap file: %s (%s)\r\n", bmapFname, strerror(errno));
		memset(pBitmap, 0, sizeof(BitmapS));
	}
}

//
// void FileRoutines_writeBitmap(int idx, BitmapS* pBitmap)
//
// Writes the bitmap tpo flash.
//
void FileRoutines_writeBitmap(int idx, BitmapS* pBitmap)
{
	int fd;
	char bmapFname[25];
	sprintf(bmapFname, "/store/Data/bitmap%d.dat", idx);
	fd = open(bmapFname, O_WRONLY | O_CREAT, 0666);
	write(fd, pBitmap, sizeof(BitmapS));
	close(fd);
	Sync();
}

void FileRoutines_DimensionsFromConfig(int panelsConfig, int *pWidth, int *pHeight)
{
	*pWidth	= 0;
	*pHeight = 0;

#ifdef LARGER_DOTS
	switch (panelsConfig)
	{
	case 0:
	case 1:
		*pWidth = 16; 
		*pHeight = 12;
		break;
	
	case 4:
		*pWidth = 24; 
		*pHeight = 16;
		break;
	case 5:
		*pWidth = 16; 
		*pHeight = 24;
		break;
	
	case 8:
		*pWidth = 16; 
		*pHeight = 36;
		break;
	case 9:
		*pWidth = 24; 
		*pHeight = 24;
		break;
	case 10:
		*pWidth = 36; 
		*pHeight = 16;
		break;
	case 11:
		*pWidth = 24; 
		*pHeight = 24;
		break;
	
	case 13:
	    *pWidth = 32;
	    *pHeight = 24;
	    break;
	case 14:
	    *pWidth = 48;
	    *pHeight = 16;
	    break;
	case 15:
	    *pWidth = 24;
	    *pHeight = 36;
	    break;
	case 16:
	    *pWidth = 36;
	    *pHeight = 24;
	    break;
	case 17:
	    *pWidth = 32;
	    *pHeight = 36;
	    break;
	case 18:
	    *pWidth = 48;
	    *pHeight = 24;
	    break;
	case 19:
		*pWidth = 60;
	    *pHeight = 24;
	    break;
	case 20:
		*pWidth = 60;
	    *pHeight = 32;
	    break;
	case 21:
		*pWidth = 48;
	    *pHeight = 32;
	    break;
	}
#else	
	switch (panelsConfig)
	{
	case 0:
	case 1:
		*pWidth = 16; 
		*pHeight = 10;
		break;
	
	case 4:
		*pWidth = 20; 
		*pHeight = 16;
		break;
	case 5:
		*pWidth = 16; 
		*pHeight = 20;
		break;
	
	case 8:
		*pWidth = 16; 
		*pHeight = 30;
		break;
	case 9:
		*pWidth = 20; 
		*pHeight = 24;
		break;
	case 10:
		*pWidth = 30; 
		*pHeight = 16;
		break;
	case 11:
		*pWidth = 24; 
		*pHeight = 20;
		break;
	
	case 13:
	    *pWidth = 32;
	    *pHeight = 20;
	    break;
	case 14:
	    *pWidth = 40;
	    *pHeight = 16;
	    break;
	case 15:
	    *pWidth = 24;
	    *pHeight = 30;
	    break;
	case 16:
	    *pWidth = 30;
	    *pHeight = 24;
	    break;
	case 17:
	    *pWidth = 32;
	    *pHeight = 30;
	    break;
	case 18:
	    *pWidth = 40;
	    *pHeight = 24;
	    break;
	case 19:
		*pWidth = 50;
	    *pHeight = 24;
	    break;
	case 20:
		*pWidth = 50;
	    *pHeight = 32;
	    break;
	case 21:
		*pWidth = 40;
	    *pHeight = 32;
	    break;
	}
#endif
}


//
// void FileRoutines_addLog(int logEntryType, char *pParams)
//														  ,
// Adds a log entry.
//
void FileRoutines_addLog(int logEntryType, char *pParams)
{
	FILEHANDLE fh = -1;
	char filename[160];
	char logEntry[160];

	int fileLength = 0;

	//
	// format the log entry string
	//
	switch (logEntryType)
	{
	case LOG_VOLTAGE_TOO_LOW:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tPower supply voltage low: %d.%02d V\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC,
		(int)SupplyVoltageLevel, ((int)(SupplyVoltageLevel * 100.00)) % 100);
		break;
		
	case LOG_SYSTEM_START:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tController turned on\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC);
		break;

	case LOG_ROTARY_CHANGE:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tRotary value changed: %s\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC, pParams);
		break;

	case LOG_CHANGE_TIME:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tTime changed: %s\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC, pParams);
		break;

	case LOG_REMOVE_SDCARD:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tMemory card set to readonly mode\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC);
		break;

	case LOG_CONNECTED:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tRemote peer connected (%s)\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC, pParams);
		break;

	case LOG_DISCONNECTED:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tRemote peer disconnected\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC);
		break;

	case LOG_PASSWORD_CHANGED:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tPassword changed\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC);
		break;

	case LOG_CAMERA_RESET:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tCamera not responding - resetting camera\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC);
		break;

	case LOG_CAMERA_DETECTED:
		sprintf(logEntry, "%d-%02d-%02d %02d:%02d:%02d\tCamera detected\r\n", 
		RTC_YEAR, RTC_MONTH, RTC_DOM, RTC_HOUR, RTC_MIN, RTC_SEC);
		break;
		
	default:
		return;
	}

	//
	// open a file
	//
	if (0 != CanWriteMemoryCard() )	/* Can we write to the USB? */ 
	{
		sprintf(filename, "/store/System/Events%d%02d.txt", RTC_YEAR, RTC_MONTH);
		fh = _sys_open((const char*)filename, OPEN_A);		
		if (fh == -1)
		{
			printf("ERR-1: %s %d - %s\r\n", filename, errno, strerror(errno));
		}
	}
	else
	{
		return;
	}
	
	fileLength =_sys_flen(fh);
	_sys_seek(fh, fileLength);

	//
	// write the log entry
	//
	_sys_write(fh, (const unsigned char*)logEntry, strlen(logEntry), 0);
	 
	//
	// close the file
	//
	_sys_close(fh);

	Sync();
}

void FileRoutines_addCSVLog(char *logEntry)
{
	FILEHANDLE fh = -1;
	char filename[160];

	// int fileLength = 0;

	//
	// open a file
	//
	if (CanWriteMemoryCard())
	{
		sprintf(filename, "/store/System/Sys%d%02d.dat", RTC_YEAR, RTC_MONTH);
		fh = _sys_open((const char*)filename, OPEN_A);		
	}
	if (fh == -1)
	{
		printf("ERR-2: %s %d - %s\r\n", filename, errno, strerror(errno));
		return;
	}
	
	//	fileLength =_sys_flen(fh);
	//	_sys_seek(fh, fileLength);

	//
	// write the log entry
	//
	_sys_write(fh, (const unsigned char*)logEntry, strlen(logEntry), 0);
	 
	//
	// close the file
	//
	_sys_close(fh);

	Sync();
}

//
// void FileRoutines_addVehicleLog(int speed)
//
// Logs vehicle data.
//
void FileRoutines_addVehicleLog(int speed)
{
	char filename[160];
	char entryData[9];
	FILEHANDLE fh = -1;

	if (!CanWriteMemoryCard())
	{
		return;
	}

	//
	// check that the value is not too small
	//
	if (speed < 10)
	{
		return;
	}
	

	//
	// format the filename
	//
	FileRoutines_readDeviceInfo(&DeviceInfo);
	sprintf(filename, "/store/VehicleCounts/VC-%d%02d.bvs", RTC_YEAR, RTC_MONTH);

	//
	// open the file for writing
	//
	fh = _sys_open((const char*)filename, OPEN_A);

	if (fh == -1)
	{
		printf("ERR-3: %s %d - %s\r\n", filename, errno, strerror(errno));
		FileRoutines_SaveDebugInfo("ERR-3\n", 6); 
		return;
	}

	_sys_seek(fh, _sys_flen(fh));


	//
	// format and write the entry data
	//
	entryData[0] = RTC_YEAR - 2000;
	entryData[1] = RTC_MONTH;
	entryData[2] = RTC_DOM;
	entryData[3] = RTC_HOUR;
	entryData[4] = RTC_MIN;
	entryData[5] = RTC_SEC;

	entryData[6] = speed;
	
	_sys_write(fh, (const unsigned char*)entryData, 7, 0);


	char debug[64];
	sprintf(debug, "speed: %d\n", speed);
	FileRoutines_SaveDebugInfo(debug, strlen(debug)); 

	//
	// close the file
	//
	_sys_close(fh);

	Sync();
}

//
// int FileRoutines_autoconfFromCard()
//
// Searched for autoconf files on the SD card and configures the system accordingly. Returns true if new config is applied.
//
int FileRoutines_autoconfFromCard(char* fname)
{
	int i, j;
	char useAutodimming;
	int numImages;
	FILEHANDLE fh = -1;
	int largeSign = 0;
	int pos = 0;

	//
	// read autoconf file
	//
	fh = _sys_open(fname, OPEN_R);
	if (fh == -1)
	{
		printf("ERR-4: %s %d - %s\r\n", fname, errno, strerror(errno));
		return 0;
	}

	FileRoutines_readDeviceInfo(&DeviceInfo);

	_sys_read(fh, (unsigned char*)(&DeviceInfo.radarBaudRate), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.radarProtocol), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.sensitivity), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.minDisplaySpeed), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.blinkLimit), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.blinkLimitFromRotary), 1, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.maxDisplaySpeed), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.maxSpeedFromRotary), 1, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.blinkOnDurationMs), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.blinkOffDurationMs), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.updateDisplayLengthMs), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.updateDisplaySpeedDelta), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.keepLastSpeedLengthMs), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.displayBrightness), 4, 0);
	_sys_read(fh, (unsigned char*)(&useAutodimming), 1, 0);
	pos = 12 * 4 + 3;


	if (useAutodimming)
		DeviceInfo.displayBrightness |= 0x80;
	_sys_read(fh, (unsigned char*)(&DeviceInfo.panelsConfiguration), 4, 0);
	pos += 4;

	// test if LARGE sign compatible
#ifdef LARGER_DOTS
	largeSign = 1;
#endif

	if (   	(largeSign && ( (!DeviceInfo.panelsConfiguration) & 0x40)) ||  
			( (!largeSign) && (DeviceInfo.panelsConfiguration & 0x40))  
		)
	{
		_sys_close(fh);
		FileRoutines_readDeviceInfo(&DeviceInfo);
		return 0;
	}

	DeviceInfo.panelsConfiguration &= ~0x40;	


	for (i = 0; i < 16; i++)
  {
		_sys_read(fh, (unsigned char*)(&DeviceInfo.autoDimming[i].brightness), 4, 0);
		_sys_read(fh, (unsigned char*)(&DeviceInfo.autoDimming[i].luminance), 4, 0);
		pos += 8;
  }

  for (i = 0; i < 4; i++)
  {        
		int speedDisplayMode;
		int x;
		int y;
		int font;
		short frameLength;
		int numFrames;

		_sys_read(fh, (unsigned char*)(&speedDisplayMode), 4, 0);
		_sys_read(fh, (unsigned char*)(&x), 4, 0);
		_sys_read(fh, (unsigned char*)(&y), 4, 0);
		_sys_read(fh, (unsigned char*)(&font), 4, 0);
		_sys_read(fh, (unsigned char*)(&frameLength), 2, 0);
		_sys_read(fh, (unsigned char*)(&numFrames), 4, 0);
		pos += 22;		

		DeviceInfo.bitmapsConfig[i].speedDisplayMode = speedDisplayMode;
		DeviceInfo.bitmapsConfig[i].x = x;
		DeviceInfo.bitmapsConfig[i].y = y;
		DeviceInfo.bitmapsConfig[i].font = font;
		DeviceInfo.bitmapsConfig[i].frameLength = frameLength;
		DeviceInfo.bitmapsConfig[i].numFrames = numFrames;		
		
    for (j = 0; j < numFrames; j++)
    {
			int frame;
			_sys_read(fh, (unsigned char*)(&frame), 4, 0);
			pos += 4;
      DeviceInfo.bitmapsConfig[i].frames[j] = frame;
    }
  }

	_sys_read(fh, (unsigned char*)(&DeviceInfo.scheduleType), 4, 0);
	_sys_read(fh, (unsigned char*)(&DeviceInfo.schedulesCount), 4, 0);
	pos += 8;

  for (i = 0; i < DeviceInfo.schedulesCount; i++)
  {
		int entryType;
		int dow;
		int year;
		int month;
		int day;
		int start;
		int duration;
		int periodStartDay = 1;
		int periodStartMonth = 1;
		int periodEndDay = 31;
		int periodEndMonth = 12;

		_sys_read(fh, (unsigned char*)(&entryType), 4, 0);
		_sys_read(fh, (unsigned char*)(&dow), 4, 0);
		_sys_read(fh, (unsigned char*)(&year), 4, 0);
		_sys_read(fh, (unsigned char*)(&month), 4, 0);
		_sys_read(fh, (unsigned char*)(&day), 4, 0);
		_sys_read(fh, (unsigned char*)(&start), 4, 0);
		_sys_read(fh, (unsigned char*)(&duration), 4, 0);
		pos += 7 * 4;

		if (dow & 0x80000000)
		{
			periodStartMonth = (dow >> 4) & 0x000F;
			periodStartDay = (dow >> 8) & 0x00FF;
			periodEndMonth = (dow >> 16) & 0x00FF;
			periodEndDay = (dow >> 24) & 0x007F;
			dow = dow & 0x000F;
		}
		

    DeviceInfo.schedules[i].entryType = entryType;
    DeviceInfo.schedules[i].dow = dow;
    DeviceInfo.schedules[i].year = year;
    DeviceInfo.schedules[i].month = month;
    DeviceInfo.schedules[i].day = day;
    DeviceInfo.schedules[i].start = start;
    DeviceInfo.schedules[i].duration = duration;
    DeviceInfo.schedules[i].periodStartDay = periodStartDay;
    DeviceInfo.schedules[i].periodStartMonth = periodStartMonth;
    DeviceInfo.schedules[i].periodEndDay = periodEndDay;
    DeviceInfo.schedules[i].periodEndMonth = periodEndMonth;
  }

	FileRoutines_writeDeviceInfo(&DeviceInfo);

	_sys_read(fh, (unsigned char*)(&numImages), 4, 0);
	pos += 4;
  for (i = 0; i < numImages; i++)
  {
    int len;
		_sys_read(fh, (unsigned char*)(&len), 4, 0);
		pos += 4;
		if (len == (60 * 48)/8)
		{
			_sys_read(fh, (unsigned char*)(&Bitmap.bitmapData), len, 0);
			pos += len;
			FileRoutines_writeBitmap(i, &Bitmap);
		}
  }

	if (_sys_flen(fh) >= (pos + 20))
	{
		int magic;
		_sys_read(fh, (unsigned char*)(&magic), 4, 0);
		if (magic == 0x0BABAD1D)
		{
			int v;
			_sys_read(fh, (unsigned char*)(&DeviceInfo.ipAddress), 4, 0);
			_sys_read(fh, (unsigned char*)(&DeviceInfo.subnetMask), 4, 0);
			_sys_read(fh, (unsigned char*)(&DeviceInfo.gatewayAddress), 4, 0);
			_sys_read(fh, (unsigned char*)(&v), 4, 0);
			DeviceInfo.useDHCP = (v != 0);
			FileRoutines_writeDeviceInfo(&DeviceInfo);	
		}
	}

	_sys_close(fh);

	return 1;
}

int FileRoutines_libraryCheckWidthHeight(FILEHANDLE fh, int *w, int *h)
{
	int tw, th;
	_sys_read(fh, (unsigned char*)(w), 4, 0);
	_sys_read(fh, (unsigned char*)(h), 4, 0);

	FileRoutines_readDeviceInfo(&DeviceInfo);
	FileRoutines_DimensionsFromConfig(DeviceInfo.panelsConfiguration, &tw, &th);

	return (*w == tw) && (*h == th);
}

char LibBuf[512];

void FileRoutines_appendLibName(char* pDestArray)
{
	int i;
	for(i = 0; i < strlen(LibBuf); i++)
	{
		char b[8];
		if (isalnum(LibBuf[i]))
		{
			b[0] = LibBuf[i];
			b[1] = 0;
		}
		else
		{
			sprintf(b, "+%02X", LibBuf[i]);
		}

		strcat(pDestArray, b);
	}
	strcat(pDestArray, ","); 
}

void FileRoutines_libraryEnumImages(int startIdx, char* pDestArray)
{
	int w, h, i;
	int images = 0;
	FILEHANDLE fh = -1;

	fh = _sys_open("/store/library.vil", OPEN_R);
	if (fh == -1)
	{
		printf("ERR-5: library.vil %d - %s\r\n", errno, strerror(errno));
		strcpy(pDestArray, "0,");
		return;
	}
	if (!FileRoutines_libraryCheckWidthHeight(fh, &w, &h))
	{
		strcpy(pDestArray, "0,");
		_sys_close(fh);
		return;
	}

	_sys_read(fh, (unsigned char*)(&images), 4, 0);
	sprintf(pDestArray, "%d,", images);
	for(i = 0; i < images; i++)
	{
		int nameLen;
		_sys_read(fh, (unsigned char*)(&nameLen), 4, 0);
		_sys_read(fh, (unsigned char*)LibBuf, nameLen, 0);
		LibBuf[nameLen] = 0;

		if (i >= (startIdx + 10))
			break;

		if (i >= startIdx)
			FileRoutines_appendLibName(pDestArray);

		_sys_read(fh, (unsigned char*)LibBuf, (w * h / 8 + 1), 0);
	}

	_sys_close(fh);
}

int FileRoutines_libraryReadImage(int imageIdx, char* pDestArray)
{
	int w, h, i;
	int images = 0;
	FILEHANDLE fh = -1;

	fh = _sys_open("/store/library.vil", OPEN_R);
	if (fh == -1)
	{
		printf("ERR-6: library.vil %d - %s\r\n", errno, strerror(errno));
		return 0;
	}
	if (!FileRoutines_libraryCheckWidthHeight(fh, &w, &h))
	{
		_sys_close(fh);
		return 0;
	}

	_sys_read(fh, (unsigned char*)(&images), 4, 0);
	sprintf(pDestArray, "%d,", images);
	for(i = 0; i < images; i++)
	{
		int nameLen;
		_sys_read(fh, (unsigned char*)(&nameLen), 4, 0);
		_sys_read(fh, (unsigned char*)LibBuf, nameLen, 0);
		LibBuf[nameLen] = 0;

		if (i > imageIdx)
			break;

		_sys_read(fh, (unsigned char*)LibBuf, (w * h / 8 + 1), 0);
		if (i == imageIdx)
		{
			HexToString((unsigned char*)LibBuf, pDestArray, (w * h / 8 + 1));
		}
	}

	_sys_close(fh);
	return 1;
}

void FileRoutines_libraryEnumSequences(int startIdx,  char* pDestArray)
{
	int w, h, i;
	int images, sequences;
	FILEHANDLE fh = -1;

	fh = _sys_open("/store/library.vil", OPEN_R);
	if (fh == -1)
	{
		printf("ERR-7: library.vil %d - %s\r\n", errno, strerror(errno));
		strcpy(pDestArray, "0,");
		return;
	}
	if (!FileRoutines_libraryCheckWidthHeight(fh, &w, &h))
	{
		strcpy(pDestArray, "0,");
		_sys_close(fh);
		return;
	}

	_sys_read(fh, (unsigned char*)(&images), 4, 0);
	for(i = 0; i < images; i++)
	{
		int nameLen;
		_sys_read(fh, (unsigned char*)(&nameLen), 4, 0);
		_sys_read(fh, (unsigned char*)LibBuf, nameLen, 0);
		LibBuf[nameLen] = 0;
		_sys_read(fh, (unsigned char*)LibBuf, (w * h / 8 + 1), 0);
	}

	_sys_read(fh, (unsigned char*)(&sequences), 4, 0);
	sprintf(pDestArray, "%d,", sequences);
	for(i = 0; i < sequences; i++)
	{
		int nameLen, frameDelay, numFrames;
		_sys_read(fh, (unsigned char*)(&nameLen), 4, 0);
		_sys_read(fh, (unsigned char*)LibBuf, nameLen, 0);
		LibBuf[nameLen] = 0;

		if (i >= (startIdx + 10))
			break;

		if (i >= startIdx)
			FileRoutines_appendLibName(pDestArray);

		_sys_read(fh, (unsigned char*)(&frameDelay), 4, 0);
		_sys_read(fh, (unsigned char*)(&numFrames), 4, 0);

		_sys_read(fh, (unsigned char*)LibBuf, (numFrames * 4), 0);
	}

	_sys_close(fh);
}

int FileRoutines_libraryReadSequence(int seqIdx,  char* pDestArray)
{
	int w, h, i;
	int images, sequences;
	FILEHANDLE fh = -1;

	fh = _sys_open("/store/library.vil", OPEN_R);
	if (fh == -1)
	{
		printf("ERR-8: library.vil %d - %s\r\n", errno, strerror(errno));
		return 0;
	}
	if (!FileRoutines_libraryCheckWidthHeight(fh, &w, &h))
	{
		_sys_close(fh);
		return 0;
	}

	_sys_read(fh, (unsigned char*)(&images), 4, 0);
	for(i = 0; i < images; i++)
	{
		int nameLen;
		_sys_read(fh, (unsigned char*)(&nameLen), 4, 0);
		_sys_read(fh, (unsigned char*)LibBuf, nameLen, 0);
		LibBuf[nameLen] = 0;
		_sys_read(fh, (unsigned char*)LibBuf, (w * h / 8 + 1), 0);
	}

	_sys_read(fh, (unsigned char*)(&sequences), 4, 0);
	for(i = 0; i < sequences; i++)
	{
		int nameLen, frameDelay, numFrames;
		_sys_read(fh, (unsigned char*)(&nameLen), 4, 0);
		_sys_read(fh, (unsigned char*)LibBuf, nameLen, 0);
		LibBuf[nameLen] = 0;

		_sys_read(fh, (unsigned char*)(&frameDelay), 4, 0);
		_sys_read(fh, (unsigned char*)(&numFrames), 4, 0);

		_sys_read(fh, (unsigned char*)LibBuf, (numFrames * 4), 0);

		if (i == seqIdx)
		{
			int j;
			char buf[8];
			sprintf(pDestArray, "%d,%d,",frameDelay,numFrames);
			for(j = 0; j < numFrames; j++)
			{
				int v;
				memcpy(&v, LibBuf+4*j, 4);
				sprintf(buf, "%d,", v);
				strcat(pDestArray, buf);
			}
			break;
		}
	}

	_sys_close(fh);
	return 1;
}

int FileRoutines_SaveDebugInfo(char *pData, int length)
{
	char filename[128];
	FILEHANDLE fh = -1;

	return 0; // todo no debug info stored

	sprintf(filename, "/store/System/RadarDebug.txt");

	//
	// open the file for writing
	//
	fh = _sys_open((const char*)filename, OPEN_A);
	
	
	if (fh == -1)
	{
		printf("ERR-10: %s %d - %s\r\n", filename, errno, strerror(errno));
		return 0;
	}

	_sys_seek(fh, _sys_flen(fh));

	//
	// write the data
	//
	_sys_write(fh, (const unsigned char*)pData, length, 0);

	//
	// close the file
	//
	_sys_close(fh);

	return 0;
}
