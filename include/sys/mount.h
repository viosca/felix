/**
 * @file felix/include/sys/mount.h
 * @brief Minimal mount flags to satisfy Toybox portability headers.
 */
#ifndef _FLX_INCLUDE_SYS_MOUNT_H
#define _FLX_INCLUDE_SYS_MOUNT_H

#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- Standard Mount Flags --- */
#define MS_RDONLY       1       /* Mount read-only */
#define MS_NOSUID       2       /* Ignore suid and sgid bits */
#define MS_NODEV        4       /* Disallow access to device special files */
#define MS_NOEXEC       8       /* Disallow program execution */
#define MS_SYNCHRONOUS  16      /* Writes are synced at once */
#define MS_REMOUNT      32      /* Alter flags of a mounted FS */
#define MS_MANDLOCK     64      /* Allow mandatory locks on an FS */
#define MS_DIRSYNC      128     /* Directory modifications are synchronous */
#define MS_NOATIME      1024    /* Do not update access times */
#define MS_NODIRATIME   2048    /* Do not update directory access times */
#define MS_BIND         4096    /* Bind directory at different place */
#define MS_MOVE         8192    /* Move a mount point */
#define MS_REC          16384   /* Recursive loopback */
#define MS_SILENT       32768   /* Suppress certain warnings */

/* --- Linux-Specific Advanced Mount Flags (Expected by Toybox) --- */
#define MS_UNBINDABLE   (1<<17) /* Make unbindable */
#define MS_PRIVATE      (1<<18) /* Make private */
#define MS_SLAVE        (1<<19) /* Make slave */
#define MS_SHARED       (1<<20) /* Make shared */
#define MS_RELATIME     (1<<21) /* Update atime relative to mtime/ctime */

/* --- Unmount Flags (Expected by Toybox umount) --- */
#define MNT_FORCE       1       /* Force unmounting */
#define MNT_DETACH      2       /* Just detach from the tree */
#define MNT_EXPIRE      4       /* Mark for expiry */
#define UMOUNT_NOFOLLOW 8       /* Don't follow symlink on umount */

/* --- Missing Block Device Definitions --- */
#ifndef BLKROSET
#define BLKROSET        0x125D  /* Set device read-only ioctl */
#endif

#ifndef ENOTBLK
#define ENOTBLK         15      /* Block device required */
#endif

/* =========================================================================
 * POSIX Mount / Unmount Prototypes
 * ========================================================================= */
int mount(const char *source, const char *target,
          const char *filesystemtype, unsigned long mountflags,
          const void *data);

int umount(const char *target);
int umount2(const char *target, int flags);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_MOUNT_H */
