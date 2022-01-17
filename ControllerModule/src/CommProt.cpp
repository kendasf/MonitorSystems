#include "FileRoutines.h"
#include "CommProt.h"
#include "main.h"  
#include "../CryptLibs/aes.h"
#include "../CryptLibs/diffie-hellman.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "radar.h"
#include "camera.h"
#include "cameraFuncts.h"
#include <ctype.h>
#include <sys/types.h>
#include <dirent.h>
#include "../Ethernet/TcpInterface.h"

#include "SystemUtilities.h"


extern int CurrentlyDisplayedSpeed;

//
// Precalculated CRC lookup table
//
static const unsigned int CRCLookupTable[] = {
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA, 0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC, 0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116, 0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818, 0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2, 0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4, 0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE, 0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60, 0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A, 0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C, 0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6, 0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

char HexDigit[16] = 
{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

void lowercase(char* s)
{
	for (; *s; ++s) 
	{
		*s = tolower(*s);
	}
}

//
// Current communications state
//
int  ActiveChannel;
long ExpectedSentenceNum;
long LastCommunicationTime;

char CrntFileTransferName[200];
int	 CrntFileTransferPos = 0;
int	 UserLoggedOn = 0;
int  UserLevel = 0; // 0 = Admin, 1 = Ordinary User, 2 = Stupid User

//
// Incoming buffers
//
char IncomingBufferSerialA[COMM_BUFFER_SIZE];
char IncomingBufferBluetooth[COMM_BUFFER_SIZE];
char IncomingBufferEthernet[COMM_BUFFER_SIZE];
char* IncommingBuffer[3];
unsigned long IncommingBufferLength[3];

//
// Outgoing buffers
//
char OutgoingBufferSerialA[COMM_BUFFER_SIZE];
char OutgoingBufferBluetooth[COMM_BUFFER_SIZE];
char OutgoingBufferEthernet[COMM_BUFFER_SIZE];
char* OutgoingBuffer[3];
unsigned long OutgoingBufferLength[3];
char LastSentSentence[COMM_BUFFER_SIZE];
int LastSentSentenceSize;

//
// used internally
//
char response[820];
FINFO FileSearchInfo;
char FileSearchMask[512];
int FileSearchFirstID;

//
// encryption globals
//
unsigned char SharedSecret[32];
int UseEncryption = 0;
char EncryptionBuffer[COMM_BUFFER_SIZE * 2];
char TempSentenceBuffer[COMM_BUFFER_SIZE * 2];

extern int ActiveEthSock;
//
// external globals
//
extern int MemoryCardStatus;
extern int MemoryCardSafeRemove;
extern float SupplyVoltageLevel;
extern int MemoryCardTotalSpace;
extern int MemoryCardFreeSpace;

extern unsigned char ServerSocket;

extern unsigned short TCPServerCallback (unsigned char socket, unsigned char event, unsigned char *pData, unsigned short param);

//
// void CommProt_init()
//
// Initializes the communications library.
//
void CommProt_init()
{
	IncommingBufferLength[0] = 0;
	IncommingBufferLength[1] = 0;
	IncommingBufferLength[2] = 0;

	IncommingBuffer[0] = IncomingBufferSerialA;
	IncommingBuffer[1] = IncomingBufferBluetooth;
	IncommingBuffer[2] = IncomingBufferEthernet;

	OutgoingBufferLength[0] = 0;
	OutgoingBufferLength[1] = 0;
	OutgoingBufferLength[2] = 0;

	OutgoingBuffer[0] = OutgoingBufferSerialA;
	OutgoingBuffer[1] = OutgoingBufferBluetooth;
	OutgoingBuffer[2] = OutgoingBufferEthernet;

	ActiveChannel = CHANNEL_NONE;

	LastSentSentenceSize = 0;
	LastCommunicationTime = 0;
}

//
// void CommProt_resetChannel(int dataChannel)
//
// Resets communication on the given channel.
//
void CommProt_resetChannel(int dataChannel)
{
	IncommingBufferLength[dataChannel] = 0;
	OutgoingBufferLength[dataChannel] = 0;

	if (dataChannel == ActiveChannel)
	{
		ActiveChannel = CHANNEL_NONE;
		UserLoggedOn = 0;
		UseEncryption = 0;
		LastSentSentenceSize = 0;
		LastCommunicationTime = 0;
		FileRoutines_addLog(LOG_DISCONNECTED, NULL);
	}
}

//
// CommProt_checkForTimeout()
//
// Checks if communication timeout has occured.
//
void CommProt_checkForTimeout()
{
	if (LastCommunicationTime == 0)
		return;

	if (GetSecondsTimer() - LastCommunicationTime > CONNECTION_TIMEOUT_TIME)
	{
		CommProt_resetChannel(ActiveChannel);
	}
}

//
// void CommProt_KeepAlive()
//
// If Ethernet is used, sends one byte each second, so that the socket is not closed automatically by
// the TCP/IP stack.
//
void CommProt_KeepAlive()
{
	if ((ActiveChannel == CHANNEL_ETHERNET) && (GetSecondsTimer() - LastCommunicationTime > 3))
	{
		if (OutgoingBufferLength[CHANNEL_ETHERNET] < COMM_BUFFER_SIZE)
		{
			OutgoingBuffer[CHANNEL_ETHERNET][OutgoingBufferLength[CHANNEL_ETHERNET]] = ' ';
			OutgoingBufferLength[CHANNEL_ETHERNET] ++;	
		}
	}
}

//
// int CommProt_getOutputBuffer(char* pBuffer, int maxBufferSize, int dataChannel)
//
// pBuffer - pointer to receive the data
// maxBufferSize - maximum buffer size
// dataChannel - identifies the channel
//               which transferred the data
// return: int - total number of bytes placed into pBuffer
//
// Reads data from the outgoing stream.
//
int CommProt_getOutputBuffer(char* pBuffer, int maxBufferSize, int dataChannel)
{
	int i;
	int sizeToCopy;

	if (OutgoingBufferLength[dataChannel] == 0)
	{
		return 0;
	}

	//
	// determine the size of bytes to copy
	//
	sizeToCopy = OutgoingBufferLength[dataChannel];
	if (sizeToCopy > maxBufferSize)
	{
		sizeToCopy = maxBufferSize;
	}

	//
	// copy the data from the outgoing buffer to pBuffer
	//
	memcpy(pBuffer, OutgoingBuffer[dataChannel], sizeToCopy);

	for(i = sizeToCopy; i < OutgoingBufferLength[dataChannel]; i++)
	{
		OutgoingBuffer[dataChannel][i - sizeToCopy] =
			OutgoingBuffer[dataChannel][i];
	}

	OutgoingBufferLength[dataChannel] -= sizeToCopy;

	return sizeToCopy;
}

//
// void CommProt_processStream
//
// pData - pointer to the buffer with data
// dataLength - the length of the buffer
// dataChannel - identifies the channel
//               which transferred the data
//
// return: void
//
// Appends incoming data to the incoming
// queue ad processes the queue.
//
void CommProt_processStream(char* pData,
                            unsigned long dataLength,
                            long dataChannel)
{
	//
	// update last communication time
	//
	if (ActiveChannel == dataChannel)
	{
		LastCommunicationTime = GetSecondsTimer();
	}

	//
	// sanity check
	//
	if ((dataChannel != CHANNEL_SERIAL_A) &&
		(dataChannel != CHANNEL_BLUETOOTH) &&
		(dataChannel != CHANNEL_ETHERNET))
	{
		return;
	}

	//
	// append the data to the appropriate buffer; in case of overflow, reset
	// the buffer
	//
	if (IncommingBufferLength[dataChannel] + dataLength > COMM_BUFFER_SIZE)
	{
		IncommingBufferLength[dataChannel] = 0;
	}

	memcpy(IncommingBuffer[dataChannel] + IncommingBufferLength[dataChannel],
	       pData, dataLength);
	IncommingBufferLength[dataChannel] += dataLength;

	//
	// process the data
	//
	CommProt_processIncommingBuffer(dataChannel);
}

//
// void CommProt_processIncommingBuffer(long dataChannel)
//
// dataChannel - identifies the channel
//               which transferred the data
//
// return: void
//
// Extracts sentence from the stream.
//
void CommProt_processIncommingBuffer(long dataChannel)
{
	long i;
	long sentenceStartPos;
	long sentenceEndPos;

	//
	// search for sentence start (0x19)
	//
	sentenceStartPos = -1;
	for(i = 0; i < IncommingBufferLength[dataChannel]; i++)
	{
		if (IncommingBuffer[dataChannel][i] == 0x19)
		{
			sentenceStartPos = i;
			break;
		}
	}

	if (sentenceStartPos == -1)
	{
		return;
	}

	//
	// remove everything before sentence start
	//
	for(i = sentenceStartPos; i < IncommingBufferLength[dataChannel]; i++)
	{
	IncommingBuffer[dataChannel][i - sentenceStartPos] =
		IncommingBuffer[dataChannel][i];
	}
	IncommingBufferLength[dataChannel] -= sentenceStartPos;

	//
	// find sentence end: 0x18 or replaced "\0"
	//
	sentenceEndPos = -1;
	for(i = 0; i < IncommingBufferLength[dataChannel]; i++)
	{
		if ((IncommingBuffer[dataChannel][i] == 0x18) ||
		    (IncommingBuffer[dataChannel][i] == '\0'))
		{
			sentenceEndPos = i;
			break;
		}
	}

	if (sentenceEndPos == -1)
	{
		return;
	}

	//
	// process the sentence
	//
	CommProt_processSentence(IncommingBuffer[dataChannel], sentenceEndPos,
	                         dataChannel);

	//
	// remove the processed sentence from the buffer
	//
	sentenceEndPos ++;
	for(i = sentenceEndPos; i < IncommingBufferLength[dataChannel]; i++)
	{
		IncommingBuffer[dataChannel][i - sentenceEndPos] =
			IncommingBuffer[dataChannel][i];
	}
	IncommingBufferLength[dataChannel] -= sentenceEndPos;
}

//
// void CommProt_processSentence(char *pData, long dataLength, long dataChannel)
//
// pData - pointer to the sentence
// dataLength - length of the sentence
// dataChannel - identifies the channel
//               which transferred the data
//
// return: void
//
// Processes a single communication sentence.
//
void CommProt_processSentence(char *pData, long dataLength, long dataChannel)
{
	int i;
	int crcPos;
	char tag[4];
    char cmdstr[4];
	unsigned long sentenceNumber;
	unsigned long checksum;
	char* pSubstring;

	//
	// check for log-off request
	//
	if (strncmp(pData + 1, "LOF", 3) == 0)
	{
		UserLoggedOn = 0;
		ActiveChannel = CHANNEL_NONE;
		UseEncryption = 0;
		FileRoutines_addLog(LOG_DISCONNECTED, NULL);
		return;
	}

	//
	// check for un-encrypted GDT request
	//
	if (strncmp(pData + 1, "GDT", 3) == 0)
	{
		UserLoggedOn = 0;
		ActiveChannel = CHANNEL_NONE;
		UseEncryption = 0;
	}

	//
	// if encryption is used, decrypt the data
	//
	if (UseEncryption)
	{
		DecryptSentence(pData, dataLength + 1, EncryptionBuffer);
		pData = EncryptionBuffer;
	}

	//
	// replace the first character with $ and zero-terminate the sentence end
	//
	pData[0] = '$';
	pData[dataLength] = '\0';
	
	//
	// check that the sentence has a minimum length
	//
	if (dataLength < 5)
	{
		return;
	}

	//
	// read the tag
	//
	strncpy(tag, pData + 1, 3);
	tag[3] = '\0';
	//
	// read the sentence number
	//
	pSubstring = strchr(pData, ';');
	if (pSubstring == NULL)
	{
		return;
	}
	sentenceNumber = atol(pSubstring + 1);

	//
	// if this sentence has already been received, resend the last response,
	// unless it is a Login request (which already has the lowest sentence
	// number) or the GDT request, or the GSN, or the last response was checksum 
	// error response
	//
	if ((ActiveChannel == dataChannel) &&
	   (sentenceNumber < ExpectedSentenceNum) &&
	   (strcmp(tag, "LGN") != 0) &&
	   (strcmp(tag, "GDT") != 0) &&
	   (strcmp(tag, "GSN") != 0) &&
	   (strncmp(LastSentSentence, "$CHK", 4) != 0))
	{
		CommProt_resendLastResponse(dataChannel);
		return;
	}

	//
	// read the checksum
	//
	pSubstring = strchr(pSubstring + 1, ';');
	if (pSubstring == NULL)
	{
		return;
	}
	checksum = atol(pSubstring + 1);
	
	crcPos = -2;
	for(i = 0; i < dataLength; i++)
	{
		if (pData[i] == ';')
			crcPos ++;
		
		if (crcPos == 0)
		{
			crcPos = i;
			break;
		}
	}

	//
	// verify the checksum
	//
	if (CalculateCRC32(pData, crcPos + 1) != checksum)
	{
		CommProt_appendResponse("$CHK", sentenceNumber, dataChannel);
		return;
	}

	//
	// check if the message is on the right channel
	//
	if (ActiveChannel != dataChannel)
	{
		if (ActiveChannel == CHANNEL_NONE)
		{
			ActiveChannel = dataChannel;
		}
		else
		{
			// respond with an error
			CommProt_appendResponse("$ALC", sentenceNumber, dataChannel);
			return;
		}
	}

	//
	// message accepted, process it!
	//
	ExpectedSentenceNum = sentenceNumber + 1;

	if (strcmp(tag, "LGN") == 0)
	{
		// login request
		CommProt_processLGN(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GDT") == 0)
	{
		// info request
		CommProt_processGDT(pData+5, sentenceNumber, dataChannel);
		return;
	}

	//
	// check that logged in
	//
	if (UserLoggedOn == 0)
	{
		CommProt_appendResponse("$NTL", sentenceNumber, dataChannel);
		return;
	}

	if (strcmp(tag, "INF") == 0)
	{
		// info request
		CommProt_processINF(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "TIM") == 0)
	{
		// set time and date
		CommProt_processTIM(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "PSS") == 0)
	{
		// change user password
		CommProt_processPSS(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "LOF") == 0)
	{
		// log off
		CommProt_processLOF(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "EMC") == 0)
	{
		// enable / disable memory card
		CommProt_processEMC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GFI") == 0)
	{
		// gets the file from the device
		CommProt_processGFI(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "CFI") == 0)
	{
		// asks device to continue sending file
		CommProt_processCFI(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GDL") == 0)
	{
		// gets the directory listing from the device
		CommProt_processGDL(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "CDL") == 0)
	{
		// asks device to continue sending directory listing
		CommProt_processCDL(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "EFC") == 0)
	{
		// executes the filesystem command
		CommProt_processEFC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SBR") == 0)
	{
		// set baud rate for radar communication
		CommProt_processSBR(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SRS") == 0)
	{
		// set radar sensitivity
		CommProt_processSRS(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SPT") == 0)
	{
		// set protocol type for radar communication
		CommProt_processSPT(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SMI") == 0)
	{
		// set min speed
		CommProt_processSMI(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SMA") == 0)
	{
		// set max speed
		CommProt_processSMA(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SLI") == 0)
	{
		// set speed limit
		CommProt_processSLI(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SDL") == 0)
	{
		// set brightness
		CommProt_processSDL(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SBP") == 0)
	{
		// set blink params
		CommProt_processSBP(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SDU") == 0)
	{
		// set display update params
		CommProt_processSDU(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GPC") == 0)
	{
		// get panel configuration
		CommProt_processGPC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SPC") == 0)
	{
		// set panel configuration
		CommProt_processSPC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GAD") == 0)
	{
		// get auto dimming
		CommProt_processGAD(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SAD") == 0)
	{
		// set auto dimming
		CommProt_processSAD(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "BMP") == 0)
	{
		// upload bitmap
		CommProt_processBMP(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GBM") == 0)
	{
		// download bitmap
		CommProt_processGBM(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GCB") == 0)
	{
		// download bitmap
		CommProt_processGCB(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GBC") == 0)
	{
		// get bitmap configuration
		CommProt_processGBC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SBC") == 0)
	{
		// set bitmap configuration
		CommProt_processSBC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "TST") == 0)
	{
		// set test mode
		CommProt_processTST(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GSC") == 0)
	{
		CommProt_processGSC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SSC") == 0)
	{
		CommProt_processSSC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SEP") == 0)
	{
		CommProt_processSEP(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SDN") == 0)
	{
		CommProt_processSDN(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "RBC") == 0)
	{
		CommProt_processRBC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "ELI") == 0)
	{
		CommProt_processELI(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "ELS") == 0)
	{
		CommProt_processELS(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "RLI") == 0)
	{
		CommProt_processRLI(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "RLS") == 0)
	{
		CommProt_processRLS(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GRI") == 0)
	{
		CommProt_processGRI(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "RAC") == 0)
	{
		CommProt_processRAC(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "CAM") == 0)
	{
		CommProt_processCAM(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GSS") == 0)
	{
		CommProt_processGSS(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "GUN") == 0)
	{
		CommProt_processGUN(pData+5, sentenceNumber, dataChannel);
		return;
	}
	else if (strcmp(tag, "SUN") == 0)
	{
		CommProt_processSUN(pData+5, sentenceNumber, dataChannel);
		return;
	}
}

//
// void CommProt_appendResponse(char *pData, long sentenceNumber, long dataChannel)
//
// Appends response to the response buffer.
//
void CommProt_appendResponse(const char *pData, long sentenceNumber, long dataChannel)
{
	char tmpBuffer[32];
	unsigned long checksum;
	int startPos;
	int i;
	int size;

	//
	// if the buffer is too small for the response, ignore the response
	//
	if (OutgoingBufferLength[dataChannel] + strlen(pData) + 32 > COMM_BUFFER_SIZE)
	{
		return;
	}

	startPos = OutgoingBufferLength[dataChannel];

	if (UseEncryption)
	{
		int paddingSize;
		int newSize;

		//
		// copy the data, ignoring the first character
		//
		strcpy(TempSentenceBuffer+4, pData+1);
		TempSentenceBuffer[3] = 0x19;
	
		// append ;
		strcat(TempSentenceBuffer+4, ";");
	
		// append the sentence number
		sprintf(tmpBuffer, "%ld", sentenceNumber );
		strcat(TempSentenceBuffer+4, tmpBuffer);
	
		// append ;
		strcat(TempSentenceBuffer+4, ";");
	
		// calculate and append the checksum
		checksum = CalculateCRC32(TempSentenceBuffer+3, strlen(TempSentenceBuffer+3));
		sprintf(tmpBuffer, "%lu", checksum);
		strcat(TempSentenceBuffer+4, tmpBuffer);

		// append \n characters as needeed
		size = strlen(TempSentenceBuffer + 4);
		paddingSize = 16 - (size % 16);
		if (paddingSize == 16)
			paddingSize = 0;

		for(i = 0; i < paddingSize; i++)
		{
			TempSentenceBuffer[size + i + 4] = '\n';
		}

		TempSentenceBuffer[size + paddingSize + 4] = '\0';

		// encrypt the data
		newSize = EncryptSentence(TempSentenceBuffer + 4, EncryptionBuffer);

		// append the data to outgoing buffer
		if (OutgoingBufferLength[dataChannel] + newSize + 8 > COMM_BUFFER_SIZE)
		{
			return;
		}

		OutgoingBuffer[dataChannel][OutgoingBufferLength[dataChannel]] = 0x19;
		memcpy(OutgoingBuffer[dataChannel] + OutgoingBufferLength[dataChannel] + 1,
				EncryptionBuffer, newSize);
		OutgoingBuffer[dataChannel][OutgoingBufferLength[dataChannel] + newSize + 1] = 0x18;

		OutgoingBufferLength[dataChannel] += newSize + 2;
		OutgoingBuffer[dataChannel][OutgoingBufferLength[dataChannel]] = '\0';

		//
		// save the last sent sentence
		//
		memcpy(LastSentSentence,
		      OutgoingBuffer[dataChannel] + startPos,
		      OutgoingBufferLength[dataChannel] - startPos);
		LastSentSentenceSize = OutgoingBufferLength[dataChannel] - startPos;
	}
	else
	{
		//
		// append the data
		//
		strcpy(OutgoingBuffer[dataChannel] + OutgoingBufferLength[dataChannel],
		      pData);
	
		// change the first character from $ to 0x19
		*(OutgoingBuffer[dataChannel] + OutgoingBufferLength[dataChannel]) = 0x19;
	
		OutgoingBufferLength[dataChannel] += strlen(pData);
	
		//
		// append ';'
		//
		OutgoingBuffer[dataChannel][OutgoingBufferLength[dataChannel]] = ';';
		OutgoingBufferLength[dataChannel] ++;
	
		//
		// append the sentence number
		//
		sprintf(tmpBuffer, "%ld", sentenceNumber);
		strcpy(OutgoingBuffer[dataChannel] + OutgoingBufferLength[dataChannel],
		      tmpBuffer);
		OutgoingBufferLength[dataChannel] += strlen(tmpBuffer);
	
		//
		// append ';'
		//
		OutgoingBuffer[dataChannel][OutgoingBufferLength[dataChannel]] = ';';
		OutgoingBufferLength[dataChannel] ++;
	
	
		//
		// calculate and append the checksum
		//
		checksum = CalculateCRC32(OutgoingBuffer[dataChannel], OutgoingBufferLength[dataChannel]);
		sprintf(tmpBuffer, "%lu", checksum);
		strcpy(OutgoingBuffer[dataChannel] + OutgoingBufferLength[dataChannel],
		      tmpBuffer);
		OutgoingBufferLength[dataChannel] += strlen(tmpBuffer);
	
		//
		// append 0x18
		//
		OutgoingBuffer[dataChannel][OutgoingBufferLength[dataChannel]] = 0x18;
		OutgoingBufferLength[dataChannel] ++;
		OutgoingBuffer[dataChannel][OutgoingBufferLength[dataChannel]] = '\0';
		
		//
		// save the last sent sentence
		//
		memcpy(LastSentSentence,
		      OutgoingBuffer[dataChannel] + startPos,
		      OutgoingBufferLength[dataChannel] - startPos);
		LastSentSentenceSize = OutgoingBufferLength[dataChannel] - startPos;
	}

	#ifdef DEBUG
	printf("Appended to output buffer: %s\n", OutgoingBuffer[dataChannel]);
	#endif
}

//
// void CommProt_resendLastResponse(long dataChannel)
//
// Resends last response.
//
void CommProt_resendLastResponse(long dataChannel)
{
	//
	// if the buffer is too small for the response, ignore the response
	//
	if (OutgoingBufferLength[dataChannel] + LastSentSentenceSize > COMM_BUFFER_SIZE)
	{
		return;
	}
	
	//
	// append the data
	//
	memcpy(OutgoingBuffer[dataChannel] + OutgoingBufferLength[dataChannel],
	      LastSentSentence, LastSentSentenceSize);
	OutgoingBufferLength[dataChannel] += LastSentSentenceSize;
}

//
// void CommProt_processLGN(char *pData, long sentenceNumber, long dataChannel)
// 
// Processes Login request.
//
void CommProt_processLGN(char *pData, long sentenceNumber, long dataChannel)
{
	DeviceInfoS deviceInfo;
	int bLoginValid;
	char password[MAX_STRING_LENGTH];
	int reqLevel = 1;

	char *pToken;
	UserLevel = 2;

	//
	// read the password; if sentence is invalid, just leave
	//
	pToken = strtok(pData, ",");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	strcpy(password, pToken);

	pToken = strtok(NULL, ",;");
	if ((pToken != NULL) && (strcmp(pToken, "ADM") == 0))
		reqLevel = 0;
	if ((pToken != NULL) && (strcmp(pToken, "UL2") == 0))
		reqLevel = 2;

	//
	// verify username and password
	//
	bLoginValid = 0;
	FileRoutines_readDeviceInfo(&deviceInfo);

	lowercase(password);
	lowercase(deviceInfo.adminPassword);
	lowercase(deviceInfo.password);
	lowercase(deviceInfo.stupidPassword);

	//
	// check the password combination
	//
	if (reqLevel == 0)
	{
		if (strcmp(password, deviceInfo.adminPassword) == 0)
		{
			bLoginValid = 1;
			UserLevel = 0;
		}
		
	}
	else if (reqLevel == 2)
	{
		if (strcmp(password, deviceInfo.stupidPassword) == 0)
		{
			bLoginValid = 1;
			UserLevel = 2;
		}
		
	}
	else
	{
		if (strcmp(password, deviceInfo.password) == 0)
		{
			bLoginValid = 1;
			UserLevel = 1;
		}
	}

	//
	//  respond
	//
	if (bLoginValid)
	{
		UserLoggedOn = 1;
		ActiveChannel = dataChannel;
		CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
//		CameraReset();

		// add log
		{
			char params[32];
			if (ActiveChannel == CHANNEL_SERIAL_A)
				strcpy(params, "Serial");
			if (ActiveChannel == CHANNEL_BLUETOOTH)
				strcpy(params, "Bluetooth");
			if (ActiveChannel == CHANNEL_ETHERNET)
				strcpy(params, "Ethernet");
			FileRoutines_addLog(LOG_CONNECTED, params);
		}
	}
	else
	{
		UserLoggedOn = 0;
		ActiveChannel = CHANNEL_NONE;
		CommProt_appendResponse("$LGF", sentenceNumber, dataChannel);
	}
}

//
// void CommProt_processTIM(char *pData, long sentenceNumber, long dataChannel)
//
// Processes the set time request.
//
void CommProt_processTIM(char *pData, long sentenceNumber, long dataChannel)
{
	int year;
	int month;
	int day;
	int hour;
	int minute;
	int second;
	char params[70];
	
	char *pToken;
	char *resp;

	//
	// parse the time/date parameters
	//
	pToken = strtok(pData, ",");
	if (pToken == NULL)
	{
		return;
	}
	year = atoi(pToken);
	
	pToken = strtok(NULL, ",");
	if (pToken == NULL)
	{
		return;
	}
	month = atoi(pToken);
	
	pToken = strtok(NULL, ",");
	if (pToken == NULL)
	{
		return;
	}
	day = atoi(pToken);
	
	pToken = strtok(NULL, ",");
	if (pToken == NULL)
	{
		return;
	}
	hour = atoi(pToken);
	
	pToken = strtok(NULL, ",");
	if (pToken == NULL)
	{
		return;
	}
	minute = atoi(pToken);
	
	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
	{
		return;
	}
	second = atoi(pToken);

	//sprintf(params, "hwclock --set -f /dev/rtc1 --date=\"%d-%02d-%02d %02d:%02d:%02d\"", year, month, day, hour, minute, second);
	sprintf(params, "sudo timedatectl set-time \"%d-%02d-%02d %02d:%02d:%02d\" ", year, month, day, hour, minute, second);
	// add log
	FileRoutines_addLog(LOG_CHANGE_TIME, params);

	//
	// set the time/dateof the RTC
	//
	resp = SysCmd(params); // Send command to set RTC
	if (resp != NULL) printf("%s\r\n", resp);
	if (resp != NULL) free(resp);

	/*
	resp = SysCmd("hwclock -s -f /dev/rtc1"); // Updates OS from RTC
	if (resp != NULL) printf("%s\r\n", resp);
	if (resp != NULL) free(resp);
	*/

	setCameraTime((char*) CAMERA_IP_ADDR);

	//
	// respond with EOK
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processPSS(char *pData, long sentenceNumber, long dataChannel)
//
// Process change password request.
//
void CommProt_processPSS(char *pData, long sentenceNumber, long dataChannel)
{
	int bLoginValid;
	DeviceInfoS deviceInfo;
	char *pToken;
	char oldPassword[MAX_STRING_LENGTH];
	char newPassword[MAX_STRING_LENGTH];
	int reqLevel = 0;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	strcpy(oldPassword, pToken);
	
	pToken = strtok(NULL, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	strcpy(newPassword, pToken);

	pToken = strtok(NULL, ",;");
	if (pToken != NULL)
	{
		reqLevel = atoi(pToken);
	}
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	lowercase(deviceInfo.adminPassword);
	lowercase(oldPassword);
	lowercase(deviceInfo.password);
	lowercase(deviceInfo.stupidPassword);

	//
	// check if provided old password is valid
	//
	if (UserLevel == 0)
	{
		bLoginValid = 0;
		if (strcmp(oldPassword, deviceInfo.adminPassword) == 0)
		{
			bLoginValid = 1;
		}
	
		if (bLoginValid == 0)
		{
			CommProt_appendResponse("$LGF", sentenceNumber, dataChannel);
			return;
		}
	
		if (reqLevel == 0)
			strcpy(deviceInfo.adminPassword, newPassword);
		else if (reqLevel == 1)
			strcpy(deviceInfo.password, newPassword);
		else if (reqLevel == 2)
			strcpy(deviceInfo.stupidPassword, newPassword);
	}
	else if (UserLevel == 1)
	{
		bLoginValid = 0;
		if (strcmp(oldPassword, deviceInfo.password) == 0)
		{
			bLoginValid = 1;
		}
	
		if (bLoginValid == 0)
		{
			CommProt_appendResponse("$LGF", sentenceNumber, dataChannel);
			return;
		}
	
		strcpy(deviceInfo.password, newPassword);
	}
	else if (UserLevel == 2)
	{
		bLoginValid = 0;
		if (strcmp(oldPassword, deviceInfo.stupidPassword) == 0)
		{
			bLoginValid = 1;
		}
	
		if (bLoginValid == 0)
		{
			CommProt_appendResponse("$LGF", sentenceNumber, dataChannel);
			return;
		}
	
		strcpy(deviceInfo.stupidPassword, newPassword);
	}

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);
	
	//
	// add log
	//
	FileRoutines_addLog(LOG_PASSWORD_CHANGED, NULL);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSBR(char *pData, long sentenceNumber, long dataChannel)
//
// Set baud rate.
//
void CommProt_processSBR(char *pData, long sentenceNumber, long dataChannel)
{
	int baudRate;
	DeviceInfoS deviceInfo;
	char *pToken;

	if (UserLevel != 0)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	baudRate = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.radarBaudRate = baudRate;

	//
	// reinit radar
	//
	Radar_Init((int)deviceInfo.radarBaudRate, (RadarProtocolE)deviceInfo.radarProtocol, deviceInfo.sensitivity);

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSRS(char *pData, long sentenceNumber, long dataChannel)
//
// Set radar sensitivity.
//
void CommProt_processSRS(char *pData, long sentenceNumber, long dataChannel)
{
	int sensitivity;
	DeviceInfoS deviceInfo;
	char *pToken;

	if (UserLevel != 0)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	sensitivity = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.sensitivity = sensitivity;

	//
	// reinit radar
	//
	Radar_Init((int)deviceInfo.radarBaudRate, (RadarProtocolE)deviceInfo.radarProtocol, deviceInfo.sensitivity);

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}


//
// void CommProt_processEMC(char *pData, long sentenceNumber, long dataChannel)
//
// Enable / disable memory card write access.
//
void CommProt_processEMC(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	char *pStatus;
	pToken = strtok(pData, ",;");
	

	//
	// disable command
	//
	if (strcmp(pToken, "0") == 0)
	{
		if ((MemoryCardStatus == MC_STATUS_OK_MMC) ||
			(MemoryCardStatus == MC_STATUS_OK_SD) ||
			(MemoryCardStatus == MC_STATUS_OK_SDHC) ||
			(MemoryCardStatus == MC_STATUS_FULL) ||
			(MemoryCardStatus  == MC_STATUS_READONLY))
		{
			FileRoutines_addLog(LOG_REMOVE_SDCARD, NULL);
			MemoryCardSafeRemove = 1;			
			pStatus = SysCmd("sync");
			if (pStatus != NULL) free(pStatus);
//			pStatus = SysCmd("umount /dev/sda1");
//			if (pStatus != NULL) free(pStatus);
			
			CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
			return;
		}
		else
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
//			pStatus = SysCmd("umount /dev/sda1");
//			if (pStatus != NULL) free(pStatus);
			return;
		}
	}

	//
	// enable command, recheck memory card
	//
	if (strcmp(pToken, "1") == 0)
	{
//		pStatus = SysCmd("mount /dev/sda1");
//		if (pStatus != NULL) free(pStatus);
		CheckMemoryCard();

		CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
		return;
	}

	CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);	
}

//
// void CommProt_processSPT(char *pData, long sentenceNumber, long dataChannel)
//
// Set protocol type.
//
void CommProt_processSPT(char *pData, long sentenceNumber, long dataChannel)
{
	int protocolType;
	DeviceInfoS deviceInfo;
	char *pToken;

	if (UserLevel != 0)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	protocolType = atoi(pToken);
	
	//
	// read device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	//
	// if the nonexistent protocol is being set, return an error
	//
	if ((protocolType < 0) || (protocolType >= Protocol_ENUM_SIZE))
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	deviceInfo.radarProtocol = (ProtocolTypeE)protocolType;

	//
	// reinit radar
	//
	Radar_Init((int)deviceInfo.radarBaudRate, (RadarProtocolE)deviceInfo.radarProtocol, deviceInfo.sensitivity);

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSMI(char *pData, long sentenceNumber, long dataChannel)
//
// Set minimum speed limit.
//
void CommProt_processSMI(char *pData, long sentenceNumber, long dataChannel)
{
	int speed;
	DeviceInfoS deviceInfo;
	char *pToken;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	speed = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.minDisplaySpeed = speed;

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);
	CurrentlyDisplayedSpeed = -1;	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSMA(char *pData, long sentenceNumber, long dataChannel)
//
// Set maximum speed limit.
//
void CommProt_processSMA(char *pData, long sentenceNumber, long dataChannel)
{
	int speed, useRotary;
	DeviceInfoS deviceInfo;
	char *pToken;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	speed = atoi(pToken);

	pToken = strtok(NULL, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	useRotary = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.maxDisplaySpeed = speed;
	deviceInfo.maxSpeedFromRotary = useRotary;

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);
	CurrentlyDisplayedSpeed = -1;	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSLI(char *pData, long sentenceNumber, long dataChannel)
//
// Set blink speed limit.
//
void CommProt_processSLI(char *pData, long sentenceNumber, long dataChannel)
{
	int speed, limitFromRotary;
	DeviceInfoS deviceInfo;
	char *pToken;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	speed = atoi(pToken);
	pToken = strtok(NULL, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	limitFromRotary = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.blinkLimit = speed;
	deviceInfo.blinkLimitFromRotary = limitFromRotary;

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);
	CurrentlyDisplayedSpeed = -1;	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSBR(char *pData, long sentenceNumber, long dataChannel)
//
// Set brightness.
//
void CommProt_processSDL(char *pData, long sentenceNumber, long dataChannel)
{
	int brightness;
	DeviceInfoS deviceInfo;
	char *pToken;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	brightness = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.displayBrightness = brightness;

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);
	
	DoAutoDimming(0);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSBP(char *pData, long sentenceNumber, long dataChannel)
//
// Set blink params..
//
void CommProt_processSBP(char *pData, long sentenceNumber, long dataChannel)
{
	int delayOn, delayOff;
	DeviceInfoS deviceInfo;
	char *pToken;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	delayOn = atoi(pToken);

	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
		return;

	delayOff = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.blinkOnDurationMs = delayOn;
	deviceInfo.blinkOffDurationMs = delayOff;

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);
	CurrentlyDisplayedSpeed = -1;	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSDU(char *pData, long sentenceNumber, long dataChannel)
//
// Set display update params..
//
void CommProt_processSDU(char *pData, long sentenceNumber, long dataChannel)
{
	int frameLength, minSpeedChange, lastSpeedVisible;
	DeviceInfoS deviceInfo;
	char *pToken;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	frameLength = atoi(pToken);

	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
		return;

	minSpeedChange = atoi(pToken);
	
	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
		return;

	lastSpeedVisible = atoi(pToken);
	
	//
	// read users info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.updateDisplayLengthMs = frameLength;
	deviceInfo.updateDisplaySpeedDelta = minSpeedChange;
	deviceInfo.keepLastSpeedLengthMs = lastSpeedVisible;

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);
	CurrentlyDisplayedSpeed = -1;	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSPC(char *pData, long sentenceNumber, long dataChannel)
//
// Set panel configuration
//
void CommProt_processSPC(char *pData, long sentenceNumber, long dataChannel)
{
	int configuration;
	DeviceInfoS deviceInfo;
	char *pToken;

	if (UserLevel != 0)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	configuration = atoi(pToken);

		
	//
	// read device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	deviceInfo.panelsConfiguration = configuration;

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processGPC(char *pData, long sentenceNumber, long dataChannel)
//
// Get panel configuration
//
void CommProt_processGPC(char *pData, long sentenceNumber, long dataChannel)
{
	DeviceInfoS deviceInfo;
	
		
	//
	// read device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	sprintf(response, "$PCO,%d", deviceInfo.panelsConfiguration);

	//
	// return result
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processSBC(char *pData, long sentenceNumber, long dataChannel)
//
// Set bitmap configuration
//
void CommProt_processSBC(char *pData, long sentenceNumber, long dataChannel)
{
	DeviceInfoS deviceInfo;
	char *pToken;
	int i, j;


		
	//
	// read and update device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	pToken = strtok(pData, ",;");
	

	//
	// iterate through all data
	//
	for(i = 0; i < 4; i++)
	{
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}

		deviceInfo.bitmapsConfig[i].speedDisplayMode = atoi(pToken);
		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}

		deviceInfo.bitmapsConfig[i].x = atoi(pToken);
		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}

		deviceInfo.bitmapsConfig[i].y = atoi(pToken);
		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}

		deviceInfo.bitmapsConfig[i].font = atoi(pToken);
		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}

		deviceInfo.bitmapsConfig[i].frameLength = atoi(pToken);
		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.bitmapsConfig[i].numFrames = atoi(pToken);

		if (deviceInfo.bitmapsConfig[i].numFrames > MAX_FRAMES)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}

		for(j = 0; j < deviceInfo.bitmapsConfig[i].numFrames; j++)
		{
			pToken = strtok(NULL, ",;");
			if (pToken == NULL)
			{
				CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
				return;
			}
			deviceInfo.bitmapsConfig[i].frames[j] = atoi(pToken);
		}

		pToken = strtok(NULL, ",;");
	}

	//
	// save the changed settings
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);	
	CurrentlyDisplayedSpeed = -1;
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processTST(char *pData, long sentenceNumber, long dataChannel)
//
// Set test mode
//
void CommProt_processTST(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;


	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	TestModeBitmap = atoi(pToken);

	pToken = strtok(NULL, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	TestModeDuration = atoi(pToken) + 1;
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processGSC(char *pData, long sentenceNumber, long dataChannel)
//
// Get schedules
//
void CommProt_processGSC(char *pData, long sentenceNumber, long dataChannel)
{
	int i;
	char buff[64];
	DeviceInfoS deviceInfo;	
		
	//
	// read device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	sprintf(response, "$SCH,%d,%d,", deviceInfo.scheduleType, deviceInfo.schedulesCount);

	for(i = 0; i < deviceInfo.schedulesCount; i++)
	{
		sprintf(buff, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
			deviceInfo.schedules[i].entryType,
			deviceInfo.schedules[i].dow,
			deviceInfo.schedules[i].month,
			deviceInfo.schedules[i].day,
			deviceInfo.schedules[i].year,
			deviceInfo.schedules[i].start,
			deviceInfo.schedules[i].duration,
			deviceInfo.schedules[i].periodStartDay,
			deviceInfo.schedules[i].periodStartMonth,
			deviceInfo.schedules[i].periodEndDay,
			deviceInfo.schedules[i].periodEndMonth);
		strcat(response, buff);
	}	
	
	//
	// return success
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processSSC(char *pData, long sentenceNumber, long dataChannel)
//
// Set schedules
//
void CommProt_processSSC(char *pData, long sentenceNumber, long dataChannel)
{
	int i;
	char *pToken;
	DeviceInfoS deviceInfo;
	FileRoutines_readDeviceInfo(&deviceInfo);

	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	deviceInfo.scheduleType = atoi(pToken);

	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	deviceInfo.schedulesCount = atoi(pToken);

	for(i = 0; i < deviceInfo.schedulesCount; i++)
	{
		unsigned char periodStartDay = 0;
		unsigned char periodStartMonth = 0; 
		unsigned char periodEndDay = 31;
		unsigned char periodEndMonth = 12;
		int periodValsCnt = 0;

		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.schedules[i].entryType = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.schedules[i].dow = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.schedules[i].month = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.schedules[i].day = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.schedules[i].year = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.schedules[i].start = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if (pToken == NULL)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}
		deviceInfo.schedules[i].duration = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if (pToken != NULL)
		{
			periodStartDay = atoi(pToken);
			periodValsCnt ++;
			pToken = strtok(NULL, ",;");
		}
		if (pToken != NULL)
		{
			periodStartMonth = atoi(pToken);
			periodValsCnt ++;
			pToken = strtok(NULL, ",;");
		}
		if (pToken != NULL)
		{
			periodEndDay = atoi(pToken);
			periodValsCnt ++;
			pToken = strtok(NULL, ",;");
		}
		if (pToken != NULL)
		{
			periodEndMonth = atoi(pToken);
			periodValsCnt ++;
		}

		if (periodValsCnt == 4)
		{
			deviceInfo.schedules[i].periodStartDay = periodStartDay;
			deviceInfo.schedules[i].periodStartMonth = periodStartMonth;
			deviceInfo.schedules[i].periodEndDay = periodEndDay;
			deviceInfo.schedules[i].periodEndMonth = periodEndMonth;
		}
		else
		{
			deviceInfo.schedules[i].periodStartDay = 0;
			deviceInfo.schedules[i].periodStartMonth = 0;
			deviceInfo.schedules[i].periodEndDay = 31;
			deviceInfo.schedules[i].periodEndMonth = 12;
		}
		
	}										  

	FileRoutines_writeDeviceInfo(&deviceInfo);

	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSEP(char *pData, long sentenceNumber, long dataChannel)
//
// Set ethernet params
//
void CommProt_processSEP(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	DeviceInfoS deviceInfo;

	//
	// read the current device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	deviceInfo.useDHCP = atoi(pToken);

	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	deviceInfo.ipAddress = atoll(pToken);

	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	deviceInfo.subnetMask = atoll(pToken);

	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	deviceInfo.gatewayAddress = atoll(pToken);

	//
	// store the new parameters
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);

	//
	// reinit Ethernet controller
	//
	SetupEthernetParams();
	if (ActiveChannel == CHANNEL_ETHERNET)
	{
		CommProt_resetChannel(CHANNEL_ETHERNET);
	}

	//
	// return the response
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processSDN(char *pData, long sentenceNumber, long dataChannel)
//
// Set device name
//
void CommProt_processSDN(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	DeviceInfoS deviceInfo;

	//
	// read the current device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	strncpy(deviceInfo.deviceName, pToken, MAX_STRING_LENGTH);
	deviceInfo.deviceName[MAX_STRING_LENGTH-1] = 0;
	ChangeWifiName(deviceInfo.deviceName);
//	SetFriendlyName(deviceInfo.deviceName);

	//
	// store the new parameters
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);

	//
	// return the response
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processRBC(char *pData, long sentenceNumber, long dataChannel)
//
// Read binary config
//
void CommProt_processRBC(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	DeviceInfoS deviceInfo;
	int offset = 0;
	int length;

	//
	// read the current device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	offset = atoi(pToken);
	length = 128;
	if (offset + length > sizeof(DeviceInfoS))
		length =  sizeof(DeviceInfoS) - offset;

	strcpy(response, "$BCD,");

	HexToString((unsigned char*)&deviceInfo + offset, response + strlen(response), length);

	//
	// return the response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processELI(char *pData, long sentenceNumber, long dataChannel)
//
// Enumerate library images
//
void CommProt_processELI(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	int startIdx;
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	startIdx = atoi(pToken);	

	//
	// create the response
	//
	strcpy(response, "$LIE,");
	FileRoutines_libraryEnumImages(startIdx, response + 5);
	
	//
	// return the response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processELI(char *pData, long sentenceNumber, long dataChannel)
//
// Enumerate library sequences
//
void CommProt_processELS(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	int startIdx;
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	startIdx = atoi(pToken);	

	//
	// create the response
	//
	strcpy(response, "$LSE,");
	FileRoutines_libraryEnumSequences(startIdx, response + 5);
	
	//
	// return the response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processELI(char *pData, long sentenceNumber, long dataChannel)
//
// Read library image
//
void CommProt_processRLI(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	int imageIdx;
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	imageIdx = atoi(pToken);	

	//
	// create the response
	//
	strcpy(response, "$LID,");
	if (!FileRoutines_libraryReadImage(imageIdx, response + 5))
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	
	//
	// return the response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processELI(char *pData, long sentenceNumber, long dataChannel)
//
// Read library sequence
//
void CommProt_processRLS(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	int seqIdx;
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	seqIdx = atoi(pToken);	

	//
	// create the response
	//
	strcpy(response, "$LSD,");
	if (!FileRoutines_libraryReadSequence(seqIdx, response + 5))
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	
	//
	// return the response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processGRI(char *pData, long sentenceNumber, long dataChannel)
//
// Get radar intensity
//
void CommProt_processGRI(char *pData, long sentenceNumber, long dataChannel)
{	
	//
	// create the response
	//
	sprintf(response, "$RDI,%d", Radar_CurrentIntensity());
		
	//
	// return the response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processRAC(char *pData, long sentenceNumber, long dataChannel)
//
// Radar auto configure
//
void CommProt_processRAC(char *pData, long sentenceNumber, long dataChannel)
{	
	int isOK = 1;

	//
	// do radar autoconf
	//
	isOK = Radar_AutoConf();

		
	//
	// return the response
	//
	CommProt_appendResponse(isOK ? "$EOK" : "$ERR" , sentenceNumber, dataChannel);
}

//
// void CommProt_processCAM(char *pData, long sentenceNumber, long dataChannel)
//
// Get camera snapshot
//
void CommProt_processCAM(char *pData, long sentenceNumber, long dataChannel)
{	
	char *pToken;
	int quality;
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
	quality = atoi(pToken);
	CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
	return;
	
	if (takePic(quality, "/dev/shm/snap.jpg") != 0)
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
	else
		CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processGSS(char *pData, long sentenceNumber, long dataChannel)
//
// Get camera snapshot
//
void CommProt_processGSS(char *pData, long sentenceNumber, long dataChannel)
{	
	char *pToken;
	int offset;

	int useEncryption = UseEncryption;
	UseEncryption = 0;

	return;
	
	//
	// get the command parameters
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		UseEncryption = useEncryption;
		return;
	}
	offset = atoi(pToken);

	FILE *img = fopen("/dev/shm/snap.jpg", "rb");
	if (img) {
		char chunk[1024];
		int length = 900;
		fseek(img, 0, SEEK_END);
		int totalLen = ftell(img);
		fseek(img, offset, SEEK_SET);
		sprintf(chunk, "\x19\x20%d,%d,%d,\x18", offset, totalLen, length);
		int chunkLen = strlen(chunk);
		fread(chunk + strlen(chunk), 1, length, img);
		fclose(img);
		chunkLen += 900;

		if (ActiveChannel == CHANNEL_ETHERNET)
			tcpWrite(ActiveEthSock, (unsigned char*)chunk, chunkLen);
	}
	else
		CommProt_appendResponse("$RTR", sentenceNumber, dataChannel);
	
	UseEncryption = useEncryption;
		
	//
	// no response returned immiately!!!
	//
}


//
// void CommProt_processGBC(char *pData, long sentenceNumber, long dataChannel)
//
// Get bitmap configuration
//
void CommProt_processGBC(char *pData, long sentenceNumber, long dataChannel)
{
	int i, j;
	char buff[32];
	DeviceInfoS deviceInfo;	
		
	//
	// read device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	sprintf(response, "$BCF,");

	for(i = 0; i < 4; i++)
	{
		sprintf(buff, "%d,", deviceInfo.bitmapsConfig[i].speedDisplayMode);
		strcat(response, buff);
		sprintf(buff, "%d,", deviceInfo.bitmapsConfig[i].x);
		strcat(response, buff);
		sprintf(buff, "%d,", deviceInfo.bitmapsConfig[i].y);
		strcat(response, buff);
		sprintf(buff, "%d,", deviceInfo.bitmapsConfig[i].font);
		strcat(response, buff);

		sprintf(buff, "%d,", deviceInfo.bitmapsConfig[i].frameLength);
		strcat(response, buff);

		sprintf(buff, "%d,", deviceInfo.bitmapsConfig[i].numFrames);
		strcat(response, buff);

		for(j = 0; j < deviceInfo.bitmapsConfig[i].numFrames; j++)
		{
			sprintf(buff, "%d,", deviceInfo.bitmapsConfig[i].frames[j]);
			strcat(response, buff);
		}
	}
	

	//
	// return result
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processSAD(char *pData, long sentenceNumber, long dataChannel)
//
// Set auto dimming
//
void CommProt_processSAD(char *pData, long sentenceNumber, long dataChannel)
{
	int i;
	DeviceInfoS deviceInfo;
	char *pToken;

	if (UserLevel != 0)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}

	FileRoutines_readDeviceInfo(&deviceInfo);
	
	for(i = 0; i < 16; i++)
	{
		int brightness, luminance;
		brightness = atoi(pToken);

		pToken = strtok(NULL, ",;");
		if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
		{
			return;
		}
		luminance = atoi(pToken);
		pToken = strtok(NULL, ",;");
		if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
		{
			return;
		}

		deviceInfo.autoDimming[i].brightness = brightness;
		deviceInfo.autoDimming[i].luminance = luminance;		
	}

	//
	// save the changed password
	//
	FileRoutines_writeDeviceInfo(&deviceInfo);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processBMP(char *pData, long sentenceNumber, long dataChannel)
//
// Upload bitmap
//
void CommProt_processBMP(char *pData, long sentenceNumber, long dataChannel)
{
	BitmapS bitmap;
	char *pToken;
	int imageIdx, imageLength;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	imageIdx = atoi(pToken) - 1;
	pToken = strtok(NULL, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	imageLength = atoi(pToken);

	if (imageIdx >= MAX_BITMAPS)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	pToken = strtok(NULL, ",;");
	if (pToken == NULL)
	{
		return;
	}	

	StringToHex(pToken, (unsigned char*)&bitmap, imageLength); 
	
	//
	// save the changed password
	//
	FileRoutines_writeBitmap(imageIdx, &bitmap);	
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// void CommProt_processGBM(char *pData, long sentenceNumber, long dataChannel)
//
// Download bitmap
//
void CommProt_processGBM(char *pData, long sentenceNumber, long dataChannel)
{
	BitmapS bitmap;
	char *pToken;
	int imageIdx;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	imageIdx = atoi(pToken) - 1;	
	
	//
	// read the bitmap
	//
	sprintf(response, "$BMP,%d,", imageIdx);
	FileRoutines_readBitmap(imageIdx, &bitmap);
	HexToString(bitmap.bitmapData, response + strlen(response), sizeof(bitmap.bitmapData));

	//
	// return success
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processGCB(char *pData, long sentenceNumber, long dataChannel)
//
// Download bitmap (compressed)
//
void CommProt_processGCB(char *pData, long sentenceNumber, long dataChannel)
{
	BitmapS bitmap;
	char *pToken;
	int imageIdx;

	//
	// read parameters
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= MAX_STRING_LENGTH))
	{
		return;
	}
	imageIdx = atoi(pToken) - 1;	
	
	//
	// read the bitmap
	//
	sprintf(response, "$BMC,%d,", imageIdx);
	FileRoutines_readBitmap(imageIdx, &bitmap);
	EncodeBase64((char*)bitmap.bitmapData, sizeof(bitmap.bitmapData), response + strlen(response));
	strcat(response, ",");
	FileRoutines_readBitmap(imageIdx + 1, &bitmap);
	EncodeBase64((char*)bitmap.bitmapData, sizeof(bitmap.bitmapData), response + strlen(response));

	//
	// return success
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processGAD(char *pData, long sentenceNumber, long dataChannel)
//
// Get auto dimming
//
void CommProt_processGAD(char *pData, long sentenceNumber, long dataChannel)
{
	int i;
	DeviceInfoS deviceInfo;
	char tmpBuffer[64];


	//
	// read parameters
	//
	FileRoutines_readDeviceInfo(&deviceInfo);

	strcpy(response, "$ADT,");
	
	for(i = 0; i < 16; i++)
	{

		sprintf(tmpBuffer, "%d,", deviceInfo.autoDimming[i].brightness);
		strcat(response, tmpBuffer);

		sprintf(tmpBuffer, "%d,", deviceInfo.autoDimming[i].luminance);
		strcat(response, tmpBuffer);		
	}

	
	//
	// return success
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processLOF(char *pData, long sentenceNumber, long dataChannel)
//
// Log off request.
//
void CommProt_processLOF(char *pData, long sentenceNumber, long dataChannel)
{
	//
	// reset login data
	//
	UserLoggedOn = 0;
	ActiveChannel = CHANNEL_NONE;

	FileRoutines_addLog(LOG_DISCONNECTED, NULL);
	
	//
	// return success
	//
	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}


//
// void CommProt_processINF(char *pData, long sentenceNumber, long dataChannel)
//
// Processes the INF request.
//
void CommProt_processINF(char *pData, long sentenceNumber, long dataChannel)
{
	char tmpBuffer[64];
	DeviceInfoS deviceInfo;

	//
	// read the current device info
	//
	FileRoutines_readDeviceInfo(&deviceInfo);
	
	//
	// format the response
	//
	strcpy(response, "$INR,");

	//
	// add the firmware version
	//
	sprintf(tmpBuffer, "%s,", FIRMWARE_VERSION);
	strcat(response, tmpBuffer);

	//
	// add the rotary switch status - it does not exist on interface board, return zero
	//
	sprintf(tmpBuffer, "%d,", 0);
	strcat(response, tmpBuffer);
	
	//
	// get the device time
	//
	sprintf(tmpBuffer, "%d", RTC_YEAR);
	strcat(response, tmpBuffer);
	strcat(response, "-");
	sprintf(tmpBuffer, "%02d", RTC_MONTH);
	if (strlen(tmpBuffer) == 1)
		strcat(response, "0");
	strcat(response, tmpBuffer);
	strcat(response, "-");
	sprintf(tmpBuffer, "%02d", RTC_DOM);
	if (strlen(tmpBuffer) == 1)
		strcat(response, "0");
	strcat(response, tmpBuffer);
	strcat(response, " ");
	sprintf(tmpBuffer, "%02d", RTC_HOUR);
	if (strlen(tmpBuffer) == 1)
		strcat(response, "0");
	strcat(response, tmpBuffer);
	strcat(response, ":");
	sprintf(tmpBuffer, "%02d", RTC_MIN);
	if (strlen(tmpBuffer) == 1)
		strcat(response, "0");
	strcat(response, tmpBuffer);
	strcat(response, ":");
	sprintf(tmpBuffer, "%02d,", RTC_SEC);
	if (strlen(tmpBuffer) == 1)
		strcat(response, "0");
	strcat(response, tmpBuffer);

	//
	// add Ethernet parameters
	//
	sprintf(tmpBuffer, "%d,", deviceInfo.useDHCP);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", (2 << 24) | (0 << 16) | (168 << 8) | (192));
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.ipAddress);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.subnetMask);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.gatewayAddress);
	strcat(response, tmpBuffer);

	//
	// add the voltage level
	//
	sprintf(tmpBuffer, "%d.%01d,", (int)SupplyVoltageLevel, ((int)(SupplyVoltageLevel * 10.00)) % 10);
	strcat(response, tmpBuffer);

	//
	// add the device info
	//
	sprintf(tmpBuffer, "%d,", deviceInfo.radarBaudRate);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.radarProtocol);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.minDisplaySpeed);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.blinkLimit);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.blinkLimitFromRotary);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.maxDisplaySpeed);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.maxSpeedFromRotary);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.blinkOnDurationMs);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.blinkOffDurationMs);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.updateDisplayLengthMs);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.updateDisplaySpeedDelta);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.keepLastSpeedLengthMs);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", deviceInfo.displayBrightness);
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%d,", GetLuxmeterValue());
	strcat(response, tmpBuffer);

	sprintf(tmpBuffer, "%s,", (char*)deviceInfo.deviceName);
	strcat(response, tmpBuffer);

	//
	// add the memory card status and safe remove status
	//
	sprintf(tmpBuffer, "%d,%d,", MemoryCardStatus, MemoryCardSafeRemove);
	strcat(response, tmpBuffer);

	//
	// add the memory card total capacity and free capacity
	//
	sprintf(tmpBuffer, "%d,%d,", MemoryCardTotalSpace, MemoryCardFreeSpace);
	strcat(response, tmpBuffer);

	//
	// the radar sensitivity
	//
	sprintf(tmpBuffer, "%d", deviceInfo.sensitivity);
	strcat(response, tmpBuffer);


	//
	// return response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processGDT(char *pData, long sentenceNumber, long dataChannel)
//
// Processes the Get Device Type request.
//
void CommProt_processGDT(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	char response[256];

	unsigned char G[1];
	unsigned char P[32];
	unsigned char publicKeyA[32];
	unsigned char publicKeyB[32];

	int tokenCount = 0;

	//
	// check if encryption is required; extract all tokens
	//
	pToken = strtok(pData, ",;");
	if ((pToken != NULL) && (strlen(pToken) < 8))
	{
		StringToHex(pToken, G, 1);
		tokenCount ++;
	}

	pToken = strtok(NULL, ",;");
	if ((pToken != NULL) && (strlen(pToken) <= 64))
	{
		StringToHex(pToken, P, 32);
		tokenCount ++;
	}

	pToken = strtok(NULL, ",;");
	if ((pToken != NULL) && (strlen(pToken) <= 64))
	{
		StringToHex(pToken, publicKeyA, 32);
		tokenCount ++;
	}

	pToken = strtok(NULL, ",;");
	if (pToken != NULL)
	{
		tokenCount ++;
	}

	//
	// look at token count to see if encryption parameters are received
	// and crate response
	//
	if (tokenCount == 4)
	{
		char publicKeyBHex[65];

		DiffieHellman_GetKeys(G, P, publicKeyA, SharedSecret, publicKeyB);
		
		HexToString(publicKeyB, publicKeyBHex, 32);

		sprintf(response, "$DTY,%s,", DEVICE_TYPE_STRING);
		strcat(response, publicKeyBHex);
	}
	else
	{
		sprintf(response, "$DTY,%s", DEVICE_TYPE_STRING);
	}

	//
	// return response
	//
	CommProt_appendResponse(response, sentenceNumber, dataChannel);

	//
	// start to use encryption with all subsequent messages
	//
	if (tokenCount == 4)
	{
		UseEncryption = 1;
		printf("UseEncryption = 1\n");
	}
}


//
// void CommProt_processGFI(char *pData, long sentenceNumber, long dataChannel)
//
// Starts retrieving the file.
//
void CommProt_processGFI(char *pData, long sentenceNumber, long dataChannel)
{
	FILEHANDLE fh;
	char *pToken;
	char response[512];
	char tmpBuffer[64];
	char fileData[345];
	char encodedData[480];
	int fileDataLength;
	char fname[128];
	int i;
	//
	// get the desired filename
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= 512))
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	for (i = 0; i<strlen(pToken); i++)
		if (pToken[i] == '\\') pToken[i] = '/';

    strcpy(fname, BASE_DATA);
	strcat(fname, pToken);
	printf("fname: %s\r\n", fname);

	strcpy(CrntFileTransferName, fname);
	//
	// try to open a file
	//
	fh = _sys_open(fname, OPEN_R);
	if (fh == -1)
	{
		printf("ERR-11: %s - %s\r\n", fname, strerror(errno));
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	//
	// create the response
	//
	strcpy(response, "$FIL,");

	sprintf(tmpBuffer, "%d", _sys_flen(fh));
	strcat(response, tmpBuffer);
	strcat(response, ",");

	if (_sys_flen(fh) <= 345)
	{
		strcat(response, "1,");
		fileDataLength = _sys_flen(fh);
	}
	else
	{
		strcat(response, "0,");
		fileDataLength = 345;
	}

	CrntFileTransferPos = fileDataLength;

	//
	// read file data and encode it
	//
	_sys_read(fh, (unsigned char*)fileData, fileDataLength, 0);
	_sys_close(fh);

	EncodeBase64(fileData, fileDataLength, encodedData);

	strcat(response, encodedData);	
	
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
	printf("Response: %s\r\n", response);
}

//
// void CommProt_processCFI(char *pData, long sentenceNumber, long dataChannel)
//
// Continues retrieving the file.
//
void CommProt_processCFI(char *pData, long sentenceNumber, long dataChannel)
{
	FILEHANDLE fh;
	char response[512];
	char tmpBuffer[64];
	char fileData[345];
	char encodedData[480];
	int fileDataLength;

	//
	// try to open a file
	//
	fh = _sys_open(CrntFileTransferName, OPEN_R);
	if (fh == -1)
	{
        printf("ERR:%s %s\n", CrntFileTransferName, strerror(errno));
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}


	_sys_seek(fh, CrntFileTransferPos);


	//
	// create the response
	//
	strcpy(response, "$FIL,");

	sprintf(tmpBuffer, "%d", _sys_flen(fh));
	strcat(response, tmpBuffer);
	strcat(response, ",");

	if ((_sys_flen(fh) - CrntFileTransferPos) <= 345)
	{
		strcat(response, "1,");
		fileDataLength = _sys_flen(fh) - CrntFileTransferPos;
	}
	else
	{
		strcat(response, "0,");
		fileDataLength = 345;
	}

	//
	// read file data and encode it
	//
	_sys_read(fh, (unsigned char*)fileData, fileDataLength, 0);
	
	_sys_close(fh);

	CrntFileTransferPos += fileDataLength;

	EncodeBase64(fileData, fileDataLength, encodedData);

	strcat(response, encodedData);	
	
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processGDL(char *pData, long sentenceNumber, long dataChannel)
//
// Starts sending the directory listing.
//
void CommProt_processGDL(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	static char response[512];
	char tmpBuffer[32];
	int i;
	
	//
	// get the desired directory
	//
	pToken = strtok(pData, ",;");
	if ((pToken == NULL) || (strlen(pToken) >= 450))
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}
    
	//
	// create the file search mask
	//
	strcpy(FileSearchMask, BASE_DATA);
	strcat(FileSearchMask, pToken);

    // Translate \ with /
    for (i = 0; i<strlen(FileSearchMask); i++)
	{
        if ( '\\' == FileSearchMask[i] ) 
		{
            FileSearchMask[i] = '/';
		}
	}

    if ('/' == FileSearchMask[strlen(FileSearchMask) - 1] )
	{
        FileSearchMask[strlen(FileSearchMask) - 1] = 0;
	}

	FileSearchFirstID = 0;

	//
	// format the response
	//
	strcpy(response, "$DIR");

	// force new search
	ffind("/dev/null/xyz", &FileSearchInfo);

	//
	// start searching
	//
	while(ffind(FileSearchMask, &FileSearchInfo) == 0)
	{
		// check if the foudn file was already listed
		if ((FileSearchFirstID == FileSearchInfo.fileID) || (!FileSearchInfo.valid) || (strlen(FileSearchInfo.name) == 0))
		{
			break;
		}

		// check if the response is too large
		if ((strlen(response) + strlen(FileSearchInfo.name)) > 450)
		{
			break;
		}

		if (FileSearchInfo.attrib & ATTR_DIRECTORY)
		{
			strcat(response, ",");
			strcat(response, FileSearchInfo.name);
			strcat(response, "\\,-1,");

			sprintf(tmpBuffer, "%d-%02d-%02d  %02d:%02d", FileSearchInfo.time.year, FileSearchInfo.time.mon, FileSearchInfo.time.day,
				FileSearchInfo.time.hr, FileSearchInfo.time.min);
			strcat(response, tmpBuffer);
		}
		else
		{
			strcat(response, ",");
			strcat(response, FileSearchInfo.name);
			sprintf(tmpBuffer, ",%d,", FileSearchInfo.size);
			strcat(response, tmpBuffer);

			sprintf(tmpBuffer, "%d-%02d-%02d  %02d:%02d", FileSearchInfo.time.year, FileSearchInfo.time.mon, FileSearchInfo.time.day,
				FileSearchInfo.time.hr, FileSearchInfo.time.min);
			strcat(response, tmpBuffer);
		}
	}	

	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processCDL(char *pData, long sentenceNumber, long dataChannel)
//
// Processes the "continue sending directory listing" command.
//
void CommProt_processCDL(char *pData, long sentenceNumber, long dataChannel)
{
	char response[512];
	char tmpBuffer[32];
	
	//
	// format the response
	//
	strcpy(response, "$DIR");

	//
	// continue searching
	//
	if (FileSearchInfo.valid) {
		do
		{
			// check if the foudn file was already listed
			if ((FileSearchFirstID == FileSearchInfo.fileID) || (!FileSearchInfo.valid) || (strlen(FileSearchInfo.name) == 0))
			{
				break;
			}

			// check if the response is too large
			if ((strlen(response) + strlen(FileSearchInfo.name)) > 450)
			{
				break;
			}

			if (FileSearchInfo.attrib & ATTR_DIRECTORY)
			{
				strcat(response, ",");
				strcat(response, FileSearchInfo.name);
				strcat(response, "\\,-1,");

				sprintf(tmpBuffer, "%d-%02d-%02d  %02d:%02d", FileSearchInfo.time.year, FileSearchInfo.time.mon, FileSearchInfo.time.day,
					FileSearchInfo.time.hr, FileSearchInfo.time.min);
				strcat(response, tmpBuffer);
			}
			else
			{
				strcat(response, ",");
				strcat(response, FileSearchInfo.name);
				sprintf(tmpBuffer, ",%d,", FileSearchInfo.size);
				strcat(response, tmpBuffer);

				sprintf(tmpBuffer, "%d-%02d-%02d  %02d:%02d", FileSearchInfo.time.year, FileSearchInfo.time.mon, FileSearchInfo.time.day,
					FileSearchInfo.time.hr, FileSearchInfo.time.min);
				strcat(response, tmpBuffer);
			}
		}	while(ffind(FileSearchMask, &FileSearchInfo) == 0);
	}

	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processEFC(char *pData, long sentenceNumber, long dataChannel)
//
// Executes a filesystem command.
//
void CommProt_processEFC(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
    char filename[128];
	
	//
	// get the command
	//
	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	//
	// is this the DELETE command ?
	//
	if (strcmp(pToken, "DEL") == 0)
	{
        int i;
		pToken = strtok(NULL, ",;");
        strcpy(filename, BASE_DATA);
        if (pToken != NULL)
            strcat(filename, pToken);
        for (i = 0; i<strlen(filename); i++)
             if (filename[i] == '\\')
                 filename[i] = '/';

//		if ((pToken == NULL) || 
//		    (MemoryCardStatus == MC_STATUS_NOCARD) || 
//			(MemoryCardStatus == MC_STATUS_CARDERROR) || 
//			(MemoryCardStatus == MC_STATUS_READONLY) ||
//			(MemoryCardSafeRemove == 1) ||
		if (fdelete(filename) != 0)
		{
			CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
			return;
		}

		Sync();

		CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
		return;
	}

	//
	// unknown command, return error
	//
	CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
}

//
// void CommProt_processGUN(char *pData, long sentenceNumber, long dataChannel)
//
// Gen UNit type - returns the currently set Unit type
//
void CommProt_processGUN(char *pData, long sentenceNumber, long dataChannel)
{
	DeviceInfoS deviceInfo;
	FileRoutines_readDeviceInfo(&deviceInfo);
	sprintf(response, "$UNT,%d", deviceInfo.unitType);
	CommProt_appendResponse(response, sentenceNumber, dataChannel);
}

//
// void CommProt_processSUN(char *pData, long sentenceNumber, long dataChannel)
//
// Set Unit Type - sets the unit type
//
void CommProt_processSUN(char *pData, long sentenceNumber, long dataChannel)
{
	char *pToken;
	DeviceInfoS deviceInfo;
	FileRoutines_readDeviceInfo(&deviceInfo);

	if (UserLevel != 0)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	pToken = strtok(pData, ",;");
	if (pToken == NULL)
	{
		CommProt_appendResponse("$ERR", sentenceNumber, dataChannel);
		return;
	}

	deviceInfo.unitType = atoi(pToken);
	FileRoutines_writeDeviceInfo(&deviceInfo);

	CommProt_appendResponse("$EOK", sentenceNumber, dataChannel);
}

//
// unsigned int CalculateCRC32(char *pInputStream, int dataLength)
//
// Calculates CRC-32 checksum.
//
unsigned int CalculateCRC32(char *pInputStream, int dataLength)
{
	int i;
	unsigned int hash;
	unsigned char outByte;
	
	hash = 0;
	for(i = 1; i < dataLength; i++) // skip the first character
	{
		outByte = (unsigned char) (hash & 0x000000FF);
		hash = CRCLookupTable[((unsigned char)pInputStream[i]) ^ outByte] ^ (hash >> 8);
	}

	return hash & 0x7FFFFFFF;
}

//
// void EncodeBase64(char *pData, unsigned int dataLength, char *pEncodedData)
//
// Base64 encodes the data in the param1. 
//
void EncodeBase64(char *pData, unsigned int dataLength, char *pEncodedData)
{
	int i;
	int outPos = 0;
	const char EncoderLookup[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned int token;

	//
	// convert the data
	//
	for(i = 0; i < dataLength / 3; i++)
	{
		token = ((pData[i*3] & 0x000000FF) << 16) |
		        ((pData[i*3 + 1] & 0x000000FF) << 8) |
				((pData[i*3 + 2] & 0x000000FF));

		pEncodedData[outPos] = EncoderLookup[(token >> 18) & 0x003F];
		outPos ++;

		pEncodedData[outPos] = EncoderLookup[(token >> 12) & 0x003F];
		outPos ++;

		pEncodedData[outPos] = EncoderLookup[(token >> 6) & 0x003F];
		outPos ++;

		pEncodedData[outPos] = EncoderLookup[(token >> 0) & 0x003F];
		outPos ++;
	}

	// add the padding
	// right-pad the data with zeroes
	if ((dataLength % 3) == 1)
	{
		token = ((pData[dataLength - 1] & 0x000000FF) << 4);

		pEncodedData[outPos] = EncoderLookup[(token >> 6) & 0x003F];
		outPos ++;

		pEncodedData[outPos] = EncoderLookup[(token >> 0) & 0x003F];
		outPos ++;

		pEncodedData[outPos] = '=';
		outPos ++;

		pEncodedData[outPos] = '=';
		outPos ++;
	}
	if ((dataLength % 3) == 2)
	{
		token = ((pData[dataLength - 2] & 0x000000FF) << 10) |
				((pData[dataLength - 1] & 0x000000FF) << 2);

		pEncodedData[outPos] = EncoderLookup[(token >> 12) & 0x003F];
		outPos ++;
		
		pEncodedData[outPos] = EncoderLookup[(token >> 6) & 0x003F];
		outPos ++;

		pEncodedData[outPos] = EncoderLookup[(token >> 0) & 0x003F];
		outPos ++;

		pEncodedData[outPos] = '=';
		outPos ++;
	}	

	//
	// add the zero-terminator
	//
	pEncodedData[outPos] = '\0';	 
}

//
// float StringToFloat(char *pString)
//
// Converts string to float.
//
float StringToFloat(char *pString)
{
	float result = 0;
	int sign = 1;
	float divisor = 10;

	//
	// check the sign
	//
	if (pString[0] == '-')
	{
		sign = -1;
		pString ++;
	}

	//
	// parse the integer part
	//
	while ((*pString != '.') && (*pString != '\0'))
	{
		// check for invalid digits
		if ((*pString < '0') || (*pString > '9'))
			break;

		result = result * 10 + (*pString - '0');

		*pString ++;
	}

	//
	// parse the fraction part
	//
	if (*pString == '.')
	{
		*pString ++;
		while (*pString != '\0')
		{
			// check for invalid digits
			if ((*pString < '0') || (*pString > '9'))
				break;
	
			result = result + (*pString - '0') / divisor;
			divisor = divisor * 10;

			*pString ++;
		}
	}

	return result * sign;
}

//
// void StringToHex(char *pHexString, unsigned char *pValueBigNum, int bigNumLength)
//
// Converts the hex string to bignum value of the specified length.
// 
//
void StringToHex(char *pHexString, unsigned char *pValueBigNum, int bigNumLength)
{
	int i;
	int pos;

	//
	// initialize the array to all zeros
	//
	memset(pValueBigNum, 0, bigNumLength);

	//
	// parse HEX data
	//
	pos = bigNumLength - 1;

	for(i = strlen(pHexString) - 1; i >= 0, pos >= 0; i--)
	{
		unsigned char digitValue = 0;

		if ((pHexString[i] >= '0') && (pHexString[i] <= '9'))
			digitValue = pHexString[i] - '0';
		else if ((pHexString[i] >= 'A') && (pHexString[i] <= 'F'))
			digitValue = pHexString[i] - 'A' + 10;
		else if ((pHexString[i] >= 'a') && (pHexString[i] <= 'f'))
			digitValue = pHexString[i] - 'a' + 10;

		if ((i % 2) == 0)
			digitValue = digitValue << 4;

		pValueBigNum[pos] |= digitValue;

		if ((i % 2) == 0)
			pos --;
	}
}

//
// void HexToString(unsigned char *pValueBigNum, char *pHexString, int bigNumLength)
//
// Converts the big num value to a hex string.
//
void HexToString(unsigned char *pValueBigNum, char *pHexString, int bigNumLength)
{
	int i;

	for(i = 0; i < bigNumLength; i++)
	{
		pHexString[i*2] = HexDigit[(pValueBigNum[i] >> 4) & 0x0F];
		pHexString[i*2 + 1] = HexDigit[pValueBigNum[i] & 0x0F];
	}

	pHexString[bigNumLength * 2] = '\0';
}

//
// void DecryptSentence(char *pData, int dataLength, char *pDecryptedSentence)
//
// Decrypts the sentence.
//
void DecryptSentence(char *pData, int dataLength, char *pDecryptedSentence)
{
	int i, pos = 0;

	//
	// first, unescape the sentence
	//
	for(i = 1; i < (dataLength - 1); i++)
	{
		if (pData[i] == 0x01)
		{
			i ++;
			if (pData[i] == 0x01)
				pDecryptedSentence[pos] = 0x01;
			else if (pData[i] == 0x02)
				pDecryptedSentence[pos] = 0x00;
			else if (pData[i] == 0x03)
				pDecryptedSentence[pos] = 0x18;
			else if (pData[i] == 0x04)
				pDecryptedSentence[pos] = 0x19;
		}
		else
			pDecryptedSentence[pos] = pData[i];

		pos ++;
	}

	//
	// decrypt the sentence
	//
	memcpy(pData, pDecryptedSentence, pos);

	AES__AES();
	AES__SetParameters(128, 128);
	
	AES__StartDecryption(SharedSecret);
	AES__Decrypt((unsigned char*)pData, (unsigned char*)pDecryptedSentence, pos / 16, CBC);

	//
	// shift the sentence
	//
	for(i = pos; i > 0; i--)
		pDecryptedSentence[i] = pDecryptedSentence[i-1];


	//
	// add the sentence start and end
	//
	pDecryptedSentence[0] = 0x19;
	pDecryptedSentence[pos + 1] = 0x18;
}

//
// int EncryptSentence(char *pData, char *pEncryptedSentence)
//
// Encrypts the data to be transferred over link.
//
int EncryptSentence(char *pData, char *pEncryptedSentence)
{
	int i, pos;
	int size;

	//
	// encrypt the data
	//
	size = strlen(pData);

	AES__AES();
	AES__SetParameters(128, 128);
	
	AES__StartEncryption(SharedSecret);
	AES__Encrypt((unsigned char*)pData, (unsigned char*)pEncryptedSentence, size / 16, CBC);


	//
	// escape the encrypted data
	//
	memcpy(pData, pEncryptedSentence, size);
	pos = 0;
	for(i = 0; i < size; i++)
	{
		if (pData[i] == 0x00)
		{
			pEncryptedSentence[pos++] = 0x01;
			pEncryptedSentence[pos++] = 0x02;
		}
		else if (pData[i] == 0x01)
		{
			pEncryptedSentence[pos++] = 0x01;
			pEncryptedSentence[pos++] = 0x01;
		}
		else if (pData[i] == 0x18)
		{
			pEncryptedSentence[pos++] = 0x01;
			pEncryptedSentence[pos++] = 0x03;
		}
		else if (pData[i] == 0x19)
		{
			pEncryptedSentence[pos++] = 0x01;
			pEncryptedSentence[pos++] = 0x04;
		}
		else
		{
			pEncryptedSentence[pos++] = pData[i];
		}
	}

	//
	// return new size
	//
	return pos;
}
