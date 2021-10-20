#ifndef __SYSTEMUTILITIES_H__
#define __SYSTEMUTILITIES_H__

#include <unistd.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define fdelete unlink

#define _sys_open open
#define _sys_close close

typedef int FILEHANDLE;

#define GPIO(A, B) (A*32 + B)

#define OPEN_A ((int) (O_WRONLY | O_APPEND | O_CREAT))
#define OPEN_R ((int) O_RDONLY)

typedef enum
{
	USB_CHANGE_NODATA,
	USB_CHANGE_NONE,
	USB_CHANGE_REMOVED,
	USB_CHANGE_ADDED
} USB_CHANGE_STATUS;

int CheckUsbChange(FILE* fp);

char *readSerial(int iFd, int max, long lTmoMs);

void _sys_read(int fh, unsigned char *buffer, int len, int flag);
void _sys_write(int fh, const unsigned char *buffer, int len, int flag);
void _sys_seek(int fh, int offset);
int _sys_flen(int fh);

unsigned char ahtoi(char inChr);
unsigned short ahtos(char* data);
unsigned char ahtob(char* data);

void Dismount(char* device);
void Sync(void);
void Directory(char* dirData);
void MoveFile(char* srcDir, char* destDir, char* mask);
void MountDrive(char* device, char* mntPnt);
char* SysCmd(char *Command);
char* SysCmdf(char* fmt, ...);
void DisplayMem(char *Disp, unsigned char *pBuff, int size);
int finit(void);

int rtc_sec(void);
int rtc_min(void);
int rtc_hour(void);
int rtc_mday(void);
int rtc_mon(void);
int rtc_year(void);
int rtc_wday(void);
int rtc_yday(void);
int rtc_isdst(void);

typedef enum
{
	MC_STATUS_CARDERROR,
	MC_STATUS_READONLY,
	MC_STATUS_OK_MMC,
	MC_STATUS_OK_SD,
	MC_STATUS_OK_SDHC,
	MC_STATUS_FULL,
	MC_STATUS_NOCARD
} MC_STATUS_TYPES; 

#define CARD_SDHC 0
#define CARD_SD 1

#define RTC_YEAR rtc_year()
#define RTC_MONTH rtc_mon()
#define RTC_DOM   rtc_mday()
#define RTC_HOUR rtc_hour()
#define RTC_MIN  rtc_min()
#define RTC_SEC  rtc_sec()

extern int FIO2PIN;
extern int MCI_POWER;
extern int VICVectAddr;

typedef struct
{
	unsigned char IpAdr[4];
	unsigned char NetMask[4];
	unsigned char DefGW[4];
} LOCALMX;

typedef struct
{
    int hr;
    int min;
    int sec;
    int mon;
    int day;
    int year;
} FILE_TIME;

typedef struct
{
	int DataClusCnt;
} FATINFO;

int main_TcpNet(void);
int fat_clus_size(void);
int fat_free_clus(void); 
int finit(); 
//GetBtAddress
int init_TcpNet(void);

#define ATTR_DIRECTORY 1

typedef struct 
{
	int fileID;
	char* name;
	FILE_TIME time;
	int attrib;
	int size;
  bool valid;
} FINFO;

int ffind(char* mask, FINFO* fileInfo);
int fileStats(char* fname, FINFO* fileInfo);

void InitPwmTask(void);
void  setDuty(unsigned char duty);
void  setRun(unsigned char  run);

#endif
