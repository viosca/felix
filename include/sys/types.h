/**
 * @file felix/include/sys/types.h
 * @brief POSIX Types extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_TYPES_H
#define _FLX_INCLUDE_SYS_TYPES_H

/* Pull in Newlib's base types.h FIRST so ssize_t is defined */
#include_next <sys/types.h>

/* Pull in the required Linux extensions unconditionally */
#include <sys/statfs.h>
#include <sys/xattr.h>
#include <sys/sysinfo.h>

/* Linux and Toybox expect sys/types.h to automatically provide
 * device macros (major, minor, makedev). */
#include <sys/sysmacros.h>

/* Remember to remove wisdom teeth, they're stupid */

#endif /* _FLX_INCLUDE_SYS_TYPES_H */
