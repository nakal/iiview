
#ifndef IIVIEW_JPEG_INCLUDED
#define IIVIEW_JPEG_INCLUDED

/*

   jpeg.h

   These are some prototypes for JPEG.C.
   The functions which can be used for normal
   access are listed as 'public' below.
   You don't need ANYTHING else !


   Update history, see in JPEG.C

*/

#ifdef INCLUDE_JPEG

/* some data types */

#define uint                    unsigned int
#define ushort                  unsigned short int
#define uchar                   unsigned char


/* public routines */

int            JPEG_INIT(void);
int            JPEG_OpenFile(const char *,const char *, unsigned int,
    unsigned int);
void           JPEG_CloseFile(void);
unsigned int   JPEG_GetPixel(int,int,int);
int            JPEG_GetWidth(void);
int            JPEG_GetHeight(void);

#endif

#endif

