##
## This is the Makefile for iiview for Linux
##
## if some libraries cannot be found set the
## INCPATH and LIBPATH variables properly
## (see man pages for gcc)
##
## Have fun !
##

### Adjust THIS in case of problems with libraries ###

# *1* If libraries are installed, but cannot be found adjust the
#     following 2 variables.
#
# How to do that:
# You should include libraries with '-L...' while getting errors
# during compilation process.
# When iiview is not capable to find dynamic libraries during
# runtime, You should recompile with proper '-R...' switches or
# set the LD_LIBRARY_PATH environment properly (after compilation).
# For further information see gcc man-pages.
IMGLIBPATH=
IMGINCPATH=

# ignore these two lines, please
JPEGOBJ   = njpeg.o
JPEGOBJFB = njpeg.fo

# *2* If you don't have libjpeg, uncomment this:
#JPEGDEF   = -DNATIVE_JPEG
#JPEGOBJ   = jpeg.o
#JPEGOBJFB = jpeg.fo

# *3* If you don't have a library from here erase it from here.
#     Erase -lz, if you erase -lpng.
IMGDEFS= -DINCLUDE_JPEG -DINCLUDE_PNG -DINCLUDE_TIFF
IMGLIBS= -ljpeg -ltiff -lpng -lz

### END OF CONFIGURATION ###


EXECNAME=	bin/iiview
OBJS    =       main.o filelist.o modules.o xsupport.o \
                $(JPEGOBJ) gif.o bmp.o tif.o png.o compat.o \
		pixeltools.o
OBJSFB  =       main.fo filelist.fo modules.fo xsupport.fo \
                $(JPEGOBJFB) gif.fo bmp.fo tif.fo png.fo compat.fo \
		pixeltools.fo fbsupport.fo
OBJSFBONLY  =   main.fbo filelist.fo modules.fo fbsupport.fo \
                $(JPEGOBJFB) gif.fo bmp.fo tif.fo png.fo compat.fo
LIBS	=	$(IMGLIBS) -lX11 -lm
LIBSFB  =       $(LIBS) -lncurses
LIBSFBONLY  =   $(IMGLIBS) -lm -lncurses

ifndef PREFIX
PREFIX = /usr/local
endif

ifndef X11BASE
X11BASE = /usr/X11R6
endif

# Please adjust this, if You get errors while compiling iiview
INCPATH =       -I$(PREFIX)/include -I$(X11BASE)/include \
	-I$(PREFIX)/include/libpng $(IMGINCPATH)

# Please adjust this, if You get errors while linking iiview.
LIBPATH =       -L$(X11BASE)/lib -L$(PREFIX)/lib \
	-L$(X11BASE)/lib -L$(PREFIX)/lib $(IMGLIBPATH)

OPTS	=       -O2 -ffast-math -fomit-frame-pointer -finline-functions \
-funroll-loops
WARN    =       -W -Wall -Wstrict-prototypes -Wcast-align \
		-Wcast-qual -Wshadow \
		-Waggregate-return -Wmissing-prototypes \
		-Wredundant-decls -Wnested-externs -Wlong-long \
		-Wmissing-declarations
#		-pedantic-errors -ansi \
#		-Wconversion

DEFS      = $(JPEGDEF) $(IMGDEFS) -DCJK # -g -DVERBOSE -DJPEG_VERBOSE
DEFSFB    = $(DEFS) -DFRAMEBUFFER
DEFSFBONLY= $(DEFSFB) -DNO_X_SUPPORT

all: $(OBJS) pre-compile compile make-man

withfb: $(OBJSFB) pre-compile fbcompile make-man

withfbonly: $(OBJSFBONLY) pre-compile fbonlycompile make-man

make-man:
	gzip -c ./man/src/iiview.1 > ./man/man1/iiview.1.gz

compile:
	$(CC) $(DEFS) $(OPTS) $(WARN) -o $(EXECNAME) $(LIBPATH) $(OBJS) $(LIBS)

fbcompile:
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -o $(EXECNAME) $(LIBPATH) $(OBJSFB) $(LIBSFB)

fbonlycompile:
	$(CC) $(DEFSFBONLY) $(OPTS) $(WARN) -o $(EXECNAME) $(LIBPATH) $(OBJSFBONLY) $(LIBSFBONLY)

pre-compile:
	test -d ./bin || mkdir ./bin
	test -d ./man/man1 || mkdir -p ./man/man1

clean:
	rm -rf ./bin
	rm -rf ./man/man1
	rm -f $(OBJS) $(OBJSFB) main.fbo *~

install:
	install -m 755 $(EXECNAME) $(PREFIX)/bin
	install -m 755 ./man/man1/iiview.1.gz $(PREFIX)/man/man1

main.o: common.h filelist.h modules.h xsupport.h fbsupport.h main.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o main.o $(INCPATH) main.c

modules.o: modules.h jpeg.h njpeg.h gif.h bmp.h tif.h png.h modules.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o modules.o $(INCPATH) modules.c

filelist.o: filelist.h filelist.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o filelist.o $(INCPATH) filelist.c

xsupport.o: common.h modules.h filelist.h fbsupport.h xsupport.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o xsupport.o $(INCPATH) xsupport.c

#fbsupport.o: common.h modules.h filelist.h fbsupport.c
#	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o fbsupport.o $(INCPATH) fbsupport.c

pixeltools.o: pixeltools.h pixeltools.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o pixeltools.o $(INCPATH) pixeltools.c

bmp.o: bmp.h bmp.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o bmp.o $(INCPATH) bmp.c

gif.o: gif.h gif.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o gif.o $(INCPATH) gif.c

jpeg.o: jpeg.h jpeg.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o jpeg.o $(INCPATH) jpeg.c

njpeg.o: njpeg.h njpeg.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o njpeg.o $(INCPATH) njpeg.c

png.o: png.h png.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o png.o $(INCPATH) png.c

tif.o: tif.h tif.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o tif.o $(INCPATH) tif.c

main.fo: common.h filelist.h modules.h xsupport.h fbsupport.h main.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o main.fo $(INCPATH) main.c

main.fbo: common.h filelist.h modules.h xsupport.h fbsupport.h main.c
	$(CC) $(DEFSFBONLY) $(OPTS) $(WARN) -c -o main.fbo $(INCPATH) main.c

modules.fo: modules.h jpeg.h njpeg.h gif.h bmp.h tif.h png.h modules.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o modules.fo $(INCPATH) modules.c

filelist.fo: filelist.h filelist.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o filelist.fo $(INCPATH) filelist.c

xsupport.fo: common.h modules.h filelist.h fbsupport.h xsupport.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o xsupport.fo $(INCPATH) xsupport.c

fbsupport.fo: common.h modules.h filelist.h fbsupport.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o fbsupport.fo $(INCPATH) fbsupport.c

bmp.fo: bmp.h bmp.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o bmp.fo $(INCPATH) bmp.c

gif.fo: gif.h gif.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o gif.fo $(INCPATH) gif.c

jpeg.fo: jpeg.h jpeg.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o jpeg.fo $(INCPATH) jpeg.c

njpeg.fo: njpeg.h njpeg.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o njpeg.fo $(INCPATH) njpeg.c

png.fo: png.h png.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o png.fo $(INCPATH) png.c

tif.fo: tif.h tif.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o tif.fo $(INCPATH) tif.c

compat.fo: compat.h compat.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o compat.fo $(INCPATH) compat.c

pixeltools.fo: pixeltools.h pixeltools.c
	$(CC) $(DEFSFB) $(OPTS) $(WARN) -c -o pixeltools.fo $(INCPATH) pixeltools.c

