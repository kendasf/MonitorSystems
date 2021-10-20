#pragma once

#include <string>

extern char* CAMERA_IP_ADDR;

int stopZoom(char* hostIP);
int setZoom(char* hostIP, int position);
int readZoom(char* hostIP);
int updateSpeed(char* hostIP, int speed);
int takePic(int quality, char* fname);
std::string takePicToMemory(int quality);
int takePics(char* hostIP, int speed, int frames, char* fname);
void ConfigImage(char* hostIP, int image, int pre_buffer, int store);
int setCameraTime(char* hostIP);
int sendCameraCommand(char* hostIP, char* command);
int GetImageUpload(char* hostIP, int image);
char* queryCmd(char* hostIP, char* command, char* store, int *length);

