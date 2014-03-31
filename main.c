/* This is the main program. */


/* ansi C includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>

#include "compat.h"

/* unix specific includes */
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

#include "common.h"

#include "filelist.h"
#include "modules.h"
#ifndef NO_X_SUPPORT
#include "xsupport.h"
#else
#ifndef FRAMEBUFFER
#error !!! Illegal combination of compiler switches !!! (NO_XSUPPORT + !FRAMEBUFFER)
#endif
#endif
#ifdef _FREEBSD
#error !!! No framebuffer support for FreeBSD !!!
#else
#include "fbsupport.h"
#endif

struct file_p *file=NULL;
static char selectedfile[MAXPATHLEN];
unsigned int dirsel;                         /* thumb selection vars */
char *dir_int=NULL;
unsigned char THUMB_MODE;                    /* current thumb mode */
const char *HOMEEnv="HOME";
const char *configfilename=".iiviewrc";
char configpath[MAXPATHLEN];
int fbmode=0;
int event_occured=0;
int update_needed=0;
int exitnow=0;
unsigned int thumbwidth,thumbheight;

/* structure for window coordinates */
config_struct cf;

const unsigned int THUMB_COUNT[]={50,12,30,54};
const unsigned int THUMB_INLINE[]={0,4,6,9};
const unsigned int THUMB_LINES[]={0,3,5,6};
const int THUMB_YOFS=10;
const char MAX_CACHEDDIRS=20;
char HelpHeader[]="For help go to: https://github.com/nakal/iiview";
char Help1[]="H                         - help (this page)";
char Help2[]="cursor/page keys          - select image";
char Help3[]="return/mouse click        - show image";
char Help4[]="+/-                       - larger/smaller thumbs";
char Help5[]="Esc,q                     - quit";

void (*tidyup)(int);

#ifndef MAXPATHLEN
#error MAXPATHLEN definition missing
#endif

/* analyses the directory contents */
void GetFileList(void)
{
	DIR *dir;
	struct dirent *de;
	struct stat st;
	char curdir[MAXPATHLEN];
	char filename[MAXPATHLEN];

	if (getcwd(curdir,MAXPATHLEN)==NULL) return;

#ifdef VERBOSE
	fprintf(stderr,"Getting file list (dir: %s).\n", curdir);
#endif

	delete_all(&file);
	selectedfile[0]=0;

	if ((dir=opendir(curdir))==NULL) {
		insert_file(&file,"/..");
		return;
	}

	while ((de=readdir(dir))!=NULL) {

		if (stat(de->d_name,&st)==0)
			if ((S_ISREG(st.st_mode) && modules_knownextension(de->d_name))
			    ||(S_ISDIR(st.st_mode)&&(strcmp(de->d_name,".")!=0))) {

				if (S_ISDIR(st.st_mode))
					strlcpy(filename,"/", sizeof(filename));
				else filename[0]=0;

				strncat(filename, de->d_name, MAXPATHLEN);
				insert_file(&file, filename);
			}

	}

	closedir(dir);
	seekfirst_file(&file);
	dirsel=0;

#ifdef VERBOSE
	fprintf(stderr,"File list complete.\n");
#endif

#if 0
	{
		unsigned int i=0;

		while (get_file(file, i)!=NULL) {
			fprintf(stderr, "File: %s\n", get_file(file, i));
			i++;
		}
	}
#endif
}

void destroy_data(void **data) {

#ifdef VERBOSE
	fprintf(stderr,"main: destroy_data\n");
#endif

#ifdef FRAMEBUFFER
	if (fbmode) {

		if (*data!=NULL) free(*data);
	} else {
#endif

#ifndef NO_X_SUPPORT
		x_FreeImage(*(image_t **)data);
#endif

#ifdef FRAMEBUFFER
	}
#endif

	*data=NULL;
}

/* main program routine */
int main (int argc,char **argv)
{
	char *displayname=NULL;

	strlcpy(configpath,(char *)getenv(HOMEEnv), sizeof(configpath));
	strlcat(configpath,"/", sizeof(configpath));
	strlcat(configpath,configfilename, sizeof(configpath));

	modules_init();

	displayname=getenv("DISPLAY");

	if (displayname==NULL) {
#ifdef FRAMEBUFFER
#ifdef _FREEBSD
		VGL_Init();
#else
		FB_Init();
#endif
#else
		printf("Cannot connect to X-Server.\n");
		printf("And it's not compiled for framebuffer or glide.\n");
		exit(-1);
#endif
	}
#ifndef NO_X_SUPPORT
	else x_InitConnection(NULL);
#endif

	signal(SIGHUP,tidyup);signal(SIGINT,tidyup);
	signal(SIGQUIT,tidyup);signal(SIGTERM,tidyup);
	signal(SIGPIPE,tidyup);

#ifndef NO_X_SUPPORT
#ifdef VERBOSE
	printf("Trying setting X stuff...\n");
#endif
	if (!fbmode) x_InitXWindowStuff();
#ifdef VERBOSE
	printf("X stuff done.\n");
#endif
#endif

	dirsel=0;selectedfile[0]=0;
	THUMB_MODE=2;
	event_occured=update_needed=0;

#ifdef VERBOSE
	printf("Getting file list...\n");
#endif
	GetFileList();
#ifdef VERBOSE
	printf("Got file list.\n");
#endif

	if (!fbmode) {

#ifndef NO_X_SUPPORT
		if (argc!=1) {
			if (x_FullScreenIMG(argv[1], argc>2 ? argv[2] : "")==0) {
				printf("Error while displaying image.\n");
			}
		} else x_EventLoop(selectedfile, sizeof(selectedfile));
#else
		printf("Framebuffer could not be initialized.\n");
		exit(-1);
#endif
	}
#ifdef FRAMEBUFFER
	else {
#ifdef _FREEBSD
		if (argc!=1) {
			if (VGL_FullScreenIMG(argv[1], argc>2 ? argv[2] : "")==0) {
				printf("Error while displaying image.\n");
			}
		}
		else VGL_KeyBoardLoop(selectedfile, sizeof(selectedfile));
#else
		if (argc!=1) {
			if (FB_FullScreenIMG(argv[1], argc>2 ? argv[2] : "")==0) {
				printf("Error while displaying image.\n");
			}
		}
		else FB_KeyBoardLoop(selectedfile, sizeof(selectedfile));
#endif
	}
#endif

	delete_all(&file);
	tidyup(0);
	return 0;
}
