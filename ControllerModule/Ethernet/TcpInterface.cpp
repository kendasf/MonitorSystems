#include <stdio.h>				// printf, sprintf
#include <errno.h>				// errno
#include <string.h> 			// memset, strlen, strstr, ... etc
#include <arpa/inet.h> 		// struct sockaddr_in, sockaddr, 
#include <unistd.h> 			// usleep
#include <stdlib.h> 			// atioi
#include <sys/socket.h>
#include <netdb.h>
#include <limits.h>				// PTHREAD_STACK_MIN
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "TcpInterface.h"

int getTcpListen(int listenPort, int depth)
{
	int listener = -1;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;
	int yes = 1;
	socklen_t addrlen;
	long ipaddr;

	if((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		printf("Server: socket() error! errno=%d\n", errno);
	}

	if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1)
	{
		printf("Server: setsockopt() error! errno: %d\n", errno);
		close(listener);
	return -1;
	}

	/* bind */
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = INADDR_ANY;
	serveraddr.sin_port = htons(listenPort);
	memset(&(serveraddr.sin_zero), '\0', 8);
	if(bind(listener, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
	{
	   printf("Server: bind() error! errno: %d - %s\n", errno, strerror(errno));
	   close(listener);
	   return -1;
	}
	if(listen(listener, depth) == -1)
	{
	     printf("Server: listen() error! errno: %d\n", errno);
	     close(listener);
		return -1;
	}
	printf("TCP Listen Port: %d , Depth %d\r\n", listenPort, depth);

	return listener;
}


int getTcpActiveSocket(int listener)
{
	int newfd = -1;
	struct timeval tv;
	int status;
	fd_set read_fds, error_fds;
    	FD_ZERO(&read_fds);
    	FD_ZERO(&error_fds);
	FD_SET(listener, &read_fds);
	FD_SET(listener, &error_fds);

	tv.tv_sec = 0;
	tv.tv_usec = 1000 * 5; // 5 ms 

	if((status = select(listener+1, &read_fds, 0, &error_fds, &tv)) == -1)
	{
		printf("Server: select() error!\n");
	}
	else if (status > 0)
	{
		if (FD_ISSET(listener, &read_fds))
		{
			struct sockaddr_in clientaddr;
			long ipaddr;
    	socklen_t addrlen = sizeof(clientaddr);
			if((newfd = accept(listener, (struct sockaddr *)&clientaddr, &addrlen)) < 0)
			{
				printf("accept error: %s - %d\n", strerror(errno), errno);
			}
			else if (newfd > 0)                               
			{
				printf("Server: accept() is OK... fd = %d\n", newfd);

				ipaddr = ntohl(clientaddr.sin_addr.s_addr);
				printf("get connect from: ");
				printf("%d.%d.%d.%d", ((unsigned char *)&ipaddr)[3],
							   ((unsigned char *)&ipaddr)[2],
							   ((unsigned char *)&ipaddr)[1],
							   ((unsigned char *)&ipaddr)[0]);
				printf(":%d\n", ntohs(clientaddr.sin_port));
			}
		}
	}
	// else status == 0 -- Nothing to do

	return newfd;
}

int tcpDataWait(int sockFd, unsigned long lTmoMs)
{
	int iSel;
	struct timeval delay;
	fd_set fd;
	
	delay.tv_sec = (long) (lTmoMs/1000);           // How many seconds
	delay.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in 

	FD_ZERO(&fd);
	FD_SET(sockFd, &fd);
	iSel = select(sockFd+1, &fd, (fd_set *) 0, (fd_set *) 0, &delay);
	if (iSel < 0)
		printf("Sock Error: %d - %s\r\n", errno, strerror(errno));
	return iSel;
}

unsigned char* tcpRecv(int sockFd, int *pLen, unsigned long lTmoMs)
{
	int res;
	struct timeval delay;
	fd_set fd;
	int iSel;
	unsigned char *bufin = (unsigned char*)malloc(1550);

	delay.tv_sec = (long) (lTmoMs/1000);           // How many seconds
	delay.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in 

	FD_ZERO(&fd);
	FD_SET(sockFd, &fd);
	iSel = select(sockFd+1, &fd, (fd_set *) 0, (fd_set *) 0, &delay);
	if (iSel >= 0)
	{
		res = recv(sockFd, bufin, 1550, 0);
		if (res >0)
		{
			if (pLen != NULL)
			{
				*pLen = res;
			}
		}
		else
		{
			free(bufin);
			bufin = NULL;
		}
	}
	return bufin;
}

int tcpWrite(int sockFd, unsigned char* buffer,  int len)
{
	int retval = -1;
	retval = write(sockFd, buffer, len);
	if ((retval < 0)  && (errno != EAGAIN))
		printf("Socket Write Error: %s - %d\r\n", strerror(errno), errno);
	return retval;
}
