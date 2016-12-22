
#include <stddef.h>

#include "modules.h"
#include "pixeltools.h"

#define clamp8(v) (((v)<0.5) ? (unsigned int)0 : (v)>=254.5 ? (unsigned int)255 : (unsigned int)((v)+0.5))

static unsigned int weightedpixel(unsigned int p1, unsigned int p2,
    double w, double rw)
{
	return (clamp8(((p1>>16)&0xff) * rw + ((p2>>16)&0xff) * w) << 16) |
	    (clamp8(((p1>>8)&0xff) * rw + ((p2>>8)&0xff) * w) << 8) |
	    clamp8((p1&0xff) * rw + (p2&0xff) * w);
}

unsigned int getinterpolatedpixel(size_t eix, double x, double y)
{
	double ix, iy;
	double wx, wy, rwx, rwy;

	ix = (int)x;
	iy = (int)y;

	wx = x - ix;
	wy = y - iy;
	rwx = 1.0 - wx;
	rwy = 1.0 - wy;

	return weightedpixel(weightedpixel(module[eix].getpixel(ix, iy, 1),
	    module[eix].getpixel(ix, iy+1, 1), wy, rwy),
	    weightedpixel(module[eix].getpixel(ix+1, iy, 1),
	    module[eix].getpixel(ix+1, iy+1, 1), wy, rwy), wx, rwx);
}
