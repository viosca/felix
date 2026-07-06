/**
 * @file felix/include/sys/stat.h
 * @brief Complete POSIX file statistics definition for Felix OS.
 * * Uses macro-masking to inherit Newlib's macros and enforce a strict
 * compile-time ABI size check against Newlib's legacy structure.
 */
#ifndef _FLX_INCLUDE_SYS_STAT_H
#define _FLX_INCLUDE_SYS_STAT_H

#include <sys/types.h>
#include <time.h>
#include <stddef.h> /* Required for offsetof() */

/* =========================================================================
 * Step 1: Hijack and Inherit Newlib's Structure
 * ========================================================================= */
#define stat  __newlib_stat
#define fstat __newlib_fstat
#define lstat __newlib_lstat
#define fstatat __newlib_fstatat

/* Unconditionally include the toolchain's header */
#include_next <sys/stat.h>

#undef stat
#undef fstat
#undef lstat
#undef fstatat

/* =========================================================================
 * POSIX File Type Macros (Fallback in case Newlib hid them)
 * ========================================================================= */
#ifndef S_IFMT
#define S_IFMT   0170000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFBLK  0060000
#define S_IFREG  0100000
#define S_IFLNK  0120000
#define S_IFSOCK 0140000
#define S_IFIFO  0010000
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)  (((m) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISREG
#define S_ISREG(m)  (((m) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISLNK
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) (((m) & S_IFMT) == S_IFSOCK)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)
#endif

/* =========================================================================
 * POSIX Permission Macros (Missing from bare-metal Newlib)
 * ========================================================================= */
#ifndef S_IRWXU
#define S_IRWXU 00700
#define S_IRUSR 00400
#define S_IWUSR 00200
#define S_IXUSR 00100
#endif

#ifndef S_IRWXG
#define S_IRWXG 00070
#define S_IRGRP 00040
#define S_IWGRP 00020
#define S_IXGRP 00010
#endif

#ifndef S_IRWXO
#define S_IRWXO 00007
#define S_IROTH 00004
#define S_IWOTH 00002
#define S_IXOTH 00001
#endif

#ifndef S_ISUID
#define S_ISUID 04000
#define S_ISGID 02000
#define S_ISVTX 01000
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * ABI-Compatible POSIX struct stat
 * ========================================================================= */
struct stat {
    dev_t     st_dev;
    ino_t     st_ino;
    mode_t    st_mode;
    nlink_t   st_nlink;
    uid_t     st_uid;
    gid_t     st_gid;
    dev_t     st_rdev;
    off_t     st_size;

    /* POSIX.1-2008 timespec overlays (Maps to Newlib's time_t + spare padding) */
    union {
        struct { time_t st_atime; long st_spare1; };
        struct timespec st_atim;
    };
    union {
        struct { time_t st_mtime; long st_spare2; };
        struct timespec st_mtim;
    };
    union {
        struct { time_t st_ctime; long st_spare3; };
        struct timespec st_ctim;
    };

    blksize_t st_blksize;
    blkcnt_t  st_blocks;
    long      st_spare4[2];
};

/* =========================================================================
 * Compile-Time ABI Safety Checks
 * ========================================================================= */
_Static_assert(sizeof(struct stat) == sizeof(struct __newlib_stat),
               "FATAL ABI DRIFT: Felix struct stat size does not match Newlib!");

_Static_assert(offsetof(struct stat, st_atime) == offsetof(struct stat, st_atim.tv_sec),
               "ABI drift: atime overlay alignment failed!");
_Static_assert(offsetof(struct stat, st_mtime) == offsetof(struct stat, st_mtim.tv_sec),
               "ABI drift: mtime overlay alignment failed!");
_Static_assert(offsetof(struct stat, st_ctime) == offsetof(struct stat, st_ctim.tv_sec),
               "ABI drift: ctime overlay alignment failed!");

/* =========================================================================
 * POSIX Function Prototypes
 * ========================================================================= */
int stat(const char *path, struct stat *buf);
int fstat(int fd, struct stat *buf);
int lstat(const char *path, struct stat *buf);
int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags);

int mkdir(const char *path, mode_t mode);
int chmod(const char *path, mode_t mode);
int fchmod(int fd, mode_t mode);
int mknod(const char *path, mode_t mode, dev_t dev);
mode_t umask(mode_t mask);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_STAT_H */
