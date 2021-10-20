#include "main.h"
#include "CommProt.h"
#include "FileRoutines.h"
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>	// gettimeofday
#include <sys/times.h>	// gettimeofday
#include <stdarg.h>
#include "SystemUtilities.h"
#include "../Common/inc/timer.h"
#include "VMSDriver.h"

static TCB flashTaskTCB;

int flashDrivePresent           = 0;
int MemoryCardCheckTimer = 0;
int MemoryCardStatus 		= MC_STATUS_OK_SDHC;
int MemoryCardSafeRemove 	= 0;
int MemoryCardDetectPin		= 1;
unsigned char CardType;
int MemoryCardTotalSpace	= 0;
int MemoryCardFreeSpace		= 0;

extern bool HaveExternalFlash;

int isFlashDrivePresent(void)
{
	// Return 1 for /dev/sda and 2 for /dev/sdb

	int retval = 0;
	char* resp = SysCmd("ls /dev/sd*1 2> /dev/null");
	if (resp != NULL)
	{
		if (strstr(resp, "No such file or directory") != NULL)
		{
			retval = 0;	
		}
		else
		{
			char* posn = strstr(resp, "/dev/sd");
			printf("RESP: %s %p\n", resp, resp);
			if (posn != NULL)
			{
				retval = ((int) posn[7] - (int) 'a')  + 1;
				printf("Device: %d\n", retval);
			}
		}
		free(resp);
	}

	return retval;
}

void SetTime();

void* MemoryMonTask(void* argPtr)
{
	unsigned long tickCnt = 0;
	unsigned long long flashDriveCheckTimer = GetTickCount(); // check every 30 seconds

#ifdef SIMULATOR_MODE
	while(true) {}
#endif

	CheckMemoryCard();

	flashDrivePresent = isFlashDrivePresent();

	while(1)
	{

		usleep(1000*1000); // Sleep a second
		tickCnt++;
		if (tickCnt > 60*60*6)
		{
			SetTime();
		}
	
		if ((flashDriveCheckTimer > 0) && (flashDriveCheckTimer < GetTickCount()))
		{
			flashDriveCheckTimer = GetTickCount() + 10000; // check every 10 seconds
			if ((flashDrivePresent == 0) && (flashDrivePresent = isFlashDrivePresent()))
			{
				char* resp;
				char driveDes[10] = "/dev/sd_1";
				int validKey = 0;
				

				driveDes[7] = 'a' + (flashDrivePresent - 1);
				printf("Flash Drive %d has been inserted\r\n", flashDrivePresent);
				displayWait(); // Wait for Display Control

				VMSDriver_Off();
				MountDrive(driveDes, "/mnt/data");

				// Check for download key
				resp = SysCmd("/root/bin/diffFiles /mnt/data/vms_download.key /root/vms_download.key");
				if (resp != NULL) 
				{
					if (atoi(resp) == 1)
						validKey = 1;
					free(resp);
					resp = NULL;
				}

				if (validKey)	
				{
					// Process Moving Flash Information
					DisplayFlashingCorners ++;
					usleep(1000*1000);

					printf("Matching download key\n");
					MoveFile("/store/System", "/mnt/data/System", "Events");
					MoveFile("/store/Photo", "/mnt/data/Photo", "raw");
					MoveFile("/store/VehicleCounts", "/mnt/data/VehicleCounts", "bvs");
					//
					Sync();

					usleep(2000*1000);
					DisplayFlashingCorners --;
				 }
				 else
				 {
					int i;
					printf("No Match download key\n");
					// todo ?!? prije bilo 5 blinkanja
				 }

				 resp = SysCmd("umount /mnt/data");
				 if (resp != NULL)
					free(resp);

				printf("Done\r\n");
				if (validKey)
					usleep(1000*5000);

				VMSDriver_Off();
				displayRelease(); // Release Display Control

			}
			else if ((flashDrivePresent != 0) && (isFlashDrivePresent()==0))
			{
				flashDrivePresent = 0;
				printf("Flash Drive has been removed\r\n");
			}
		}

		//
		// check if memory card was inserted/removed
		//
		UpdateMemoryCardStatus();

	}
}

//
// void UpdateMemoryCardStatus()
//
// Check if the card was inserted/replaced, or if the card is full.
//
void UpdateMemoryCardStatus()
{
	MemoryCardStatus = MC_STATUS_OK_SDHC;

	//
	// every 1 minute check if memory card is full
	//
	if (MemoryCardCheckTimer != RTC_MIN)
	{
		//
		// if the memory card is present and OK, check if it is full
		//
		CheckMemoryCardCapacity();

		MemoryCardCheckTimer = RTC_MIN;
	}

}

char* findSpace(char* data)
{
	while ((*data != 0) && (*data != ' ')) data++;
	return data;
}

char* findNonSpace(char* data)
{
	while ((*data != 0) && (*data == ' ')) data++;
	return data;
}

// void CheckMemoryCardCapacity()
//
// Checks the total size and available space of the memory card.
//
void CheckMemoryCardCapacity()
{
	long totalBlocks = 0;
	long freeBlocks = 0;
	long usedBlocks = 0;
	int percentFull = 100;
	char* cardStatus = NULL;
	char* infoPtr = 0;
	char* cardDev;

#ifdef SIMULATOR_MODE
	return;
#endif

	printf("CheckMemoryCardCapacity: %d\n", (int)HaveExternalFlash);

	if (HaveExternalFlash)
		cardStatus = SysCmd("df /mnt/ext");
	else
		cardStatus = SysCmd("df /store");

	printf("Card status: %s\n", cardStatus);
	
	if (((infoPtr = strstr(cardStatus, "/store")) != NULL) || ((infoPtr = strstr(cardStatus, "/mnt/ext")) != NULL))
	{
		infoPtr = strstr(cardStatus, "/dev");
		infoPtr = findSpace(infoPtr);
		if (infoPtr != NULL) infoPtr = findNonSpace(++infoPtr);
		if (infoPtr != NULL) 
		{
			char* percntPtr = strstr(infoPtr, "%");
			if (percntPtr != NULL) *percntPtr = 0;
			if (infoPtr != NULL) printf("%s\n", infoPtr);
			totalBlocks = atol(infoPtr);
			infoPtr = findSpace(++infoPtr);
			if (infoPtr != NULL) infoPtr = findNonSpace(++infoPtr);
			if (infoPtr != NULL) usedBlocks = atol(infoPtr);	
			if (infoPtr != NULL) infoPtr = findSpace(++infoPtr);
			if (infoPtr != NULL) infoPtr = findNonSpace(++infoPtr);
			if (infoPtr != NULL) freeBlocks = atol(infoPtr);
			if (infoPtr != NULL) infoPtr = findSpace(++infoPtr);
			if (infoPtr != NULL) infoPtr = findNonSpace(++infoPtr);
			if (infoPtr != NULL) printf("%s\n", infoPtr);
			if (infoPtr != NULL) percentFull = atoi(infoPtr);

			printf("/Store: %ld %ld %ld %d\r\n", totalBlocks, usedBlocks, freeBlocks, percentFull);
		}

	// get the total size, in 1K byte blocks
//	totalBlocks = mmc.DataClusCnt * (fat_clus_size() / 512);

	// get free space in 512 byte blocks so SDHC can be used
//	freeBlocks = fat_free_clus() * (fat_clus_size() / 512);

	// copy to global variables 
	//MemoryCardTotalSpace = totalBlocks / 2; // convert from 512-byte blocks to Kbyte blocks
	//MemoryCardFreeSpace = freeBlocks / 2;
	MemoryCardTotalSpace = totalBlocks; //  Size in 1 K
	MemoryCardFreeSpace = freeBlocks;


	// if there is less than 512 kB free, we declare the card "FULL" and further writing is disabled
	if (freeBlocks < 1024)
	{
		MemoryCardStatus = MC_STATUS_FULL;
		printf("Status full\r\n");
	}
	}
	else
	{
		MemoryCardTotalSpace = 0;
		MemoryCardFreeSpace = 0;
	}
	if (cardStatus != NULL)
		free(cardStatus);
}


//
// void CheckMemoryCard(void)
//
// Updates memory card status.
//
void CheckMemoryCard()
{
	//
	// check the status of the card inserted in the slot
	//
	switch (finit())
	{
	case 0:
		if (CardType == CARD_SD)
		{
			printf("SD card found\r\n");
			MemoryCardStatus = MC_STATUS_OK_SD;
		}
		else if (CardType == CARD_SDHC)
		{
			printf("SDHC card found\r\n");
			MemoryCardStatus = MC_STATUS_OK_SDHC;
		}
		else
		{
			printf( "MMC card found\r\n");
			MemoryCardStatus = MC_STATUS_OK_MMC;
		}

//		SetMemoryCardLED(LEDColor_Green);

		break;
	case 1:
//		MemoryCardStatus = MC_STATUS_NOCARD;
		printf( "No card found\r\n");
//		SetMemoryCardLED(LEDColor_None);
		break;
	case 2:	
	case 3:	
	case 4:	
		MemoryCardStatus = MC_STATUS_CARDERROR;
		printf( "Card error\r\n");
//		SetMemoryCardLED(LEDColor_Red);
		break;
	}

	//
	// check write-protect pin status
	//
//	if ((MemoryCardStatus != MC_STATUS_NOCARD) &&
//		(MemoryCardStatus != MC_STATUS_CARDERROR))
//		{
//			if ((FIO2PIN & (1 << 5)) != 0)
//			{
//				MemoryCardStatus = MC_STATUS_READONLY;
//			}
//		}

	//
	// check card capacity
	//
	CheckMemoryCardCapacity();
	MemoryCardCheckTimer = RTC_MIN; // reset the recheck timer
	//
	// reset safe remove flag
	//
}

//
// int CanWriteMemoryCard(void)
//
// Checks if memory card can be written.
//
int CanWriteMemoryCard(void)
{
	return 1;
}

void CreateMemoryTask(int localPort)
{
   pthread_attr_t attr;
   int i;
   int stack_size = 2048;
   int status;
	
    status = pthread_attr_init(&attr);
    if (status != 0)
	{
        printf("ERROR pthread_attr_init\r\n");
		return;
	}

#if 0
    if (stack_size > 0) {
        status = pthread_attr_setstacksize(&attr, stack_size);
        if (status != 0)
            printf("%s ERROR: %d pthread_attr_setstacksize: %s - %d\r\n", __FUNCTION__, status, strerror(errno), errno);
    }
#endif

	flashTaskTCB.localPort = localPort;
	status = pthread_create(&flashTaskTCB.threadID, &attr,  MemoryMonTask, (void*) &flashTaskTCB);

	status = pthread_attr_destroy(&attr);
    if (status != 0)
	{
        printf("ERROR: %d pthread_attr_destroy: %s - %d\r\n", status, strerror(errno), errno);
	}

}
