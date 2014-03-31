
#ifndef IIVIEW_BMP_INCLUDED
#define IIVIEW_BMP_INCLUDED

/*

   bmp.h

   These are some prototypes for bmp.c.

   Update history, see in bmp.c
   */

#define BMP_SIGNATURE 0x4D42
typedef struct {
	unsigned short int bfType;
	unsigned int bfSize;
	unsigned short int bfReserved[2];
	unsigned int bfOffBits;
} BitmapFileHeader;

typedef struct {
	unsigned int biSize,biWidth,biHeight;
	unsigned short int biPlanes,biBitCount;
	unsigned int biCompression,biSizeImage,
		     biXPelsPerMeter,biYPelsPerMeter,
		     biClrUsed,biClrImportant;
} BitmapInfoHeader;


typedef unsigned long int RGBQuad;


typedef struct {
	BitmapInfoHeader bmiHeader;
} BitmapInfo;

extern int BMP_OpenFile(const char *,const char *, unsigned int, unsigned int);
extern void BMP_CloseFile(void);
extern unsigned int BMP_GetPixel(int,int,int);
extern int BMP_GetWidth(void);
extern int BMP_GetHeight(void);

#endif

