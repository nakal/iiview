/*

   fbsupport.c
   -----------

   Framebuffer support

*/

/*#define VERBOSE*/


#ifdef FRAMEBUFFER

/* ansi C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

/* unix specific includes */
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

/* framebuffer specific */
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <curses.h>
#include <termios.h>
#include <linux/vt.h>

#include "common.h"
#include "modules.h"
#include "filelist.h"
#include "fbsupport.h"

/* FB supporting stuff */
#define FONTPATHCOUNT 4
const char *fontpath[]= {"gzip -cd /usr/lib/kbd/consolefonts/lat1-08.psf.gz",
	"gzip -cd /usr/share/consolefonts/lat1-08.psf.gz",
	"gzip -cd /usr/share/consolefonts/lat1-08.psfu.gz",
	"gzip -cd /lib/kbd/consolefonts/lat1-08.psf.gz"
};
char *font,*fbmem;
int fontheight,fbdev;
struct fb_fix_screeninfo fb_finfo;
struct fb_var_screeninfo fb_vinfo,fb_vinfo_orig;
int keycode;
int tty;
struct vt_mode vt_modesave;

/*
   void FB_UpdateThumbs(void);
   void FB_Print(int x,int y,const char *text,unsigned int fg,unsigned int bg);
   void FB_ClearScreen(void);
   void FB_TidyUp(int signr);
   void FB_Init(void);
   void FB_OutputFilename(unsigned int nr);
   void FB_UpdateScreen(int repaint);
   void FB_HelpScreen(void);
   int FB_FullScreenIMG(char *filename,char *imagename);
   void FB_KeyBoardLoop(void);
   void FB_PutPixel(int x,int y,unsigned int color);
   */
void FB_ClearThumbs(void);
void tty_switchhandler(int signr);
unsigned int *FB_AllocImage(void);
int FB_ActivateImage(int nr, const char *selectedfile);


void FB_PutPixel(int x,int y,unsigned int color)
	/* color => RGB (xxxxxxxxRRRRRRRRGGGGGGGGBBBBBBBB) */
{
	unsigned int c,rgbval;
	char *ptr;

	if (x<0||x>=(int)fb_vinfo.xres||y<0||y>=(int)fb_vinfo.yres) return;

	rgbval=
	    ((((color >> 16)&0xFF)>>(8-fb_vinfo.red.length))<< fb_vinfo.red.offset)|
	    ((((color >> 8)&0xFF)>>(8-fb_vinfo.green.length))<< fb_vinfo.green.offset)|
	    (((color&0xFF)>>(8-fb_vinfo.blue.length))<< fb_vinfo.blue.offset);

	ptr=fbmem+(y*fb_finfo.line_length+x*fb_vinfo.bits_per_pixel/8);
	for (c=0;c<fb_vinfo.bits_per_pixel/8;c++) {
		*ptr++=rgbval&0xFF;rgbval>>=8;
	}
}

void FB_Print(int x,int y,const char *text,unsigned int fg,unsigned int bg)
{
	int xx,yy;
	unsigned int i;

	for (i=0;i<strlen(text);i++) {
		for (yy=0;yy<fontheight;yy++) {
			for (xx=7;xx>=0;xx--) {
				FB_PutPixel(x+(i<<3)+7-xx,y+yy,
				    font[((int)text[i]<<3)+yy] & (1<<xx) ? fg : bg);
			}
		}
	}
}

void FB_ClearScreen(void)
{
	int x,y;

	for (y=0;y<(int)cf.h;y++)
		for (x=0;x<(int)cf.w;x++)
			FB_PutPixel(x,y,0);
}

void FB_ClearThumbs(void) {

#ifdef VERBOSE
	fprintf(stderr,"fbsupport: FB_ClearThumbs\n");
#endif

	delete_all_data(file);
}

void FB_TidyUp(int signr)
{
	signr=signr;

	/* clean up */

	FB_ClearScreen();

	ioctl(tty,VT_SETMODE,&vt_modesave);
	close(tty);

	if (ioctl(fbdev,FBIOPUT_VSCREENINFO,&fb_vinfo_orig)<0) {
		printf("Error: FBIOPUT_VSCREENINFO. (restoring state)\n");
	}

	if (munmap(fbmem,fb_finfo.smem_len)) {
		printf("Error: munmap failed.\n");
	}
	close(fbdev);
#ifdef VERBOSE
	printf("Framebuffer device closed.\n");
#endif
	free(font);
#ifdef VERBOSE
	printf("Font memory released.\n");
#endif

	/* curses clean */
	endwin();

	printf("Exiting.\n");
	/*  printLastJPEGError(); */
}

void tty_switchhandler(int signr)
{
	const char selectedfile[1]={ 0 };

	switch (signr) {
	case SIGUSR1:
		/*    ttystate=0;*/
		ioctl(tty, VT_RELDISP, 1);
		break;
	case SIGUSR2:
		/*    ttystate=1;*/
		ioctl(tty, VT_RELDISP, VT_ACKACQ);
		/*FB_ClearScreen();*/
		FB_UpdateThumbs(selectedfile);
		break;
	}
}

void FB_Init(void)
{
	FILE *fp;
	char *fbdevname=NULL;
	int fontpathnr=0;
	struct vt_mode vtm;
	struct sigaction old,now;

	while (fontpathnr<FONTPATHCOUNT) {
		fp=popen(fontpath[fontpathnr],"r");
		if (fp!=NULL) {
			if ((fgetc(fp)!=0x36)||(fgetc(fp)!=0x04))
				pclose(fp);
			else break;
		}

		fontpathnr++;
	}

	if (fontpathnr>=FONTPATHCOUNT) {
		printf("Could not access console fonts.");
		exit(1);
	}

	fgetc(fp);fontheight=fgetc(fp);

	if ((font=(char *)malloc((size_t)fontheight*256))==NULL) {
		printf("Error: could not malloc font-memory.\n");
		exit(1);
	}

	fread(font,256,(size_t)fontheight,fp);
	fclose(fp);

	fbdevname=getenv("FRAMEBUFFER");

	fbdev=open(fbdevname==NULL ? "/dev/fb0" : fbdevname, O_RDWR, 0);

	if (!fbdev) {
		printf("There is no way to display graphics.\n");
		printf("Error: opening X connection.\n");
		printf("Error: opening '/dev/fb0'.\n");
		exit(1);
	}

	if (ioctl(fbdev,FBIOGET_FSCREENINFO,&fb_finfo)<0) {
		printf("Error: FBIOGET_FSCREENINFO\n");
		exit(1);
	}
	if (ioctl(fbdev,FBIOGET_VSCREENINFO,&fb_vinfo)<0) {
		printf("Error: FBIOGET_VSCREENINFO\n");
		exit(1);
	}
	memcpy(&fb_vinfo_orig,&fb_vinfo,sizeof(fb_vinfo_orig));

#ifdef VERBOSE
	printf("FB_TYPE  : %d\n",fb_finfo.type);
	printf("FB_VISUAL: %d\n",fb_finfo.visual);
	printf("Resolution: %dx%dx%d\n",fb_vinfo.xres,fb_vinfo.yres,
	    fb_vinfo.bits_per_pixel);
	printf("Pixel format: red  : %d-%d\n",fb_vinfo.red.offset,
	    fb_vinfo.red.offset+fb_vinfo.red.length-1);
	printf("              green: %d-%d\n",fb_vinfo.green.offset,
	    fb_vinfo.green.offset+fb_vinfo.green.length-1);
	printf("              blue : %d-%d\n",fb_vinfo.blue.offset,
	    fb_vinfo.blue.offset+fb_vinfo.blue.length-1);
	sleep(2);
#endif

	fbmem=mmap(NULL,fb_finfo.smem_len,PROT_WRITE,
	    MAP_SHARED,fbdev,0);

	if (fbmem==MAP_FAILED) {
		printf("Error: mmap failed.\n");
		exit(1);
	}

	if (fb_vinfo.xoffset!=0 || fb_vinfo.yoffset!=0) {
		fb_vinfo.xoffset=0;
		fb_vinfo.yoffset=0;
		if (ioctl(fbdev,FBIOPAN_DISPLAY,&fb_vinfo)<0) {
			printf("Error: FBIOPAN_DISPLAY\n");
			exit(1);
		}
	}


	tidyup=FB_TidyUp;

	cf.w=fb_vinfo.xres;
	cf.h=fb_vinfo.yres;

	/* tty init */
	if ((tty=open("/dev/tty",O_RDWR))<0) {
		printf("Error opening tty device.\n");
		exit(1);
	}
	if (ioctl(tty,VT_GETMODE,&vt_modesave)<0) {
		printf("Error writing to tty device.\n");
		exit(1);
	}
	memcpy(&vtm,&vt_modesave,sizeof(struct vt_mode));
	now.sa_handler  = tty_switchhandler;
	now.sa_flags    = 0;
	now.sa_restorer = NULL;
	sigemptyset(&now.sa_mask);
	sigaction(SIGUSR1,&now,&old);
	sigaction(SIGUSR2,&now,&old);

	vtm.mode   = VT_PROCESS;
	vtm.waitv  = 0;
	vtm.relsig = SIGUSR1;
	vtm.acqsig = SIGUSR2;

	if (ioctl(tty,VT_SETMODE,&vtm)<0) {
		printf("Error writing to tty device.\n");
		exit(1);
	}

	/* curses stuff */
	initscr();
	cbreak();
	noecho();
	nodelay(stdscr,TRUE);
	keypad(stdscr,TRUE);
	curs_set(0);
	while (getch()!=ERR);

	fbmode=1;
}

/* fb-ver. output thumb subscription */
void FB_OutputFilename(unsigned int nr)
{
	int x,y,xx,yy;
	unsigned int fg,bg;
	char *fn;

	if (dirsel==nr)
		bg=0x00FFFFFF;
	else bg=0x00000000;

	x=(nr % THUMB_INLINE[THUMB_MODE])*thumbwidth;
	y=((nr / THUMB_INLINE[THUMB_MODE])+1)*(thumbheight+10)-10+THUMB_YOFS;

	for (yy=0;yy<(int)fontheight;yy++) for (xx=0;xx<(int)thumbwidth;xx++) {
		FB_PutPixel(x+xx,y+yy,bg);
	}

	if (is_file_dir(file,nr)) fg=0x0000FF00; else fg=0x0000FFFF;

	fn=get_file(file,nr);
	if (fn!=NULL) FB_Print(x,y,fn,fg,bg);
}

unsigned int *FB_AllocImage(void) {
	return (unsigned int *)malloc(sizeof(unsigned int)*thumbwidth*thumbheight);
}

/* fb-ver. updates thumbs (!) */
void FB_UpdateThumbs(const char *selectedfile)
{
	const char *curfile;
	char *curdir;
	int x,y,diralloc;
	int xx,yy,tw,th,xp,yp;
	unsigned char n;
	int eix,width,height;
	unsigned int *img,pix;
	char imgdir[1025];


	if (THUMB_MODE==0) {
		update_needed=0;
		return;
	}
	if ((curdir=getcwd(NULL,512))==NULL) {
		update_needed=0;
		fprintf(stderr,"FAILED (getcwd).\n");
		return;
	}
	diralloc=1;

	n=0;event_occured=0;

	for (y=0;y<fontheight;y++)
		for (x=0;x<(int)cf.w;x++)
			FB_PutPixel(x,y,0x0000FF);

	for (;y<THUMB_YOFS;y++)
		for (x=0;x<(int)cf.w;x++)
			FB_PutPixel(x,y,0x000000);

	FB_Print(8,0,WINDOW_TITLE,0xFFFFFF,0x0000FF);

#ifdef VERBOSE
	fprintf(stderr,"Updating thumbs.");
#endif

	for (y=0;y<(int)THUMB_LINES[THUMB_MODE];y++) {
		for (x=0;x<(int)THUMB_INLINE[THUMB_MODE];x++,n++) {
			yp=y*(thumbheight+10)+THUMB_YOFS;
			xp=x*thumbwidth;
			if ((get_file(file,n)==NULL) ||
			    ((*get_file(file,n))==0) || (is_file_dir(file,n))) {
				/* is a directory or nothing */

				for (yy=0;yy<(int)(thumbheight+10);yy++)
					for (xx=0;xx<(int)thumbwidth;xx++)
						FB_PutPixel(xp+xx,yp+yy,0);

				if (is_file_dir(file,n)) FB_OutputFilename(n);
				continue;
			}

			/* is a file ! */

			if ((img=get_data(file,n))==NULL) {

				/* read in file, because not cached yet */
				/* read in file */

				if ((img=FB_AllocImage())!=NULL) {

					/* allocation ok */

					set_data(file,n,img);

					if (selectedfile[0]==0) {
						curfile=get_file(file,n);
						eix=modules_extensionindex(curfile);
						imgdir[0]=0;
					}
					else {
						eix=modules_extensionindex(selectedfile);
						curfile=selectedfile;
						if (dir_int!=NULL) sprintf(imgdir,"%s/",dir_int);
						else sprintf(imgdir,"%s/%s",dir_int,get_file(file,n));
					}

					if (module[eix].open(curfile,imgdir,thumbwidth,thumbheight)) {
						width=module[eix].getwidth();
						height=module[eix].getheight();

						if (((float)width/height)>
						    ((float)thumbwidth/thumbheight)) {
							tw=thumbwidth;
							th=(float)height/width*thumbwidth;
						} else {
							tw=(float)width/height*thumbheight;
							th=thumbheight;
						}

						for (yy=0;yy<(int)(thumbheight+10);yy++)
							for (xx=0;xx<(int)thumbwidth;xx++) {
								pix=module[eix].getpixel((int)((float)xx/tw*width),
								    (int)((float)yy/th*height),1);
								FB_PutPixel(xp+xx,yp+yy,pix);
								if (yy<(int)thumbheight) img[yy*thumbwidth+xx]=pix;
							}
					} else {
						/* error while reading */
						for (yy=0;yy<(int)(thumbheight+10);yy++)
							for (xx=0;xx<(int)thumbwidth;xx++)
								FB_PutPixel(xp+xx,yp+yy,0);
					}
					module[eix].close();
				} else {
					/* no memory */
					for (yy=0;yy<(int)(thumbheight+10);yy++)
						for (xx=0;xx<(int)thumbwidth;xx++)
							FB_PutPixel(xp+xx,yp+yy,0);
				}
			} else {
				/* pic cached */
				for (yy=0;yy<(int)thumbheight;yy++)
					for (xx=0;xx<(int)thumbwidth;xx++)
						FB_PutPixel(xp+xx,yp+yy,img[yy*thumbwidth+xx]);

				for (yy=thumbheight;yy<(int)(thumbheight+10);yy++)
					for (xx=0;xx<(int)thumbwidth;xx++)
						FB_PutPixel(xp+xx,yp+yy,0);
			}

			FB_OutputFilename(n);
			if ((keycode=getch())!=ERR) {
				event_occured=1;
				update_needed=1;
				return;
			}
		}
	}
	update_needed=0;
}

/* fb-ver. updates the whole screen (thumbs, font) */
void FB_UpdateScreen(int repaint, const char *selectedfile)
{
	unsigned int i;

	/*  GetFileList();*/
	for (i=0;i<THUMB_COUNT[THUMB_MODE];i++)
		FB_OutputFilename(i);

	if (repaint)
		FB_UpdateThumbs(selectedfile);
}

void FB_HelpScreen(void)
{

	FB_ClearScreen();

	FB_Print(0,0,HelpHeader,0x0000FF,0x000000);

	FB_Print(0,5*fontheight,Help1,0xFFFF00,0x000000);
	FB_Print(0,6*fontheight,Help2,0xFFFF00,0x000000);
	FB_Print(0,7*fontheight,Help3,0xFFFF00,0x000000);
	FB_Print(0,8*fontheight,Help4,0xFFFF00,0x000000);
	FB_Print(0,9*fontheight,Help5,0xFFFF00,0x000000);

	while (getch()==ERR);
	update_needed=1;
}

/* fb-ver. shows image in full-screen */
int FB_FullScreenIMG(const char *filename,const char *imagename)
{
	int x,y;
	int w,h;
	int eix,width,height;

	if ((eix=modules_extensionindex(filename))<0) return 0;
	if (module[eix].open(filename,imagename,0,0)==0) return 0;
	width=module[eix].getwidth();
	height=module[eix].getheight();
	if ((width==0)||(height==0)) return 0;

	if (((double)width/height)>((double)cf.w/cf.h)) {
		w=cf.w;
		h=height*cf.w/width;
	} else {
		w=width*cf.h/height;
		h=cf.h;
	}

	for (y=0;y<(int)cf.h;y++)
		for (x=0;x<(int)cf.w;x++)
			FB_PutPixel(x,y,module[eix].getpixel(x*width/w,
			    y*height/h,1));

	module[eix].close();

	while (getch()==ERR) usleep(10000);

	return 1;
}

int FB_ActivateImage(int nr, const char *selectedfile) {
	char dircomp[1025];
	char *p;
	int eix;

	if ((p=get_file(file,nr))!=NULL) {

		if (!is_file_dir(file,nr) || selectedfile[0]!=0) {

			if (selectedfile[0]==0)
				eix=modules_extensionindex(get_file(file,nr));
			else eix=modules_extensionindex(selectedfile);

			if (eix>=0) {
				/* internal directory ? */

				/* this is still wrong !!! need to jump only ONE dir up */
				if (!strcmp(get_file(file,nr),"/..")) {
					nr=0;
					GetFileList();
					return 1;
				}

				if (dir_int!=NULL && strlen(dir_int)>0) {
					sprintf(dircomp,"%s/%s",dir_int,get_file(file,nr));
				} else {
					if (selectedfile[0]!=0) strcpy(dircomp,get_file(file,nr));
					else dircomp[0]=0;
				}
				if (module[eix].setdirectory(selectedfile[0]==0 ?
				    get_file(file,nr) :
				    selectedfile,dircomp)==0) {

					/* show picture */
					if (selectedfile[0]==0)
						FB_FullScreenIMG(get_file(file,nr),NULL);
					else FB_FullScreenIMG(selectedfile,dircomp);

				} else {
					/* change internal directory */
					if (selectedfile[0]==0) {

						/*strcpy(selectedfile,get_file(file,nr));*/
						module[eix].getfilelist(get_file(file, nr),dir_int);
					} else module[eix].getfilelist(selectedfile,dir_int);

					seekfirst_file(&file);
					dirsel=0;
					return 1;
				}
			}
		} else {
			/* change directory */
			p++;
			strcpy(dircomp,p);
			dirsel=0;
			chdir(dircomp);
			GetFileList();
		}
	}
	return 1;
}

void FB_KeyBoardLoop(const char *selectedfile, size_t selectedfilelen)
{
	int i;

	exitnow=event_occured=0;update_needed=1;

	while (!exitnow) {
		thumbwidth =cf.w/THUMB_INLINE[THUMB_MODE];
		thumbheight=(cf.h-THUMB_YOFS-10*(THUMB_LINES[THUMB_MODE]))/
		    THUMB_LINES[THUMB_MODE];
		FB_UpdateScreen(update_needed, selectedfile);
		if (!event_occured)
			while ((keycode=getch())==ERR) usleep(10000);

		event_occured=0;

		switch (keycode) {
		case KEY_UP:
			if (THUMB_MODE) {
				if (dirsel>=THUMB_INLINE[THUMB_MODE])
					dirsel-=THUMB_INLINE[THUMB_MODE];
				else {
					i=THUMB_INLINE[THUMB_MODE];
					while (file->prevfile!=NULL && i-->0)
						file=file->prevfile;
					update_needed=1;
				}
				break;
			}
		case KEY_LEFT:
			if (dirsel>0) dirsel--;
			else {
				if (file->prevfile!=NULL) {
					file=file->prevfile;
					update_needed=1;
				}
			}
			break;
		case KEY_DOWN:
			if (THUMB_MODE) {
				if (dirsel<THUMB_COUNT[THUMB_MODE]-THUMB_INLINE[THUMB_MODE])
					dirsel+=THUMB_INLINE[THUMB_MODE];
				else {
					i=THUMB_INLINE[THUMB_MODE];
					while (file->nextfile!=NULL && i-->0)
						file=file->nextfile;
					dirsel+=THUMB_INLINE[THUMB_MODE];
					if (dirsel>=THUMB_COUNT[THUMB_MODE])
						dirsel-=THUMB_INLINE[THUMB_MODE];

					update_needed=1;
				}
				break;
			}
		case KEY_RIGHT:
			if (dirsel<THUMB_COUNT[THUMB_MODE]-1) dirsel++;
			else {
				if (file->nextfile!=NULL) {
					file=file->nextfile;
					update_needed=1;
				}
			}
			break;
		case 'h':
		case 'H':
			FB_HelpScreen();
			update_needed=1;
			break;
		case KEY_PPAGE:
			dirsel=0;
			i=THUMB_COUNT[THUMB_MODE];
			while (file->prevfile!=NULL && i-->0)
				file=file->prevfile;
			update_needed=1;
			break;
		case KEY_NPAGE:
			dirsel=THUMB_COUNT[THUMB_MODE]-1;
			i=THUMB_COUNT[THUMB_MODE];
			while (file->nextfile!=NULL && i-->0)
				file=file->nextfile;
			update_needed=1;
			break;
		case KEY_HOME:
			dirsel=0;
			seekfirst_file(&file);
			update_needed=1;
			break;
		case KEY_END:
			dirsel=THUMB_COUNT[THUMB_MODE]-1;
			seeklast_file(&file);
			i=THUMB_COUNT[THUMB_MODE]-1;
			while (file->prevfile!=NULL && i-->0)
				file=file->prevfile;
			update_needed=1;
			break;
		case '\n':
			/*
			   if ((p=get_file(file,dirsel))!=NULL) {
			   if (!is_file_dir(file,dirsel)) {
			   FB_FullScreenIMG(get_file(file,dirsel));
			   } else {
			   p++;i=0;
			   do { str[i++]=*p; } while (*p++);
			   dirsel=0;
			   chdir(str);
			   GetFileList();
			   }
			   update_needed=1;
			   }
			   */
			update_needed=FB_ActivateImage(dirsel, selectedfile);
			break;
		case '-':
			if (THUMB_MODE<3) {
				THUMB_MODE++;
			}
			FB_ClearThumbs();
			update_needed=1;
			break;
		case '+':
			if (THUMB_MODE>1) {
				THUMB_MODE--;
				if (dirsel>=THUMB_COUNT[THUMB_MODE]) dirsel=THUMB_COUNT[THUMB_MODE]-1;
			}
			FB_ClearThumbs();
			update_needed=1;
			break;
		case 27:
		case 'q':
		case 'Q':
			exitnow=1;
			break;
		default:
			printw("Key not recognized: (%d)\n",keycode);
		}
	}
}

#endif /* FRAMEBUFFER */

