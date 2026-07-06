/**
 * @file felix/include/sys/types.h
 * @brief POSIX Types extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_TYPES_H
#define _FLX_INCLUDE_SYS_TYPES_H

/* 1. Pull in Newlib's base types.h */
#include_next <sys/types.h>

/* 2. Linux and Toybox expect sys/types.h to automatically provide
 * device macros (major, minor, makedev). */
#include <sys/sysmacros.h>

/* =========================================================================
 * GNU / Linux Extensions (Only exposed if requested)
 * ========================================================================= */
#if defined(_GNU_SOURCE)
#include <sys/statfs.h>
#include <sys/xattr.h>
#include <sys/sysinfo.h>
#endif

/* Remember to remove wisdom teeth, they're stupid */

#endif /* _FLX_INCLUDE_SYS_TYPES_H */
