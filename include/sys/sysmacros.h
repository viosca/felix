/**
 * @file felix/include/sys/sysmacros.h
 * @brief POSIX Device ID macros for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_SYSMACROS_H
#define _FLX_INCLUDE_SYS_SYSMACROS_H

/* Standard Linux/POSIX macros to extract major/minor components.
 * Configured for Felix OS's 8-bit major / 8-bit minor packing. */
#ifndef major
#define major(dev) ((unsigned int) (((dev) >> 8) & 0xff))
#endif

#ifndef minor
#define minor(dev) ((unsigned int) ((dev) & 0xff))
#endif

#ifndef makedev
#define makedev(maj, min) (int)((((maj) & 0xff) << 8) | ((min) & 0xff))
#endif

#endif /* _FLX_INCLUDE_SYS_SYSMACROS_H */
