#ifndef __COMMPROT_H_INCLUDED 
#define __COMMPROT_H_INCLUDED

#include "main.h"

//
// Constants that define communications channel.
//
#define CHANNEL_NONE		-1
#define CHANNEL_SERIAL_A	0
#define CHANNEL_BLUETOOTH	1
#define CHANNEL_ETHERNET	2
#define COMM_BUFFER_SIZE	1536

#ifdef LARGER_DOTS
#define	FIRMWARE_VERSION	"3.0.0-L"
#else
#define	FIRMWARE_VERSION	"4.0.1.0"
#endif

#define DEVICE_TYPE_STRING	"MS_SpeedVMS"
			 
#define CONNECTION_TIMEOUT_TIME   600   // seconds


//
// Global communication variables
//
extern char IncomingBufferSerialA[COMM_BUFFER_SIZE];
extern char IncomingBufferBluetooth[COMM_BUFFER_SIZE];
extern char IncomingBufferEthernet[COMM_BUFFER_SIZE];
extern char* IncommingBuffer[3];
extern unsigned long IncommingBufferLength[3];
extern char OutgoingBufferSerialA[COMM_BUFFER_SIZE];
extern char OutgoingBufferBluetooth[COMM_BUFFER_SIZE];
extern char OutgoingBufferEthernet[COMM_BUFFER_SIZE];
extern char* OutgoingBuffer[3];
extern unsigned long OutgoingBufferLength[3];
extern char LastSentSentence[COMM_BUFFER_SIZE];
extern int LastSentSentenceSize;
extern int ActiveChannel;
extern long ExpectedSentenceNum;
extern long LastCommunicationTime;

//
// Function prototypes
//
void CommProt_init(void);
void CommProt_resetChannel(int dataChannel);
void CommProt_checkForTimeout(void);
void CommProt_KeepAlive(void);
int CommProt_getOutputBuffer(char* pBuffer, int maxBufferSize, int dataChannel);
void CommProt_processStream(char* pData,
                            unsigned long dataLength,
                            long dataChannel);
void CommProt_processIncommingBuffer(long dataChannel);
void CommProt_processSentence(char *pData, long dataLength, long dataChannel);
void CommProt_appendResponse(const char *pData, long sentenceNumber, long dataChannel);
void CommProt_resendLastResponse(long dataChannel);
void CommProt_processLGN(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processTIM(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processPSS(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processLOF(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processINF(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGDT(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGFI(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processCFI(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGDL(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processCDL(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSBR(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSRS(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSPT(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSMI(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSMA(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSLI(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSDL(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSBP(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processEFC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processEMC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSDU(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGPC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSPC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGAD(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSAD(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processBMP(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGBM(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGCB(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGBC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSBC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processTST(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGSC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSSC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSEP(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSDN(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processRBC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processELI(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processELS(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processRLI(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processRLS(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGRI(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processRAC(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processCAM(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGSS(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processGUN(char *pData, long sentenceNumber, long dataChannel);
void CommProt_processSUN(char *pData, long sentenceNumber, long dataChannel);

unsigned int CalculateCRC32(char *pInputStream, int dataLength);
void EncodeBase64(char *pData, unsigned int dataLength, char *pEncodedData);
float StringToFloat(char *pString);
void StringToHex(char *pHexString, unsigned char *pValueBigNum, int bigNumLength);
void HexToString(unsigned char *pValueBigNum, char *pHexString, int bigNumLength);
void DecryptSentence(char *pData, int dataLength, char *pDecryptedSentence);
int EncryptSentence(char *pData, char *pEncryptedSentence);

#endif // __COMMPROT_H_INCLUDED
