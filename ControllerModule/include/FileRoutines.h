#ifndef _FILE_ROUTINES_H_INCLUDED_
#define _FILE_ROUTINES_H_INCLUDED_

#include "SystemUtilities.h"

//
// Various defines.
//

#define OP_CODE					0x00001243   // increment for every new software version!
#define OP_CODE_ADDR 			0x00068000

#define MAX_STRING_LENGTH		32
#define MAX_FRAMES				10

#define MAX_SCHEDULES			22

#define MAX_BITMAPS				500

typedef enum ProtocolTypeE_
{
	Protocol_Special28 = 0,
	Protocol_ASCII64,
	Protocol_NoRadar,
	Protocol_NMEA,
	Protocol_ENUM_SIZE
} ProtocolTypeE;

// #pragma packed(1)
typedef struct ScheduleEntryS_
{
	unsigned char entryType : 2; 	// 0 - every day, 1 - weekday, 2 - exact date
	unsigned char dow : 6; 			// day of week
	unsigned short year : 12;
	unsigned char month : 4;
	unsigned char day;
	short start;					// minute in day of interval start
	short duration;					// duration, in minutes
	unsigned char periodStartDay;
	unsigned char periodStartMonth : 4;
	unsigned char periodEndDay;
	unsigned char periodEndMonth : 4;
} ScheduleEntryS;

typedef struct AutoDimmingEntryS_
{
	unsigned char brightness;
	unsigned int luminance;
} AutoDimmingEntryS;

typedef struct BitmapS_
{
	unsigned char bitmapData[(60 * 48)/8];
} BitmapS;

typedef struct DisplayConfigS_
{
	unsigned char speedDisplayMode;
	unsigned char x;
	unsigned char y;
	unsigned char font;
	short frameLength;
	unsigned char numFrames;
	unsigned char frames[MAX_FRAMES];
} DisplayConfigS;


//
// system log defines
//
#define	LOG_VOLTAGE_TOO_LOW		0x01
#define	LOG_SYSTEM_START		0x02
#define LOG_ROTARY_CHANGE		0x03
#define LOG_CHANGE_TIME			0x04
#define LOG_REMOVE_SDCARD		0x05
#define LOG_CONNECTED			0x06
#define LOG_DISCONNECTED		0x07
#define LOG_PASSWORD_CHANGED	0x08
#define LOG_CAMERA_RESET		0x09
#define LOG_CAMERA_DETECTED		0x0A

//
// DeviceInfo structure
//
typedef struct DeviceInfoS_
{
	int VERSION;
	char stupidPassword[MAX_STRING_LENGTH];
	char password[MAX_STRING_LENGTH];
	char adminPassword[MAX_STRING_LENGTH];
	char deviceName[MAX_STRING_LENGTH];
	int radarBaudRate;					// comm with radar baud rate
	int sensitivity;					// radar sensitivity: 0-7; 7 most sensitivity
	ProtocolTypeE radarProtocol;		// which protocol is used: 0=special28, 1=ASCII64 etc
	int minDisplaySpeed;				// min speed in mph that will be displayed
	int blinkLimit;						// min speed which activates blinking
	unsigned char blinkLimitFromRotary;	// if nonzero, indicates that the blinking limit should be read from rotary switches
	int maxDisplaySpeed;				// max displayed speed / offset
	unsigned char maxSpeedFromRotary;   // if nonzero, indicates that the blinking limit should be read from rotary switches
	int blinkOnDurationMs;				// blink ON period duration
	int blinkOffDurationMs;				// blink OFF period duration
	int updateDisplayLengthMs;			// the delay between two updates of the displayed speed
	int updateDisplaySpeedDelta;		// speed needs to change at least this mph to be updated
	int keepLastSpeedLengthMs;			// how long to keep last displayed speed on display
	int displayBrightness;				// output display brightness, 0-100, bit7 (0x80) indicates if auto dimming is used

	int panelsConfiguration;			// index of the panel configuration setup

	AutoDimmingEntryS autoDimming[16];	// auto dimming table

	DisplayConfigS bitmapsConfig[4];	// defines which images are displayed
	unsigned char scheduleType;
	unsigned char schedulesCount;
	ScheduleEntryS schedules[MAX_SCHEDULES];
	// ethernet properties
	int useDHCP;
	unsigned int ipAddress;
	unsigned int subnetMask;
	unsigned int gatewayAddress;
	char unitType; // 0 = mph, 1 = kmh
} DeviceInfoS;
// #pragma packed()

//
// Function prototypes
//
void FileRoutines_init(void);
void FileRoutines_readDeviceInfo(DeviceInfoS* pDeviceInfo);
void FileRoutines_writeDeviceInfo(DeviceInfoS* pDeviceInfo);

void FileRoutines_readBitmapFile(char* bmapFname, BitmapS* pBitmap);
void FileRoutines_readBitmap(int idx, BitmapS* pBitmap);
void FileRoutines_writeBitmap(int idx, BitmapS* pBitmap);

void FileRoutines_addLog(int logEntryType, char *pParams);
void FileRoutines_addCSVLog(char *pParams);
void FileRoutines_addVehicleLog(int speed);

int FileRoutines_autoconfFromCard(char*);

void FileRoutines_libraryEnumImages(int, char*);
void FileRoutines_libraryEnumSequences(int, char*);
int FileRoutines_libraryReadImage(int imageIdx, char* pDestArray);
int FileRoutines_libraryReadSequence(int seqIdx, char* pDestArray);

int FileRoutines_libraryCheckWidthHeight(FILEHANDLE fh, int *w, int *h);

int FileRoutines_SaveDebugInfo(char *pData, int length);

void FileRoutines_DimensionsFromConfig(int panelsConfig, int *pWidth, int *pHeight);

#endif // _FILE_ROUTINES_H_INCLUDED_
