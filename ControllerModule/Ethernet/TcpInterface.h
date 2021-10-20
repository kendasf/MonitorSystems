#pragma once

int getTcpListen(int listenPort, int depth);
// Returns a File descriptor for the Lister
int createTcpListen(int listenPort);
// Returns a File descriptor for the TCP connection
int getTcpActiveSocket(int listener);
// Wait for data from Tcp Socket
int tcpDataWait(int sockFd, unsigned long lTmoMs);
// Write Data to Tcp Socket
int tcpWrite(int sockFd, unsigned char* buffer, int len);
// Read Data from Tcp Socket
unsigned char* tcpRecv(int sockFd, int *pLen, unsigned long lTmoMs);
