
/* Abstraction layer for handling different image formats. */

#include <string.h>

#include "modules.h"


#ifdef INCLUDE_JPEG
#ifdef NATIVE_JPEG
#include "jpeg.h"
#else
#include "njpeg.h"
#endif

#define JPEG_EXT_COUNT 3
static const char *jpeg_extensions[JPEG_EXT_COUNT]={".jpg",".JPG",".jpeg"};
#endif

#include "gif.h"
#define GIF_EXT_COUNT 2
static const char *gif_extensions[GIF_EXT_COUNT]={".gif",".GIF"};

#include "bmp.h"
#define BMP_EXT_COUNT 2
static const char *bmp_extensions[BMP_EXT_COUNT]={".bmp",".BMP"};

#ifdef INCLUDE_TIFF
#include "tif.h"
#define TIF_EXT_COUNT 5
static const char *tif_extensions[TIF_EXT_COUNT]={".tif",".TIF",".tiff",".nef",".NEF"};
#endif

#ifdef INCLUDE_PNG
#include "png.h"
#define PNG_EXT_COUNT 2
static const char *png_extensions[PNG_EXT_COUNT]={".png",".PNG"};
#endif

module_type module[MODULES_COUNT];

int getfilelist_null(const char *,const char *);
int setdirectory_null(const char *,const char *);

void modules_init(void)
{

#ifdef INCLUDE_JPEG
	/* implementing jpeg-routines */

#ifdef NATIVE_JPEG
	JPEG_INIT();
	module[0].extension_count=JPEG_EXT_COUNT;
	module[0].extension=jpeg_extensions;
	module[0].open=JPEG_OpenFile;
	module[0].close=JPEG_CloseFile;
	module[0].getpixel=JPEG_GetPixel;
	module[0].getwidth=JPEG_GetWidth;
	module[0].getheight=JPEG_GetHeight;
#else
	module[0].extension_count=JPEG_EXT_COUNT;
	module[0].extension=jpeg_extensions;
	module[0].open=open_jpeg;
	module[0].close=close_jpeg;
	module[0].getpixel=getpixel_jpeg;
	module[0].getwidth=getwidth_jpeg;
	module[0].getheight=getheight_jpeg;
#endif
	module[0].getfilelist=getfilelist_null;
	module[0].setdirectory=setdirectory_null;
#else
	module[0].extension_count=0;
#endif

	/* implementing gif-routines */

	module[1].extension_count=GIF_EXT_COUNT;
	module[1].extension=gif_extensions;
	module[1].open=GIF_OpenFile;
	module[1].close=GIF_CloseFile;
	module[1].getpixel=GIF_GetPixel;
	module[1].getwidth=GIF_GetWidth;
	module[1].getheight=GIF_GetHeight;
	module[1].getfilelist=getfilelist_null;
	module[1].setdirectory=setdirectory_null;


	/* implementing bmp-routines */

	module[2].extension_count=BMP_EXT_COUNT;
	module[2].extension=bmp_extensions;
	module[2].open=BMP_OpenFile;
	module[2].close=BMP_CloseFile;
	module[2].getpixel=BMP_GetPixel;
	module[2].getwidth=BMP_GetWidth;
	module[2].getheight=BMP_GetHeight;
	module[2].getfilelist=getfilelist_null;
	module[2].setdirectory=setdirectory_null;


#ifdef INCLUDE_TIFF
	/* implementing tif-routines */

	module[3].extension_count=TIF_EXT_COUNT;
	module[3].extension=tif_extensions;
	module[3].open=TIF_OpenFile;
	module[3].close=TIF_CloseFile;
	module[3].getpixel=TIF_GetPixel;
	module[3].getwidth=TIF_GetWidth;
	module[3].getheight=TIF_GetHeight;
	module[3].getfilelist=TIF_GetFileList;
	module[3].setdirectory=TIF_SetDirectory;
#else
	module[3].extension_count=0;
#endif


#ifdef INCLUDE_PNG
	/* implementing png-routines */

	module[4].extension_count=PNG_EXT_COUNT;
	module[4].extension=png_extensions;
	module[4].open=PNG_OpenFile;
	module[4].close=PNG_CloseFile;
	module[4].getpixel=PNG_GetPixel;
	module[4].getwidth=PNG_GetWidth;
	module[4].getheight=PNG_GetHeight;
	module[4].getfilelist=getfilelist_null;
	module[4].setdirectory=setdirectory_null;
#else
	module[4].extension_count=0;
#endif
}

int modules_knownextension(const char *extension)
{
	return modules_extensionindex(extension)>=0;
}

int modules_extensionindex(const char *extension)
{
	size_t m, i;
	const char *ext;

	ext = strrchr(extension, '.');
	if (ext==NULL) return -1;

	for (m=0;m<MODULES_COUNT;m++) {
		for (i=0;i<module[m].extension_count;i++) {
			if (strstr(ext,module[m].extension[i]))
				return (int)m;
		}
	}
	return -1;
}

int getfilelist_null(const char *filename, const char *dir) {
	UNUSED(filename); UNUSED(dir);
	return 0;
}

int setdirectory_null(const char *filename, const char *newdir) {
	UNUSED(filename); UNUSED(newdir);
	return 0;
}
