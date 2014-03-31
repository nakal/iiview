
/* PNG file module for IIViewer. */

#ifdef INCLUDE_PNG

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <png.h>

#include "modules.h"
#include "png.h"

static FILE *pngfile;
static int readsuccessful;
static int width,height;

static png_bytep *row_pointers;

int PNG_OpenFile(const char *filename,const char *imgdir,
    unsigned int owidth, unsigned int oheight) {

	unsigned char testbuf[8];
	png_structp png_ptr;
	png_infop info_ptr;
	int i,rowbytes,color_type,bit_depth;

	UNUSED(imgdir); UNUSED(owidth); UNUSED(oheight);

	readsuccessful=0;
	row_pointers=NULL;

	if (filename==NULL) {
#ifdef DEBUG
		fprintf(stderr,"PNG: Filename is NULL\n");
#endif
		return 0;
	}

	pngfile=NULL;
	if ((pngfile=fopen(filename,"r"))==NULL) {
#ifdef DEBUG
		fprintf(stderr,"PNG: File '%s' not found (in: '%s').\n",filename,
		    getcwd(NULL,256));
#endif
		return 0;
	}

	fread(testbuf,1,4,pngfile);
	if (png_sig_cmp(testbuf,0,4)) {
#ifdef DEBUG
		fprintf(stderr,"PNG: header not valid.\n");
#endif
		return 0;
	}

	png_ptr = png_create_read_struct
	    (PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if (!png_ptr) {
		fprintf(stderr,"PNG: png_create_read_struct failed.\n");
		return 0;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr,
		    (png_infopp)NULL, (png_infopp)NULL);
		fprintf(stderr,"PNG: png_create_info_struct failed.\n");
		return 0;
	}

	png_init_io(png_ptr,pngfile);
	png_set_sig_bytes(png_ptr,4);

	png_read_info(png_ptr, info_ptr);
	if ((width=png_get_image_width(png_ptr,info_ptr))==0) {
		fprintf(stderr,"PNG: Image width needed.\n");
		return 0;
	}
	if ((height=png_get_image_height(png_ptr,info_ptr))==0) {
		fprintf(stderr,"PNG: Image width needed.\n");
		return 0;
	}

	if ((bit_depth=png_get_bit_depth(png_ptr,info_ptr))==16) {
		png_set_strip_16(png_ptr);
	}

	color_type=png_get_color_type(png_ptr,info_ptr);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);

	if (color_type == PNG_COLOR_TYPE_GRAY ||
	    color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	if (color_type & PNG_COLOR_MASK_ALPHA)
		png_set_strip_alpha(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	rowbytes = png_get_rowbytes(png_ptr, info_ptr);

#ifdef DEBUG
	fprintf(stderr,"PNG: Allocating memory (image size: %dx%d) (rowbytes: %d).\n",
	    width,height,rowbytes);
#endif

	if ((row_pointers=malloc(sizeof(png_bytep)*height))==NULL) {
		fprintf(stderr,"PNG: Out of memory\n");
		return 0;
	}
	for (i=0;i<height;i++) {
		if ((row_pointers[i]=malloc((unsigned int)rowbytes))==NULL) {
			fprintf(stderr,"PNG: Out of memory\n");
			return 0;
		}
	}

#ifdef DEBUG
	fprintf(stderr,"PNG: Allocation done, reading image...\n");
#endif

	png_read_image(png_ptr, row_pointers);

#ifdef DEBUG
	fprintf(stderr,"PNG: Completed.\n");
#endif

	readsuccessful=1;

	fclose(pngfile);
	pngfile=NULL;
	return 1;
}

void PNG_CloseFile(void) {

	int i;

	if (row_pointers!=NULL) {
		for (i=0;i<height;i++) {
			if (row_pointers[i]!=NULL) free(row_pointers[i]);
		}

		free(row_pointers);
	}

	if (pngfile!=NULL) fclose(pngfile);
	pngfile=NULL;
}

unsigned int PNG_GetPixel(int x,int y,int color) {

	unsigned char *p;

	UNUSED(color);

	if (!readsuccessful || x<0 || x>=width || y<0 || y>=height) {
		return 0x000000;
	}

	p=(unsigned char *)row_pointers[y]+x*3;

	return ((int)*p<<16)|((int)*(p+1)<<8)|(*(p+2));
}

int PNG_GetWidth(void) {
	return width;
}

int PNG_GetHeight(void) {
	return height;
}

#endif
