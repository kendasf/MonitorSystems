#include <sys/types.h> 
#include <sys/wait.h> 
#include <string.h>    //strlen 
#include <sys/socket.h>    //socket 
#include <arpa/inet.h> //inet_addr 
#include <stdio.h>				// printf, sprintf 
#include <errno.h>				// errno 
#include <string.h> 			// memset, strlen, strstr, ... etc 
#include <unistd.h> 			// usleep, unlink 
#include <stdlib.h> 			// atioi 

#include <termios.h>			// B115200, CS8, CLOCAL, CREAD, IGNPAR, VMIN, VTIME, TCIFLUSH, TCSANOW 
#include <pthread.h> 
#include <fcntl.h>				// O_RDWR, 0_NOCTTY, 0_NONBLOCK 
#include "main.h" 
#include "SystemUtilities.h" 
#include "../Ethernet/UdpInterface.h" 
#include "cameraFuncts.h" 
#include "../Common/inc/timer.h"
#include "VMSDriver.h"

#include <thread>
#include <chrono>

extern int PWMDutyCycle;

static int delay_image[2] = {7, 7};
static int initialDelay = 0;

static unsigned long long busy_timer = 0;
static unsigned long long ping_timer = 0;
static unsigned long long busy_duration = 0;
static unsigned long long messageDelayTimer = 0;

static char delayedMsg[80];
static int messageDelayMs = 0;
static char* configuration = NULL;
static int lastScreenSpeed = 0;

static char configFile[38000]; // The physical store location
static int configFileLength = 0;

static int firstTime = 1;


typedef enum 
{ 
	EVENT_MANUAL_SET1, 
	EVENT_PUSH,
	EVENT_ZOOM_STOP,
	EVENT_DO1_0,
	EVENT_DO1_1,
	EVENT_CONFIG_EVT1,
	EVENT_CONFIG_EVT2,
	EVENT_CONFIG_DAYNIGHT,
	EVENT_FTPCLIENT_SET,
	EVENT_DAY_SETTING,
	EVENT_NIGHT_SETTING, 
	EVENT_END = 255
} CAMERA_EVENT_ENUM; 

#define FTP_CONFIG_FMT "GET /cgi-bin/encoder?USER=admin&PWD=123456&FTP_SERVER=%s,ftpclient,ftpclient,21,0,30000 HTTP/1.1"

char* cameraEvent[] = 
{ 
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&EVENT_MANUAL=1 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&EVENT_PUSH HTTP/1.1", 
        "GET /cgi-bin/encoder?USER=Admin&PWD=123456&ZOOM=STOP HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&SET_DO=1,0 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&SET_DO=1,1 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&EVENT_CONFIG=1,1,1234567,00:00,24:00,DISK_REMOVE,IMG1 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&EVENT_CONFIG=2,1,1234567,00:00,24:00,DI1,IMG2 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&VIDEO_DAYNIGHT_MODE=AUTO&DAY_GAIN_THD=40&VIDEO_DN_IRLED=1 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&FTP_SERVER=10.0.0.35,ftpclient,ftpclient,21,0,10000 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&VIDEO_DAYNIGHT_MODE=DAY&DAY_GAIN_THD=75&VIDEO_DN_IRLED=1 HTTP/1.1",
	"GET /cgi-bin/encoder?USER=admin&PWD=123456&VIDEO_DAYNIGHT_MODE=NIGHT&DAY_GAIN_THD=75&VIDEO_DN_IRLED=1 HTTP/1.1",
	NULL
};

unsigned char configList[] = 
{
	EVENT_CONFIG_EVT1,
	EVENT_CONFIG_EVT2,
//	EVENT_CONFIG_DAYNIGHT,
	EVENT_MANUAL_SET1,
//	EVENT_FTPCLIENT_SET,
	EVENT_END
};

TCB taskTCB;

int movePic(char* from, char* to)
{
	char* cmd = (char*) malloc(512);
	char* resp;
	memset(cmd, 0, 512);
	snprintf(cmd, 512, "mv %s %s", from, to);
	 resp = SysCmd(cmd);
	if (resp != NULL)
	{
		free(resp); 
	} 
	free(cmd);

	return 0;
}

int split(char* data, char delim, char** params, int max)
{
	char* paramPtr = data;
	int cnt = 0;
	int i;
	for (i = 0; i<max; i++)
		params[i] = NULL;
	do 
	{
		params[cnt++] = paramPtr;
		paramPtr = strchr(paramPtr, delim);
		if (paramPtr != NULL)
		{
			*paramPtr = 0;// null out delimiter
			paramPtr++;
		}
	} while (paramPtr != NULL);
	return cnt;
}

void SendCommand(char* hostIP, int event)
{
	sendCameraCommand(hostIP, cameraEvent[event]);
}

void ConfigureCamera(char* hostIP)
{
	int i;
	for (i = 0; configList[i] != EVENT_END; i++)
	{
		sendCameraCommand(hostIP, cameraEvent[configList[i]]);	
	}
}

void GetMyIpAddress(char* store)
{
	char* resp = SysCmd("ifconfig eth0");
	if (resp != NULL)
	{
		char* posn = strstr(resp, "inet addr:");
		if (posn != NULL)
		{
			int i;
			posn = strchr(posn, ':') + 1;
			for (i = 0; posn[i] != ' '; i++)
				store[i] = posn[i];
			store[i] = 0;
		}
		
		free(resp);
	}
}

int entryParamLen(char* entry)
{
	char* begin = strchr(entry, '\'') + 1;
	char* end = strchr(begin, '\'');
	int len = (long) end - (long) begin;
	if (len > 80)
		len = 80;
	return len;
}

char* GetConfigEntry(char* search)
{
	char* result = NULL;
	if (configuration != NULL)
	{
		char* posn = strstr(configuration, search); // Get position in the list
		if (posn != NULL)
		{
			int len = entryParamLen(posn);
			void* begin = strchr(posn, '\'') + 1;
			result = (char*) malloc(len+1);
			result[len] = 0;
			memcpy(result, begin, len);
		}	
	}
	return result;
}

int paramCount(char* entry)
{
	unsigned int i;
	int count = 1; // 1 more value than commas
	for (i = 0; i < strlen(entry); i++)
	{
		if (entry[i] == ',')
		{
			count++;
		}
	}
	
	return count;
}

void ConfigureFtpServer(char* hostIP, char* ipaddr)
{
	char* cmd = (char*) malloc(strlen(FTP_CONFIG_FMT) + strlen(ipaddr) + 1);
	sprintf(cmd, FTP_CONFIG_FMT, ipaddr);
	
	sendCameraCommand(hostIP, cmd);
	
	free(cmd);
}

// Get the integer value of a param from a data entry
long getIntValueParam(char* entry, int paramNum)
{
	int paramVal = 0;
	char* param = entry;
	int count = paramCount(entry);
	if (paramNum <= count)
	{
		int i;
		for (i = 0; (param != NULL) && (i<paramNum); i++)
			param = strchr(param, ',') + 1;

		if (param != NULL)
		{
			paramVal = atol(param);
		}
	}
	return paramVal;
}

int paramLen(char* param)
{
	int len;
	for (len = 0; (len < (int)strlen(param)) && (param[len] != ',') && (param[len] != 0); len++); // all the work in cond

	return len;		
}

int getFileCount(char* directory)
{
	int retval = 0;
	char* resp = SysCmdf("ls %s | wc", directory);
	if (resp != NULL)
	{
		retval = atoi(resp);
		free(resp);
	}
	return retval;
}

// Get the integer value of a param from a data entry
char* getStrParam(char* entry, int paramNum)
{
	char* paramVal = NULL;
	char* param = entry;
	int count = paramCount(entry);
	if (paramNum <= count)
	{
		int i;
		for (i = 0; (param != NULL) && (i<paramNum); i++)
			param = strchr(param, ',') + 1;

		if (param != NULL)
		{
			int len = paramLen(param);
			paramVal = (char*) malloc(len + 1);
			paramVal[len] = 0;
			memcpy(paramVal, param, len);
		}
	}
	return paramVal;
}

// Typical format
//EVENT_RSPIMG1='7,0,7,0,VMSVideo1_%YYYY%MM%DD_%hh%mm%ss,~/Photo,,,1' 


int getUploadTime(char* uploadEvent)
{
	int uploadTime = 0;
	char* params[9];
 	char* entry = GetConfigEntry(uploadEvent);
	if (entry != NULL)
	{
		uploadTime = (int) getIntValueParam(entry, 2);
		free(entry);
	}
	return uploadTime;	
}

void SendDelayedMessage(void)
{ 
	char* params[5];
	int paramCnt = split(delayedMsg, ',', params, 5);	
	int speed = atoi(params[2]);
	int delayMs = atoi(params[3]);
	int image = atoi(params[4]);	

	messageDelayMs = 0;
	messageDelayTimer = 0;

	Sync();

//	printf("Send Delay Event: spd %d image %d\n", speed, image);
	//if (image == 1)
	{
//		SendCommand(params[1], EVENT_MANUAL_SET1);
//		usleep(1000);
		SendCommand(params[1], EVENT_PUSH);
		busy_duration = (unsigned long long) delay_image[0] * 1000;
	}
	/*else
	{
		// Toggle GPIO to force shot
		busy_duration = (unsigned long long) delay_image[1] * 1000;
		SendCommand(params[1], EVENT_DO1_0);
		usleep(1000*1000);
		SendCommand(params[1], EVENT_DO1_1);
	}*/

	busy_timer = GetTickCount();
//	printf("Busy Timer Set: %ld\n", (long) busy_duration);
}

void ProcessMessage(char* msg)
{
	char* params[5];
	char* msgOrig = (char*) malloc(strlen(msg)+2);
	strcpy(msgOrig, msg);	

	split(msg, ',', params, 5);	
	
	if (strncmp(params[0], "SNAP",4) == 0) 
	{ 
		memset(delayedMsg, 0, sizeof(delayedMsg));
		strcpy(delayedMsg, msgOrig);
//        	printf("MSG: %s\n", delayedMsg);

		int speed = atoi(params[2]);
		int delayMs = atoi(params[3]);
		int image = atoi(params[4]);	
//		printf("PictureCmd: spd %d %d img\n", speed, image);

		messageDelayMs = delayMs;
		messageDelayTimer = GetTickCount();

//		printf("Pre Delay = %d\n", delayMs);
		updateSpeed(params[1], speed);
		
		if (messageDelayMs == 0)
			SendDelayedMessage();
	}
	else if (strncmp(params[0], "SPEEDADJ", 8) == 0)
	{
	       int speed = atoi(params[2]);
         	updateSpeed(params[1], speed);
		lastScreenSpeed = speed;
	} 
	else if (strncmp(params[0], "CONFIG", 6) == 0)
	{
		ConfigureCamera(params[1]);
	}
	else if (strncmp(params[0], "DAYTIME", 7) == 0)
	{
//		printf("Switch to daytime\n");
		sendCameraCommand((char*) CAMERA_IP_ADDR, "GET /cgi-bin/encoder?USER=admin&PWD=123456&PTZ_PRESET_GO=1 HTTP/1.1"); 
		SendCommand((char*) CAMERA_IP_ADDR, EVENT_DAY_SETTING); 
		firstTime = 1;
	}
	else if (strncmp(params[0], "NIGHTTIME", 9) == 0)
	{
//		printf("Switch to night time\n");
		sendCameraCommand((char*) CAMERA_IP_ADDR, "GET /cgi-bin/encoder?USER=admin&PWD=123456&PTZ_PRESET_GO=2 HTTP/1.1"); 
		SendCommand((char*) CAMERA_IP_ADDR, EVENT_NIGHT_SETTING); 
		firstTime = 1;
	}
	else if (strncmp(params[0], "IMAGECONFIG", 6) == 0) 
	{
	}
	else 
	{
//		printf("UNSUPPROTED EVENT: %s\n", params[0]);
	}
	free(msgOrig);
}

int ping(char* hostIP)
{
	int retval = 0;
	char* resp = SysCmdf("ping -c 1 -w 1 %s", hostIP);
	if (resp != NULL)
	{
		char* posn = strstr(resp, "received");
		if (posn != NULL)
		{
			int i;
			posn -= 2;
			for (i = 0; i<5; i++, posn--)
			{
				if (*posn == ' ')
				{
					retval = atoi(&posn[1]);
					break;
				}
			}	
		}
		free(resp);
	}
	return retval;
} 


void WriteConfigFile(char* fname)
{
	FILE* fp = fopen(fname, "w");
	if (fp != NULL)
	{
		int i;
		int size = 1024;
		for (i = 0; i<configFileLength; i+=size)
		{
			int written = fwrite(&configFile[i], size, 1, fp);	
			if (written != 1)
			{
				break;
			}
			if ((configFileLength - i) < size)
				size = configFileLength - i;
		}
		fclose(fp);
	}
}

void GetCameraConfig(char* hostIP)
{
	int length = sizeof(configFile);
	char* resp;
	memset(configFile, 0, sizeof(configFile));
	resp = queryCmd(hostIP, "GET /cgi-bin/update?USER=admin&PWD=123456&CONFIG_GET HTTP/1.1", 
			configFile, &length);
	if (resp != NULL)
	{
		configuration = resp;
		configFileLength = length + 1;
		WriteConfigFile("/root/cameraConfig.dat");
	}
}


void* cameraTask(void* argPtr)
{
	TCB *args = (TCB*) argPtr;
	int isize;
	int fd1 = -1;
	int iFdx = -1;
	int hSock = -1;
	int iSel;
	fd_set fdr, fde;
	unsigned char csum;
	unsigned char *pCsum;
	long lTmoMs = 1000;
	struct timeval delay, origDelay;
	int abort = 0;
	char *postMsg = NULL;
	
	long lostPing = 0;
	char myIp[17] = "";
	//int lastFileCount = 0;
	int pingCount = 0;
	unsigned long idleCount = 0;


	while (hSock == -1)
	{
		hSock = GetUdpServerHandle(args->localPort);
		if (hSock < 0)
		{
			sleep(1);
		}
	}
	
	origDelay.tv_sec = (long) (lTmoMs/1000);           // How many seconds
	origDelay.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in uS

	FD_ZERO(&fdr);
	FD_ZERO(&fde); // error fd
	iFdx = hSock + 1;

	ping_timer = 0;
	//lastFileCount = getFileCount("/store/Photo");
	//printf("Photo Files: %d\n", lastFileCount);

	while (abort == 0)
	{
		while (1)
		{
			FD_SET(hSock, &fdr);
			FD_SET(hSock, &fde);
			delay = origDelay;
			
			iSel = select(iFdx, &fdr, 0, &fde, &delay); // Select for both Cmd Pipe and For IO
			if (iSel < 0)
				printf("ERROR: %s\n", strerror(errno));
			if (iSel == 0)
				idleCount++;
			else if (iSel > 0)
				idleCount = 0;

			if ((iSel > 0) && (FD_ISSET(hSock, &fdr)))
			{
				int len;
				postMsg = udpRead(hSock, &len, 50);
				if (postMsg != NULL)
				{
					printf("RXM: %s\n", postMsg);
					if (strstr(postMsg, "ABORT") != NULL)
					{
						abort = 1;
						break; // Abort While Loop
					}
					else
					{
						ProcessMessage(postMsg); 
					}
					free(postMsg);
					postMsg = NULL;
				}
			}

			if ((busy_timer != 0) && (TicksElapsed(busy_timer) > busy_duration))
			{
//				printf("Ticks Elapsed: %ld\n", (long) TicksElapsed(busy_timer));
				busy_timer = 0;
	//			printf("Busy Expired\n");
				udpSendLocal(hSock, args->arg1, "SNAP", (int) 4);
			}

			if (iSel == 0) // Timeout
			{
				// Ping Timeout
				pingCount++;
				if (idleCount > 60*30) // 30 minutes idle
				{
					char* dispTime;
					idleCount = 0;
					firstTime = 1;
					SetTime();
					dispTime = SysCmd("date");
					if (dispTime != NULL)
					{
//						printf("Date: %s\r\n", dispTime);
						free(dispTime);
					}
					
				}

				if ((ping_timer == 0) || (TicksElapsed(ping_timer) > 15000) || (pingCount > 120))
				{
					ping_timer = GetTickCount();
//					printf("Ping: ");
					if (pingCount > 120)
						firstTime = 1;
					pingCount = 0;
	
					int cameraPing = ping((char*) CAMERA_IP_ADDR);
					if (cameraPing)
					{
						// printf("present\n");
						if (firstTime)
						{
							char* param = NULL;
 							char* entry = NULL;
							firstTime = 0;
							// printf("Camera Alive\n");
							GetMyIpAddress(myIp);
							GetCameraConfig((char*) CAMERA_IP_ADDR); 
	
							setCameraTime((char*) CAMERA_IP_ADDR);
							ConfigureCamera((char*) CAMERA_IP_ADDR);
	
							delay_image[0] = getUploadTime("EVENT_RSPIMG1");
							// printf("Image delay 0 = %d\n", delay_image[0]);
							delay_image[1] = getUploadTime("EVENT_RSPIMG2");
							// printf("Image delay 1 = %d\n", delay_image[1]);
							if (delay_image[0] <= 0)
									delay_image[0] = 8;
								if (delay_image[1] <= 0)
									delay_image[1] = 8;

								entry = GetConfigEntry("EVENT_FTPD");
							if (entry != NULL)
							{
								param = getStrParam(entry, 0);	
								if (param != NULL)
								{
								//	printf("Param: %s\n", param);
								//		printf("MyIP: %s\n", myIp);
									if (strcmp(myIp, param) != 0)
									{
										ConfigureFtpServer(CAMERA_IP_ADDR, myIp);

									}
									free(param);
								}
								free(entry);
							}
							if (PWMDutyCycle > 30)
							{
								sendCameraCommand((char*) CAMERA_IP_ADDR, "GET /cgi-bin/encoder?USER=admin&PWD=123456&PTZ_PRESET_GO=1"); 
								SendCommand((char*) CAMERA_IP_ADDR, EVENT_DAY_SETTING); 
							} 
							else
							{
								sendCameraCommand((char*) CAMERA_IP_ADDR, "GET /cgi-bin/encoder?USER=admin&PWD=123456&PTZ_PRESET_GO=2"); 
								SendCommand((char*) CAMERA_IP_ADDR, EVENT_NIGHT_SETTING); 
							}
							// printf("Configuration Completed\n");

						} 
					} 
					else
					{
						// Actions to take for missing ping?	
						// printf(" no response\n");
						//	FlashCaution(2, 500);
						firstTime = 1;
					}

					// Check for proftpd
					{
						char* resp = SysCmd("ps -ef | grep proftpd: | grep accepting");
						if (resp != NULL)
						{
							if (strstr(resp, "(accepting ") == NULL)
							{
								

								char* result = SysCmd("killall -9 proftpd");
								if (result != NULL)
								{

//									printf("Killing proftpd: \n%s\n", result);
									free(result);
								}

								usleep(1000*1000);

								result = SysCmd("/usr/sbin/proftpd");
								if (result != NULL)
								{

//									printf("Starting proftpd: \n%s\n", result);
									free(result);
								}

							}
							free(resp);
						}
					}
				   }
			   }

			   if ((messageDelayTimer != 0) && (TicksElapsed(messageDelayTimer) >= (unsigned long long)messageDelayMs))
			   {
				SendDelayedMessage();
				messageDelayMs = 0;
			   }
		}
	}
	
	return 0; // This is unreachable as the task will remain active all the time
}

void RequestPicture(int speed, char* fname)
{
	
}

void CreateCameraTask(int localPort, int reportPort)
{
	pthread_attr_t attr;
	int i;
	int stack_size = 2048;
	int status;

	status = pthread_attr_init(&attr);
	if (status != 0) {
//		printf("ERROR pthread_attr_init: %d - %s\r\n", errno, strerror(errno));
		return;
	}

	if (stack_size > 0) {
		status = pthread_attr_setstacksize(&attr, stack_size);
		if (status != 0)
			printf("%s ERROR: %d pthread_attr_setstacksize: %s - %d\r\n", __FUNCTION__, status, strerror(errno), errno);
	}

	taskTCB.localPort = localPort;
	taskTCB.arg1 = reportPort;
	status = pthread_create(&taskTCB.threadID, &attr, cameraTask, (void*) &taskTCB);

	status = pthread_attr_destroy(&attr);
	if (status != 0) {
		printf("ERROR: %d pthread_attr_destroy: %s - %d\r\n", status, strerror(errno), errno);
	}

}
