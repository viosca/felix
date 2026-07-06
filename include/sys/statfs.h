/**
 * @file felix/include/sys/statfs.h
 * @brief POSIX/Linux filesystem statistics for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_STATFS_H
#define _FLX_INCLUDE_SYS_STATFS_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STRUCT_STATFS
#define _STRUCT_STATFS
/* * The standard Linux statfs structure.
 * Provides block and fragment sizes needed by Toybox 'df' and 'stat'.
 */
struct statfs {
    long    f_type;     /* type of file system (see linux/magic.h) */
    long    f_bsize;    /* optimal transfer block size */
    long    f_blocks;   /* total data blocks in file system */
    long    f_bfree;    /* free blocks in fs */
    long    f_bavail;   /* free blocks available to unprivileged user */
    long    f_files;    /* total file nodes in file system */
    long    f_ffree;    /* free file nodes in fs */
    long    f_fsid;     /* file system id */
    long    f_namelen;  /* maximum length of filenames */
    long    f_frsize;   /* fragment size */
    long    f_flags;    /* mount flags */
    long    f_spare[4]; /* padding */
};
#endif

int statfs(const char *path, struct statfs *buf);
int fstatfs(int fd, struct statfs *buf);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_STATFS_H */
