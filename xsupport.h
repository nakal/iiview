
#ifndef IIVIEW_XSUPPORT_INCLUDED
#define IIVIEW_XSUPPORT_INCLUDED

/* prototypes */

#include <X11/Xlib.h>

typedef struct {
	size_t width;
	size_t height;
	size_t unit;
	void *buffer;
	XImage *ximage;
} image_t;

extern void x_InitConnection(const char *display_name);
extern void x_InitXWindowStuff(void);
extern void x_EventLoop(char *selectedfile, size_t selectedfilesize);
extern int x_FullScreenIMG(char *filename,char *imgname);
extern void x_FreeImage(image_t *i);

#endif

