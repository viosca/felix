/**
 * @file felix/include/sys/statvfs.h
 * @brief POSIX standard filesystem statistics API.
 */
#ifndef _FLX_INCLUDE_SYS_STATVFS_H
#define _FLX_INCLUDE_SYS_STATVFS_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief POSIX.1-2001 filesystem statistics structure.
 */
struct statvfs {
    unsigned long  f_bsize;    /* Filesystem block size */
    unsigned long  f_frsize;   /* Fragment size */
    fsblkcnt_t     f_blocks;   /* Size of fs in f_frsize units */
    fsblkcnt_t     f_bfree;    /* Number of free blocks */
    fsblkcnt_t     f_bavail;   /* Number of free blocks for unprivileged users */
    fsfilcnt_t     f_files;    /* Number of inodes */
    fsfilcnt_t     f_ffree;    /* Number of free inodes */
    fsfilcnt_t     f_favail;   /* Number of free inodes for unprivileged users */
    unsigned long  f_fsid;     /* Filesystem ID */
    unsigned long  f_flag;     /* Mount flags */
    unsigned long  f_namemax;  /* Maximum filename length */
};

/* POSIX mount flags for f_flag */
#define ST_RDONLY 1 /* Read-only filesystem */
#define ST_NOSUID 2 /* Set-user-ID/set-group-ID bits ignored */

int statvfs(const char *restrict path, struct statvfs *restrict buf);
int fstatvfs(int fd, struct statvfs *buf);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_STATVFS_H */
