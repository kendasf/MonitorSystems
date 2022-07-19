#include "../Common/inc/uart.h"
#include "main.h"
#include "radar.h"
#include "CommProt.h"
#include "FileRoutines.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <cmath>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include "../Common/inc/timer.h"
#include "../Common/inc/pinctl.h"

#include "../Ethernet/UdpInterface.h"

/*
TODO::
 - lock/mutex na citanje threada
 - makni te neke portove
 - kad se promijeni baud rate prema radaru kroz protokol, to ne valja
*/

static TCB radarTaskTCB;
static uart_ptr radar_uart;
static int RADAR_DEBUG = 0;

#define RADAR_POWER  GPIO(2,1)  // P8.18 Active High Radar Power

#define MAX_RADAR_SENTENCE_LENGTH	512
char Radar_IncomingSentence[MAX_RADAR_SENTENCE_LENGTH];
unsigned int Radar_IncomingSentenceLength = 0;

char Radar_DebugData[2048];
unsigned int Radar_DebugDataLength = 0;

RadarProtocolE SelectedProtocol;
unsigned int LastSpeed = 0;
unsigned long long LastSpeedTime = 0;
unsigned int LastDisplayedSpeed = 0;
unsigned long LastDisplayedSpeedTime = 0;
int Sensitivity = 7;
int LastIntensity = 0;
unsigned long long LastIntensityTime = 0;

static DeviceInfoS DeviceInfo;

extern int NumAnimationFrames;
extern int AnimationFrameLengthMs;


uart_ptr Radar_Init(int baudRate, RadarProtocolE protocol, int sensitivity)
{
	char request[64];
	uart_ptr u = uart_ptr(new uart(uart::get_uart_path(uart::radar), baudRate));
	SelectedProtocol = protocol;
	Radar_IncomingSentenceLength = 0;
	Sensitivity = sensitivity;
	
	sprintf(request, "\r\n#set_sensitivity=%d\r\n", sensitivity);
	u->write(request);
	printf(">%s\r\n", request);
	usleep(100000);
	u->write(request);
	usleep(100000);
	LastDisplayedSpeedTime = GetTickCount();

	return u;
}

int Radar_AutoConf()
{
	int i;
	char request[64];
	static int baudRates[] = {9600, 38400, 57600, 115200, -1};

	FileRoutines_readDeviceInfo(&DeviceInfo);

	for(i = 0; baudRates[i] != -1; i++)
	{
		uart_ptr u = uart_ptr(new uart(uart::get_uart_path(uart::radar), baudRates[i]));
		sprintf(request, "\r\n#set_baud_rate=%d\r\n", DeviceInfo.radarBaudRate);
		u->write(request);
		printf(">%s\r\n", request);
		usleep(100000);
		u->write(request);
		usleep(100000);
	}

	radar_uart = uart_ptr(new uart(uart::get_uart_path(uart::radar), DeviceInfo.radarBaudRate));

	sprintf(request, "\r\n#set_out_freq=20\r\n");
	radar_uart->write(request);
	printf(">%s\r\n", request);
	usleep(100000);
	radar_uart->write(request);
	usleep(100000);

	if (DeviceInfo.radarProtocol == 0)
		sprintf(request, "\r\n#set_proto=nmea\r\n");
	else if (DeviceInfo.radarProtocol == 1)
		sprintf(request, "\r\n#set_proto=ascii64\r\n");
	else
		sprintf(request, "\r\n#set_proto=nmea\r\n");
	radar_uart->write(request);
	printf(">%s\r\n", request);
	usleep(100000);
	radar_uart->write(request);
	usleep(100000);

	sprintf(request, "\r\n#set_units=mph\r\n");
	radar_uart->write(request);
	printf(">%s\r\n", request);
	usleep(100000);
	radar_uart->write(request);
	usleep(100000);

	radar_uart->purge_in_buffers();

	sprintf(request, "\r\n#set_units=mph\r\n");
	radar_uart->write(request);
	printf(">%s\r\n", request);
	usleep(100000);
	radar_uart->write(request);
	usleep(100000);
	
	sprintf(request, "\r\n#set_direction=in\r\n");
	radar_uart->write(request);
	printf(">%s\r\n", request);
	usleep(100000);
	radar_uart->write(request);
	usleep(100000);

	sprintf(request, "\r\n#set_sensitivity=%d\r\n", DeviceInfo.sensitivity);
	radar_uart->write(request);
	printf(">%s\r\n", request);
	usleep(100000);
	radar_uart->write(request);
	usleep(100000);

	// check if radar sent back response
	unsigned long long startTime = GetTickCount();

	while(GetTickCount() - startTime < 2000)
	{
		char ch[2];
		ch[1] = 0;

		if (radar_uart->read(ch))
		{
			if (*ch == '#')
				return 1;
			if (*ch == '\r')
				return 1;
			if (*ch == '\n')
				return 1;
		}
	}

	radar_uart->purge_in_buffers();
	
	return 0;	
}

void Radar_ReceiveData(DeviceInfoS *pDeviceInfo)
{
	char ch;
	while (radar_uart->read(&ch))
	{
		Radar_ReceiveByte(ch, pDeviceInfo);
	}
}

void Radar_ReceiveByte(unsigned char byte, DeviceInfoS *pDeviceInfo)
{
	switch (SelectedProtocol)
	{
	case Radar_ProtoSpecial28:
		Radar_ReceiveByte_Special28(byte, pDeviceInfo);
	break;
	case Radar_ProtoASCII64:
		Radar_ReceiveByte_ASCII64(byte, pDeviceInfo);
	break;
	case Radar_ProtoNMEA:
		Radar_ReceiveByte_NMEA(byte, pDeviceInfo);
	break;
	case Radar_ProtoNone:
	break;
	}

	if (RADAR_DEBUG)
	{
		Radar_DebugData[Radar_DebugDataLength] = byte;
		Radar_DebugDataLength ++;
		if ((byte == 0x0d) || (Radar_DebugDataLength == 2048))
		{
			if (RADAR_DEBUG == 1)
				printf("R:%s", Radar_DebugData);
//				FileRoutines_SaveDebugInfo(Radar_DebugData, Radar_DebugDataLength);
			memset(Radar_DebugData, 0, sizeof(Radar_DebugData));
			Radar_DebugDataLength = 0;
		}
	}

}

void Radar_ReceiveByte_Special28(unsigned char byte, DeviceInfoS *pDeviceInfo)
{
	int i;
	if ((Radar_IncomingSentenceLength == 0) && (byte != 0x02))
		return;

	Radar_IncomingSentence[Radar_IncomingSentenceLength] = byte;
	Radar_IncomingSentenceLength ++;
	if (Radar_IncomingSentenceLength >= MAX_RADAR_SENTENCE_LENGTH)
	{
		Radar_IncomingSentenceLength = 0;
		return;
	}

	if (byte == 0x03)
	{
		int max = 0;
		int maxInRange = 0;
		int maxSpeed = pDeviceInfo->maxDisplaySpeed;
		if (pDeviceInfo->unitType == 1)
			maxSpeed = maxSpeed * 10000 / 16093; // Convert to MPH

		for(i = 1; i < (Radar_IncomingSentenceLength-1); i+= 3)
		{
			unsigned char speed; //dir, mag;
			speed = Radar_IncomingSentence[i]; 
			// dir = Radar_IncomingSentence[i + 1];
			// mag = Radar_IncomingSentence[i + 2];

			if ((speed > max) ) //&& ((dir == 0x00) || (dir == 0x01))) TODO
			{
				max = speed;
			}

			if ((speed <= maxSpeed) && (speed > maxInRange) ) //&& ((dir == 0x00) || (dir == 0x01))) TODO
			{
				maxInRange = speed;
			}
		}

		if (maxInRange != 0)
		{		
			LastSpeed = maxInRange;
			LastSpeedTime = GetTickCount();
		}
		else if (max != 0)
		{
			LastSpeed = max;
			LastSpeedTime = GetTickCount();
		}

		Radar_IncomingSentenceLength = 0;
	}
}

void Radar_ReceiveByte_ASCII64(unsigned char byte, DeviceInfoS *pDeviceInfo)
{
	Radar_IncomingSentence[Radar_IncomingSentenceLength] = byte;
	Radar_IncomingSentenceLength ++;
	if (Radar_IncomingSentenceLength >= MAX_RADAR_SENTENCE_LENGTH)
	{
		Radar_IncomingSentenceLength = 0;
		return;
	}

	if (byte == 0x0D)
	{
		char dir;
		int speed;

		dir = Radar_IncomingSentence[0];
		speed = (Radar_IncomingSentence[1] - '0') * 100 + 
				(Radar_IncomingSentence[2] - '0') * 10 + 
				(Radar_IncomingSentence[3] - '0');

		if (((dir == '?') || (dir == '+') || (dir == '-')) &&
			(speed > 0))
		{
			LastSpeed = speed;
			LastSpeedTime = GetTickCount();
		}

		Radar_IncomingSentenceLength = 0;
	}
}

int Radar_CheckNMEAChecksum(char *pSentence)
{
 	int i;
	unsigned char checksum = 0;
	char b[3];

	for(i = 1; (pSentence[i] != '\0') && (pSentence[i] != '*'); i++)
	{
		checksum ^= (unsigned char)(pSentence[i]);
	}

	HexToString(&checksum, b, 1);

	return (strncmp(pSentence + i + 1, b, 2) == 0);
}

void Radar_ReceiveNMEASentence_RDTGT(char *pSentence, DeviceInfoS *pDeviceInfo)
{
	char *pToken;
	char infoText[50];
	int speed, level;

	if (!Radar_CheckNMEAChecksum(pSentence))
		return;

	pToken = strtok(pSentence, ",");
	if (!pToken)
		return;
	pToken = strtok(NULL, ",");
	if (!pToken)
		return;

	pToken = strtok(NULL, ",");
	if (!pToken)
		return;
	speed = atoi(pToken);
	pToken = strtok(NULL, ",*");
	if (!pToken)
		return;
	level = atoi(pToken);

	if (level > LastIntensity)
	{
		LastIntensity = level;
		LastIntensityTime = GetTickCount();
	}

	{
		int printInfo  = (LastSpeed != (speed / 10));
		
		LastSpeed = speed / 10;
		LastSpeedTime = GetTickCount();
		if (RADAR_DEBUG)
		{
			if (printInfo)
			{
				sprintf(infoText, "Last %d - Time %llu\r\n", LastSpeed, LastSpeedTime); 
				FileRoutines_SaveDebugInfo(infoText, strlen(infoText));
			}
		}
	}
}

void Radar_ReceiveNMEASentence_RDCNT(char *pSentence, DeviceInfoS *pDeviceInfo)
{
	char *pToken;
	int speed;
	
	if (!Radar_CheckNMEAChecksum(pSentence))
		return;

	pToken = strtok(pSentence, ",");
	if (!pToken)
		return;
	pToken = strtok(NULL, ",");
	if (!pToken)
		return;

	pToken = strtok(NULL, ",");
	if (!pToken)
		return;
	speed = atoi(pToken);
	pToken = strtok(NULL, ",*");
	if (!pToken)
		return;

	FileRoutines_addVehicleLog(speed / 10);
}

void Radar_ReceiveByte_NMEA(unsigned char byte, DeviceInfoS *pDeviceInfo)
{
	if ((Radar_IncomingSentenceLength == 0) && (byte != '$'))
		return;

	Radar_IncomingSentence[Radar_IncomingSentenceLength] = byte;
	Radar_IncomingSentenceLength ++;
	if (Radar_IncomingSentenceLength >= MAX_RADAR_SENTENCE_LENGTH)
	{
		Radar_IncomingSentenceLength = 0;
		return;
	}

	if (byte == 0x0D)
	{
		static char lastSentence[50] = "";
		Radar_IncomingSentence[Radar_IncomingSentenceLength] = 0;
		
		if (RADAR_DEBUG == 2)
		{
			if (strstr(Radar_DebugData, "$RDTGT*51") == NULL) 
			{
					printf("%s\n", Radar_IncomingSentence);
					strcmp(lastSentence, Radar_IncomingSentence);
			}
		}

		if (strncmp((char*)Radar_IncomingSentence, "$RDTGT,", 7) == 0)
			Radar_ReceiveNMEASentence_RDTGT((char*)Radar_IncomingSentence, pDeviceInfo);

		if (strncmp((char*)Radar_IncomingSentence, "$RDCNT,", 7) == 0)
			Radar_ReceiveNMEASentence_RDCNT((char*)Radar_IncomingSentence, pDeviceInfo);


		Radar_IncomingSentenceLength = 0;
	}
}

int Radar_GetLastSpeed()
{
	return LastSpeed;
}

int Radar_GetLastSpeedTime()
{
	return LastSpeedTime;
}

//
// int Radar_DetermineSpeedForDisplay(DeviceInfoS *pDeviceInfo)
//
// Runs a simple state machine which reads the settings and decides what should be displayed
//
int Radar_DetermineSpeedForDisplay(DeviceInfoS *pDeviceInfo)
{
	int keepLastSpeedLengthMs = pDeviceInfo->keepLastSpeedLengthMs;
	int updateDisplayLengthMs = pDeviceInfo->updateDisplayLengthMs;

	if (keepLastSpeedLengthMs < NumAnimationFrames * AnimationFrameLengthMs)
		keepLastSpeedLengthMs = NumAnimationFrames * AnimationFrameLengthMs;	
	if (updateDisplayLengthMs < NumAnimationFrames * AnimationFrameLengthMs)
		updateDisplayLengthMs = NumAnimationFrames * AnimationFrameLengthMs;


	//
	// update intensity
	//
	if (TicksElapsed(LastIntensityTime) > 2500)
	{
		LastIntensity = 0;
	}


	//
	// determine if the speed should not be updated, because it was recently changed
	//

	if (TicksElapsed(LastDisplayedSpeedTime) < updateDisplayLengthMs)
	{
		return LastDisplayedSpeed;
	}
	
	//
	// if the speed should be updated, check if the radar received the new speed value that is greatly different from the current value 
	//
	if ((fabs((double)LastDisplayedSpeed - (double)LastSpeed) >= pDeviceInfo->updateDisplaySpeedDelta) &&
		(TicksElapsed(LastSpeedTime) < keepLastSpeedLengthMs))
	{
		LastDisplayedSpeed = LastSpeed;
		LastDisplayedSpeedTime = GetTickCount();
		return LastDisplayedSpeed;
	}

	//
	// check if no vehicle was detected for an extended period of time
	//
	if (TicksElapsed(LastSpeedTime) >= keepLastSpeedLengthMs)
	{
		LastDisplayedSpeed = 0;
		LastSpeed = 0;
		LastDisplayedSpeedTime = GetTickCount();
		return LastDisplayedSpeed;
	}

	return LastDisplayedSpeed;
}

int Radar_CurrentIntensity()
{
	if ((LastIntensity / 5000) > 100)
		return 100;

	return LastIntensity / 5000;
}

char* ReadCommand(int hSock, long lTmoMs)
{
	fd_set fdr, fde;
	int iFdx = -1;
	struct timeval delay;
	int iSel;
	int len;
	char* retval = NULL;

	delay.tv_sec = (long) (lTmoMs/1000);           // How many seconds
	delay.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in uS

	FD_ZERO(&fdr);
	FD_ZERO(&fde); // error fd

	FD_SET(hSock, &fdr);
	FD_SET(hSock, &fde);
			
	iSel = select(hSock+1, &fdr, 0, &fde, &delay); // Select for both Cmd Pipe and For IO

	if ((iSel >0) && (FD_ISSET(hSock, &fdr)))
	{
		int len;
		retval = udpRead(hSock, &len, 50);
	}
	return retval;
}

void* radarTask(void* argPtr)
{
	TCB *args = (TCB*) argPtr;
	int radarFd = -1;
	int speedToDisplay;
	int lastSpeedReported = -1;
	char newSpeed[25];
	fd_set fdr, fde;
	struct timeval delay;
	int iSel;
	char* retval = NULL;
	long lTmoMs;
	int iFdx = -1;
	int hSock = GetUdpServerHandle(args->localPort);
	unsigned long long timer = GetTickCount();
	unsigned long long serialActive = 0;

	FD_ZERO(&fdr);
	FD_ZERO(&fde); // error fd

	FD_SET(hSock, &fdr);
	FD_SET(hSock, &fde);
				
	FileRoutines_readDeviceInfo(&DeviceInfo);

	radar_uart = Radar_Init((int)DeviceInfo.radarBaudRate, (RadarProtocolE)DeviceInfo.radarProtocol, DeviceInfo.sensitivity);
	printf("Setup RADAR_POWER - GPIO 65 - Port 2 Pin 1\n");
	radarFd = pinctl::inst().export_pin(RADAR_POWER, 0); //65
	pinctl::inst().set(radarFd, 0);
	timer = GetTickCount(); 
	lTmoMs = 1000; // Resets the Radar in 1 second
	serialActive = GetTickCount() - 55000; // Set Serial to expire in 5 seconds
	pinctl::inst().set(radarFd, 0);
	usleep(1000 * 100);
	pinctl::inst().set(radarFd, 1);

	while(1)
	{
		iFdx = hSock;
		if (radar_uart->fd() > hSock)
			iFdx = radar_uart->fd();

		delay.tv_sec = (long) (lTmoMs/1000);           // How many seconds
		delay.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in uS

		FD_ZERO(&fdr);
		FD_ZERO(&fde); // error fd

		FD_SET(hSock, &fdr);
		FD_SET(hSock, &fde);
		FD_SET(radar_uart->fd(), &fdr);
		FD_SET(radar_uart->fd(), &fde);

		iSel = select(iFdx+1, &fdr, 0, &fde, &delay); // Select for both Cmd Pipe and For IO
		if (iSel > 0)
		{
			if (FD_ISSET(hSock, &fdr))
			{
				char* cmd = ReadCommand(hSock, 10); // 10ms minimum delay
				if (cmd != NULL)
				{
					// Process the command
					free(cmd);
					cmd = NULL;
				}
			}
			if (FD_ISSET(radar_uart->fd(), &fdr))
			{
				serialActive = GetTickCount();
				Radar_ReceiveData(&DeviceInfo);
			}
		}
		else // Timeout
		{
			// Timeout actions
			if ((serialActive == 0) || (TicksElapsed(serialActive) > 60000))
			{
				printf("Resetting Radar\n");
				serialActive = GetTickCount();
				lTmoMs = 10000;
				pinctl::inst().set(radarFd, 0);
				usleep(1000*100);
				pinctl::inst().set(radarFd, 1);
			}
		}
		if (TicksElapsed(timer) > 10000)
		{
			char request[64];
			sprintf(request, "\r\n#set_sensitivity=%d\r\n", DeviceInfo.sensitivity);
			radar_uart->write(request);
			usleep(100000);
			radar_uart->write(request);
			usleep(100000);
			timer = GetTickCount();
		}
	}
}

void Radar_CreateTask(int localPort, int reportPort)
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

	radarTaskTCB.localPort = localPort;
	radarTaskTCB.arg1 = reportPort;
	status = pthread_create(&radarTaskTCB.threadID, &attr, radarTask, (void*) &radarTaskTCB);
	status = pthread_attr_destroy(&attr);
	if (status != 0)
	{
		printf("ERROR: %d pthread_attr_destroy: %s - %d\r\n", status, strerror(errno), errno);
	}
}
