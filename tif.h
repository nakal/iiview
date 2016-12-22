
#ifndef IIVIEW_TIF_INCLUDED
#define IIVIEW_TIF_INCLUDED

/*

   tif.h

   These are some prototypes for tif.c.


   Update history, see in tif.c

*/

#ifdef INCLUDE_TIFF

extern int TIF_OpenFile(const char *,const char *, unsigned int, unsigned int);
extern void TIF_CloseFile(void);
extern unsigned int TIF_GetPixel(int,int,int);
extern size_t TIF_GetWidth(void);
extern size_t TIF_GetHeight(void);

extern int TIF_SetDirectory(const char *, const char *);
extern int TIF_GetFileList(const char *, const char *);

#endif

#endif

