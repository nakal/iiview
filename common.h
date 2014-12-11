
#ifndef IIVIEW_COMMON_INCLUDED
#define IIVIEW_COMMON_INCLUDED

#define WINDOW_TITLE "iiview-0.31"

extern unsigned int dirsel;                         /* thumb selection vars */
extern char *dir_int;                               /* internal image
						       directory */
extern unsigned char THUMB_MODE;                    /* current thumb mode */
extern const char *HOMEEnv;
extern const char *configfilename;
extern char configpath[];
extern int fbmode;
extern int event_occured;
extern int update_needed;
extern int exitnow;
extern unsigned int thumbwidth;
extern unsigned int thumbheight;
extern const unsigned int THUMB_COUNT[];
extern const unsigned int THUMB_INLINE[];
extern const unsigned int THUMB_LINES[];
extern const int THUMB_YOFS;
extern const char MAX_CACHEDDIRS;
extern char HelpHeader[];
extern char Help1[];
extern char Help2[];
extern char Help3[];
extern char Help4[];
extern char Help5[];

typedef struct {
	unsigned int x,y,w,h;
} config_struct;

extern config_struct cf;

/* function prototypes */

extern void (*tidyup)(int);
extern void destroy_data(void **);
extern void GetFileList(void);

#endif

