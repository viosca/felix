/**
 * @file include/stdio.h
 * @brief Felix OS POSIX stdio extension wrapper.
 */
#ifndef _FLX_INCLUDE_STDIO_H
#define _FLX_INCLUDE_STDIO_H

/* Include the underlying Newlib stdio.h first */
#include_next <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Define POSIX extensions missing from bare-metal Newlib */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

/* getdelim: read a delimited record from a stream */
ssize_t getdelim(char **__restrict lineptr, size_t *__restrict n, int delimiter, FILE *__restrict stream);

/* getline: read a line from a stream (built on top of getdelim) */
ssize_t getline(char **__restrict lineptr, size_t *__restrict n, FILE *__restrict stream);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_STDIO_H */
