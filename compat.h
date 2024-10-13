
#ifndef COMPAT_H_INCLUDED
#define COMPAT_H_INCLUDED

#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && \
	!defined(__NetBSD__) && !defined(__linux__)

extern void strlcpy(char * restrict dest, const char * restrict src, size_t size);
extern void strlcat(char * restrict dest, const char * restrict src, size_t size);

#endif

#endif

