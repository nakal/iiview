/* Own implementation of JPEG API (deprecated), but
 * I keep it because for historical reasons that I
 * implemented it once, only having a vague description
 * how the format works. */

#ifdef INCLUDE_JPEG
#ifdef NATIVE_JPEG

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "jpeg.h"



char JPEG_LASTERROR[80];                      /* contains the
						 last error msg */

typedef float JPEG_FLOATTYPE;                 /* the float type
						 used in the JPEG-
						 class */

FILE* JPEG_FILE;			      /* file handle */

unsigned long int JPEG_FILELEN;               /* file size */

unsigned long int JPEG_FILEPOS;		      /* current JPEG-file
						 position */

int JPEG_BITPOS;			      /* bit which is being
						 handled */

ushort JPEG_FRAMEHEADERTYPE;		      /* header-type for
						 future use */

uchar *QuantisationTable[4];                  /* QuantisationTable needs
						 only 1 byte per value (!) */

ushort JPEG_RESTARTINT,JPEG_RESTART;          /* for restart
						 intervals */

ushort JPEG_COMPSIZE;			      /* (YUV) component
						 size */

ushort JPEG_XCOMPCOUNT,JPEG_YCOMPCOUNT;       /* components count
						 horiz./vert. */

int JPEG_DIFF[4];		              /* Last DC value for
						 a Y/U/V-component */

ushort JPEG_DECBYTE;			      /* current byte which is
						 being decoded */

JPEG_FLOATTYPE CosMatrix[8][8][8][8];	      /* stored IDCT matrix */

ushort JPEG_COMPOFS[4];                       /* offset of a Y/U/V-
						 component in the
						 buffer-array */

uint JPEG_RESTARTINDEX;                       /* holds the index of the next
						 restart marker 
						 (error recovery) */

uint JPEG_MARKER;                             /* stores the current marker */

short int *JPEG_BUFFER;                       /* image buffer
						 will be VERY large
						 in some cases */

int COMP_ASSIGN[256];                         /* component assignment
						 array, because of
						 some stupid programs
						 which write big
						 numbers as component ids */

/* JPEG-markers */

#define JM_SOF        0xC0	   /* 0xC0-0xCF */
#define JM_DHT        0xC4
#define	JM_RST        0xD0	   /* 0xD0-0xD7 */
#define	JM_SOI        0xD8
#define	JM_EOI        0xD9
#define	JM_SOS        0xDA
#define	JM_DQT        0xDB
#define	JM_DNL        0xDC	   /* not implemented yet, perhaps never */
#define	JM_DRI        0xDD


/* the zigzag sequence */

const uchar ZIGZAG[]=
{
	0, 1, 8,16, 9, 2, 3,10,
	17,24,32,25,18,11, 4, 5,
	12,19,26,33,40,48,41,34,
	27,20,13, 6, 7,14,21,28,
	35,42,49,56,57,50,43,36,
	29,22,15,23,30,37,44,51,
	58,59,52,45,38,31,39,46,
	53,60,61,54,47,55,62,63
};


/* category values */

const unsigned int Category[]=
{
	0,1,2,4,8,16,32,64,128,256,512,1024,2048
};


/* category masks */

const unsigned int CompCategory[]=
{
	0,1,3,7,15,31,63,127,255,511,1023,2047
};


/* JPEG-structs: */

/* Frameheader */
typedef struct
{
	unsigned char		P;
	ushort			Y,X;
	unsigned char		Nf;
	struct
	{
		unsigned char C;
		unsigned V:4;
		unsigned H:4;
		unsigned char Tq;
	} Csp[4];
} jpeg_frameheader;

jpeg_frameheader FrameHeader;            /* holds data for the frame */

/* Scanheader */
struct                          /* holds data for the current scan */
{
	unsigned char Ns;
	struct
	{
		unsigned char Cs;
		unsigned Ta:4;
		unsigned Td:4;
	} Csp[4];
	unsigned char Ss;
	unsigned char Se;
	unsigned Al:4;
	unsigned Ah:4;
} ScanHeader;


/* Huffman node */
struct AHuffmanNode    /* a huffman-tree element (leaf or branch) */
{
	struct AHuffmanNode* Son[2];
	int	      byte;
};
typedef struct AHuffmanNode HuffmanNode;

/* Huffman decoder trees */
HuffmanNode *HuffmanTable[2][4]; /* the roots of each huffman-tree */

/* MCU */
typedef short int MCU[8][8];      /* the smallest unit in the JPEG-format */


/* last pixel will be cached, because we often
   want to magnify images (simple speedup) */
struct
{
	ushort x,y;
	uint value;
} JPEG_LASTPIXEL;

/* number of end of band runs
   used for progressive methods */
unsigned int JPEG_EOBCOUNT;

/* macros */
#define MakeWordLittleEndian(w) {w=(w >> 8)|(w << 8);}  /* big-endian to
							   little-endian */



/* functions declarations */

/* jpeg-module initialisation;
   must be called (once) at the beginning
   */
int JPEG_INIT(void)
{
	uchar x,y,u,v;
	JPEG_FLOATTYPE cu,cv;

	JPEG_LASTERROR[0]=0;

	/* defining the four-dimensional IDCT-matrix
	   will be a "little bit" faster, when you don't
	   use cos in main programs.
	   Of course, I'm too stupid to implement faster
	   algorithms, so feel free to tell me about them.
	   */

	for (y=0;y<8;y++) for (x=0;x<8;x++)
		for (v=0;v<8;v++) for (u=0;u<8;u++)
		{
			if (u==0) cu=1/sqrt(2); else cu=1;
			if (v==0) cv=1/sqrt(2); else cv=1;

			CosMatrix[y][x][v][u]=cu*cv*
			    cos(M_PI*u*(y*2+1)/16)*cos(M_PI*v*(x*2+1)/16)/4;
		}

#ifdef JPEG_VERBOSE
	printf("JPEG: Init successful.\n");
#endif
	return 1;
}

/* recursively delete the huffman tree */
void FreeHuffmanTree(HuffmanNode *hn)
{
	if (hn==NULL) return;

	if ((*hn).Son[0]!=NULL) FreeHuffmanTree((*hn).Son[0]);
	if ((*hn).Son[1]!=NULL) FreeHuffmanTree((*hn).Son[1]);

	if (hn!=NULL) free(hn);
	hn=NULL;

}

/* tidy up routine */
void JPEG_CloseFile(void)
{
	int i,j;

#ifdef JPEG_VERBOSE
	printf("JPEG_Close reached.\n");
#endif

	/* close file, if open */
	if (JPEG_FILE!=NULL) fclose(JPEG_FILE);
	JPEG_FILE=NULL;

	/* free image buffer */
	if (JPEG_BUFFER!=NULL) free(JPEG_BUFFER);

	/* free quantisation-tables and huffman-trees */
	for (i=0;i<4;i++)
	{
		if (QuantisationTable[i]!=NULL) free(QuantisationTable[i]);
		for (j=0;j<2;j++)
			if (HuffmanTable[j][i]!=NULL) FreeHuffmanTree(HuffmanTable[j][i]);
	}

	/* set variables, should be zero when not used */
	JPEG_FILE=NULL;JPEG_BUFFER=NULL;
	memset(QuantisationTable,0,sizeof(QuantisationTable));
	memset(HuffmanTable,0,sizeof(HuffmanTable));
}

void OutOfMem(void)
{
	/* a function to set an "out of mem"
	   (saves memory) */

	strcpy(JPEG_LASTERROR,"Out of memory occured.");
}

/* scan for markers:

   scan for a 0xFF-byte, then if found
   scan for a non-0xFF-byte
   if it is a 0x00, which can happen at the end of
   a marker segment skip it and repeat the steps

   when an error occurs (eof) return JM_EOI
   end of image
   */
ushort GetMarker(void)
{
	JPEG_BITPOS=7;
	do {
		while ((JPEG_DECBYTE!=0xFF) && (!feof(JPEG_FILE))) {
			JPEG_FILEPOS++;
			JPEG_DECBYTE=fgetc(JPEG_FILE);
		}
		while ((JPEG_DECBYTE==0xFF) && (!feof(JPEG_FILE))) {
			JPEG_FILEPOS++;
			JPEG_DECBYTE=fgetc(JPEG_FILE);
		}
		if (feof(JPEG_FILE)) return JM_EOI;
	}
	while (JPEG_DECBYTE==0x00);

	return JPEG_DECBYTE;
}

/* reads a big endian word */
ushort ReadWord(void)
{
	return ((ushort)fgetc(JPEG_FILE) << 8)|
	    fgetc(JPEG_FILE);
}

/* reads the segment-lenght which is
   stored in big-endian format */
ushort ReadSegmentLength(void)
{
	JPEG_FILEPOS+=2;
	return ReadWord()-2;
}

/* reads in the parameters for the
   whole frame */
int DefineFrameHeader(void)
{
	ushort Length,t;
	int JPEG_IMAGESIZE,i;

	/* read in the frameheader structure */
	memset(&FrameHeader,0,sizeof(FrameHeader));
	for (i=0;i<256;i++) COMP_ASSIGN[i]=-1;
	Length=ReadSegmentLength();
	JPEG_FILEPOS+=Length;
	Length-=6;

	FrameHeader.P=fgetc(JPEG_FILE);
	FrameHeader.Y=ReadWord();
	FrameHeader.X=ReadWord();
	FrameHeader.Nf=fgetc(JPEG_FILE);
	if (FrameHeader.Nf*3!=Length) {
		strcpy(JPEG_LASTERROR,"Unknown frameheader format.");
		return 0;
	}

	for (i=0;i<FrameHeader.Nf;i++) {
		FrameHeader.Csp[i].C=i+1;
		COMP_ASSIGN[fgetc(JPEG_FILE)]=i+1;
		t=fgetc(JPEG_FILE);
		FrameHeader.Csp[i].H= t >> 4;
		FrameHeader.Csp[i].V= t & 0x0F;
		FrameHeader.Csp[i].Tq=fgetc(JPEG_FILE);
	}

#ifdef JPEG_VERBOSE
	printf("Frameheader: P:%i, Y:%i, X:%i, Nf:%i\n",
	    FrameHeader.P,FrameHeader.Y,FrameHeader.X,FrameHeader.Nf);
	for (i=0;i<FrameHeader.Nf;i++)
	{
		printf("             %i: C:%i, H:%i, V:%i, Tq:%i\n",
		    i,FrameHeader.Csp[i].C,FrameHeader.Csp[i].H,
		    FrameHeader.Csp[i].V,FrameHeader.Csp[i].Tq);
	}
#endif

	/* analyse */
	if ((FrameHeader.Nf>3)||(FrameHeader.Nf==0)) {
		strcpy(JPEG_LASTERROR,"Unknown frameheader format");
		return 0;
	}

	/* calculate the offsets of the YUV-components */
	JPEG_COMPSIZE=0;
	for (i=0;i<FrameHeader.Nf;i++) {
		JPEG_COMPOFS[FrameHeader.Csp[i].C-1]=JPEG_COMPSIZE;
		JPEG_COMPSIZE+=(ushort)FrameHeader.Csp[i].H*FrameHeader.Csp[i].V*64;
	}

	/* calculatations for interleaved images */
	JPEG_XCOMPCOUNT=((FrameHeader.X-1)/(FrameHeader.Csp[0].H*8))+1;
	JPEG_YCOMPCOUNT=((FrameHeader.Y-1)/(FrameHeader.Csp[0].V*8))+1;

	/* the size of the image (in bytes) */
	JPEG_IMAGESIZE=(int)JPEG_XCOMPCOUNT*JPEG_YCOMPCOUNT*
	    JPEG_COMPSIZE*2;

#ifdef JPEG_VERBOSE
	printf("Trying to allocate %i bytes ...\n",JPEG_IMAGESIZE);
#endif

	/* try the allocation of the VVVEEERRRYYY big array */

	JPEG_BUFFER=(short int *)malloc(JPEG_IMAGESIZE);
	if (JPEG_BUFFER==NULL) {

#ifdef JPEG_VERBOSE
		printf("FAILED.\n");
#endif

		sprintf(JPEG_LASTERROR,
		    "Out of memory. Needed %i bytes.",
		    JPEG_IMAGESIZE);
		return 0;
	}

#ifdef JPEG_VERBOSE
	printf("OK.\n");
#endif

	/* zero-init the image
	   this is needed for progressive modes */
	memset(JPEG_BUFFER,0,JPEG_IMAGESIZE);

#ifdef JPEG_VERBOSE
	printf("SOFn marker complete.\n");
#endif

	return 1;
}

/* reads in the parameters for the current scan
   image data follows this structure ! */
int DefineScanHeader(void)
{
	ushort Length,i,u,cn;

	memset(&ScanHeader,0,sizeof(ScanHeader));
	Length=ReadSegmentLength();

	ScanHeader.Ns=fgetc(JPEG_FILE);
	if ((ScanHeader.Ns*2+4)!=Length) {
		strcpy(JPEG_LASTERROR,"Unknown scanheader format");
		return 0;
	}

	/* fetch the component data (with component assignment) */

	for (i=0;i<ScanHeader.Ns;i++) {
		cn=fgetc(JPEG_FILE);
		if (COMP_ASSIGN[cn]<0) {
			strcpy(JPEG_LASTERROR,"Illegal component id");
			return 0;
		}
		ScanHeader.Csp[i].Cs=COMP_ASSIGN[cn];
		u=fgetc(JPEG_FILE);
		ScanHeader.Csp[i].Td=u >> 4;
		ScanHeader.Csp[i].Ta=u & 0x0F;
	}

	ScanHeader.Ss=fgetc(JPEG_FILE);
	ScanHeader.Se=fgetc(JPEG_FILE);
	u=fgetc(JPEG_FILE);
	ScanHeader.Ah=u >> 4;
	ScanHeader.Al=u & 0x0F;

#ifdef JPEG_VERBOSE
	printf("Scanheader:\n");
	printf("Ns: %i\n",ScanHeader.Ns);
	for (i=0;i<ScanHeader.Ns;i++)
	{
		printf("%i: Cs: %i, Td: %i, Ta: %i\n",i,
		    ScanHeader.Csp[i].Cs,
		    ScanHeader.Csp[i].Td,
		    ScanHeader.Csp[i].Ta);
	}
	printf("Ss: %i, Se: %i, Ah:%i, Al:%i\n",
	    ScanHeader.Ss,ScanHeader.Se,
	    ScanHeader.Ah,ScanHeader.Al);
#endif

	JPEG_FILEPOS+=Length;
	return 1;
}

/* reads quantisation tables, which
   are often stored as an array */
int DefineQuantisationTable(void)
{
	ushort Length,PqTq,i;

	if (!JPEG_FILE) return 0;

	Length=ReadSegmentLength();
	while (Length) {
		PqTq=fgetc(JPEG_FILE);

#ifdef JPEG_VERBOSE
		printf("Quantisation-table: Pq: %i, Tq: %i\n",PqTq>>4,PqTq & 0x0F);
#endif  

		if (PqTq >> 4) {
			strcpy(JPEG_LASTERROR,"Unknown quantisation table format.");
			return 0;
		}
		if (!QuantisationTable[PqTq & 0x0F])
			QuantisationTable[PqTq & 0x0F]=(uchar *)malloc(64);

		if (QuantisationTable[PqTq & 0x0F]==0) {
			OutOfMem();
			return 0;
		}
		for (i=0;i<64;i++)
			QuantisationTable[PqTq & 0x0F][i]=fgetc(JPEG_FILE);

		Length-=65;
		JPEG_FILEPOS+=65;
	}
	return 1;
}

/* reads in the huffman table
   table definition for a scan, which
   can be stored as an array */
int DefineHuffmanTable(void)
{
	ushort Length,TcTh;
	uchar L[17];
	uchar i,j,Index;
	HuffmanNode *Root,*P;
	unsigned long BitR;

	Length=ReadSegmentLength();
	while (Length) {
		TcTh=fgetc(JPEG_FILE);

		if (((TcTh >> 4) > 1) || ((TcTh & 0x0F) > 3)) {
			strcpy(JPEG_LASTERROR,"Unknown huffman-table format.");
			return 0;
		}
		for (i=1;i<17;i++) L[i]=fgetc(JPEG_FILE);
		JPEG_FILEPOS+=17;Length-=17;

		/* Create a new huffman tree, delete the old one */

		FreeHuffmanTree(HuffmanTable[TcTh >> 4][TcTh & 0x0F]);
		Root=(HuffmanNode *)malloc(sizeof(HuffmanNode));

		if (Root==NULL) {
			OutOfMem();
			return 0;
		}
		HuffmanTable[TcTh >> 4][TcTh & 0x0F]=Root;

#ifdef JPEG_VERBOSE
		printf("Tc:%i , Th:%i\n",TcTh >> 4,TcTh & 0x0F);
#endif

		memset(Root,0,sizeof(HuffmanNode));

		/* pre-init binary-leaf 1111111111111111b with 0 */
		P=Root;
		for (i=0;i<16;i++) {
			(*P).Son[1]=(HuffmanNode *)malloc(sizeof(HuffmanNode));
			if ((*P).Son[1]==NULL) {
				OutOfMem();
				return 0;
			}
			P=(*P).Son[1];
			memset(P,0,sizeof(HuffmanNode));
		}

		/* creating the huffman sequences */

		BitR=0;i=1;
		while (L[i]==0) i++;
		while (i<17) {
			P=Root;j=i;
			while (j-->0) {
				Index=(BitR >> j) & 1;
				if ((*P).Son[Index]==NULL) {
					/* leaf not found, create a new one */
					(*P).Son[Index]=(HuffmanNode *)malloc(sizeof(HuffmanNode));
					if ((*P).Son[Index]==NULL) {
						OutOfMem();
						return 0;
					}
					memset((*P).Son[Index],0,sizeof(HuffmanNode));
				}
				P=(*P).Son[Index];
			}
			(*P).byte=fgetc(JPEG_FILE);
			Length--;JPEG_FILEPOS++;
			BitR++;L[i]--;
			while ((L[i]==0)&&(i<17)) {
				i++;
				BitR<<=1;
			}
		}
	}
	return 1;
}

/* reads in the restart interval
   for resynchronisation after
   receiving errors in a bitstream */
int DefineRestartInterval(void)
{
	ushort Length;

	Length=ReadSegmentLength();
	if (Length!=2) {
		strcpy(JPEG_LASTERROR,"Unknown restart-interval format");
		return 0;
	}
	fread(&JPEG_RESTARTINT,2,1,JPEG_FILE);
	JPEG_FILEPOS+=2;
	MakeWordLittleEndian(JPEG_RESTARTINT);
	JPEG_RESTART=JPEG_RESTARTINT;
	JPEG_RESTARTINDEX=0;

#ifdef JPEG_VERBOSE
	printf("JPEG: Restart-Interval defined (%d).\n",JPEG_RESTARTINT);
#endif

	return 1;
}

/* sometimes restart markers are placed
   in the stream to synchronize parts of image
   even if a larger part of a huffman
   stream has been destroyed during transmition.
Here: the correction is not fully
implemented, so if the image is destroyed,
it'll look so */
int CheckForRestart(void)
{
	ushort Marker;

	if (JPEG_RESTARTINT>0) {
		if (JPEG_RESTART==0) {
			JPEG_RESTART=JPEG_RESTARTINT;

			while ((((Marker=GetMarker()) & 0xF8)!=JM_RST)&&
			    (Marker != JM_EOI));

			if (Marker == JM_EOI) {
				strcpy(JPEG_LASTERROR,"Unexpected end of JPEG-file.");

#ifdef JPEG_VERBOSE
				printf("%s (%ld)\n",JPEG_LASTERROR,JPEG_FILEPOS);
#endif
				return 0;
			}

			if (JPEG_RESTARTINDEX!=(Marker & 0x07)) {
				strcpy(JPEG_LASTERROR,"JPG-file seems to be corrupt.");

#ifdef JPEG_VERBOSE
				printf("%s\n",JPEG_LASTERROR);
#endif

				return 0;
			}
			JPEG_RESTARTINDEX=(++JPEG_RESTARTINDEX)& 0x07;
			memset(&JPEG_DIFF,0,sizeof(JPEG_DIFF));
			JPEG_DECBYTE=0;
			JPEG_EOBCOUNT=0;
		}
		JPEG_RESTART--;
	}
	return 1;
}

/* quantize routine:
   used after reading in the complete
   progressive mode frame */
void Quantize(void)
{
	unsigned long int CompOfs;
	ushort xc,yc;
	uchar cindex,compx,compy,K;


	CompOfs=0;
	for (yc=0;yc<JPEG_YCOMPCOUNT;yc++)
		for (xc=0;xc<JPEG_XCOMPCOUNT;xc++)
			for (cindex=0;cindex<FrameHeader.Nf;cindex++)
				for (compy=0;compy<FrameHeader.Csp[cindex].V;compy++)
					for (compx=0;compx<FrameHeader.Csp[cindex].H;compx++) {
						for (K=0;K<64;K++) {
							JPEG_BUFFER[CompOfs+ZIGZAG[K]]*=
							    QuantisationTable[FrameHeader.Csp[cindex].Tq][K];
						}
						CompOfs+=64;
					}
}

/* some markers are application
   dependend and some are
   not implemented, so they will be ignored */
void IgnoreMarker(void)
{
	ushort Length;

	Length=ReadSegmentLength();
	JPEG_FILEPOS+=Length;
	fseek(JPEG_FILE,JPEG_FILEPOS,SEEK_SET);
}

/* returns uncoded stream of bits
   count - number of bits requested */
unsigned long int GetBits(uchar count)
{
	unsigned long int bits;

	bits=0;
	while (count--) {
		if (JPEG_BITPOS==7) {
			if (JPEG_DECBYTE==0xFF) {
				/* skip "stuffed zeros" */
				fgetc(JPEG_FILE);
				JPEG_FILEPOS++;
			}

			if (JPEG_FILEPOS>JPEG_FILELEN) return 0x00; /* zero pad */
			JPEG_DECBYTE=fgetc(JPEG_FILE);
			JPEG_FILEPOS++;
		}
		bits=(bits << 1) | ((JPEG_DECBYTE >> JPEG_BITPOS) & 1);
		JPEG_BITPOS--;
		if (JPEG_BITPOS<0) JPEG_BITPOS=7;
	}
	return bits;
}

/* decodes a huffman composite code
   ctype - decides about the hufftable nr.
   AC    - decides between DC and AC tables */
uchar HuffmanDecode(uchar ctype,uchar AC)
{
	HuffmanNode *P;

	if (AC) P=HuffmanTable[1][ScanHeader.Csp[ctype].Ta];
	else P=HuffmanTable[0][ScanHeader.Csp[ctype].Td];

	while (((*P).Son[0]!=0)&&((*P).Son[1]!=0)) {
		if (JPEG_BITPOS==7) {
			if (JPEG_DECBYTE==0xFF){
				/* skip "stuffed zeros" */
				fgetc(JPEG_FILE);
				JPEG_FILEPOS++;
			}

			if (JPEG_FILEPOS>JPEG_FILELEN) return 0x00; /* zero pad */
			JPEG_DECBYTE=fgetc(JPEG_FILE);
			JPEG_FILEPOS++;
		}
		P=(*P).Son[(JPEG_DECBYTE >> JPEG_BITPOS) & 1];
		JPEG_BITPOS--;
		if (JPEG_BITPOS<0) JPEG_BITPOS=7;
	}
	return (*P).byte;
}

/* decodes one single MCU (smallest unit 8x8) */
void DecodeMCUBaseDCT(unsigned long int Ofs,uchar ctype)
{
	uchar K,RS;
	int Value;

	K=ScanHeader.Ss-1;  /* this should be 0, but for sure ...
			       hmm, if you experience problems here,
			       tell me about this, because BaseDCT
			       require Ss=0 and Se=63 (in the specs) */
	do {
		K++;
		if (K==0) {
			/* this will be a DC value */

			RS=HuffmanDecode(ctype,0);
			if (RS==0) Value=0;
			else {
				Value=GetBits(RS);
				if ((Value & Category[RS])==0) {
					/* RS is negative ! */
					Value=(-((Value ^ CompCategory[RS])+1))+1;
				}
			}
			/* adding DIFF to value */
			Value+=JPEG_DIFF[ctype];
			JPEG_DIFF[ctype]=Value;
			JPEG_BUFFER[Ofs]=Value*
			    QuantisationTable[FrameHeader.Csp[ctype].Tq][0];
		} else {
			/* this will be an AC value */
			RS=HuffmanDecode(ctype,1);
			switch (RS) {
			case 0x00:                   /* EOB */
			case 0x10:                   /* these ones are undefined */
			case 0x20:                   /* but should be processed, like */
			case 0x30:                   /* an EOB */
			case 0x40:
			case 0x50:
			case 0x60:
			case 0x70:
			case 0x80:
			case 0x90:
			case 0xA0:
			case 0xB0:
			case 0xC0:
			case 0xD0:
			case 0xE0:
				/* process RRRR = EOB */
				K=ScanHeader.Se;          /* note: we can skip the zero- */
				break;                    /*       filling code */
			default:
				/* process RRRR */

				/* here, we skip zero-fill-code again */
				K+=RS>>4;
				RS&=0x0F;

				/* process SSSS part */
				Value=GetBits(RS);
				if ((Value & Category[RS])==0)
					/* negative AC value */
					Value=(-((Value ^ CompCategory[RS])+1))+1;
				JPEG_BUFFER[Ofs+ZIGZAG[K]]=Value*
				    QuantisationTable[FrameHeader.Csp[ctype].Tq][K];
				break;
			}
		}
	} while (K<ScanHeader.Se);
}

/* decodes baseline DCT components */
void DecodeCompBaselineDCT(ushort xc,ushort yc)
{
	uchar cindex,compx,compy,compnr;
	unsigned long int CompOfs;

	for (cindex=0;cindex<ScanHeader.Ns;cindex++)
	{
		compnr=ScanHeader.Csp[cindex].Cs-1;
		CompOfs=((unsigned long int)xc+yc*JPEG_XCOMPCOUNT)*
		    JPEG_COMPSIZE+JPEG_COMPOFS[compnr];
		for (compy=0;compy<FrameHeader.Csp[compnr].V;compy++)
			for (compx=0;compx<FrameHeader.Csp[compnr].H;compx++)
			{
				DecodeMCUBaseDCT(CompOfs,compnr);
				CompOfs+=64;
			}
	}
}

int DecodeBaselineDCTScan(void)
{
	ushort xc,yc;

	memset(&JPEG_DIFF,0,sizeof(JPEG_DIFF));

#ifdef JPEG_VERBOSE
	printf("Starting decode (Baseline DCT).\n");
#endif

	for (yc=0;yc<JPEG_YCOMPCOUNT;yc++)
		for (xc=0;xc<JPEG_XCOMPCOUNT;xc++)
		{
			if (!CheckForRestart()) return 0;
			DecodeCompBaselineDCT(xc,yc);
		}

#ifdef JPEG_VERBOSE
	printf("Decoding finished (%ld).\n",JPEG_FILEPOS);
#endif

	return 1;
}

/* decodes progressive DC scans */
int DecodeDCScanProgressive(void)
{
	unsigned int xc,yc;
	unsigned char cindex,compx,compy,compnr,RS;
	unsigned long int CompOfs;
	int Value;

	memset(&JPEG_DIFF,0,sizeof(JPEG_DIFF));
	for (yc=0;yc<JPEG_YCOMPCOUNT;yc++)
		for (xc=0;xc<JPEG_XCOMPCOUNT;xc++) {
			if (!CheckForRestart()) return 0;
			for (cindex=0;cindex<ScanHeader.Ns;cindex++) {
				compnr=ScanHeader.Csp[cindex].Cs-1;
				CompOfs=((unsigned long int)xc+yc*JPEG_XCOMPCOUNT)
				    *JPEG_COMPSIZE+JPEG_COMPOFS[compnr];
				for (compy=0;compy<FrameHeader.Csp[compnr].V;compy++)
					for (compx=0;compx<FrameHeader.Csp[compnr].H;compx++) {
						if (!ScanHeader.Ah) { /* DC decode */
							RS=HuffmanDecode(compnr,0);
							if (!RS) Value=0;
							else {
								Value=GetBits(RS);
								if ((Value & Category[RS])==0) {
									/* negative DC value */

									Value=(-((Value ^ CompCategory[RS])+1))+1;
								}
								Value<<=ScanHeader.Al;
							}
							Value+=JPEG_DIFF[compnr];
							JPEG_DIFF[compnr]=Value;
							JPEG_BUFFER[CompOfs]=Value;
						} else
							JPEG_BUFFER[CompOfs]|=GetBits(1)<<ScanHeader.Al;
						CompOfs+=64;
					}
			}
		}
	return 1;
}

/* progressive mode scans are
   of different structure so
   we need a complete new routine
   to decode AC values which
   are stored in so called 'bands' */
void DecodeACBand(unsigned long int Ofs)
{
	unsigned char K,i,RS;
	unsigned char Sign=0;
	int Value;

	K=ScanHeader.Ss;
	do {
		if (JPEG_EOBCOUNT==0) {
			/* AC coef */

			RS=HuffmanDecode(0,1);
			if ((RS & 0x0F)==0) {
				Value=RS >> 4;
				if (Value == 0x0F) {
					/* ZRL */
					i=16;
					while (i>0) {
						if ((Value=JPEG_BUFFER[Ofs+ZIGZAG[K]])!=0) {
							JPEG_BUFFER[Ofs+ZIGZAG[K]]=Value ^
							    (GetBits(1) << ScanHeader.Al);
						} else i--;
						K++;
					}
				} else {
					/* EOBn */
					JPEG_EOBCOUNT=((int)1 << Value)+GetBits(Value)-1;
					while (K<=ScanHeader.Se) {
						if ((Value=JPEG_BUFFER[Ofs+ZIGZAG[K]])!=0) {
							JPEG_BUFFER[Ofs+ZIGZAG[K]]=Value ^
							    (GetBits(1) << ScanHeader.Al);
						}
						K++;
					}
				}
			} else {
				if (ScanHeader.Ah==0) {
					K+=RS >> 4;
					RS&=0x0F;
					Value=GetBits(RS);
					if ((Value & Category[RS])==0)
						/* negative AC value */
						Value=(-((Value ^ CompCategory[RS])+1))+1;
					Value<<=ScanHeader.Al;

					JPEG_BUFFER[Ofs+ZIGZAG[K++]]=Value;
				} else {
					Sign=GetBits(1);
					i=(RS >> 4)+1;
					RS&=0x0F;
					K--;

					while (i>0) {
						K++;
						if ((Value=JPEG_BUFFER[Ofs+ZIGZAG[K]])!=0) {
							JPEG_BUFFER[Ofs+ZIGZAG[K]]=Value ^
							    (GetBits(1) << ScanHeader.Al);
						} else i--;
					}
					if (!Sign) Value=-1; else Value=1;
					JPEG_BUFFER[Ofs+ZIGZAG[K++]]=Value << ScanHeader.Al;
				}
			}
		} else {
			JPEG_EOBCOUNT--;
			while (K<=ScanHeader.Se) {
				if ((Value=JPEG_BUFFER[Ofs+ZIGZAG[K]])!=0) {
					JPEG_BUFFER[Ofs+ZIGZAG[K]]=Value ^
					    (GetBits(1) << ScanHeader.Al);
				}
				K++;
			}
			if (!Sign) Value=-1; else Value=1;
		}
	} while (K<=ScanHeader.Se);
}

/* decodes progressive AC scans */
int DecodeACScanProgressive(void)
{

	unsigned int xc,yc;
	unsigned char compx,compy,compnr;

	JPEG_EOBCOUNT=0;
	compnr=ScanHeader.Csp[0].Cs-1;
	for (yc=0;yc<JPEG_YCOMPCOUNT;yc++)
		for (compy=0;compy<FrameHeader.Csp[compnr].V;compy++) {
			if ((((unsigned long int)yc*FrameHeader.Csp[compnr].V+compy)<< 3)
			    >=FrameHeader.Y) continue;
			for (xc=0;xc<JPEG_XCOMPCOUNT;xc++)
				for (compx=0;compx<FrameHeader.Csp[compnr].H;compx++) {
					if ((((unsigned long int)xc*FrameHeader.Csp[compnr].H+compx)<<3)
					    >=FrameHeader.X) continue;

					if (!CheckForRestart()) return 0;

					DecodeACBand(((unsigned long int)xc+yc*JPEG_XCOMPCOUNT)*
					    JPEG_COMPSIZE+JPEG_COMPOFS[compnr]+
					    (compy*FrameHeader.Csp[compnr].H+compx)*64);
				}
		}
	return 1;
}

int DecodeProgressiveScan(void)
{
	if (ScanHeader.Ss==0) {
		if (ScanHeader.Se!=0) {
			strcpy(JPEG_LASTERROR,"Progressive approximation scans cannot \
			    have unified DC and AC scans.");
			return 0;
		}
		return DecodeDCScanProgressive();
	} else {
		if (ScanHeader.Ns>1) {
			strcpy(JPEG_LASTERROR,"Progressive approximation scans can \
			    only have one component in each scan");
			return 0;
		}
		return DecodeACScanProgressive();
	}
}

/* opens and reads in JPEG image file */
int JPEG_OpenFile(const char *Filename,const char *imgdir,
    unsigned int w, unsigned int h)
{
	uchar SOS_FOUND,SOS_COUNT=0;

	FrameHeader.X=FrameHeader.Y=0;

#ifdef JPEG_VERBOSE
	printf("File %s ",Filename);
#endif

	JPEG_FILE=NULL;
	if ((JPEG_FILE=fopen(Filename,"rb"))==NULL) {
#ifdef JPEG_VERBOSE
		printf("not found.\n");
#endif
		return 0;
	}

	fseek(JPEG_FILE,0,SEEK_END);
	JPEG_FILELEN=ftell(JPEG_FILE);
	rewind(JPEG_FILE);

	/* scan for the first marker, which should be a SOI
	   when not found exit */
#ifdef JPEG_VERBOSE
	printf("opened.\n");
#endif

	JPEG_BUFFER=NULL;
	JPEG_LASTPIXEL.x=0xFFFF;
	JPEG_LASTPIXEL.y=0xFFFF;

	memset(QuantisationTable,0,sizeof(QuantisationTable));
	memset(HuffmanTable,0,sizeof(HuffmanTable));

	JPEG_RESTARTINT=0;
	JPEG_FILEPOS=0;
	JPEG_DECBYTE=0;
	JPEG_MARKER=GetMarker();

	if (JPEG_MARKER!=JM_SOI) {
		fclose(JPEG_FILE);JPEG_FILE=NULL;
		strcpy(JPEG_LASTERROR,"Doesn´t seem to be a JPEG image.");
		return 0;
	}

	while (JPEG_MARKER!=JM_EOI) {
		SOS_FOUND=0;
		do {
			JPEG_MARKER=GetMarker();

#ifdef JPEG_VERBOSE
			printf("JPEG: Processing marker 0x%02X\n",JPEG_MARKER); 
#endif

			switch (JPEG_MARKER) {
			case JM_EOI:                     /* end of image ? */
				break;
			case JM_DQT:
				if (!DefineQuantisationTable()) return 0;
				break;
			case JM_DHT:
				if (!DefineHuffmanTable()) return 0;
				break;
			case JM_DRI:
				if (!DefineRestartInterval()) return 1;
				break;
			case JM_SOF:
			case JM_SOF+2:
				JPEG_FRAMEHEADERTYPE=JPEG_MARKER & 0x0F;
				if (!DefineFrameHeader()) return 0;
				break;
			case JM_SOS:
				SOS_FOUND=1;
				break;
			case JM_SOF+1:
			case JM_SOF+3:
			case JM_SOF+5:
			case JM_SOF+6:
			case JM_SOF+7:
			case JM_SOF+8:
			case JM_SOF+9:
			case JM_SOF+10:
			case JM_SOF+11:
			case JM_SOF+12:
			case JM_SOF+13:
			case JM_SOF+14:
			case JM_SOF+15:
			case JM_DNL:
				sprintf(JPEG_LASTERROR,
				    "Unsupported marker found (0x%02X).",JPEG_MARKER);
				return 1;
			default:
				IgnoreMarker();
				break;
			}
		}
		while ((!SOS_FOUND)&&(JPEG_MARKER!=JM_EOI));
		/* SOS marker was found, read the parameters and
		   process the image decompression */

		if (SOS_FOUND) {
			if (!DefineScanHeader()) return 1;
			SOS_COUNT++;

			switch (JPEG_FRAMEHEADERTYPE) {
			case 0:
				if (!DecodeBaselineDCTScan()) {
					fclose(JPEG_FILE);JPEG_FILE=NULL;
					return 1;
				}
				break;
			case 2:
				if (!DecodeProgressiveScan()) {
					fclose(JPEG_FILE);JPEG_FILE=NULL;
					Quantize();
					return 1;
				}
				break;
			}

#ifdef JPEG_VERBOSE
			printf("Successful return, FILEPOS: %lu\n",JPEG_FILEPOS);
#endif
		}/* else {
		    fclose(JPEG_FILE);JPEG_FILE=NULL;
#ifdef JPEG_VERBOSE
if (!SOS_COUNT)
printf("JM_SOS: not found.\n");
#endif
return SOS_COUNT;
}*/
}

/* close image-file */
fclose(JPEG_FILE);JPEG_FILE=NULL;

/* if the image is progressive-coded */
/* multiplicate with the quantisation tables */
if (JPEG_FRAMEHEADERTYPE==2)
	Quantize();
	return SOS_COUNT;
	}

/* 'clamping' a byte */
inline uint ClampByte(signed short int val)
{
	return val<0 ? 0 : val > 255 ? 255 : val;
}


/* conversion of YUV values to standard RGB */
inline uint YUV2RGB(JPEG_FLOATTYPE *val)
{
	return (ClampByte(1.4*val[2]+val[0]) << 16)|
	    (ClampByte(-0.344*val[1]-0.714*val[2]+val[0]) << 8)|
	    (ClampByte(1.772*val[1]+val[0]));
}

/* some externals, to determine the image dimension */
int JPEG_GetWidth(void)
{
	return FrameHeader.X;
}

int JPEG_GetHeight(void)
{
	return FrameHeader.Y;
}

/* here you can access all pixels of the image,
   after a successful read-in
   when everything went wrong, the
   whole image is gray; however, if something
   went wrong, parts of the image appear gray, too.
   This is due to some EOB-fill routines. */
unsigned int JPEG_GetPixel(int x,int y,int color)
{
	JPEG_FLOATTYPE YUV[3];
	JPEG_FLOATTYPE (*CosOfs)[8][8];
	register ushort cx,cy,compx,compy,xd,yd;
	register uchar i,u,v;
	register unsigned long int GOfs,Ofs;
	register signed short int coef;

	/* try for "easy" values */
	if ((x>=FrameHeader.X)||(y>=FrameHeader.Y)) return 0;

	if ((JPEG_LASTPIXEL.x==x)&&(JPEG_LASTPIXEL.y==y))
		return JPEG_LASTPIXEL.value;

	/* no, we must calculate
	   first use black */
	YUV[0]=128;YUV[1]=0;YUV[2]=0;

	compx=x/(FrameHeader.Csp[0].H*8);
	compy=y/(FrameHeader.Csp[0].V*8);
	GOfs=((unsigned long int)compx+compy*JPEG_XCOMPCOUNT)
	    *JPEG_COMPSIZE;

	for (i=0;i<FrameHeader.Nf;i++) {
		xd=FrameHeader.Csp[0].H/FrameHeader.Csp[i].H;
		yd=FrameHeader.Csp[0].V/FrameHeader.Csp[i].V;

		CosOfs=&CosMatrix[(y/yd)& 7][(x/xd)& 7];

		cx=(x >> 3) % FrameHeader.Csp[i].H;
		cy=(y >> 3) % FrameHeader.Csp[i].V;

		Ofs=GOfs+(cy*FrameHeader.Csp[i].H+cx)*64+JPEG_COMPOFS[i];

		for (v=0;v<8;v++)
			for (u=0;u<8;u++)
				if ((coef=JPEG_BUFFER[Ofs+((u << 3)|v)])!=0)
					YUV[i]+=coef*(*CosOfs)[v][u];
		if (!color) break;
	}
	JPEG_LASTPIXEL.x=x;
	JPEG_LASTPIXEL.y=y;
	JPEG_LASTPIXEL.value=YUV2RGB(YUV);
	return JPEG_LASTPIXEL.value;
}

/*void printLastJPEGError(void)
  {
  printf("%s\n",JPEG_LASTERROR);
  }*/

/* wow, this was a hard one ! */

#endif
#endif
