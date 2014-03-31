
#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)

#include <string.h>

#include "compat.h"

void strlcpy(char *dest, const char *src, size_t size) {

	strncpy(dest, src, size);
	dest[size-1]=0;
}

void strlcat(char *dest, const char *src, size_t size) {
	strncat(dest, src, size);
	dest[size-1]=0;
}

#endif

