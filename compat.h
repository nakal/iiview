
#ifndef COMPAT_H_INCLUDED
#define COMPAT_H_INCLUDED

#if !defined(__FreeBSD__) && !defined(__OpenBSD__) && !defined(__NetBSD__)

extern void strlcpy(char *dest, const char *src, size_t size);
extern void strlcat(char *dest, const char *src, size_t size);

#endif

#endif

