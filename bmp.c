
/* Decodes bmp images. */

#include <stdio.h>
#include <stdlib.h>

#include "modules.h"
#include "bmp.h"

#define BMP_BI_UNCOMPRESSED	0
#define BMP_BI_BITFIELD		3

static FILE *bmpfile;
static int bitsperpixel,bytesinline, width,height;
static RGBQuad *bmppalette;
static unsigned char *bmpdata;
static unsigned int redmask, greenmask, bluemask;
static unsigned int redshift, greenshift, blueshift;

/* prototypes */
static unsigned char BMP_Clamp(double);
static unsigned int BMP_ToBW(unsigned int);
static unsigned short int BMP_ReadWord(void);
static unsigned int BMP_ReadLong(void);
static unsigned int BMP_ShiftCal(unsigned int mask);

static unsigned short int BMP_ReadWord(void)
{
	return fgetc(bmpfile)|(fgetc(bmpfile)<<8);
}
static unsigned int BMP_ReadLong(void)
{
	return BMP_ReadWord()|((unsigned int)BMP_ReadWord()<<16);
}

static unsigned int BMP_ShiftCal(unsigned int mask) {

	unsigned int shift=0;

	if (mask==0) return 0;

	if (mask>255) {
		while (mask>255) {
			mask >>= 1;
			shift++;
		}
	} else {
		while ((mask&128)==0) {
			mask <<= 1;
			shift++;
		}
	}

	return shift;
}

/* output size will be ignored */
int BMP_OpenFile(const char *filename, const char *imgdir,
    unsigned int owidth, unsigned int oheight)
{
	BitmapFileHeader bfh;
	BitmapInfo bi;
	int palettecolors,cnr,arraysize;

	UNUSED(imgdir); UNUSED(owidth); UNUSED(oheight);

	width=height=0;
	bmpfile=NULL;
	bmppalette=NULL;
	if ((bmpfile=fopen(filename,"rb"))==NULL) return 0;

	if ((bfh.bfType=BMP_ReadWord())!=BMP_SIGNATURE) {
		fclose(bmpfile);
		return 0;
	}
	bfh.bfSize=BMP_ReadLong();
	bfh.bfReserved[0]=BMP_ReadWord();
	bfh.bfReserved[1]=BMP_ReadWord();
	bfh.bfOffBits=BMP_ReadLong();

#ifdef VERBOSE
	fprintf(stderr,"bfOffBits: %d\n",bfh.bfOffBits);
#endif

	palettecolors=(bfh.bfOffBits-54)/4;
	/* minus sizeof(BitmapFileHeader) minus sizeof(BitmapInfoHeader) */

	bi.bmiHeader.biSize=BMP_ReadLong();
	width=bi.bmiHeader.biWidth=BMP_ReadLong();
	height=bi.bmiHeader.biHeight=BMP_ReadLong();
	bi.bmiHeader.biPlanes=BMP_ReadWord();
	bi.bmiHeader.biBitCount=BMP_ReadWord();
	bitsperpixel=bi.bmiHeader.biPlanes*bi.bmiHeader.biBitCount;

	if ((bi.bmiHeader.biCompression=BMP_ReadLong())!=0) {

		switch (bi.bmiHeader.biCompression) {
		case BMP_BI_BITFIELD:
			break;
		default:
#ifdef VERBOSE
			fprintf(stderr,"Compression not supported (compression type: %u).\n", bi.bmiHeader.biCompression);
#endif
			return 0;
		}
	}

	switch (bitsperpixel) {
	case 1:
	case 4:
	case 8:
	case 24:
		break;
	case 16:
		if (bi.bmiHeader.biCompression==0) {
			redmask=0x7c00; redshift=7;
			greenmask=0x3e0; greenshift=2;
			bluemask=0x1f; blueshift=3;
		} else if (bi.bmiHeader.biCompression!=3) return 0;
		break;
	case 32:
		if (bi.bmiHeader.biCompression==0) {
			redmask=0xff0000; redshift=16;
			greenmask=0xff00; greenshift=8;
			bluemask=0xff; blueshift=0;
		} else if (bi.bmiHeader.biCompression!=3) return 0;
		break;
	default:
		fclose(bmpfile);
		return 0;
	}

#ifdef VERBOSE
	fprintf(stderr,"Bits per pixel: %i.\n", bitsperpixel);
#endif
	bi.bmiHeader.biSizeImage=BMP_ReadLong();
	bi.bmiHeader.biXPelsPerMeter=BMP_ReadLong();
	bi.bmiHeader.biYPelsPerMeter=BMP_ReadLong();
	bi.bmiHeader.biClrUsed=BMP_ReadLong();
	bi.bmiHeader.biClrImportant=BMP_ReadLong();

	if (palettecolors) {
		if ((bmppalette=(RGBQuad *)malloc(palettecolors*sizeof(RGBQuad)))==NULL) {
			fclose(bmpfile);
#ifdef VERBOSE
			fprintf(stderr,"Palette alloc failed.\n");
#endif
			return 0;
		}

#ifdef VERBOSE
		fprintf(stderr,"Palette has %u color entries.\n", palettecolors);
#endif
		switch (bi.bmiHeader.biCompression) {
		case BMP_BI_UNCOMPRESSED:
			for (cnr=0;cnr<palettecolors;cnr++) {
				bmppalette[cnr]=fgetc(bmpfile)|((long int)fgetc(bmpfile)<<8)|
				    ((long int)fgetc(bmpfile)<<16);
				fgetc(bmpfile);
			}
			break;
		case BMP_BI_BITFIELD:
			redmask = BMP_ReadLong();
			greenmask = BMP_ReadLong();
			bluemask = BMP_ReadLong();
			redshift = BMP_ShiftCal(redmask);
			greenshift = BMP_ShiftCal(greenmask);
			blueshift = BMP_ShiftCal(bluemask);
#ifdef VERBOSE
			fprintf(stderr, "Bitfield masks: %08X, %08X, %08X\n",
			    redmask, greenmask, bluemask);
			fprintf(stderr, "Bitfield shifts: %u, %u, %u\n",
			    redshift, greenshift, blueshift);
#endif
			break;
		}
	} else bmppalette=NULL;

	bytesinline=width*bitsperpixel/8;
	while (bytesinline&3) bytesinline++;

	if (fseek(bmpfile, bfh.bfOffBits, SEEK_SET)!=0) return 0;
	arraysize=bfh.bfSize-bfh.bfOffBits;
	if ((bmpdata=(unsigned char *)malloc((size_t)arraysize))==NULL) {
		free(bmppalette);
#ifdef VERBOSE
		fprintf(stderr,"Bitmap alloc failed.\n");
#endif
		return 0;
	}

#ifdef VERBOSE
	fprintf(stderr,"Data offset: %ld\n",ftell(bmpfile));
	fprintf(stderr,"Data size/loaded: %d/%d\n",height*bytesinline,arraysize);
#endif

	fread(bmpdata,1,(size_t)arraysize,bmpfile);
	return 1;
}

void BMP_CloseFile(void)
{
	if (bmppalette!=NULL) free(bmppalette);
	if (bmpdata!=NULL) free(bmpdata);

	if (bmpfile!=NULL) fclose(bmpfile);
	bmpfile=NULL;
	bmppalette=NULL;
	bmpdata=NULL;
}

unsigned char BMP_Clamp(double val)
{
	if (val<0) val=0;
	if (val>255) val=255;
	return (unsigned char)val;
}

unsigned int BMP_ToBW(unsigned int color)
{
	unsigned char y;

	y=BMP_Clamp(((color>>16)&0xFF)*0.333+((color>>8)&0xFF)*0.586+
	    (color&0xFF)*0.114);

	return (y<<16)|(y<<8)|y;
}

unsigned int BMP_GetPixel(int x,int y,int col)
{
	unsigned char *offs;
	unsigned int color;
	unsigned int colorquad;

	if ((x<0)||(x>=width)||(y<0)||(y>=height)) return 0x000000;

	offs=bmpdata+(height-y-1)*bytesinline; /* BMP from bottom */

	switch (bitsperpixel) {
	case 1:
		color=bmppalette[(offs[x/8]>>(7-(x&7)))&1];
		break;
	case 4:
		color=bmppalette[(offs[x/2]>>(x&1 ? 0 : 4))&0x0F];
		break;
	case 8:
		color=bmppalette[offs[x]];
		break;
	case 16:
		colorquad=offs[x*2]|(offs[x*2+1]<<8);
		color = ((colorquad & redmask)>>redshift)<<16 |
		    ((colorquad & greenmask)>>greenshift)<<8 |
		    ((colorquad & bluemask)<<blueshift);
		break;
	case 24:
		color=offs[x*3]|(offs[x*3+1]<<8)|(offs[x*3+2]<<16);
		break;
	case 32:
		colorquad=offs[x*4]|(offs[x*4+1]<<8)|(offs[x*4+2]<<16)|(offs[x*4+3]<<24);
		color = ((colorquad & redmask)>>redshift)<<16 |
		    ((colorquad & greenmask)>>greenshift)<<8 |
		    ((colorquad & bluemask)>>blueshift);
		break;
	default:
		return 0;
	}

	if (col) return color;
	else return BMP_ToBW(color);
}

size_t BMP_GetWidth(void)
{
	return width;
}

size_t BMP_GetHeight(void)
{
	return height;
}
