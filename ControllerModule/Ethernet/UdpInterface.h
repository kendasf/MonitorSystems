#ifndef __UDPINTERFACE_H__
#define __UDPINTERFACE_H__
#include <sys/socket.h>
#include <netinet/in.h>

int PostMessage(int hSock, char *HostAddr, int port, void *pMsg, int msgLen, int binary);
char *udpReadf(int hSock, int *pLen, struct sockaddr *from, unsigned long lTmoMs);
char *udpRead(int hSock, int *pLen, unsigned long lTmoMs);
int GetUdpServerHandle(int PortNo);

int udpSendAddr(int hSock, struct sockaddr *addr, void *pMsg,  int msgLen);
void udpSendHost(int hSock, char *HostAddr, int port, unsigned char *pMsgBytes, int size);
void udpSendLocal(int hSock, int port, const char *pMsgBytes, int size);

bool udp_send (
    unsigned char  socket,     /* UDP socket to send the data packet from. */
    unsigned char* remip,      /* Pointer to the IP address of the remote machine. */
    unsigned short remport,    /* Port number of remote machine to send the data to. */
    unsigned char* buf,        /* Pointer to buffer containing the data to send. */
    unsigned short dlen );     /* Number of bytes of data to send. */

unsigned char* udp_get_buf (
    unsigned short size);    /* Number of bytes to be sent. */

unsigned char udp_get_socket (
    unsigned char   tos,           /* Type Of Service. */
    unsigned char   opt,           /* Option to calculate or verify the checksum. */
    unsigned short (*listener)(    /* Function to call when TCPnet receives a data packet. */
        unsigned char  socket,     /* Socket handle of the local machine. */
        unsigned char* remip,      /* Pointer to IP address of remote machine. */
        unsigned short port,       /* Port number of remote machine. */
        unsigned char* buf,        /* Pointer to buffer containing the received data. */
        unsigned short len ));     /* Number of bytes in the received data packet. */

bool udp_open (
    unsigned char  socket,      /* Socket handle to use for communication. */
    unsigned short locport);    /* Local port to use for communication. */

typedef struct
{
	int connFd;
	pthread_t thread_id;
	void* callback;
	struct sockaddr_in connAddr;
	socklen_t addrlen;
} TCP_SERVER_ARGS;

typedef struct
{	
	pthread_t thread_id;
	int port;
	void* callbackfunction;
} TCP_LISTENER_ARGS;

typedef struct
{
	bool runflag; // Set to false when closing port to exit thread
	unsigned char   tos;           /* Type Of Service. */
	unsigned char   opt;           /* Option to calculate or verify the checksum. */


	// Callback to handle received data
	unsigned short (*callback)(    /* Function to call when TCPnet receives a data packet. */
        unsigned char  socket,     /* Socket handle of the local machine. */
        unsigned char* remip,      /* Pointer to IP address of remote machine. */
        unsigned short port,       /* Port number of remote machine. */
        unsigned char* buf,        /* Pointer to buffer containing the received data. */
        unsigned short len );

	pthread_t thread_id; // thread ID for UDP Listener

	int connFd; // File descriptor for the socket connection

	int localPort;

	int socketBlockIndex; // Index in the array
} UDP_SOCKET_CONTROL_BLOCK;

#endif
