
/* Decodes tiff images. */

#ifdef INCLUDE_TIFF

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>

#include "modules.h"
#include "common.h"
#include "filelist.h"
#include "tif.h"

static TIFF* tif_file;
static uint32_t* tif_raster = NULL;
static unsigned int tif_width=0;
static unsigned int tif_height=0;

int TIF_OpenFile(const char *filename,const char *imgdir,
    unsigned int width, unsigned int height) {

	size_t npixels;

	UNUSED(width); UNUSED(height);

#ifdef VERBOSE
	fprintf(stderr,"%s:%s\n",filename,imgdir);
#endif

	tif_file=NULL;
	tif_raster=NULL;
	tif_file=TIFFOpen(filename,"r");

	if (imgdir!=NULL && imgdir[0]!=0) {
		int diridx;

		diridx=atoi(imgdir);
		/*    while (i-->0) {
		      TIFFReadDirectory(tif_file);
		      }*/
		if (TIFFSetDirectory(tif_file, (tdir_t)diridx)) {
#ifdef VERBOSE
			printf("OK set\n");
#endif
		}
	}

	if (tif_file) {

		TIFFGetField(tif_file, TIFFTAG_IMAGEWIDTH, &tif_width);
		TIFFGetField(tif_file, TIFFTAG_IMAGELENGTH, &tif_height);
		npixels = tif_width * tif_height;
		tif_raster = _TIFFmalloc((int)(npixels * sizeof (uint32_t)));
		if (tif_raster != NULL) {
			if (!TIFFReadRGBAImage(tif_file,tif_width,tif_height,tif_raster, 0)) {
#ifdef VERBOSE
				printf("Couldn't read raster-data.\n");
#endif
				return 0;
			}
		} else {
#ifdef VERBOSE
			printf("Couldn't get raster-data.\n");
#endif
			return 0;
		}
	} else {
#ifdef VERBOSE
		printf("Couldn't open.\n");
#endif
		return 0;
	}
	return 1;
}

void TIF_CloseFile(void) {
	if (tif_raster) _TIFFfree(tif_raster);
	if (tif_file) TIFFClose(tif_file);
	tif_file=NULL;
}

unsigned int TIF_GetPixel(int x,int y,int col) {
	int color;

	if (x<0 || ((unsigned int)x>=tif_width) ||
	    y<0 || ((unsigned int)y>=tif_height))
		return 0x000000;

	color=tif_raster[(tif_height-y-1)*tif_width+x];

	return col ? (color&0xFF00)|((color>>16)&0xFF)|((color&0xFF)<<16) :
	    ((color&0xFF)+((color>>8)&0xFF)+(color>>16))*0x010101;

}

size_t TIF_GetWidth(void) {
	return tif_width;
}

size_t TIF_GetHeight(void) {
	return tif_height;
}

int TIF_SetDirectory(const char *filename, const char *newdir) {

	int retval=0;

#ifdef VERBOSE
	printf("Set directory in %s to %s\n",filename,newdir);
#endif

	tif_file=TIFFOpen(filename,"r");

	if (newdir==NULL || strlen(newdir)<=0) {
		retval=TIFFReadDirectory(tif_file);
	} else {
		retval=0;
	}

	if (tif_file) TIFFClose(tif_file);
	tif_file=NULL;

	dir_int=NULL;

#ifdef VERBOSE
	printf("File contains directories: %s\n", retval>0 ? "yes" : "no");
#endif

	return retval;
}

int TIF_GetFileList(const char *filename, const char *dir) {

	int retval=0;
	char name[13];

#ifdef VERBOSE
	printf("TIF_GetFileList: %s %s\n",filename,dir);
#endif

	tif_file=TIFFOpen(filename,"r");

	if (dir==NULL) {
		delete_all(&file);
		insert_file(&file,"/..");
		retval=1;

		do {
			snprintf(name,sizeof(name), "%08d",TIFFCurrentDirectory(tif_file));
			insert_file(&file,name);
			retval++;
#ifdef VERBOSE
			TIFFPrintDirectory(tif_file,stderr,TIFFPRINT_NONE);
#endif

		} while (TIFFReadDirectory(tif_file));

	} else {
		retval=-1;
	}

	if (tif_file) TIFFClose(tif_file);
	tif_file=NULL;

	dir_int=NULL;

#ifdef VERBOSE
	printf("#image directories: %d\n", retval);
#endif

	return retval;
}

#endif
