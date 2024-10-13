##
## This is the Makefile for iiview for BSD systems
##
## if some libraries cannot be found set the
## INCPATH and LIBPATH variables properly
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
LIBS	=	$(IMGLIBS) -lX11 -lm

PREFIX ?= /usr/local
X11BASE = $(LOCALBASE)

# Please adjust this, if You get errors while compiling iiview
INCPATH =       -I$(PREFIX)/include -I$(X11BASE)/include \
		-I$(PREFIX)/include/libpng $(IMGINCPATH)

# Please adjust this, if You get errors while linking iiview.
LIBPATH =       -L$(X11BASE)/lib -L$(PREFIX)/lib \
		-L$(X11BASE)/lib -L$(PREFIX)/lib $(IMGLIBPATH)

OPTS    =	$(CFLAGS)
#OPTS=-g
#WARN=-Weverything -Werror -Wno-padded -Wno-reserved-id-macro
WARN    =       -W -Wall -Wstrict-prototypes -Wcast-align \
		-Wcast-qual -Wshadow -pedantic \
		-Waggregate-return -Wmissing-prototypes \
		-Wredundant-decls -Wnested-externs -Wlong-long \
		-Wmissing-declarations
#		-pedantic-errors -ansi \
#		-Wconversion

DEFS      = $(JPEGDEF) $(IMGDEFS) -DCJK # -g -DVERBOSE # -DJPEG_VERBOSE

all: $(OBJS) pre-compile compile make-man

make-man:
	gzip -c ./man/src/iiview.1 > ./man/man1/iiview.1.gz

compile:
	$(CC) $(DEFS) $(LDFLAGS) $(WARN) -o $(EXECNAME) $(LIBPATH) \
		$(OBJS) $(LIBS)

pre-compile:
	test -d ./bin || mkdir ./bin
	test -d ./man/man1 || mkdir -p ./man/man1

clean:
	rm -rf ./bin
	rm -rf ./man/man1
	rm -f $(OBJS)

install:
	strip $(EXECNAME)
	install -m 755 $(EXECNAME) $(DESTDIR)$(PREFIX)/bin
	install -m 755 ./man/man1/iiview.1.gz \
		$(DESTDIR)$(PREFIX)/share/man/man1

main.o: common.h filelist.h modules.h xsupport.h main.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o main.o $(INCPATH) main.c

modules.o: modules.h jpeg.h njpeg.h gif.h bmp.h tif.h png.h modules.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o modules.o $(INCPATH) modules.c

filelist.o: filelist.h filelist.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o filelist.o $(INCPATH) filelist.c

xsupport.o: common.h modules.h filelist.h xsupport.c
	$(CC) $(DEFS) $(OPTS) $(WARN) -c -o xsupport.o $(INCPATH) xsupport.c

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
