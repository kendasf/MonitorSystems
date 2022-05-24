//
// VMSDriver.h
//
// Interface to VMS sign, uses SPI interface.
//
#include "main.h"

#define VMS_WIDTH		8
#ifdef LARGER_DOTS
#define VMS_HEIGHT		12
#else
#define VMS_HEIGHT		10
#endif
#define VMS_PANELS		36

#define MAX_BMP_WIDTH	60

//typedef struct BitmapS_ BitmapS;

void VMSDriver_Initialize(void);
int VMSDriver_UpdateFrame(void);
int VMSDriver_UpdateFrameFast(void);

void VMSDriver_WriteSpeed(int x, int y, int panelsConfig, int speed, int font);
void VMSDriver_Off(void);

void VMSDriver_Invalidate(void);

void VMSDriver_GetDimensions(int panelsConfig, int& width, int& height);

void VMSDriver_SetPixel(int x, int y);
void VMSDriver_ClearPixel(int x, int y);

void VMSDriver_Clear(bool doUpdate);
void VMSDriver_White(unsigned char val);
int VMSDriver_Test(void);

void VMSDriver_WriteXLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[24][16]);
void VMSDriver_WriteLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[18][12]);
void VMSDriver_WriteSemiLargeChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[13][8]);
void VMSDriver_WriteChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[10][8]);
void VMSDriver_WriteSmallChar(int xPos, int yPos, int width, int height, BitmapS *pBitmap, const unsigned char Digit[][5]);

void VMSDriver_TestMode(int imageID);

void SetDisplayLE(void);

void VMSDriver_RenderBitmapFile(char* bMapFileName);
void VMSDriver_RenderBitmap(int imageID, BitmapS *pBitmap, int color = 1);
void VMSDriver_RenderBitmapToPanels(int bmpWidth, int bmpHeight, int firstPanel, int panelsRows, int panelsCols, BitmapS *pBitmap, int color = 1);

void VMSDriver_RunStartSequence(void);
