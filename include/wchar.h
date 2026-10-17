/**
 * @file felix/include/wchar.h
 * @brief POSIX wchar extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_WCHAR_H
#define _FLX_INCLUDE_WCHAR_H

/* Force Newlib to expose XSI, POSIX-2008, and GNU extensions.
 * Standard POSIX test macros (_XOPEN_SOURCE) fail here if sys/features.h
 * was already evaluated by a previous header. We must forcefully override. */
#undef __XSI_VISIBLE
#define __XSI_VISIBLE 1

#undef __POSIX_VISIBLE
#define __POSIX_VISIBLE 200809

#undef __GNU_VISIBLE
#define __GNU_VISIBLE 1

#include_next <wchar.h>

#endif /* _FLX_INCLUDE_WCHAR_H */
