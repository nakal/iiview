
#ifndef IIVIEW_GIF_INCLUDED
#define IIVIEW_GIF_INCLUDED

/*

   gif.h

   These are some prototypes for gif.c.


   Update history, see in gif.c

*/

extern int GIF_OpenFile(const char *,const char *, unsigned int, unsigned int);
extern void GIF_CloseFile(void);
extern unsigned int GIF_GetPixel(int,int,int);
extern size_t GIF_GetWidth(void);
extern size_t GIF_GetHeight(void);

#endif

