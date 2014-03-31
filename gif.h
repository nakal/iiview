
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
extern int GIF_GetWidth(void);
extern int GIF_GetHeight(void);

#endif

