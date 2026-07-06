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

#endif /* _FLX_INCLUDE_SYS_TYPES_H */
