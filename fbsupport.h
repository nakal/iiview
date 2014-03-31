
#ifndef IIVIEW_FBSUPPORT_INCLUDED
#define IIVIEW_FBSUPPORT_INCLUDED

/* framebuffer prototypes */

#ifdef FRAMEBUFFER

void FB_PutPixel(int,int,unsigned int);
void FB_Print(int x,int y,const char *text,unsigned int fg,unsigned int bg);
void FB_ClearScreen(void);
void FB_TidyUp(int);
void FB_Init(void);
void FB_OutputFilename(unsigned int nr);
void FB_UpdateThumbs(const char *selectedfile);
void FB_UpdateScreen(int, const char *selectedfile);
void FB_HelpScreen(void);
int FB_FullScreenIMG(const char *filename,const char *imagename);
void FB_KeyBoardLoop(const char *selectedfile, size_t selectedfilelen);

#endif

#endif

