
#ifndef IIVIEW_NJPEG_INCLUDED
#define IIVIEW_NJPEG_INCLUDED

/*

   njpeg.h
   -------


*/

#ifdef INCLUDE_JPEG

extern int  open_jpeg(const char *,const char *, unsigned int, unsigned int);
extern void close_jpeg(void);
extern unsigned int  getpixel_jpeg(int,int,int);
extern size_t getwidth_jpeg(void);
extern size_t getheight_jpeg(void);

#endif

#endif

