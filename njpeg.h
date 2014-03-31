
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
extern int getwidth_jpeg(void);
extern int getheight_jpeg(void);

#endif

#endif

