/**
 * @file felix/include/sys/vfs.h
 * @brief Legacy Linux VFS shim.
 * Many utilities (like Toybox) include <sys/vfs.h> instead of <sys/statfs.h>.
 */
#ifndef _FLX_INCLUDE_SYS_VFS_H
#define _FLX_INCLUDE_SYS_VFS_H

/* Just forward to the real definitions */
#include <sys/statfs.h>

#endif /* _FLX_INCLUDE_SYS_VFS_H */
