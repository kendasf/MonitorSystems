#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <regex.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>
#include <pthread.h>

#include "../Common/inc/pinctl.h"
#include "SystemUtilities.h"

int FIO2PIN = 0;
int MCI_POWER = 0;
int VICVectAddr = 0;

unsigned char ahtoi(char inChr)
{
        unsigned char retval = 0;
        if ((inChr >= '0') && (inChr <= '9'))
                retval = inChr - '0';
        else if ((inChr >= 'A') && (inChr <= 'F'))
                retval = (inChr - 'A') + 0x0A;
        else if ((inChr >= 'a') && (inChr <= 'f'))
                retval = (inChr - 'a') + 0x0A;

        return retval;
}

unsigned short ahtos(char *data)
{
        unsigned short retval = 0;
        retval = ahtoi(data[0]);
        retval <<= 4;
        retval |= ahtoi(data[1]);
        retval <<= 4;
        retval |= ahtoi(data[2]);
        retval <<= 4;
        retval |= ahtoi(data[3]);
        return retval;
}

unsigned char ahtob(char *data)
{
        unsigned char retval = 0;
        retval = ahtoi(data[0]);
        retval <<= 4;
        retval |= ahtoi(data[1]);
        return retval;
}

void substr(char *string, char *result, int posn, int len)
{
        int cnt = 0;

        for (cnt = 0; cnt < len; cnt++)
        {
                result[cnt] = string[posn + cnt];
                cnt++;
        }
        result[cnt] = '\0';
}

void Dismount(char *device)
{
        char mount[50];
        char *resp;
        sprintf(mount, "umount %s", device);
        resp = SysCmd(mount);
        if (resp != NULL)
        {
                free(resp);
        }
}

void Sync(void)
{
        char *resp = SysCmd("sync");
        if (resp != NULL)
        {
                free(resp);
        }
}

void MountDrive(char *device, char *mntPnt)
{
        char mount[50];
        char *resp = NULL;
        int devfd = open(mntPnt, O_RDONLY);
        if (devfd > 0)
        {
                close(devfd);
        }
        else
        {
                sprintf(mount, "mkdir %s", mntPnt);
                resp = SysCmd(mount);
                if (resp != NULL)
                {
                        free(resp);
                }
        }

        sprintf(mount, "mount %s %s", device, mntPnt);
        resp = SysCmd(mount);
        if (resp != NULL)
        {
                printf("RESP: %s\r\n", resp);
                free(resp);
        }
}
void Directory(char *dirData)
{
        DIR *d;
        struct dirent *dir;
        printf("Directory:\r\n");
        d = opendir(dirData);

        if (d)
        {
                while ((dir = readdir(d)) != NULL)
                {
                        if (dir->d_type == 8)
                        {
                                printf("%s\n", dir->d_name);
                        }
                }
                closedir(d);
        }
}

#define BUFF_MAX_SIZE 128
void MoveFile(char *srcDir, char *destDir, char *mask)
{
        DIR *d;
        struct dirent *dir;
        d = opendir(srcDir);

        if (d)
        {
                mkdir(destDir, DEFFILEMODE);
                while ((dir = readdir(d)) != NULL)
                {
                        if ((dir->d_type == 8) && (strstr(dir->d_name, mask) != NULL))
                        {
                                char *resp;
                                char src[BUFF_MAX_SIZE] = {0};
                                char dst[BUFF_MAX_SIZE] = {0};
                                char cmd[BUFF_MAX_SIZE] = {0};

                                snprintf(src, BUFF_MAX_SIZE, "\"%s/%s\"", srcDir, dir->d_name);
                                snprintf(dst, BUFF_MAX_SIZE, "\"%s/%s\"", destDir, dir->d_name);
                                snprintf(cmd, BUFF_MAX_SIZE, "chmod 666 %s", src);
                                resp = SysCmd(cmd);
                                if (resp)
                                        free(resp);
                                snprintf(cmd, BUFF_MAX_SIZE, "mv -f %s %s", src, dst);

                                resp = SysCmd(cmd);
                                if (resp != NULL)
                                        free(resp);
                        }
                }
                closedir(d);
        }
}

int finit(void)
{
        int retval = 0;
#if 0
int devfd = open("/dev/sda1", O_RDONLY);
if (devfd > 0)
{
char* info = NULL;
char* infoPtr = NULL;
close(devfd);
info = SysCmd("df /dev/sda1");
infoPtr = strstr(info, "/dev/sda1");
if (infoPtr != NULL)
{
retval = 0;	
}
else
{
retval = 1;
}
if (info != NULL)
free(info);	
}
#endif
        return retval;
}

char *SysCmdf(char *fmt, ...)
{
        int iCnt;
        va_list args;        /* variable argument list */
        va_start(args, fmt); /* point args to first unnamed element */
        char *retval = NULL;
        int dataSize = 0;
        FILE *fp;
        char *buff = (char *)malloc(256);
        char *cmd = (char *)malloc(128);

        iCnt = vsnprintf(cmd, 128, fmt, args);
        if (iCnt > 0)
        {
                fp = popen(cmd, "re");
                if (fp != NULL)
                {
                        while (fgets(buff, 256, fp) != NULL)
                        {
                                int newSize = dataSize + strlen(buff) + 1;
                                char *data = (char *)malloc(newSize);
                                if (data != NULL)
                                {
                                        memset(data, 0, newSize);

                                        if (retval != NULL)
                                        {
                                                strcat(data, retval);
                                                free(retval);
                                                retval = NULL;
                                        }

                                        strcat(data, buff);
                                        retval = data;
                                        dataSize = strlen(retval);
                                }
                        }
                        pclose(fp);
                }
        }
        free(buff);
        free(cmd);

        return retval;
}

char *SysCmd(char *Command)
{
        char *retval = NULL;
        int dataSize = 0;
        FILE *fp;
        char buff[256];

        fp = popen(Command, "re");
        if (fp != NULL)
        {
                while (fgets(buff, sizeof(buff), fp) != NULL)
                {
                        int newSize = dataSize + strlen(buff) + 1;
                        char *data = (char *)malloc(newSize);
                        if (data != NULL)
                        {
                                memset(data, 0, newSize);

                                if (retval != NULL)
                                {
                                        strcat(data, retval);
                                        free(retval);
                                        retval = NULL;
                                }

                                strcat(data, buff);
                                retval = data;
                                dataSize = strlen(retval);
                        }
                }
                pclose(fp);
        }
        return retval;
}

void _sys_read(int fh, unsigned char *buffer, int len, int flag)
{
        if (fh != -1)
                read(fh, buffer, len);
}

void _sys_write(int fh, const unsigned char *buffer, int len, int flag)
{
        int err = write(fh, buffer, len);
        if (err < 0)
                printf("_sys_write Err: %d  - %d %s\r\n", fh, errno, strerror(errno));
}

int _sys_flen(int fh)
{
        struct stat buf;
        int stat = fstat(fh, &buf);
        return (int)buf.st_size;
}

void _sys_seek(int fh, int offset)
{
        lseek(fh, (off_t)offset, SEEK_SET);
}

void DisplayMem(char *Disp, unsigned char *pBuff, int size)
{
        int i, j;
        long Address = (long)pBuff;
        int line = 0;
        char pucMessage[100];
        int iCnt;
        int iPosn = 0;
        int iMax = sizeof(pucMessage);

        printf("\n%s [%d] \n", Disp, size);

        for (i = 0; i < size;)
        {
                iPosn = 0;
                for (j = 0; ((i + j) < size) && (j < 16); j++)
                {
                        if (((i + j) % 16) == 0)
                        {
                                iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "\n%04lx: ", (long)Address + j);
                                iPosn += iCnt;
                        }
                        iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "%02x ", pBuff[i + j]);
                        iPosn += iCnt;
                }
                for (; j < 16; j++)
                {
                        iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "   ");
                        iPosn += iCnt;
                }
                iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "  ");
                iPosn += iCnt;
                for (j = 0; ((i + j) < size) && (j < 16); j++)
                {
                        iCnt = snprintf(&pucMessage[iPosn], (iMax - iPosn), "%c ", ::isprint((int)pBuff[i + j]) ? pBuff[i + j] : '.');
                        iPosn += iCnt;
                }

                printf(pucMessage);

                i += 16;
                Address += 16;
                line++;
        }
        printf("\n\n");
}

unsigned long long GetCurrentTimeSec(void)
{
        unsigned long long retval = 0;
        struct timeval currentTime;
        gettimeofday(&currentTime, NULL);

        retval = (unsigned long long)currentTime.tv_sec;

        return retval;
}

int rtc_sec(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        return timest.tm_sec;
}

int rtc_min(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        return timest.tm_min; /* minutes */
}

int rtc_hour(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        return timest.tm_hour; /* hours */
}

int rtc_mday(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        return timest.tm_mday; /* day of the month */
}

int rtc_mon(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        timest.tm_mon += 1;
        return timest.tm_mon; /* month */
}

int rtc_year(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        timest.tm_year += 1900;
        return timest.tm_year; /* 1900 + year */
}

int rtc_wday(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        return timest.tm_wday; /* day of the week */
}

int rtc_yday(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        return timest.tm_yday; /* day in the year */
}

int rtc_isdst(void)
{
        struct tm timest;
        time_t timep = time(NULL);
        localtime_r(&timep, &timest);
        return timest.tm_isdst; /* daylight saving time */
}

int checkExpression(char *string, char *pattern)
{
        int status;
        regex_t re;

        if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0)
        {
                return (0); /* Report error. */
        }
        status = regexec(&re, string, (size_t)0, NULL, 0);
        regfree(&re);

        return status;
}

int fileStats(char *fname, FINFO *fileInfo)
{
        int status = -1;
        struct stat buffer;
        int fd = open(fname, O_RDONLY);
        if (fd > 0)
        {
                status = fstat(fd, &buffer);
                if (status >= 0)
                {
                        char *name = fname;
                        int i;
                        struct tm result;
                        time_t timep = (time_t)buffer.st_mtim.tv_sec;
                        localtime_r(&timep, &result);

                        for (i = 0; i < strlen(fname); i--)
                        {
                                if (fname[i] == '/')
                                {
                                        fname[i] = '\\';
                                }
                        }
                        fileInfo->name = fname;
                        fileInfo->size = (int)buffer.st_size;
                        fileInfo->fileID = (int)buffer.st_ino;
                        fileInfo->time.hr = result.tm_hour;
                        fileInfo->time.min = result.tm_min;
                        fileInfo->time.sec = result.tm_sec;
                        fileInfo->time.mon = result.tm_mon + 1;
                        fileInfo->time.day = result.tm_mday;
                        fileInfo->time.year = result.tm_year + 1900;
                        fileInfo->attrib = S_ISDIR(buffer.st_mode) ? ATTR_DIRECTORY : 0;
                }
        }
        close(fd);

        return status;
}

char *directory(char *path)
{
        char *result = NULL;
        DIR *pfDir = NULL;
        struct dirent *dirp;
        int retcode = 0;
        int i;

        pfDir = opendir(path);
        if (pfDir != NULL)
        {
                dirp = readdir(pfDir);
                while (dirp != NULL)
                {
                        if (dirp->d_type == DT_REG)
                        {
                                printf("%s\n", dirp->d_name);
                                //fileStats(dirp->d_name, fileInfo);
                        }
                        else if (dirp->d_type == DT_DIR)
                        {
                                // another directory
                        }
                        dirp = readdir(pfDir);
                }
                closedir(pfDir);
        }
        return result;
}

int ffind(char *mask, FINFO *fileInfo)
{
        int retval = -2;
        int i;
        static char lastmask[128] = "";
        static char path[256];
        static char *dirList = NULL;
        static int dirListLen = 0;
        static char *indexPtr = NULL;

        for (i = 0; i < strlen(mask); i++)
                if (mask[i] == '\\')
                        mask[i] = '/';

        if (strcmp(mask, lastmask) != 0)
        {
                char cmd[128];
                strcpy(lastmask, mask);
                sprintf(cmd, "ls %s", mask);
                if (dirList != NULL)
                        free(dirList);
                dirList = SysCmd(cmd);
                if (dirList != NULL)
                {
                        char *posn = strchr(dirList, (char)0x0A);
                        dirListLen = strlen(dirList);
                        indexPtr = dirList;
                        if (posn != NULL)
                        {
                                *posn = 0;
                        }
                        strcpy(path, mask);
                        strcat(path, "/");
                        strcat(path, indexPtr);
                        if (fileStats(path, fileInfo) >= 0)
                                retval = 0;
                        else
                                retval = -1;
                        fileInfo->name = indexPtr;
                }
                else
                        retval = -2;
        }
        else if (dirList != NULL)
        {
                indexPtr += strlen(indexPtr) + 1;
                if (indexPtr > (dirList + dirListLen))
                {
                        free(dirList);
                        dirList = NULL;
                        dirListLen = 0;
                        memset(lastmask, 0, sizeof(lastmask));
                        indexPtr = NULL;
                        retval = -2;
                }
                else
                {
                        char *posn = strchr(indexPtr, (char)0x0A);
                        if (posn != NULL)
                        {
                                *posn = 0;
                        }
                        strcpy(path, mask);
                        strcat(path, "/");
                        strcat(path, indexPtr);
                        if (fileStats(path, fileInfo) >= 0)
                                retval = 0;
                        else
                                retval = -1;
                        fileInfo->name = indexPtr;
                }
        }

        if (retval == 0)
                fileInfo->valid = true;
        else
                fileInfo->valid = false;
        return retval;
}

static unsigned char pwmDuty = 100;
static unsigned char pwmRun = 0;
#define period 2500
static int pwmPeriod = (period / 100);

void setDuty(unsigned char duty)
{
        pwmDuty = duty;
}

void setRun(unsigned char run)
{
        pwmRun = run;
}

#define BUFOE GPIO(3, 19)
void *PwmTask(void *arg)
{
        int bufoeFd = -1;
        int state = 0;
        bufoeFd = pinctl::inst().export_pin(BUFOE, 0); // Output BUFOE

        while (1)
        {
                if (pwmRun)
                {
                        int duration = 0;

                        if (state)
                        {
                                duration = (pwmPeriod * pwmDuty); // On duriation
                                if (duration > 0)
                                        pinctl::inst().set(bufoeFd, state);
                                state = 0;
                        }
                        else
                        {
                                duration = (pwmPeriod * (100 - pwmDuty));
                                if (duration > 0)
                                        pinctl::inst().set(bufoeFd, state);
                                state = 1;
                        }

                        usleep(duration);
                }
                else // Stop
                {
                        if (state)
                        {
                                state = 0;
                                pinctl::inst().set(bufoeFd, state);
                        }
                        usleep(100000);
                }
        }
}

pthread_t pwm_thread_id;

void InitPwmTask(void)
{
        pthread_attr_t attr;
        int status = pthread_attr_init(&attr);
        if (status != 0)
                printf("pthread_attr_init\n");

        status = pthread_create(&pwm_thread_id, &attr, &PwmTask, NULL);
        if (status != 0)
                printf("pthread create failed for Listen Task\n");

        pthread_attr_destroy(&attr);
}

int readRaw(int iFd, long lTmoMs)
{
        int iRetval = -1;
        struct timeval delay;
        int iStatus;
        char data;
        fd_set fd;

        if (lTmoMs)
        {
                delay.tv_sec = (long)(lTmoMs / 1000);         // How many seconds
                delay.tv_usec = (long)(lTmoMs % 1000) * 1000; // milliseconds expressed in uS
                FD_ZERO(&fd);
                FD_SET(iFd, &fd);
                iStatus = select(iFd + 1, &fd, (fd_set *)0, (fd_set *)0, &delay);
                if (FD_ISSET(iFd, &fd))
                {
                        iStatus = read(iFd, &data, 1);
                        if (iStatus > 0)
                        {
                                iRetval = (int)data;
                        }
                }
        }

        return iRetval;
}

char *readSerial(int iFd, int max, long lTmoMs)
{
        int iCnt = 0; // Number of bytes
        long value;
        char *pucMsg = (char *)malloc(max);

        memset(pucMsg, 0, max);
        iCnt = 0;

        while (1)
        {
                value = readRaw(iFd, lTmoMs);
                if (value >= 0)
                {
                        pucMsg[iCnt] = (char)value;
                        if ((iCnt == 0) && ((pucMsg[iCnt] == '\r') || (pucMsg[iCnt] == '\n')))
                        {
                                // Ignore leading \r or \n
                        }
                        else
                        {
                                if ((pucMsg[iCnt] == '\n') || (pucMsg[iCnt] == '\r'))
                                {
                                        pucMsg[iCnt] = 0;
                                        break; // Responses Terminate with \r or \n
                                }
                                iCnt += 1;
                        }

                        if (iCnt >= max) // Prevent overflows
                        {
                                break;
                        }
                }
                else
                {
                        break; // Break on Tmo
                }
        }

        if (iCnt == 0)
        {

                free(pucMsg);
                pucMsg = NULL;
        }

        return pucMsg;
}
