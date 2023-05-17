#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h> 
#include <pthread.h>
#include <fcntl.h>				// F_SETFL, O_NONBLOCK
#include <netdb.h>
//#include <bits/sockaddr.h>
#include "UdpInterface.h"
#include <stdlib.h>

UDP_SOCKET_CONTROL_BLOCK* udpCtlBlocks[10] = {NULL};


static void *UdpReceiverThread(void* arg); // Receiver Thread

// udp_* functions for Keil like interface

bool udp_send (
    unsigned char  socket,     /* UDP socket to send the data packet from. */
    unsigned char* remip,      /* Pointer to the IP address of the remote machine. */
    unsigned short remport,    /* Port number of remote machine to send the data to. */
    unsigned char* buf,        /* Pointer to buffer containing the data to send. */
    unsigned short dlen )     /* Number of bytes of data to send. */
{
	bool retval = false;
	int status;
	struct sockaddr_in toaddr;
	unsigned char* addrPtr = (unsigned char*) &(toaddr.sin_addr.s_addr);
	toaddr.sin_family = AF_INET;
	toaddr.sin_port = htons(remport);
	addrPtr[0] = remip[0];
	addrPtr[1] = remip[1];
	addrPtr[2] = remip[2];
	addrPtr[3] = remip[3];

	memset(toaddr.sin_zero, 0, sizeof(toaddr.sin_zero));
					
	status = udpSendAddr(udpCtlBlocks[socket]->connFd, (struct sockaddr *)&toaddr, buf, (int) dlen);
	if (status > 0)
		retval = true;

	free(buf);

	return retval;
}

unsigned char* udp_get_buf ( unsigned short size)    /* Number of bytes to be sent. */
{
	unsigned char* retval;

	retval = (unsigned char*)malloc((int) size);

	return retval;
}

unsigned char udp_get_socket (
    unsigned char   tos,           /* Type Of Service. */
    unsigned char   opt,           /* Option to calculate or verify the checksum. */
    unsigned short (*listener)(    /* Function to call when TCPnet receives a data packet. */
        unsigned char  socket,     /* Socket handle of the local machine. */
        unsigned char* remip,      /* Pointer to IP address of remote machine. */
        unsigned short port,       /* Port number of remote machine. */
        unsigned char* buf,        /* Pointer to buffer containing the received data. */
        unsigned short len ))     /* Number of bytes in the received data packet. */
{
	unsigned char retval = -1;
	UDP_SOCKET_CONTROL_BLOCK *ctlblock = NULL;
	int i;
	
	for (i = 0; i<10; i++)
	{
		if (udpCtlBlocks[i] == NULL)
		{
			retval = i;
			break;
		}
	}

	if (retval == -1)
	{
		printf("Max Open sockets reached\r\n");
		return retval;
	}

	udpCtlBlocks[retval] = (UDP_SOCKET_CONTROL_BLOCK*)malloc(sizeof(UDP_SOCKET_CONTROL_BLOCK));
	ctlblock = udpCtlBlocks[retval];

	ctlblock->tos = tos;
	ctlblock->opt = opt;
	ctlblock->socketBlockIndex = retval;


	return retval;
}

bool udp_open (
    unsigned char  socket,      /* Socket handle to use for communication. */
    unsigned short locport)    /* Local port to use for communication. */
{
	bool retval = false;
	UDP_SOCKET_CONTROL_BLOCK *ctlblock = udpCtlBlocks[socket];
	pthread_attr_t attr;
	int status; 

	// Open Port
	ctlblock->localPort = locport;
        ctlblock->connFd = GetUdpServerHandle(locport);
	ctlblock->runflag = 1;
	if (ctlblock->connFd >= 0)
	{
		status  = pthread_attr_init(&attr);
		if (status != 0)
			printf("pthread_attr_init\n");

		printf("<5>Starting UDP Socket for JSON AJAX\\n\n");
		status = pthread_create(&(ctlblock->thread_id), &attr, &UdpReceiverThread, ctlblock);
		if (status != 0)
			printf("pthread create failed for Listen Task\n");

		pthread_attr_destroy(&attr);

		pthread_setname_np(ctlblock->thread_id, "UDP Socket");
		retval = true;
	}
	else
	{
		printf("Udp Open err: %d - %s\r\n", errno, strerror(errno));
	}

	return retval;
}

// Functions used to create thread for listending to a Udp Interface and calls callback for received data

static void *UdpReceiverThread(void* arg)
{
	UDP_SOCKET_CONTROL_BLOCK* sockCtl = (UDP_SOCKET_CONTROL_BLOCK*) arg;
	char*buffer;
	int len;
	struct sockaddr_in from;
	unsigned short port;
	unsigned short status;
	printf("UDP Socket %d on Port %d open\r\n", sockCtl->connFd, sockCtl->localPort);

	while(sockCtl->runflag)
	{
		buffer = udpReadf(sockCtl->connFd, &len, (struct sockaddr*) &from, 5000);
		if (buffer != NULL)
		{
			port = ntohs(from.sin_port);
			printf("Rcvd %d bytes\r\n", len);
    			status = (sockCtl->callback)(    /* Function to call when TCPnet receives a data packet. */
			         	sockCtl->socketBlockIndex,     /* Socket handle of the local machine. */
				        (unsigned char*)&(from.sin_addr.s_addr),      /* Pointer to IP address of remote machine. */
				        port,       /* Port number of remote machine. */
				        (unsigned char*)buffer,        /* Pointer to buffer containing the received data. */
				        len);     /* Number of bytes in the received data packet. */
				if(status)
				{
					;
				}
		}
		else
		{
//			printf("Receive TMO\r\n");
		}
	}
	printf("UDP Socket Receiver %d on Port %d Exit\r\n", sockCtl->connFd, sockCtl->localPort);
	return NULL;
}


// Functions to support sending, receiving UDP message traffic

int GetUdpServerHandle(int PortNo)
{
	int yes = 1;
	int  sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0)
	{
		 printf("ERROR opening socket: %s\n", strerror(errno));
	}
	else
	{
		struct sockaddr_in si_me;
		memset((char *) &si_me, 0, sizeof(si_me));
		si_me.sin_family = AF_INET;
		si_me.sin_port = htons(PortNo);
		si_me.sin_addr.s_addr = htonl(INADDR_ANY);

		if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
		{
			printf("setsockopt() error! errno: %d\n", errno);
			close(sockfd);
			sockfd = -1;
		}

		if ((sockfd > 0) && (bind(sockfd, (struct sockaddr *) &si_me, sizeof(si_me)) < 0))
		{
			printf("Bind Port(%d) ERROR: %s\n", PortNo, strerror(errno));
			close(sockfd);
			sockfd = -1;
		}

	}
	
	return sockfd;
}

char *udpReadf(int hSock, int *pLen, struct sockaddr *from, unsigned long lTmoMs)
{
	char *bufin = NULL;
	int res;
	socklen_t fromlen;
	struct timeval delay;
	fd_set fd;
	int iSel;
	
	delay.tv_sec = (long) (lTmoMs/1000);           // How many seconds
	delay.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in 

	FD_ZERO(&fd);
	FD_SET(hSock, &fd);
	iSel = select(hSock+1, &fd, (fd_set *) 0, (fd_set *) 0, &delay);
	if (iSel > 0)
	{
		fromlen = sizeof(struct sockaddr);
		bufin = (char *) malloc(512);
		memset(bufin, 0, 512);
		res = recvfrom(hSock, bufin, 512, 0, from, &fromlen);
		if (res < 0)
		{
			free(bufin);
			bufin = NULL;
		}
		else if (pLen != NULL)
		{
			*pLen = res;
		}
	}
	return bufin;
}

char *udpRead(int hSock, int *pLen, unsigned long lTmoMs)
{
	char *bufin = NULL;
	int res;
	struct sockaddr from;
	socklen_t fromlen;
	struct timeval delay;
	fd_set fd;
	int iSel;
	
	delay.tv_sec = (long) (lTmoMs/1000);           // How many seconds
	delay.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in 

	FD_ZERO(&fd);
	FD_SET(hSock, &fd);
	iSel = select(hSock+1, &fd, (fd_set *) 0, (fd_set *) 0, &delay);
	if (iSel > 0)
	{
		fromlen = sizeof(struct sockaddr);
		bufin = (char *) malloc(512);
		memset(bufin, 0, 512);
		res = recvfrom(hSock, bufin, 512, 0, &from, &fromlen);
		if (res < 0)
		{
			free(bufin);
			bufin = NULL;
		}
		else if (pLen != NULL)
		{
			*pLen = res;
		}
	}
	return bufin;
}

int udpSendAddr(int hSock, struct sockaddr *addr, void *pMsg, int msgLen)
{
	int result = -1;
	if (hSock >= 0) 
	{
		int i;

		int n = sendto(hSock, pMsg, msgLen, 0, addr, sizeof(struct sockaddr));
		if (n < 0)
		{
			printf("Error, send() failed: %s\n", strerror(errno));
		}
		result = n;
	}		
	return result;
}

void udpSendLocal(int hSock, int port, const char *pMsgBytes, int size)
{
	if (hSock >= 0) 
	{
		struct sockaddr_in addr;
		struct hostent *pServer = gethostbyname("127.0.0.1");
		
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr.s_addr, pServer->h_addr, pServer->h_length);
		addr.sin_port = htons(port);

		int n = sendto(hSock, pMsgBytes, size, 0, (struct sockaddr*)&addr, sizeof(addr));
		if (n < 0)
		{
			printf("Error, send() failed: %s\n", strerror(errno));
		}
	}		
}

void udpSendHost(int hSock, char *HostAddr, int port, unsigned char *pMsgBytes, int size)
{
	if (hSock >= 0) 
	{
		struct sockaddr_in addr;
		struct hostent *pServer = gethostbyname(HostAddr);
		
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		memcpy(&addr.sin_addr.s_addr, pServer->h_addr, pServer->h_length);
		addr.sin_port = htons(port);

		int n = sendto(hSock, pMsgBytes, size, 0, (struct sockaddr*)&addr, sizeof(addr));
		if (n < 0)
		{
			printf("Error, send() failed: %s\n", strerror(errno));
		}
	}		
}


