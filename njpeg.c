
/*

   njpeg.c
   -------

   libjpeg interface


*/

#ifdef INCLUDE_JPEG
#ifndef NATIVE_JPEG

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include <setjmp.h>
#include <string.h>

#include "modules.h"
#include "njpeg.h"

struct my_error_mgr {
	struct jpeg_error_mgr pub;

	jmp_buf setjmp_buffer;
};

struct jpeg_decompress_struct cinfo;
struct my_error_mgr mjerr;
static FILE * infile_jpeg;
static JSAMPARRAY imagedata;
static unsigned int row_stride;
static unsigned char *buffer;
static int error_occured=0;

typedef struct my_error_mgr * my_error_ptr;

static void error_jpeg(j_common_ptr c_info);

static void error_jpeg(j_common_ptr c_info) {
	my_error_ptr myerr = (my_error_ptr) c_info->err;
#ifdef DEBUG
	fprintf(stderr,"jpeg: error while reading file\n");
#endif
	longjmp(myerr->setjmp_buffer, 1);
}

int  open_jpeg(const char *filename,const char *imgdir,
    unsigned int owidth, unsigned int oheight) {

	UNUSED(imgdir);
#ifdef DEBUG
	fprintf(stderr,"jpeg: open_jpeg entered\n");
#endif

	error_occured=0;
	buffer=NULL;

	cinfo.err = jpeg_std_error(&mjerr.pub);
	mjerr.pub.error_exit=error_jpeg;

	if (setjmp(mjerr.setjmp_buffer)) {
		/*    jpeg_destroy_decompress(&cinfo);
		      if (infile_jpeg!=NULL) fclose(infile_jpeg);*/
		error_occured=1;
		return 0;
	}

	infile_jpeg=NULL;
	if ((infile_jpeg = fopen(filename, "rb")) == NULL) {
		return 0;
	}

#ifdef DEBUG
	printf("File opened.\n");
#endif

	jpeg_create_decompress(&cinfo);

#ifdef DEBUG
	printf("jpeg::create decompress.\n");
#endif

	jpeg_stdio_src(&cinfo, infile_jpeg);

#ifdef DEBUG
	printf("jpeg::read header.\n");
#endif

	jpeg_read_header(&cinfo, TRUE);

#ifdef DEBUG
	printf("jpeg::calc output (req: %dx%d).\n",owidth,oheight);
#endif

	jpeg_calc_output_dimensions(&cinfo);
	if (owidth!=0 && oheight!=0) {
		cinfo.scale_num=owidth;
		cinfo.scale_denom=cinfo.output_width;
		cinfo.output_components=3;
	}

#ifdef DEBUG
	printf("jpeg::start decompress.\n");
#endif

	jpeg_start_decompress(&cinfo);

	row_stride = cinfo.output_width * cinfo.output_components;
	buffer=malloc(row_stride*
	    cinfo.output_height);
	imagedata = (*cinfo.mem->alloc_sarray)
	    ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride,1);

#ifdef DEBUG
	printf("Structures allocated.\n");
	printf("Image: (%dx%dx%d).\n",cinfo.output_width,cinfo.output_height,
	    cinfo.output_components);
#endif

	while (cinfo.output_scanline < cinfo.output_height) {

		jpeg_read_scanlines(&cinfo,imagedata,1);
		memcpy(buffer+(cinfo.output_scanline-1)*cinfo.output_width*
		    cinfo.output_components,
		    imagedata[0],cinfo.output_width*cinfo.output_components);
		/*jpeg_read_scanlines(&cinfo,buffer+(cinfo.output_scanline-1)*cinfo.output_width*3,1);*/
	}

#ifdef DEBUG
	printf("All scanlines done\n");
#endif

	jpeg_finish_decompress(&cinfo);
	fclose(infile_jpeg);
	infile_jpeg=NULL;

#ifdef DEBUG
	printf("Finished\n");
#endif

	return 1;
}
void close_jpeg(void) {
#ifdef DEBUG
	fprintf(stderr,"jpeg: close_jpeg entered\n");
#endif

	jpeg_destroy_decompress(&cinfo);
	if (buffer!=NULL) free(buffer);
	buffer=NULL;

	if (infile_jpeg!=NULL) fclose(infile_jpeg);
	infile_jpeg=NULL;
}
unsigned int  getpixel_jpeg(int x, int y, int color) {
	unsigned int val=0;
	unsigned char *p;

	UNUSED(color);
	if (error_occured) return 0x000000;

	if (x<0 || (unsigned int)x>=cinfo.output_width ||
	    y<0 || (unsigned int)y>=cinfo.output_height)
		return 0x0; else {

			p=buffer+(y*cinfo.output_width+x)*cinfo.output_components;

			switch (cinfo.output_components) {
			case 1:
				val=*p;val*=0x010101;
				return val;
			default:
				val=*p++;val<<=8;
				val|=*p++;val<<=8;
				return val|*p;
			}

			/*val=(*(unsigned int *)(buffer+(y*cinfo.output_width+x)*3))&0xFFFFFF;

			  return (val>>16)|(val&0xFF00)|((val&0xFF)<<16);*/
		}
}
int getwidth_jpeg(void) {
	return cinfo.output_width;
}
int getheight_jpeg(void) {
	return cinfo.output_height;
}

#endif
#endif
