#ifndef __CAMERA_H__
#define __CAMERA_H__
/*****************************************************************************
 *   camera.h:  Camera interface header file.
 *
 *   Copyright(C) 2008, Geolux d.o.o. (www.geolux.hr)
 *   All rights reserved.
 *
 *   History
 *   2012.05.05  ver 1.00    Preliminary version, first Release
 *
******************************************************************************/
#define DMA_IMAGE_BUFFER_SIZE	2048

void CameraSPIInit(void);
void CameraSPIProcess(void);
void CameraReset(void);

int CameraActive(void);
void CameraTakeSnapshot(int);
int CameraGetSnapshot(int, int, int);
void CameraResetTask(void);
void CreateCameraTask(int localPort, int reportPort);

#endif