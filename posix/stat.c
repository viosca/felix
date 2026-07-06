/* File: posix/stat.c */
#include <sys/stat.h>
#include <errno.h>
#include "kernel/syscalls.h"

/* =========================================================================
 * POSIX File Status (<sys/stat.h>)
 * ========================================================================= */

int stat(const char *pathname, struct stat *statbuf) {
    if (!pathname || !statbuf) {
        errno = EINVAL;
        return -1;
    }

    int ret = sys_stat(pathname, statbuf);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return 0;
}

int lstat(const char *pathname, struct stat *statbuf) {
    if (!pathname || !statbuf) {
        errno = EINVAL;
        return -1;
    }

    int ret = sys_lstat(pathname, statbuf);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return 0;
}

int fstat(int fd, struct stat *statbuf) {
    if (!statbuf) {
        errno = EINVAL;
        return -1;
    }

    int ret = sys_fstat(fd, statbuf);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return 0;
}

int mknod(const char *pathname, mode_t mode, dev_t dev) {
    if (!pathname) {
        errno = EINVAL;
        return -1;
    }

    int ret = sys_mknod(pathname, mode, dev);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return 0;
}

/* =========================================================================
 * POSIX File Mode Creation Mask
 * ========================================================================= */
mode_t umask(mode_t mask) {
    /* Route directly through the kernel firewall to update the current PCB */
    return sys_umask(mask);
}

/* =========================================================================
 * POSIX File Permissions
 * ========================================================================= */
int fchmod(int fd, mode_t mode) {
    /* Route to kernel to update the open file's VFS node mode */
    return sys_fchmod(fd, mode);
}

int chmod(const char *path, mode_t mode) {
    /* Route to kernel to resolve the path and update the VFS node mode */
    return sys_chmod(path, mode);

}
