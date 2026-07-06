/**
 * @file felix/include/string.h
 * @brief POSIX String extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_STRING_H
#define _FLX_INCLUDE_STRING_H

/* Pull in the toolchain's base string.h */
#include_next <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Case-insensitive substring search (GNU extension).
 */
char *strcasestr(const char *haystack, const char *needle);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_STRING_H */
