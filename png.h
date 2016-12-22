
#ifndef IIVIEW_PNG_INCLUDED
#define IIVIEW_PNG_INCLUDED

#ifdef INCLUDE_PNG

extern int PNG_OpenFile(const char *,const char *, unsigned int, unsigned int);
extern void PNG_CloseFile(void);
extern unsigned int PNG_GetPixel(int,int,int);
extern size_t PNG_GetWidth(void);
extern size_t PNG_GetHeight(void);

#endif

#endif

