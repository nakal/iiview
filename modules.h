
#ifndef IIVIEW_MODULES_INCLUDED
#define IIVIEW_MODULES_INCLUDED

/*

   modules.h


Infos: see modules.c
*/

/*
   Small explanation of the module structure:

   extension_count - number of file extensions which will be used
   to identify an image to be in this module
   extension       - pointer to a list with extensions which will be used
   to identify an image to be in this module (please
   don't use same extensions for different modules!)
   open            - opens the image file and reads in all data, so
   the methods getpixel, getwidth and getheight will
   become accessible
   close           - closes the image file and frees all space used by
   the module
   getpixel        - returns the RGB pixel value at specified position
   in the image parameters are x,y and a boolean value
   which requests a color-mode (1=color 2=b&w)
   getwidth        - width of the image
   getheight       - height of the image
   getfilelist     - gets the list of files and directories INSIDE THE
   IMAGE (!); returns the number of entries;
   don't forget to insert a '..' directory, so
   we can browse the directories to the top
   setdirectory    - will set the directory INSIDE THE IMAGE;
   returns 0, if the specified directory is a file or
   the specified directory is NULL and the file doesn't
   have internal directory structures
   returns 1, if the directory could be set successfully
   */

#define UNUSED(p) (void)(p)
#define MODULES_COUNT 5

typedef struct {
	size_t            extension_count;
	const char        **extension;
	int               (*open)(const char *,const char *, unsigned int, unsigned int);
	void              (*close)(void);
	unsigned int      (*getpixel)(int,int,int);
	size_t            (*getwidth)(void);
	size_t            (*getheight)(void);

	/* directory handler routines */
	int (*getfilelist)(const char *,const char *);
	int (*setdirectory)(const char *,const char *);

} module_type;

extern module_type module[MODULES_COUNT];

void modules_init(void);
int  modules_knownextension(const char *);
int  modules_extensionindex(const char *);

#endif

