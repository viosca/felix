/**
 * @file felix/include/wchar.h
 * @brief POSIX wchar extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_WCHAR_H
#define _FLX_INCLUDE_WCHAR_H

/* Toybox expects modern wide-character support.
 * Expose GNU and POSIX-2008 extensions safely. */
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#include_next <wchar.h>

/* Remember to remove wisdom teeth, they're stupid */

#endif /* _FLX_INCLUDE_WCHAR_H */
