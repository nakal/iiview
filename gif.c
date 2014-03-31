
/* gif.c Decodes gif images. */

#include <stdio.h>
#include <stdlib.h>

#include "modules.h"
#include "gif.h"

#define M_TRAILER     0x3B
#define M_IMAGE       0x2C
#define M_EXTENSION   0x21

static int gif_width,gif_height;
static FILE *giffile;
static long int gif_filesize;
static const char *gifmarker="GIF";
static int gif_gctdef=0;
static int gif_resolution=0;
static int gif_colors=0;
static int gif_background=0;
static int gif_aspectratio=0;
static int *gif_pal;
static int gif_palmask;
static unsigned char *gif_data,*data;
static int gif_interlaced=0;
static int bitpos,maxbits;
static unsigned char bitbuf[256];
static int initcodesize,codesize,codemask,gif_ipixel,gif_iindex,
	   gif_ipassnr,gif_ipassinc;

/* prototypes */
void readColormap(int);


static int gif_readword(void)
{
	return (fgetc(giffile)|(fgetc(giffile)<<8))&0xFFFF;
}

void readColormap(int colors)
{
	int i;

	for (i=0;i<colors;i++)
		gif_pal[i]=gif_readword()|(fgetc(giffile)<<16);
}

static int readCode(void)
{
	int bits,size;

	bits=size=0;

	while (size<codesize) {
		if (bitpos>=maxbits) {
			if ((maxbits=fgetc(giffile))==EOF) return -1;
			maxbits<<=3;
			fread(bitbuf,1,(size_t)(maxbits>>3),giffile);
			bitpos=0;
		}
		bits=bits|(((bitbuf[bitpos>>3]>>(bitpos&7))&1)<<size);
		bitpos++;size++;
	}

	return bits & codemask;
}

static void saveColor(int nr)
{
	if (data-gif_data>=gif_width*gif_height) {
		/* printf("Error: data overflow.\n"); */
		return;
	}
	*data++=nr;

	if (gif_interlaced) {
		if ((++gif_ipixel)>=gif_width) {
			gif_ipixel=0;
			gif_iindex+=gif_ipassinc;
			if (gif_iindex>=gif_height) {
				switch (++gif_ipassnr) {
				case 1:
					gif_ipassinc=8;
					gif_iindex=4;
					break;
				case 2:
					gif_ipassinc=4;
					gif_iindex=2;
					break;
				case 3:
					gif_ipassinc=2;
					gif_iindex=1;
					break;
				}
			}
			data=gif_data+gif_iindex*gif_width;
		}
	}
}

static int readImage(void)
{
	int leftoffs,topoffs,info,CC,EOFC,savecode,
	    code,lastcode=0,lastchar=0,curcode,freecode,initfreecode,outcodenr;
	int prefix[4096];
	unsigned char suffix[4096],output[4096];

	leftoffs=gif_readword();topoffs=gif_readword();
	gif_width=gif_readword();gif_height=gif_readword();

	gif_data=(unsigned char *)malloc((size_t)(gif_width*gif_height));

	if (!gif_data) {
		printf("Out of memory.\n");
		fclose(giffile);giffile=NULL;
		return 1;
	}

	info=fgetc(giffile);
	gif_interlaced=info&0x40;
	if (info&0x80) readColormap((int)1 <<((info&7)+1));

	/*  if (leftoffs||topoffs) {
	    printf("Warning: image offset (%i, %i)\n",leftoffs,topoffs);
	    return 1;
	    }*/

	data=gif_data;
	initcodesize=fgetc(giffile);
	bitpos=maxbits=0;
	CC=1<<initcodesize;
	EOFC=CC+1;
	freecode=initfreecode=CC+2;
	codesize=++initcodesize;
	codemask=(1<<codesize)-1;
	gif_iindex=gif_ipassnr=0;
	gif_ipassinc=8;
	gif_ipixel=0;

	if ((code=readCode())>=0)
		while (code!=EOFC) {
			if (code==CC) {
				codesize=initcodesize;
				codemask=(1<<codesize)-1;
				freecode=initfreecode;
				lastcode=lastchar=code=readCode();
				if (code<0) break;
				saveColor(code);
			} else {
				outcodenr=0;savecode=code;
				if (code>=freecode) {
					if (code>freecode) break;
					output[(outcodenr++) & 0xFFF]=lastchar;
					code=lastcode;
				}
				curcode=code;

				while (curcode>gif_palmask) {
					if (outcodenr>4095) break;
					output[(outcodenr++)&0xFFF]=suffix[curcode];
					curcode=prefix[curcode];
				}

				output[(outcodenr++)&0xFFF]=lastchar=curcode;

				while (outcodenr>0)
					saveColor(output[--outcodenr]);

				prefix[freecode]=lastcode;
				suffix[freecode++]=lastchar;
				lastcode=savecode;

				if (freecode>codemask) {
					if (codesize<12) {
						codesize++;
						codemask=(codemask<<1)|1;
					}
				}
			}
			if ((code=readCode())<0) break;
		}
	return 0;
}

/* output size will be ignored */
int GIF_OpenFile(const char *filename,const char *imgdir,
    unsigned int owidth, unsigned int oheight)
{
	int i,marker;

	UNUSED(imgdir); UNUSED(owidth); UNUSED(oheight);

	gif_data=NULL;
	gif_pal=NULL;
	gif_width=gif_height=0;

	giffile=NULL;
	giffile=fopen(filename,"rb");
	if (!giffile) return 0;

	fseek(giffile,0,SEEK_END);
	gif_filesize=ftell(giffile);
	rewind(giffile);

	if (gif_filesize<13) {
		fclose(giffile);giffile=NULL;
		return 0;
	}

	for (i=0;i<3;i++)
		if (fgetc(giffile)!=gifmarker[i]) {
			fclose(giffile);giffile=NULL;
			return 0;
		}

	/* skip version info for now */
	for (i=0;i<3;i++)
		fgetc(giffile);

	gif_width=gif_readword();
	gif_height=gif_readword();

	i=fgetc(giffile);
	gif_gctdef=i&0x80;
	gif_resolution=((i>>4)&7)+1;
	gif_colors=(int)1 << ((i&7)+1);
	gif_palmask=gif_colors-1;

	gif_background=fgetc(giffile);
	gif_aspectratio=fgetc(giffile);

	if (gif_aspectratio) {
		printf("Hmm: ASPECT RATIO given ! What to do ?\n");
		fclose(giffile);giffile=NULL;
		return 0;
	}

	gif_pal=malloc(256*sizeof(int));
	if (gif_gctdef) readColormap(gif_colors);

	do {
		if ((marker=fgetc(giffile))==EOF) break;

		switch (marker) {
		case M_IMAGE:
			readImage();fgetc(giffile);
			break;
		case M_EXTENSION:
			fgetc(giffile);
			i=fgetc(giffile);
			while (i) {
				while (i--) fgetc(giffile);
				i=fgetc(giffile);
			}
			break;
		case M_TRAILER:
			break;
		default:
			i=fgetc(giffile);
			while (i) {
				while (i--) fgetc(giffile);
				i=fgetc(giffile);
			}
			break;
		}
	} while (marker!=M_TRAILER);

	fclose(giffile);giffile=NULL;
	return 1;
}

void GIF_CloseFile(void)
{
	if (gif_pal!=NULL) free(gif_pal);
	if (gif_data!=NULL) free(gif_data);

	if (giffile!=NULL) fclose(giffile);
	giffile=NULL;
}

unsigned int GIF_GetPixel(int x,int y,int col)
{
	int color;

	if ((x>=gif_width)||(y>=gif_height))
		return 0x000000;

	color=gif_pal[gif_data[y*gif_width+x]];

	return col ? (color&0xFF00)|((color>>16)&0xFF)|((color&0xFF)<<16) :
	    (((color&0xFF)+((color>>8)&0xFF)+((color>>16)&0xFF))/3)*0x010101;
}

int GIF_GetWidth(void)
{
	return gif_width;
}

int GIF_GetHeight(void)
{
	return gif_height;
}
