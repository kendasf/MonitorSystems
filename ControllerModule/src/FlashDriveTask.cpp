#include "main.h"
#include "CommProt.h"
#include "FileRoutines.h"
#include <pthread.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>  // gettimeofday
#include <sys/times.h> // gettimeofday
#include <stdarg.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "SystemUtilities.h"
#include "../Common/inc/timer.h"
#include "VMSDriver.h"

static TCB flashTaskTCB;

int flashDrivePresent = 0;
int MemoryCardCheckTimer = 0;
int MemoryCardStatus = MC_STATUS_OK_SDHC;
int MemoryCardSafeRemove = 0;
int MemoryCardDetectPin = 1;
unsigned char CardType;
int MemoryCardTotalSpace = 0;
int MemoryCardFreeSpace = 0;

extern int fileDirty;
extern bool HaveExternalFlash;


int isFlashDrivePresent(void)
{
   // Return 1 for /dev/sda and 2 for /dev/sdb, note that this finds only USB disks

   int retval = 0;
   printf("Checking for USB block device\n");
   char *resp = SysCmd("ls /dev/sd*1 2> /dev/null");
   if (resp != NULL)
   {
      if (strstr(resp, "No USB Found") != NULL)
      {
         retval = 0;
      }
      else
      {
         char *posn = strstr(resp, "/dev/sd");
         printf("RESP: %s %p\n", resp, resp);
         if (posn != NULL)
         {
            retval = ((int)posn[7] - (int)'a') + 1;
            printf("Device: %d\n", retval);
         }
      }
      free(resp);
   }

   return retval;
}

void SetTime();

unsigned char fsChanged = 0;
unsigned char fsInternal = 0;
void uSD_flash_monitor_init(void)
{
   unsigned char fsReady = 0;
   char *checkExtResp = NULL;
   printf("Checking for uSD card\n");
   char *diskList = SysCmd("lsblk | grep mmcblk0");   /* External flash is on MMC0 */ 
   if (NULL != diskList )
   {
      free(diskList);

      if( false == HaveExternalFlash )
      {
         printf("uSD attempting to mount at /mnt/ext\n\n");
         /* mount -t fsType device dir */
         // SysCmd("mount -t ext4 /dev/mmcblk0p3 /store/");    /* look for ext4 fs*/
         // SysCmd("mount -t f2fs /dev/mmcblk0p3 /store/");    /* look for f2fs fs*/
         // SysCmd("mount -t f2fs /dev/mmcblk1p3 /store/");    /* look for f2fs fs*/
         checkExtResp = SysCmd("mount /dev/mmcblk0p1 /mnt/ext/ -o rw,uid=0,gid=0,umask=000,dmask=000"); /* look for fat32 fs*/
      
         if( NULL == checkExtResp )  /* Null response means that the mount command worked */
         {
            printf("uSD mounted successfully\n\n");
            HaveExternalFlash = true;  // uSD card is present with goot directory structure
         }
         else
         {
            printf("uSD error mounting disk\n\n");
            HaveExternalFlash = false;
            return;
         }
         free(checkExtResp);
      }
      else
      {
         printf("uSD already mounted\n\n");
         return;
      }
      

      checkExtResp = SysCmd("ls /mnt/ext/");
      if (checkExtResp && strstr(checkExtResp, "Photo"))
      {
         printf("External flash has Photo dir\n");
      }  
      else  // uSD is present but bad structure
      {
         SysCmd("mkdir -p /mnt/ext/Photo");
      }
      free(checkExtResp);

      checkExtResp = SysCmd("rm -r /store/Photo");  /* delete existing link */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      // Update uSD with Photo Directory information
      checkExtResp = SysCmd("ln -sf /mnt/ext/Photo/ /store/Photo");  /* map Photo to external flash drive */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }
      
      checkExtResp = SysCmd("mv /storeInt/PhotoInt/* /store/Photo/");    /* Move internal data to uSD card */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      checkExtResp = SysCmd("ls /mnt/ext/");
      if (checkExtResp && strstr(checkExtResp, "VehicleCounts"))
      {
         printf("External flash has VehicleCounts dir\n");
      }
      else
      {
         SysCmd("mkdir -p /mnt/ext/VehicleCounts");
      }
      free(checkExtResp);

      checkExtResp = SysCmd("rm -r /store/VehicleCounts");  /* delete existing link */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      checkExtResp = SysCmd("ln -sf /mnt/ext/VehicleCounts/ /store/VehicleCounts");     /* Map to external Flash disk */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }
      
      checkExtResp = SysCmd("mv /storeInt/VehicleCountsInt/* /mnt/ext/VehicleCounts/");      /* Move internal data to uSD card */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      if (CanWriteMemoryCard() != 0)  /* USB must be present for logging */
      {
         int flashDriveRef = isFlashDrivePresent();
         char driveDes[10] = "/dev/sd_1";
         driveDes[7] = 'a' + (flashDriveRef - 1);
         printf("Mounting %s for debug\r\n", driveDes);
         MountDrive(driveDes, "/mnt/data");
      }
      else
      {
         printf("No USB flash disk ready for debug logging\r\n");
      }

      fsChanged = 1;
      fsInternal = 0;
   }
   else if( NULL == diskList && 0 == fsInternal  )
   {
      printf("No SD card present, Please insert SD card for proper operation, non-save mode selected\n");

      checkExtResp = SysCmd("rm -r /store/Photo");  /* delete existing link */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      checkExtResp = SysCmd("ln -s /storeInt/PhotoInt /store/Photo");  /* map Photo to internal flash drive */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      checkExtResp = SysCmd("rm -r /store/VehicleCounts");  /* delete existing link */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      checkExtResp = SysCmd("ln -sf /storeInt/VehicleCountsInt /store/VehicleCounts");  /* map Photo to internal flash drive */
      if( checkExtResp )
      {
         printf("Error - %s\n", checkExtResp);
         free(checkExtResp);
      }

      fsChanged = 1;
      fsInternal = 1;
   }

   if( 1 == fsChanged )
   {
      SysCmd("chmod 777 /store/Photo");
      SysCmd("chmod 777 /storeInt/PhotoInt");
      SysCmd("chmod 777 /store/VehicleCounts");
      SysCmd("chmod 777 /storeInt/VehicleCountsInt");

      if( true == HaveExternalFlash)
      {
         SysCmd("chmod 777 /mnt/ext/Photo");
         SysCmd("chmod 777 /mnt/ext/VehicleCounts");
      }

      fsChanged = 0;
   }
}


void *MemoryMonTask(void *argPtr)
{
   char *resp;
   bool copyComplete = false;
   bool driveWasPresent = false;
   unsigned long long flashDriveCheckTimer = GetTickCount(); // check every 30 seconds
   sched_param param;
   int res;
   pthread_attr_t threadAttrs;
   
   pthread_attr_init(&threadAttrs);

   param.sched_priority = -1;
   res = pthread_setschedparam(pthread_self(), SCHED_OTHER, &param);
   //res = pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);
   if( 0 != res )
   {
      strerror(errno);
   }

   CheckMemoryCard();  // Why??

   flashDrivePresent = isFlashDrivePresent();

   while (1)
   {
      sleep(1);
      pthread_yield();

      if ((flashDriveCheckTimer > 0) && (flashDriveCheckTimer < GetTickCount()))
      {
         printf("Checking for block devices\n\n");

         // Check to see if uSD card has been inserted
         uSD_flash_monitor_init(); 

         flashDriveCheckTimer = GetTickCount() + 10000; // check every 10 seconds
         flashDrivePresent = isFlashDrivePresent();   // 0, 1, 2
         if ( (flashDrivePresent > 0) && (false == copyComplete) )
         {
            
            char driveDes[10] = "/dev/sd_1";
            int validKey = 0;
            driveWasPresent = true;

            driveDes[7] = 'a' + (flashDrivePresent - 1);
            printf("USB Flash Drive %s has been inserted\r\n", driveDes);
            displayWait(); // Wait for Display Control

            VMSDriver_Off();
            MountDrive(driveDes, "/mnt/data");

            // Check for download key
            resp = SysCmd("/root/bin/diffFiles /root/vms_download.key /mnt/data/vms_download.key");
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
               DisplayFlashingCorners++;
               sleep(1);

               printf("Correct download key found\n");
               printf("Starting data move to microSD\n");
               printf("Moving system\n");
               MoveFile("/store/System", "/mnt/data/System", "Events");
               printf("Moving photo\n");
               MoveFile("/store/Photo", "/mnt/data/Photo", "raw");
               printf("Moving vehicle counts\n");
               MoveFile("/store/VehicleCounts", "/mnt/data/VehicleCounts", "bvs");
               //
               printf("Directory move complete, syncing filesystem for removal\n");
               Sync();

               copyComplete = true;

               sleep(2);
               DisplayFlashingCorners--;
            }
            else
            {
               int i;
               printf("No download key found\n");
            }

            resp = SysCmd("/root/bin/diffFiles /root/vms_update_rsa.key /mnt/data/vms_update_rsa.key");
            if (resp != NULL)
            {
               if (atoi(resp) == 1)
                  validKey = 1;
               free(resp);
               resp = NULL;
            }

            if( 1 == validKey)
            {
               resp = SysCmd("ls /mnt/data/update.vac");             
               if( NULL != resp )   /* An update file was found */
               {
                  free(resp);
                  SysCmd("cp /mnt/data/update.vac /root/update.vac");
                  
                  copyComplete = true;
                  
                  if (FileRoutines_autoconfFromCard("/root/update.vac") > 0)
                  {
                     printf("Configuration Updated\r\n");
                     unlink("/root/update.vac");
                     fileDirty = 1;  // Flag to force system to reset device info
                  }
               }
            }

            resp = SysCmd("umount /mnt/data");
            if (resp != NULL)
               free(resp);

            printf("Done\r\n");
            if (validKey)
               sleep(5);

            VMSDriver_Off();
            displayRelease(); // Release Display Control
         }
         else if ( 0 == flashDrivePresent && true == driveWasPresent)   // Change state
         {
            flashDrivePresent = 0;
            copyComplete = false;
            driveWasPresent = false;
            printf("Flash Drive has been removed\r\n");
         }
         else
         {
            printf("No USB or uSD actions performed\n\n");
         }

         // Admin can directly upload the update.vac to the /root folder
         resp = SysCmd("ls /root/update.vac 2> /dev/null ");             
         if( NULL != resp )   /* An update file was found */
         {
            free(resp);
            printf("Found update.vac in /root directory\n\n");      
            if (FileRoutines_autoconfFromCard("/root/update.vac") > 0)
            {
               printf("Configuration Updated\r\n");
               unlink("/root/update.vac");
               fileDirty = 1;  // Flag to force system to reset device info
            }
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

char *findSpace(char *data)
{
   while ((*data != 0) && (*data != ' '))
      data++;
   return data;
}

char *findNonSpace(char *data)
{
   while ((*data != 0) && (*data == ' '))
      data++;
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
   char *cardStatus = NULL;
   char *infoPtr = 0;
   char *cardDev;

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
      if (infoPtr != NULL)
         infoPtr = findNonSpace(++infoPtr);
      if (infoPtr != NULL)
      {
         char *percntPtr = strstr(infoPtr, "%");
         if (percntPtr != NULL)
            *percntPtr = 0;
         if (infoPtr != NULL)
            printf("%s\n", infoPtr);
         totalBlocks = atol(infoPtr);
         infoPtr = findSpace(++infoPtr);
         if (infoPtr != NULL)
            infoPtr = findNonSpace(++infoPtr);
         if (infoPtr != NULL)
            usedBlocks = atol(infoPtr);
         if (infoPtr != NULL)
            infoPtr = findSpace(++infoPtr);
         if (infoPtr != NULL)
            infoPtr = findNonSpace(++infoPtr);
         if (infoPtr != NULL)
            freeBlocks = atol(infoPtr);
         if (infoPtr != NULL)
            infoPtr = findSpace(++infoPtr);
         if (infoPtr != NULL)
            infoPtr = findNonSpace(++infoPtr);
         if (infoPtr != NULL)
            printf("%s\n", infoPtr);
         if (infoPtr != NULL)
            percentFull = atoi(infoPtr);

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
         printf("MMC card found\r\n");
         MemoryCardStatus = MC_STATUS_OK_MMC;
      }

      //		SetMemoryCardLED(LEDColor_Green);

      break;
   case 1:
      //		MemoryCardStatus = MC_STATUS_NOCARD;
      printf("No card found\r\n");
      //		SetMemoryCardLED(LEDColor_None);
      break;
   case 2:
   case 3:
   case 4:
      MemoryCardStatus = MC_STATUS_CARDERROR;
      printf("Card error\r\n");
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
   int retval = 0;
   
   if(0 != flashDrivePresent )
   {
      retval = 1;
   }

   return retval;
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
   printf("<5>Starting Flash drive monit\n\n");
   status = pthread_create(&flashTaskTCB.threadID, &attr, MemoryMonTask, (void *)&flashTaskTCB);

   status = pthread_attr_destroy(&attr);
   if (status != 0)
   {
      printf("ERROR: %d pthread_attr_destroy: %s - %d\r\n", status, strerror(errno), errno);
   }

   pthread_setname_np(flashTaskTCB.threadID, "FlashMon Thread");

   
}