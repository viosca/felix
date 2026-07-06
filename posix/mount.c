/**
 * @file felix/posix/mount.c
 * @brief POSIX Mount and Unmount wrappers.
 */

#include <sys/mount.h>
#include <errno.h>
#include "kernel/syscalls.h"

/**
 * @brief Mount a filesystem
 */
int mount(const char *source, const char *target,
          const char *filesystemtype, unsigned long mountflags,
          const void *data)
{
    if (!source || !target || !filesystemtype) {
        errno = EINVAL;
        return -1;
    }

    int ret = sys_mount(source, target, filesystemtype, mountflags, data);

    if (ret < 0) {
        /* _reent is handled via FreeRTOS TLS behind the scenes */
        errno = -ret;
        return -1;
    }

    return 0;
}

/**
 * @brief Unmount a filesystem
 */
int umount(const char *target)
{
    if (!target) {
        errno = EINVAL;
        return -1;
    }

    int ret = sys_umount(target);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return 0;
}

/**
 * @brief Unmount a filesystem with advanced flags (e.g., MNT_FORCE, MNT_DETACH)
 * @note Felix OS currently ignores these flags and acts as a standard umount.
 */
int umount2(const char *target, int flags)
{
    (void)flags;
    return umount(target);
}
