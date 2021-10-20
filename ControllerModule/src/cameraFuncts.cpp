#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>    //strlen
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr

#include "SystemUtilities.h"
#include "cameraFuncts.h" // Include for validation

#ifndef SIMULATOR_MODE
char* CAMERA_IP_ADDR = "192.168.0.100";
#else
extern char* CAMERA_IP_ADDR = "192.168.1.163";
#endif

#define CAMERA_PORT 80
//#define CAMERA_PORT 8001

// Time change
// GET /cgi-bin/system?USER=admin&PWD=123456&DATE_CONFIG=1,040100002016,11:57:00,-06&DAYLIGHT_SAVING_CONFIG=0,1,3,2,02:00,7,1,10,1,03:00,7 HTTP/1.1\r\n

static int var_glb; /* A global variable - limited scope*/ 
static int picnum;
static unsigned char image[100000]; //

static char message[1000];
static char server_reply[2000];

static int current_pos;

#define CONTENT_LENGTH "Content-Length: "

int serverSelect(int fd, long lTmoMs)
{
	int retval = 0;
	struct timeval tv;
	int status;
	fd_set read_fds, error_fds;
	FD_ZERO(&read_fds);
	FD_ZERO(&error_fds);
	FD_SET(fd, &read_fds);
	FD_SET(fd, &error_fds);

	tv.tv_sec = (long) (lTmoMs/1000);           // How many seconds
	tv.tv_usec = (long) (lTmoMs % 1000) * 1000; // milliseconds expressed in uS

	if((status = select(fd+1, &read_fds, 0, &error_fds, &tv)) > 0)
	{
		if (FD_ISSET(fd, &read_fds))
		{
			retval = 1;
		}
	}
	return retval;
}

int getContentLength(char* data)
{
	int retval = 0;
	char* posn = strstr(data, CONTENT_LENGTH);
	if (posn != NULL)
	{
		posn += strlen(CONTENT_LENGTH);
		retval = atoi(posn);
	}
	return retval;
}

unsigned char* getContentData(int vsock, int length)
{
	unsigned char* data = NULL;
	if(length > 0)
	{
		int cnt = 0;
		int offset = 0;
		data = (unsigned char*) malloc(length + 1);// Allocate an extra byte for null term
		if (data != NULL)
		{
			data[length] = 0;
			while(offset < length)
			{
				if(serverSelect(vsock, 50))
				{
					cnt = recv(vsock, &data[offset], (length - offset), 0);
					if (cnt > 0)
						offset += cnt; 
					else if (cnt <= 0)
						break;
				}
				else
				{
					break; // Timeout
				}
			}
		}
	}
	return data;
}

int sendCameraCommand(char* hostIP, char* command)
{
	int sock, x, index;
	int msgSize;	
	char* message;
	char* server_reply;
	struct sockaddr_in server;
	sock = socket(AF_INET , SOCK_STREAM , 0);
  
//  printf("--- sendCameraCommand %s ---\n", command);

	if (sock == -1)
	{
   // printf("Could not create socket");
	 return -1;
	}
	server.sin_addr.s_addr = inet_addr(hostIP);
	server.sin_family = AF_INET;
	server.sin_port = htons( CAMERA_PORT );
	
	//Connect to remote server
	if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
	{
			printf("connect failed. Error");
			return -1;
	}

	msgSize = strlen(command) + 70;
	message = (char*) malloc(msgSize);
	memset(message, 0, msgSize);
  index = snprintf(message, msgSize, "%s\r\n", command);
  index += snprintf(&message[index], msgSize-index, "Host: %s\r\n", hostIP);
  snprintf(&message[index], msgSize-index, "Connection: keep-alive\r\n\r\n");
  printf(message);

  if( send(sock , message , strlen(message) , 0) < 0)
  {
//  printf("Send failed");
  free(message);
  return -1;
  }

	server_reply = (char*) malloc(301);
	memset(server_reply, 0, 301);
	x = recv(sock , server_reply , 300 , 0);
	if(x < 0)
	{
//		printf("recv failed");
		close(sock);
		free(message);
		free(server_reply);
		return -1;
	}
	else
	{
	//	printf("\r\nserver reply(%d)=[%s]\r\n",x, server_reply);
#if 1
		int length = getContentLength(server_reply);
		if (length > 0)
		{
			unsigned char* content = getContentData(sock, length);
			if (content != NULL)
			{
		//		printf("Content:\n%s\n", content);
				free(content);
			}
		}
#endif
		close(sock);
		free(message);
		free(server_reply);
        	return 1;
    	}

	free(message);
	free(server_reply);
	return 0;
}


unsigned char* getContentDataInStore(int vsock, char* store, int length)
{
	unsigned char* data = NULL;
	int cnt = 0;
	int offset = 0;
	data = (unsigned char*) store;
	if (data == NULL)
	{
		data = (unsigned char*) malloc(length) + 1;
		data[length] = 0;// Add null terminator
	}
	if (data != NULL)
	{
		data[length] = 0;
		while(offset < length)
		{
			struct timeval tv;
			int status;
			fd_set read_fds, error_fds;
		    	FD_ZERO(&read_fds);
		    	FD_ZERO(&error_fds);
			FD_SET(vsock, &read_fds);
			FD_SET(vsock, &error_fds);

			tv.tv_sec = 0;
			tv.tv_usec = 1000*50; // 50 ms 

			if((status = select(vsock+1, &read_fds, 0, &error_fds, &tv)) > 0)
			{
				if (FD_ISSET(vsock, &read_fds))
				{
					cnt = recv(vsock, &data[offset], (length - offset), 0);
             					if (cnt > 0)
						offset += cnt; 
                      				else if (cnt < 0)
						break;
				}
			}
			else
			{
				break; // Timeout
			}
		}
	}
	return data;
}


// This will send a command and return all captured data from the session
// Calling task must free the response.
char* queryCmd(char* hostIP, char* command, char* store, int *length)
{
	int sock = -1;
	int x, index;
        int msgSize;	
	char* retval = NULL;
	char* message;
	int contentLength = 0;
	char* server_reply;
	char* response = NULL;
	int responseSize = 0;
        struct sockaddr_in server;
	if ((store == NULL) || (length == NULL))
	{
		printf("%s@%d - store = %p, len = %p\n", __FUNCTION__, __LINE__, store, length);
	}
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
                printf("Could not create socket");
		return NULL;
        }
        server.sin_addr.s_addr = inet_addr(hostIP);
        server.sin_family = AF_INET;
        server.sin_port = htons( CAMERA_PORT );

        //Connect to remote server
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
        {
                perror("connect failed. Error");
                return NULL;
        }

	msgSize = strlen(command) + 70;
	message = (char*) malloc(msgSize);
	memset(message, 0, msgSize);
        index = snprintf(message, msgSize, "%s\r\n", command);
        index += snprintf(&message[index], msgSize-index, "Host: %s\r\n", hostIP);
        snprintf(&message[index], msgSize-index, "Connection: keep-alive\r\n\r\n");
        printf(message);

        if( send(sock , message , strlen(message) , 0) < 0)
        {
            printf("Send failed");
	    close(sock);
	    free(message);
            return NULL;
        } 
	// Done with message so free it
	free(message);

	server_reply = (char*) malloc(500);
	memset(server_reply, 0, 500);
        x = recv(sock , server_reply , 500 , 0);
	if(x < 0) // error 
       	{
               	printf("recv failed");
       	}
       	else
       	{
		printf("%s\r\n", server_reply);
		contentLength = getContentLength(server_reply);
		printf("Content Length: %d\n", contentLength);
       	}
	
	if (contentLength > 0)
	{
		
		char* contentData = NULL;
		if (store != NULL)
		{
			if (*length < contentLength)
				contentLength = *length - 1;
		}
		contentData = (char*) getContentDataInStore(sock, store, contentLength);
		if (contentData != NULL)
		{
//			DisplayMem("ContentData:", contentData, contentLength);
//			printf(contentData);
			retval = contentData;
			if (length != NULL)
				*length = contentLength;
		}
	}

	close(sock);
	free(server_reply);
	return retval;
}

int GetImageUpload(char* hostIP, int image)
{
	int sock, x, index;
        int msgSize;	
	char* message;
	char* server_reply;
        struct sockaddr_in server;
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
                printf("Could not create socket");
		return -1;
        }
        server.sin_addr.s_addr = inet_addr(hostIP);
        server.sin_family = AF_INET;
        server.sin_port = htons( CAMERA_PORT );

        //Connect to remote server
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
        {
                perror("connect failed. Error");
                return -1;
        }

	msgSize = 128;
	message = (char*) malloc(msgSize);
	memset(message, 0, msgSize);
        index = snprintf(message, msgSize, "GET /cgi-bin/system?USER=admin&PWD=123456&EVENT_RSPIMG%d\r\n", image);
        index += snprintf(&message[index], msgSize-index, "Host: %s\r\n", hostIP);
        snprintf(&message[index], msgSize-index, "Connection: keep-alive\r\n\r\n");
        printf(message);

        if( send(sock , message , strlen(message) , 0) < 0)
        {
            printf("Send failed");
	    free(message);
            return -1;
        }

	server_reply = (char*) malloc(300);
	memset(server_reply, 0, 300);
        x = recv(sock , server_reply , 300 , 0);
        if(x < 0)
        {
                printf("recv failed");
                close(sock);
		free(message);
		free(server_reply);
                return -1;
        }
        else
        {
		int msType = 0;
		int preLoad = 0;
		int upLoad = 0;
		char* posn = strstr(server_reply, "EVENT_RSPIMG");

                printf("\r\nserver reply[%d]=[%s\r\n",x, server_reply);

		if (posn != NULL)
		{
			posn+=15;
			sscanf(posn, "%d,%d,%d", &msType, &preLoad, &upLoad);
			printf("Image#%d: reload = %d, upload = %d\r\n", image, preLoad, upLoad);
		}
		
                close(sock);
		free(message);
		free(server_reply);
                return upLoad;
        }

	free(message);
	free(server_reply);
	return -1;
}

//GET /cgi-bin/system?USER=admin&PWD=123456&DATE_CONFIG=1,040100002016,11:57:00,-06&DAYLIGHT_SAVING_CONFIG=0,1,3,2,02:00,7,1,10,1,03:00,7 HTTP/1.1\r\n
int setCameraTime(char* hostIP)
{
	// get time of day
	// send time command
	int status = 0;
	struct tm timest;
	int cmdsize = 170;
	int tzoffset = 0;
	char* command;
	command = (char*) malloc(cmdsize);
 	time_t timep = time(NULL);
	localtime_r(&timep, &timest);
	snprintf(command, cmdsize, "GET /cgi-bin/system?USER=admin&PWD=123456&DATE_CONFIG=1,%02d%02d0000%04d,%02d:%02d:%02d,%c%02d HTTP/1.1",
	timest.tm_mon + 1, timest.tm_mday, timest.tm_year += 1900,
	timest.tm_hour, timest.tm_min, timest.tm_sec,
	(tzoffset >= 0) ? '+' : '-',
	(tzoffset < 0) ? -tzoffset : tzoffset); // timezone offet -6 = CST, +0
	status = sendCameraCommand(hostIP, command);
	free(command);
	return status;
}

void ConfigImage(char* hostIP, int image, int pre_buffer, int store)
{
	char command[128];
	
	snprintf(command, sizeof(command), "GET /cgi-bin/encoder?USER=admin&PWD=123456&EVENT_RSPIMG%d=7,%d,%d,4,", image, pre_buffer, store);
	strcat(command, "VMSVideo_%YYYY%MM%DD_%hh%mm%ss,~/Photo,[],,1 HTTP/1.1");
	
	sendCameraCommand(hostIP, command);
}

int stopZoom(char* hostIP)
{
	int sock, x, index;
        struct sockaddr_in server;
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1)
        {
                printf("Could not create socket");
        }
        printf("Socket created");

        server.sin_addr.s_addr = inet_addr(hostIP);
        server.sin_family = AF_INET;
        server.sin_port = htons( CAMERA_PORT );

        //Connect to remote server
        if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
        {
                perror("connect failed. Error");
                return -1;
        }

        printf("Connected\n");
        index = sprintf(message,"GET /cgi-bin/encoder?USER=Admin&PWD=123456&ZOOM=STOP HTTP/1.1\r\n");
        sprintf(&message[index],"Host: %s\r\n", hostIP);
        strcat(message,"Connection: keep-alive\r\n\r\n");
        printf(message);
        if( send(sock , message , strlen(message) , 0) < 0)
        {
            printf("Send failed");
            return -1;
        }
        x = recv(sock , server_reply , 2000 , 0);
        if(x < 0)
        {
                printf("recv failed");
                close(sock);
                return -1;
        }
        else
        {
		int length = getContentLength(server_reply);
		printf("\r\nserver reply(%d)=[%s]\r\n",x, server_reply);
#if 0
		if (length > 0)
		{
			char* content = getContentData(sock, length);
			if (content != NULL)
			{
				printf("Content:\n%s\n", content);
				free(content);
			}
		}
#endif
                close(sock);
                return 1;
        }
		return 0;
}

int setZoom(char* hostIP, int position)
{
	int sock, x;
        struct sockaddr_in server;

	stopZoom(hostIP);


	printf("setting new position of %d\r\n", position);
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
            printf("Could not create socket");
    }
    printf("Socket created");

    server.sin_addr.s_addr = inet_addr(hostIP);
    server.sin_family = AF_INET;
    server.sin_port = htons( CAMERA_PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
            perror("connect failed. Error");
            return -1;
    }

    printf("Connected\n");
	if(position >= current_pos)
        	sprintf(message,"GET /cgi-bin/encoder?USER=Admin&PWD=123456&STEPPED_ZOOM=TELE,%d HTTP/1.1\r\n",position - current_pos);
	else
        	sprintf(message,"GET /cgi-bin/encoder?USER=Admin&PWD=123456&STEPPED_ZOOM=WIDE.%d HTTP/1.1\r\n",current_pos - position);
    sprintf(&message[strlen(message)], "Host: %s\r\n", hostIP);
    strcat(message,"Connection: keep-alive\r\n\r\n");
    printf(message);
    if( send(sock , message , strlen(message) , 0) < 0)
    {
        printf("Send failed");
        return -1;
    }
    x = recv(sock , server_reply , 2000 , 0);
    if(x < 0)
    {
            printf("recv failed");
            close(sock);
            return -1;
    }
    else
    {
#if 0
	   int length = getContentLength(server_reply);
	   if (length > 0)
	   {
	   	char* content = getContentData(sock, length);
		   printf("\r\nserver reply(%d)=[%s]\r\n",x, server_reply);
		   if (content != NULL)
		   {
		   	printf("Content:\n%s\n", content);
		   	free(content);
		   }
	    }
#endif
            printf("\r\nserver reply=[%s\r\n",server_reply);
            close(sock);
            return 1;
    }
}


int readZoom(char* hostIP)
{
	int sock, x;
	int index = 0;
	char *ptr;
    struct sockaddr_in server;

    printf("Connecting to: %s\n", hostIP);
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
            printf("Could not create socket\n");
    }
    printf("Socket created\n");

    server.sin_addr.s_addr = inet_addr(hostIP);
    server.sin_family = AF_INET;
    server.sin_port = htons( CAMERA_PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
            perror("connect failed. Error");
            return -1;
    }

    printf("Connected\n");
    index = sprintf(message,"GET /cgi-bin/encoder?USER=Admin&PWD=123456&ZOOM_POSITION HTTP/1.1\r\n");
    sprintf(&message[index],"Host: %s\r\n", hostIP);
    strcat(message,"Connection: keep-alive\r\n\r\n");
    printf(message);
    if( send(sock , message , strlen(message) , 0) < 0)
    {
        printf("Send failed: %s", strerror(errno));
        return -1;
    }
    x = recv(sock , server_reply , 2000 , 0);
    if(x < 0)
    {
        printf("recv failed");
        close(sock);
        return -1;
    }
    else
    {
        printf("\r\nserver reply=[%s\r\n",server_reply);
	int length = getContentLength(server_reply);
#if 0
	if (length > 0)
	{
		char* content = getContentData(sock, length);
		if (content != NULL)
		{ 
			printf("Content:\n%s\n", content);
		   	free(content);
		}
	}
#endif
        close(sock);
		ptr = strstr(server_reply, "ZOOM_POSITION='");
		if(ptr != NULL)
		{
			printf("------------------getting position[%s\r\n", ptr);
			ptr += 15;
			printf("------------------getting position[%s\r\n", ptr);
					return (atoi(ptr));
		}
    }

    return 0;
}

int updateSpeed(char* hostIP, int speed)
{
	int sock, x, index;
    struct sockaddr_in server;
    char* message = (char*)malloc(250);
    unsigned char* reply = (unsigned char*) malloc(250);
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
      printf("Could not create socket");
    }
    server.sin_addr.s_addr = inet_addr(hostIP);
    server.sin_family = AF_INET;
    server.sin_port = htons( CAMERA_PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
            perror("connect failed. Error");
            return -1;
    }

    printf("SpeedSet: %d\r\n", speed);
    if (speed == 0)
      index = sprintf(message, "GET /cgi-bin/encoder?USER=Admin&PWD=123456&OSD_FORMAT=1,1,000000,0,TOP,%d,Speed=%d HTTP/1.1\r\n", speed, speed);
    else
      index = sprintf(message, "GET /cgi-bin/encoder?USER=Admin&PWD=123456&OSD_FORMAT=1,1,000000,0,TOP,%d,Speed=%d HTTP/1.1\r\n",speed, speed);
    sprintf(&message[index],"Host: %s\r\n", hostIP);
    strcat(message,"Connection: keep-alive\r\n\r\n");
    printf(message);

    if( send(sock , message , strlen(message) , 0) < 0)
    {
        printf("Send failed");
        free(message);
        return -1;
    }
    free(message);

    memset(reply, 0, 250);
    x = recv(sock , reply , 249 , 0);
    if(x < 0)
    {
     	printf("recv failed");
		close(sock);
        free(reply);
       	return -1;
    }
    else
    {
	printf("\r\nserver reply(%d)=[%s]\r\n", x, reply);
#if 0
	int length = getContentLength(reply);
	if (length > 0)
	{
		char* content = getContentData(sock, length);
		if (content != NULL)
		{
			printf("Content:\n%s\n", content);
			free(content);
		}
	}
#endif
	close(sock);
        free(reply);
	return 1;
    }
}

const unsigned char marker[4] = {0xFF, 0xD8, 0xFF, 0xE0};
const unsigned char endMarker[2] = {0xFF, 0xD9};

static int findMarker(unsigned char* data, int size)
{
    int index = 0;
    int i;
    for (i = 0; i<size; i++)
    {
        index = i;
        if (memcmp(marker, &data[i], sizeof(marker)) == 0)
        {
            break;
        }
    }
    return index;
}

static int findEndMarker(unsigned char* data, int size)
{
    int index = 0;
    int i;
    for (i = size - sizeof(endMarker); i>0; i--)
    {
        if (memcmp(endMarker, &data[i], sizeof(endMarker)) == 0)
        {
            index = size - (i + sizeof(endMarker));
            break;
        }
    }
    return index;
}

#define HTTP_RESPONSE_LEN 0x9A
#define BOUNDRY_HEADER_LEN (74)
#define PREAMBLE_LEN (44)


int takePic(int quality, char *filename)
{
    int c, res;
    char buf[255];
    FILE *file1;
    int x,y, total;
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[5002];
    char *ptr;
    int offset;

    int var_lcl = 0;

    //Create socket
    ptr = (char*)malloc(1024000);
    if(ptr == NULL)
    {
       printf("could not get memory()");
       return -1;
    }
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
       printf("Could not create socket");
       free(ptr);
       return -1;
    }

    server.sin_addr.s_addr = inet_addr(CAMERA_IP_ADDR);

    server.sin_family = AF_INET;
    server.sin_port = htons( CAMERA_PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
      printf("connect failed. Error: %s (%d.%d.%d.%d", strerror(errno), 
      server.sin_addr.s_addr & 0xFF,
      (server.sin_addr.s_addr >> 8) & 0xFF,
      (server.sin_addr.s_addr >> 16) & 0xFF,
      (server.sin_addr.s_addr >> 24) & 0xFF);
      free(ptr);
      close(sock);
      return -1;
    }

    //printf("Connected\n");
    total = -1;

    sprintf(message,"GET /cgi-bin/encoder?USER=Admin&PWD=123456&SNAPSHOT=N640x480,%d&DUMMY=42 HTTP/1.0\r\n",quality);
    strcat(message,"Host: ");
    strcat(message, CAMERA_IP_ADDR);
    strcat(message, "\r\n");
    strcat(message,"Connection: keep-alive\r\n\r\n");
    //Send some data
    if( send(sock , message , strlen(message) , 0) < 0)
    {
       free(ptr);
       close(sock);
       return -1;
    }

    x = recv(sock , server_reply , 2000 , 0);
    if(x < 0)
    {
      free(ptr);
      close(sock);
      return -1;
    }
    else
    {
      y = 0;
      if(total == -1)
      {
          total = 0;
      }
      else
      {
          total = total + x;
      }
   }

   offset = 0;
   total = 0;
   while(x > 0)
   {
      x = recv(sock , server_reply , 5000 , 0);
      if(x < 0)
      {
        break;
      }
      else
      {
        y = 0;
        for(y = 0; y < x; y++)
        {
          ptr[offset++] = server_reply[y];
          if(offset > 1024000) offset = 0;
        }
        total = total + x;
      }
   }

    if((file1 = fopen(filename, "w+")) != NULL)
    {
      fwrite(ptr, 1, offset, file1);
      fclose(file1);
    }
    
    free(ptr);
    close(sock);
  return 0;
}

std::string takePicToMemory(int quality)
{
    int c, res;
    char buf[255];
    FILE *file1;
    int x,y, total;
    int sock;
    struct sockaddr_in server;
    char message[1000] , server_reply[5002];
    char *ptr;
    int offset;

    int var_lcl = 0;

    //Create socket
    ptr = (char*)malloc(1024000);
    if(ptr == NULL)
    {
       printf("could not get memory()");
       return "";
    }
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
       printf("Could not create socket");
       free(ptr);
       return "" ;
    }

    server.sin_addr.s_addr = inet_addr(CAMERA_IP_ADDR);

    server.sin_family = AF_INET;
    server.sin_port = htons( CAMERA_PORT );

    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
      printf("connect failed. Error: %s (%d.%d.%d.%d", strerror(errno), 
      server.sin_addr.s_addr & 0xFF,
      (server.sin_addr.s_addr >> 8) & 0xFF,
      (server.sin_addr.s_addr >> 16) & 0xFF,
      (server.sin_addr.s_addr >> 24) & 0xFF);
      free(ptr);
      close(sock);
      return std::string();
    }

    //printf("Connected\n");
    total = -1;

    sprintf(message,"GET /cgi-bin/encoder?USER=Admin&PWD=123456&SNAPSHOT=N640x480,%d&DUMMY=42 HTTP/1.0\r\n",quality);
    strcat(message,"Host: ");
    strcat(message, CAMERA_IP_ADDR);
    strcat(message, "\r\n");
    strcat(message,"Connection: keep-alive\r\n\r\n");
    //Send some data
    if( send(sock , message , strlen(message) , 0) < 0)
    {
       free(ptr);
       close(sock);
       return "";
    }

    
   std::string result;

   offset = 0;
   total = 0;
   bool parsing_header = true;
   x = 1;
   while(x > 0)
   {
      x = recv(sock , server_reply , 5000 , 0);
      if(x < 0)
      {
        break;
      }
      else
      {
        y = 0;
        for(y = 0; y < x; y++)
        {
          if (parsing_header && (server_reply[y] == '\n') && (y >= 3)) {
            if ((server_reply[y - 3] == '\r') && (server_reply[y - 2] == '\n') && (server_reply[y - 1] == '\r')) {
              parsing_header = false;
            }
            continue;
          }

          if (!parsing_header) {
            ptr[offset++] = server_reply[y];
            if(offset > 1024000)
              offset = 0;
          }
        }
        total = total + x;
      }
   }

   result.append(ptr, offset);
    
  free(ptr);
  close(sock);
  return result;
}

#if 0
int takePic2(int speed, char *filename)
{
	char* resp;
	char cmd[128]; 
	snprintf(cmd, 128, "wget \"http://10.0.0.123/cgi-bin/viewer/video.jpg?%s\"", filename);
	
	resp = SysCmd(cmd);
	if (resp != NULL)
	{
		free(resp);
	}

	return 0;
}
#endif
