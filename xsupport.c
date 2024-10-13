/*

   xsupport.c
   ----------

   Routines for X-Input/Output

*/

/* ansi C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "compat.h"

/* unix specific includes */
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#ifdef CJK
#  include <locale.h>
#endif

#include "common.h"

#include "modules.h"
#include "filelist.h"
#include "pixeltools.h"

#include "xsupport.h"

/* X11 specific includes */
#include <X11/Xutil.h>
#include <X11/keysym.h>

/* prototypes */

unsigned int x_GetColor(unsigned int);
void x_TidyUp(int);
void x_ResizeThumbs(void);
void x_OutputFilename(GC, unsigned int);
void x_UpdateThumbs(GC, const char *);
void x_UpdateScreen(int, const char *);
void x_HelpScreen(void);
int x_HandleKey(KeySym key, char *selectedfile, size_t selectedfilesize);
void x_ReadConfig(FILE *);
static int x_nextPic(void);
static int x_prevPic(void);
static void x_PutPixel(image_t *, size_t, size_t, unsigned int);
static image_t * x_AllocImage(size_t x, size_t y, size_t unit);
static void x_SetTitle(const char *suffix, size_t picw,
    size_t pich);

/*
   void x_InitConnection(void);
   void x_InitXWindowStuff(void);
   void x_EventLoop(void);
   void x_FreeImage(XImage **);
   int x_FullScreenIMG(char *, char *);
   */
int x_ActivateImage(unsigned int, char *, size_t);
static void x_ClearThumbs(void);

/*
   static prototypes
   */

static Display *display;                             /* connection to X11 */
static int screen_nr,                                /* misc. flags and vars */
    window_big_enough;
static unsigned int bitsperpixel=0, screen_depth=0;

static Window mywin;                                 /* window instance */
static XEvent event;                                 /* event data */
static Colormap colormap;                            /* new colormap for 256 colors */
static XSetWindowAttributes mywinattribs;            /* win attributes */
static Visual *visual=NULL;                               /* visual for our window */
static int RED_POS,GREEN_POS,BLUE_POS;               /* XWindow color masking stuff */
static int RED_BITS,GREEN_BITS,BLUE_BITS;            /* ... */
static image_t *blackimage=NULL;
static size_t thumbstride=0;
static size_t pixel_bytes=0;

/* RGB 8-bit custom conversion macro */
#define RGB2XRGB8(val) ((((val >> 16)&0xFF)/43)*36)+  \
    ((((val >>  8)&0xFF)/43)*6)+   \
((val & 0xFF)/43)

/* palette of colors for 256 modes,
   we don't want to get rid of the poor people ;-) */
static XColor *colors;

#ifdef CJK
/* japanese font support */
static XFontSet font_set;
static const char *default_font="-*-*-medium-r-*-12-*";
static const char *default_fallback_font="fixed";
#endif

/* interprets colors */
unsigned int x_GetColor(unsigned int rgb)
{
	if (screen_depth==8)
		return RGB2XRGB8(rgb);
	else
		return
		    ((((rgb >> 16)&0xFF)>>(8-RED_BITS))<< RED_POS)|
		    ((((rgb >> 8)&0xFF)>>(8-GREEN_BITS))<< GREEN_POS)|
		    (((rgb &0xFF)>>(8-BLUE_BITS))<< BLUE_POS);
}

/* sets pixels in images */
void x_PutPixel(image_t *img, size_t x, size_t y, unsigned int val)
{
	unsigned int c;

	assert(x<img->width && y<img->height);
	c = x_GetColor(val);

	switch (img->unit)
	{
	case 8:
		((unsigned char *)img->buffer)[x+y*img->width] =
		    (unsigned char)c;
		break;
	case 15:
	case 16:
		((unsigned short *)img->buffer)[x+y*img->width] =
		    (unsigned short)c;
		break;
	case 24:
	case 32:
		((unsigned int *)img->buffer)[x+y*img->width] = c;
		break;
	}
}

image_t * x_AllocImage(size_t x, size_t y, size_t unit) {
	image_t *i;

	i = (image_t *)malloc(sizeof(image_t));
	if (i!=NULL) {

		i->width = x;
		i->height = y;
		i->unit = unit;
		i->buffer=malloc(x*y*i->unit/8);

		if (i->buffer==NULL) {
			free(i);
			i=NULL;
		} else {
			i->ximage= XCreateImage(display, visual, screen_depth,
			    ZPixmap, 0, i->buffer,
			    (unsigned int)i->width, (unsigned int)i->height,
			    (int)i->unit, (int)(i->width * pixel_bytes));
			if (i->ximage==NULL) {
				free(i->buffer);
				free(i);
				i=NULL;
			}
		}
	}

	return i;
}

void x_FreeImage(image_t *i) {
	if (i!=NULL) {
		/* destroys buffer automatically! */
		if (i->ximage!=NULL) XDestroyImage(i->ximage);
		free(i);
	}
}

/*
   void x_FreeXImage(XImage *xim) {
   }
   */

void x_TidyUp(int signr)
{
#ifdef VERBOSE
	printf("Tidying up ...\n");
#endif

	if (signr==SIGPIPE) {
		return;
	}

	if (screen_depth==8) {
		XFreeColormap(display,colormap);
		free(colors);
	}

	XCloseDisplay(display);

#ifdef VERBOSE
	printf("Exiting.\n");
#endif
}

void x_ReadConfig(FILE *fp)
{
	char s[128];
	char *p,*c;

	while (!feof(fp)) {
		fgets(s,sizeof(s),fp);

		c=p=s;
		while (c[0]==' ') c++;
		if (*c=='#' || *c==0) continue;
		if ((p=strchr(s,'='))==NULL) continue;
		*p++=0;

		if (strcmp(c,"xpos") == 0) {
			cf.x = (unsigned int)strtoul(p, NULL, 10);
			continue;
		}
		if (strcmp(c,"ypos") == 0) {
			cf.y = (unsigned int)strtoul(p, NULL, 10);
			continue;
		}
		if (strcmp(c,"width") == 0) {
			cf.w = (unsigned int)strtoul(p, NULL, 10);
			continue;
		}
		if (strcmp(c,"height") == 0) {
			cf.h = (unsigned int)strtoul(p, NULL, 10);
			continue;
		}
	}
#ifdef VERBOSE
	printf("Config file params: (%d,%d ; %dx%d)\n",cf.x,cf.y,cf.w,cf.h);
#endif
}

/* initialize the X-connection */
void x_InitConnection(const char *display_name)
{
	XVisualInfo visualinfo;
	XVisualInfo *retvinfo;
	int retv;
	image_t *ii;

#ifdef CJK
	setlocale(LC_ALL,"");
#endif

	display=XOpenDisplay(display_name);
	if (display==NULL) {
		printf("Failed to connect to X display on '%s'\n",display_name);
		exit(-1);
	}

	fbmode=0;
	tidyup=x_TidyUp;

	screen_nr=DefaultScreen(display);

	visual=DefaultVisual(display, screen_nr);
	visualinfo.visualid=visual->visualid;
	retvinfo=XGetVisualInfo(display, VisualIDMask, &visualinfo, &retv);
	if (retvinfo==NULL) {
		fprintf(stderr, "Error getting VisualInfo for visual id %u.\n",
		    (unsigned int)visual->visualid);
		exit(-1);
	}
	memcpy(&visualinfo, retvinfo, sizeof(XVisualInfo));
	XFree(retvinfo);
	screen_depth = (unsigned int)visualinfo.depth; /* used bits per pixel */
	bitsperpixel=screen_depth;

	/* fill up to byte boundaries */
	while ((bitsperpixel&7)!=0) bitsperpixel++;

	/* probe for a test image */
	if ((ii=x_AllocImage(1,1,bitsperpixel))==NULL) {
		/* fall back to 32 bits, mostly because of 24 bit depth */
		bitsperpixel=32;
		if ((ii=x_AllocImage(1,1,bitsperpixel))==NULL) {
			fprintf(stderr, "Could not allocate image.\n");
			exit(-1);
		}
	}
	x_FreeImage(ii);

#ifdef VERBOSE
	printf("screen_depth: %d, bpp: %d\n", screen_depth,
	    bitsperpixel);
#endif
	if (screen_depth<8 || screen_depth>24) {
		fprintf(stderr, "You need at least a 8 bit PseudoColor visual and"
		    "maximum 24 bit TrueColor (found: %u bits).\n", screen_depth);
		exit(-1);
	}
	pixel_bytes =  bitsperpixel/8;

	if (visualinfo.class==PseudoColor) {
		unsigned short v;
		size_t i;

		/* check if Pseudo missing XXX */
		colormap=XCreateColormap(display,RootWindow(display,screen_nr),
		    visual,AllocAll);
		colors = (XColor *)calloc(216, sizeof(XColor));
		for (i = 0 ; i < 216; i++) {
			colors[i].pixel = (unsigned long)i;
			colors[i].flags = DoRed | DoGreen | DoBlue;

			v=(i / 36) % 6;
			colors[i].red = (unsigned short)(v * 13107u);
			v=(i / 6) % 6;
			colors[i].green = (unsigned short)(v * 13107u);
			v=i % 6;
			colors[i].blue = (unsigned short)(v * 13107u);
		}
		XStoreColors(display,colormap,colors,216);
	} else {
		/* adaption of a visual */
		unsigned long i;

		RED_POS = BLUE_POS = GREEN_POS = 0;
		RED_BITS = BLUE_BITS = GREEN_BITS = 0;

		for (i = visualinfo.red_mask; !(i & 1); i >>= 1, RED_POS++)
			;
		for (; i & 1; i >>= 1, RED_BITS++)
			;
		for (i = visualinfo.green_mask; !(i & 1); i >>= 1, GREEN_POS++)
			;
		for (; i & 1; i >>= 1, GREEN_BITS++)
			;
		for (i = visualinfo.blue_mask; !(i & 1); i >>= 1, BLUE_POS++)
			;
		for (; i & 1; i >>= 1, BLUE_BITS++)
			;
	}

#ifdef VERBOSE
	printf("X-Visual is: %d bits (r:%d/g:%d/b:%d bu:%d; 0x%02x)\n", screen_depth,
	    RED_BITS,GREEN_BITS,BLUE_BITS, bitsperpixel, (unsigned int)visual->visualid);
	printf("Masks: %08X %08X %08X\n",
	    (int)visualinfo.red_mask,(int)visualinfo.green_mask,
	    (int)visualinfo.blue_mask);
#endif

#ifdef CJK
	{
		char **missing,*def_str;int missing_num;
		font_set=XCreateFontSet(display,default_font,&missing,&missing_num,&def_str);
		if (font_set==NULL) {
			font_set=XCreateFontSet(display,default_fallback_font,&missing,&missing_num,&def_str);
			if (font_set==NULL) {
				fprintf(stderr, "Could not load an appropriate font.\n");
				exit(-1);
			}
		}
	}
#endif
}

void x_InitXWindowStuff(void)
{
	FILE *fp;

	/* default window position */
	cf.x = 50;
	cf.y = 10;
	cf.w = (unsigned int)DisplayWidth(display, screen_nr) - 100;
	cf.h = (unsigned int)DisplayHeight(display, screen_nr) - 100;

	if ((fp=fopen(configpath,"r"))==NULL) {
		fp=fopen(configpath,"w");

		fprintf(fp,"\n# iiviewrc - configuration file for iiview\n\n");

		fprintf(fp,"xpos=%d",cf.x);
		fprintf(fp,"ypos=%d",cf.y);
		fprintf(fp,"width=%d",cf.w);
		fprintf(fp,"height=%d",cf.h);

		fclose(fp);
		fp=fopen(configpath,"r");
	}

	if (fp) {
		x_ReadConfig(fp);
		fclose(fp);
	}

	memset(&mywinattribs, 0, sizeof(mywinattribs));
	mywinattribs.event_mask=StructureNotifyMask|ExposureMask|
	    KeyPressMask|ButtonPressMask;
	mywinattribs.colormap=colormap;
	mywinattribs.border_pixel=0;

	mywin=XCreateWindow(display,
	    RootWindow(display,screen_nr),
	    (signed)cf.x,(signed)cf.y,cf.w,cf.h,
	    0,(signed)screen_depth,
	    InputOutput,
	    visual,
	    CWEventMask|
	    CWColormap|
	    CWBorderPixel,
	    &mywinattribs);
#ifdef VERBOSE
	printf("Window: %dx%dx%d\n", cf.w, cf.h, screen_depth);
#endif

	x_SetTitle("", 0, 0);
	XMapWindow(display,mywin);
}

static void x_SetTitle(const char *suffix, size_t picw, size_t pich) {

	size_t title_len;
	char *title;
	const char *glue=" - ";
	char dim[64];
	XTextProperty wintitle;

	if (picw==0 || pich==0) dim[0]=0;
	else {
		snprintf(dim, sizeof(dim), " (%zux%zu)", picw, pich);
		dim[32]=0;
	}

	title_len = strlen(WINDOW_TITLE)+strlen(suffix)+strlen(glue)+
	    strlen(dim)+1;
	title=(char *)malloc(title_len);
	if (title==NULL) {
		printf("Out of memory.\n");
		exit(-1);
	}

	strcpy(title, WINDOW_TITLE);
	strcat(title, glue);
	strcat(title, suffix);
	strcat(title, dim);

	if (XStringListToTextProperty(&title,1,&wintitle)==0) {
		printf("Out of memory.\n");
		exit(-1);
	}
	XSetWMName(display,mywin,&wintitle);

	/* XFree(&wintitle); */
	free(title);
}

void x_ClearThumbs(void) {

#ifdef VERBOSE
	printf("xsupport: x_ClearThumbs\n");
#endif

	x_FreeImage(blackimage);

	delete_all_data(file);

	thumbwidth  = cf.w/THUMB_INLINE[THUMB_MODE];
	thumbstride = thumbwidth*pixel_bytes;
	thumbheight = cf.h > 20 * THUMB_LINES[THUMB_MODE] ?
	    (cf.h-20*(THUMB_LINES[THUMB_MODE]))/
	    THUMB_LINES[THUMB_MODE] : 0;

	if (thumbheight>10)
		window_big_enough=1;
	else
		window_big_enough=0;

	if ((blackimage=x_AllocImage(thumbwidth, thumbheight+20, bitsperpixel))!=NULL) {

		unsigned int xx,yy;

		/* printf("x_ClearThumbs: %d, %d\n", thumbwidth, thumbheight+20); */
		for (yy=0;yy<thumbheight+20;yy++)
			for (xx=0;xx<thumbwidth;xx++)
				x_PutPixel(blackimage, xx, yy, 0);
	} else {
		printf("xsupport error: cannot alloc black image\n");
		exit(1);
	}

#ifdef VERBOSE
	printf("xsupport: x_ClearThumbs left.\n");
#endif
}

/* output thumb subscription */
void x_OutputFilename(GC gc,unsigned int nr)
{
	XGCValues values;
	size_t x, y;
	char *filename;

#ifdef VERBOSE
	fprintf(stderr,"Output filename %d\n",nr);
#endif

	if (dirsel==nr)
		values.foreground=x_GetColor(0x00FFFFFF);
	else values.foreground=x_GetColor(0x00000000);
	XChangeGC(display,gc,GCForeground|GCBackground,&values);
	x = (nr % THUMB_INLINE[THUMB_MODE])*thumbwidth;
	y = ((nr / THUMB_INLINE[THUMB_MODE])+1)*(thumbheight+20) - 10;
	XFillRectangle(display, mywin, gc, (int)x, (int)(y-9), thumbwidth-1, 10);

	if (dirsel!=nr) {
		if (is_file_dir(file,nr))
			values.foreground=x_GetColor(0x0000FF00);
		else values.foreground=x_GetColor(0x0000FFFF);
	} else values.foreground=x_GetColor(0x00000000);
	filename=get_file(file,nr);
	if (filename!=NULL) {
#ifdef CJK
		size_t len=0;

		len = strlen(filename);
		XChangeGC(display,gc,GCForeground|GCBackground,&values);

		/* printf("filename: %s (%i)\n", filename, len); */
		XmbDrawString(display,mywin,font_set,gc,(int)x,(int)y,filename,(int)len);
#else
		XChangeGC(display,gc,GCForeground|GCBackground,&values);
		XDrawString(display,mywin,gc,x,y,filename,
		    (signed)strlen(filename));
#endif
	}

#ifdef VERBOSE
	fprintf(stderr,"Output filename OK\n");
#endif
}

/* updates thumbs (!) */
void x_UpdateThumbs(GC gc, const char *selectedfile)
{
	char *curdir;
	const char *curfile;
	unsigned int x,y;
	size_t xx, yy, tw, th;
	unsigned char n;
	size_t width, height;
	int eix;
	image_t *img=NULL;
	char imgdir[MAXPATHLEN];

#ifdef VERBOSE
	printf("x_UpdateThumbs entered.\n");
#endif

	if (THUMB_MODE==0)  {
		update_needed=0;
#ifdef VERBOSE
		printf("Update not needed.\n");
#endif
		return;
	}
	if (selectedfile[0]==0 && (curdir=getcwd(NULL,512))==NULL) {
		update_needed=0;
		printf("FAILED (getcwd).\n");
		return;
	}
	if (thumbwidth==0 || thumbheight==0) {
#ifdef VERBOSE
		printf("Illegal thumbwidth/thumbheight.\n");
#endif
		return;
	}

	n=0;event_occured=0;
	for (y=0;y<THUMB_LINES[THUMB_MODE];y++) {
		for (x=0;x<THUMB_INLINE[THUMB_MODE];x++,n++) {
			if ((get_file(file,n)==NULL) ||
			    (*(get_file(file,n))==0) || (is_file_dir(file,n))) {
				/* is a directory or nothing */

#ifdef VERBOSE
				printf("File is directory (%s).\n",get_file(file,n));
#endif
				XPutImage(display,mywin,gc,blackimage->ximage,0,0,
				    (signed)(x*thumbwidth),(signed)(y*(thumbheight+20)),
				    thumbwidth,thumbheight+20);
				if (is_file_dir(file,n)) x_OutputFilename(gc,n);
				continue;
			}

			/* is a file ! */

			if ((img=get_data(file,n))==NULL) {

				/* read in file, because not cached yet */

#ifdef VERBOSE
				printf("Opening file: %s\n",get_file(file,n));
#endif

				if ((img=x_AllocImage(thumbwidth, thumbheight+20, bitsperpixel))!=NULL) {

					/* allocation ok */

					set_data(file, n, img);

					if (selectedfile[0]==0) {
						curfile=get_file(file,n);
						eix=modules_extensionindex(curfile);
						imgdir[0]=0;
					}
					else {
						eix=modules_extensionindex(selectedfile);
						curfile=selectedfile;
						if (dir_int!=NULL) snprintf(imgdir,sizeof(imgdir),"%s/",dir_int);
						else snprintf(imgdir,sizeof(imgdir),"%s/%s",dir_int,get_file(file,n));
					}

					if (module[eix].open(curfile,imgdir,thumbwidth,thumbheight)) {
						width=module[eix].getwidth();
						height=module[eix].getheight();

						if (((float)width/height)>
						    ((float)thumbwidth/thumbheight)) {
							tw = thumbwidth;
							th = height * thumbwidth / width;
						} else {
							tw = thumbheight * width / height;
							th = thumbheight;
						}

#ifdef VERBOSE
						printf("Reading (%d,%d,%d,%d).\n",thumbwidth,thumbheight,
						    width,height);
#endif

						for (yy=0;yy<thumbheight+20;yy++)
							for (xx=0;xx<thumbwidth;xx++)
								x_PutPixel(img,xx,yy,
								    module[eix].getpixel((int)(xx * width / tw),
								    (int)(yy * height / th), 1));
					} else {
						/* error reading! */
						img=blackimage;
#ifdef VERBOSE
						printf("Error reading.\n");
#endif
					}
					module[eix].close();
				} else {
					img=blackimage;
#ifdef VERBOSE
					printf("Thumb alloc failed.\n");
#endif
				}
			}

			if (img!=NULL)
#ifdef VERBOSE
				printf("img->ximage %d: %p\n", n, img->ximage);
#endif
			XPutImage(display,mywin,gc,img->ximage,0,0,
			    (signed)(x*thumbwidth),(signed)(y*(thumbheight+20)),
			    thumbwidth,thumbheight+20);
			x_OutputFilename(gc,n);
			if (XCheckMaskEvent(display,
			    ExposureMask|KeyPressMask|ButtonPressMask|
			    StructureNotifyMask,
			    &event)) {
				event_occured=1;
				update_needed=1;
				return;
			}
		}
	}
	update_needed=0;
}

/* updates the whole screen (thumbs, font) */
void x_UpdateScreen(int repaint, const char *selectedfile)
{
	XGCValues values;
	GC ngc;
	char *cwd;

#ifdef VERBOSE
	printf("x_UpdateScreen; enter.\n");
#endif

	if (window_big_enough) {
		unsigned int i;

		values.background=x_GetColor(0x00FFFFFF);
		values.foreground=0;

		ngc=XCreateGC(display,mywin,GCForeground|GCBackground,&values);

		for (i=0;i<THUMB_COUNT[THUMB_MODE];i++)
			x_OutputFilename(ngc,i);

		if (repaint) x_UpdateThumbs(ngc, selectedfile);
		XFreeGC(display,ngc);
	}
	cwd = getcwd(NULL, 0);
	x_SetTitle(cwd, 0, 0);
	free(cwd);

#ifdef VERBOSE
	printf("x_UpdateScreen; leave.\n");
#endif
}

/* help screen, when <H> pressed */
void x_HelpScreen(void)
{
	XGCValues values;
	GC gc;

	values.foreground=x_GetColor(0x00000000);
	values.background=0;
	gc=XCreateGC(display,mywin,GCForeground|GCBackground,&values);
	XFillRectangle(display,mywin,gc,0,0,cf.w,cf.h);
	values.foreground=x_GetColor(0x000000FF);
	XChangeGC(display,gc,GCForeground,&values);

	XDrawString(display,mywin,gc,30,30,
	    HelpHeader,(signed)strlen(HelpHeader));

	values.foreground=x_GetColor(0x00FFFF00);
	XChangeGC(display,gc,GCForeground,&values);

	XDrawString(display,mywin,gc,30,50,Help1,(signed)strlen(Help1));
	XDrawString(display,mywin,gc,30,60,Help2,(signed)strlen(Help2));
	XDrawString(display,mywin,gc,30,70,Help3,(signed)strlen(Help3));
	XDrawString(display,mywin,gc,30,80,Help4,(signed)strlen(Help4));
	XDrawString(display,mywin,gc,30,90,Help5,(signed)strlen(Help5));

	x_SetTitle("Help", 0, 0);
	XFreeGC(display,gc);
	XWindowEvent(display,mywin,KeyPressMask|StructureNotifyMask|
	    ButtonPressMask,&event);
}

#define REQUEST_NONE 1
#define REQUEST_PREVIOUS 10
#define REQUEST_NEXT 11

/* shows image in full-screen */
int x_FullScreenIMG(char *filename,char *imgname)
{
	image_t *line;
	XImage *imageline;
	int eix,done,drawn;
	size_t w, h;
	size_t width, height;
	FILE *fp;
	GC ngc;
	XGCValues values;
	int rotation=0;
	int retval=REQUEST_NONE;

	drawn=done=0;

	while (!done) {

		if (!drawn) {
			unsigned int draww, drawh;

			if ((eix=modules_extensionindex(filename))<0) return 0;
			if (module[eix].open(filename,imgname,0,0)==0) {
				module[eix].close();
				return 0;
			}
			width=module[eix].getwidth();
			height=module[eix].getheight();

			if ((width==0)||(height==0)) {
				module[eix].close();
				return 0;
			}

			values.background=0;
			ngc=XCreateGC(display,mywin,GCBackground,&values);

			draww=cf.w;
			drawh=cf.h;
			if (rotation&1) {
				unsigned int hh;

				hh = draww;
				draww = drawh;
				drawh = hh;
			}

			if (((double)draww/drawh)<((double)width/height)) {
				w=draww;
				h=draww*height/width;
			} else {
				w=drawh*width/height;
				h=drawh;
			}

			/*  printf("%dx%d -> (%dx%d) -> %dx%d\n", width, height, draww, drawh, w, h); */

			line = x_AllocImage(draww, 1, bitsperpixel);

			if (line!=NULL) {
				unsigned int x, y;
				double ww, hh;

				ww=(double)width/w;
				hh=(double)height/h;
				imageline=XCreateImage(display,visual,screen_depth ,ZPixmap,0,
				    line->buffer,
				    (rotation&1)==0 ? (unsigned int)line->width : 1,
				    (rotation&1)==0 ? 1 : (unsigned int)line->width,
				    /* (int)(bitsperpixel==24 ? 32 : bitsperpixel) */
				    (int)line->unit, (int)(((rotation&1)==0 ? line->width : 1)*pixel_bytes));

				switch (rotation&3) {
				case 0:
					for (y=0;y<drawh;y++) {
						for (x=0;x<draww;x++)
							x_PutPixel(line, x, 0,
							    getinterpolatedpixel((size_t)eix, x*ww, y*hh));
/*							    module[eix].getpixel((int)(x*ww),
							    (int)(y*hh),1));*/
						XPutImage(display,mywin,ngc,imageline,0,0,0,(signed)y,draww,1);
					}
					break;
				case 1:
					for (y=0;y<drawh;y++) {
						for (x=0;x<draww;x++)
							x_PutPixel(line, x, 0,
							    module[eix].getpixel((int)(x*ww),
							    (int)(y*hh),1));
						XPutImage(display,mywin,ngc,imageline,0,0,(int)(drawh-y-1),0,1,draww);
					}
					break;
				case 2:
					for (y=0;y<drawh;y++) {
						for (x=0;x<draww;x++)
							x_PutPixel(line, draww-x-1, 0,
							    module[eix].getpixel((int)(x*ww),
							    (int)(y*hh),1));
						XPutImage(display,mywin,ngc,imageline,0,0,0,(int)(drawh-y-1),draww,1);
					}
					break;
				case 3:
					for (y=0;y<drawh;y++) {
						for (x=0;x<draww;x++)
							x_PutPixel(line, draww-x-1, 0,
							    module[eix].getpixel((int)(x*ww),
							    (int)(y*hh),1));
						XPutImage(display,mywin,ngc,imageline,0,0,(signed)y,0,1,draww);
					}
					break;
				}
				XDestroyImage(imageline);
			}
			module[eix].close();
			XFreeGC(display,ngc);
			drawn=1;
			x_SetTitle(filename, width, height);
		}

		XNextEvent(display,&event);

		switch (event.type) {
		case KeyPress:
			switch (XLookupKeysym(&event.xkey,0)) {
			case XK_Up:
			case XK_KP_8:
				done=1;
				retval=REQUEST_PREVIOUS;
				break;
			case XK_Down:
			case XK_KP_2:
				done=1;
				retval=REQUEST_NEXT;
				break;
			case XK_Left:
			case XK_KP_4:
				rotation--;
				drawn=0;
				break;
			case XK_Right:
			case XK_KP_6:
				rotation++;
				drawn=0;
				break;
			default:
				done=1;
			}
			break;
		case ButtonPress:
			done=1;
			break;
		case DestroyNotify:
			exitnow=1;
			break;
		case Expose:
			if (event.xexpose.count!=0) break;
			drawn=0;
			break;
		case ConfigureNotify:
			fp=fopen(configpath,"w");
			if (fp!=0) {
				cf.x = (unsigned int)event.xconfigure.x;
				cf.y = (unsigned int)event.xconfigure.y;
				cf.w = (unsigned int)event.xconfigure.width;
				cf.h = (unsigned int)event.xconfigure.height;
				fprintf(fp,"\n# iiviewrc - configuration file for iiview\n\n");
				fprintf(fp,"xpos=%d\n",cf.x);
				fprintf(fp,"ypos=%d\n",cf.y);
				fprintf(fp,"width=%d\n",cf.w);
				fprintf(fp,"height=%d\n",cf.h);
				fclose(fp);
			}
			x_ClearThumbs();
			drawn=0;
			break;
		default: break;
		}
	}

	return retval;
}

static void x_showSelectedPicture(void) {

	int ret;

	do {
		ret=x_FullScreenIMG(get_file(file,dirsel),NULL);
		switch (ret) {
		case REQUEST_PREVIOUS:
			if (!get_file(file, dirsel-1)) {
				if (file->prevfile!=NULL)
					file=file->prevfile;
			} else dirsel--;
			if (is_file_dir(file,dirsel)) dirsel++;
			break;
		case REQUEST_NEXT:
			if (get_file(file, dirsel+1)) {
				if ((unsigned int)(dirsel+1)<THUMB_COUNT[THUMB_MODE]) dirsel++;
				else file=file->nextfile;
			}
			break;
		}
	} while (ret==REQUEST_PREVIOUS || ret==REQUEST_NEXT);
}

int x_ActivateImage(unsigned int nr, char *selectedfile, size_t selectedfilesize) {
	char dircomp[MAXPATHLEN];
	int eix;

	/* determine if use selection or nr */
	if (selectedfile[0]==0) {
		selectedfile=get_file(file, nr);
		selectedfilesize=strlen(selectedfile)+1;
	}

	if (selectedfile[0]=='/') {
		/* change directory (skip "/") */
		strlcpy(dircomp,selectedfile+1, sizeof(dircomp));
		dirsel=0;
		chdir(dircomp);
		GetFileList();
		return 1;
	}

	/* determine if known extension */
	eix=modules_extensionindex(selectedfile);

	if (eix>=0) {
		if (dir_int!=NULL && strlen(dir_int)>0) {
			snprintf(dircomp,sizeof(dircomp),"%s/%s",dir_int,get_file(file,nr));
		} else {
			if (selectedfile[0]!=0)
				strlcpy(dircomp,get_file(file,nr), sizeof(dircomp));
			else dircomp[0]=0;
		}
		if (module[eix].setdirectory(selectedfile,dircomp)==0) {

			/* show picture */
			dirsel=nr;
			x_showSelectedPicture();

		} else {
			/* change internal directory */
			if (selectedfile[0]==0) {
				strlcpy(selectedfile,get_file(file,nr), selectedfilesize);
			}
			module[eix].getfilelist(selectedfile,dir_int);
			seekfirst_file(&file);
			dirsel=0;
			return 1;
		}
	}

	return 1;
}

static int x_prevPic(void) {
	if (dirsel>0) dirsel--;
	else {
		if (file->prevfile!=NULL) {
			file=file->prevfile;
			return 1;
		}
	}

	return 0;
}

static int x_nextPic(void) {
	if (dirsel<THUMB_COUNT[THUMB_MODE]-1) {
		if (get_file(file, dirsel+1)!=NULL) dirsel++;
	} else {
		if (file->nextfile!=NULL) {
			file=file->nextfile;
			return 1;
		}
	}

	return 0;
}

/* keyboard handler */
int x_HandleKey(KeySym key, char *selectedfile, size_t selectedfilesize)
{
	unsigned int i;

	switch (key) {
	case XK_Up:
	case XK_KP_8:
		if (THUMB_MODE) {
			if (dirsel>=THUMB_INLINE[THUMB_MODE])
				dirsel-=THUMB_INLINE[THUMB_MODE];
			else {
				i=THUMB_INLINE[THUMB_MODE];
				while (file->prevfile!=NULL && i-->0)
					file=file->prevfile;
				return 1;
			}
			break;
		}
		break;
	case XK_Left:
	case XK_KP_4:
		return x_prevPic();
	case XK_Down:
	case XK_KP_2:
		if (THUMB_MODE) {
			if (dirsel<THUMB_COUNT[THUMB_MODE]-THUMB_INLINE[THUMB_MODE]) {

				dirsel+=THUMB_INLINE[THUMB_MODE];
				while (get_file(file, dirsel)==NULL) dirsel--;
			} else {
				/* scroll thumbs up */
				i=THUMB_INLINE[THUMB_MODE];
				while (file->nextfile!=NULL && i>0) {
					file=file->nextfile;
					i--;
				}
				dirsel-=i;
				while (get_file(file, dirsel)==NULL) dirsel--;

				return 1;
			}
			break;
		}
		break;
	case XK_Right:
	case XK_KP_6:
		return x_nextPic();
	case XK_h:
		x_HelpScreen();
		return 1;
	case XK_r:
		GetFileList();
		return 1;
	case XK_Page_Up:
		dirsel=0;
		i=THUMB_COUNT[THUMB_MODE];
		while (file->prevfile!=NULL && i-->0) {
			file=file->prevfile;
		}
		return 1;
	case XK_Page_Down:
		dirsel=THUMB_COUNT[THUMB_MODE]-1;
		i=THUMB_COUNT[THUMB_MODE];
		while (get_file(file, dirsel)!=NULL && i-->0)
			file=file->nextfile;
		while (get_file(file, dirsel)==NULL)
			dirsel--;
		return 1;
	case XK_Home:
		dirsel=0;
		seekfirst_file(&file);
		return 1;
	case XK_End:
		dirsel=THUMB_COUNT[THUMB_MODE]-1;
		seeklast_file(&file);
		i=THUMB_COUNT[THUMB_MODE]-1;
		while (file->prevfile!=NULL && i-->0)
			file=file->prevfile;
		while (get_file(file, dirsel)==NULL)
			dirsel--;
		return 1;
	case XK_Return:
	case XK_KP_Enter:
		return x_ActivateImage(dirsel, selectedfile, selectedfilesize);
	case XK_BackSpace:
		dirsel=0;
		chdir("..");
		update_needed=1;
		GetFileList();
		break;
	case XK_minus:
	case XK_KP_Subtract:
		if (THUMB_MODE<3) {
			THUMB_MODE++;
			x_ClearThumbs();
		}
		return 1;
	case XK_plus:
	case XK_KP_Add:
		if (THUMB_MODE>1) {
			THUMB_MODE--;
			x_ClearThumbs();
			if (dirsel>=THUMB_COUNT[THUMB_MODE]) dirsel=THUMB_COUNT[THUMB_MODE]-1;
		}
		return 1;
	case XK_Escape:
	case XK_q:
		exitnow=1;
		return 0;
	}
	return update_needed;
}

void x_EventLoop(char *selectedfile, size_t selectedfilesize)
{
	FILE *fp;
	unsigned int nr;

	exitnow=0;
	x_ClearThumbs();
	while (!exitnow) {
		if (!event_occured) XNextEvent(display,&event);
		event_occured=0;

		switch (event.type) {
		case KeyPress:
			x_UpdateScreen(x_HandleKey(XLookupKeysym(&event.xkey, 0), selectedfile, selectedfilesize), selectedfile);
			break;
		case ButtonPress:
			if (event.xbutton.button == 2) {
				dirsel=0;
				chdir("..");
				GetFileList();
				x_UpdateScreen(1, selectedfile);
				break;
			}
			else if (event.xbutton.button == 4) {
				x_UpdateScreen(x_HandleKey(XK_Page_Up, selectedfile, selectedfilesize), selectedfile);
				break;
			}
			else if (event.xbutton.button == 5) {
				x_UpdateScreen(x_HandleKey(XK_Page_Down, selectedfile, selectedfilesize), selectedfile);
				break;
			}
			nr=(unsigned int)event.xbutton.y/(thumbheight+20)*
			    THUMB_INLINE[THUMB_MODE]+
			    (unsigned int)event.xbutton.x/thumbwidth;

			if (nr<THUMB_COUNT[THUMB_MODE]) {
				if ((get_file(file,nr)!=NULL)&&(*get_file(file,nr)!=0)) {
					x_ActivateImage(nr, "", 1);
				}
			}
			x_UpdateScreen(1, selectedfile);
			break;
		case DestroyNotify:
			exitnow=1;
			break;
		case Expose:
			while (XCheckTypedEvent(display,Expose,&event));
			if (event.xexpose.count!=0) break;
			x_UpdateScreen(1, selectedfile);
			break;
		case ConfigureNotify:
			while (XCheckTypedEvent(display,ConfigureNotify,&event));
			fp=fopen(configpath,"w");
			if (fp!=0) {
				cf.x = (unsigned int)event.xconfigure.x;
				cf.y = (unsigned int)event.xconfigure.y;
				cf.w = (unsigned int)event.xconfigure.width;
				cf.h = (unsigned int)event.xconfigure.height;
				fprintf(fp,"\n# iiviewrc - configuration file for iiview\n\n");
				fprintf(fp,"xpos=%d\n",cf.x);
				fprintf(fp,"ypos=%d\n",cf.y);
				fprintf(fp,"width=%d\n",cf.w);
				fprintf(fp,"height=%d\n",cf.h);
				fclose(fp);
			}
			x_ClearThumbs();
			x_UpdateScreen(1, selectedfile);
			break;
		default: break;
		}
	}
}
