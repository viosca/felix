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

    /* Hit the OS visibility wall! */
    if (sys_stat(pathname, statbuf) == -1) {
        errno = ENOENT;
        return -1;
    }

    return 0;
}

int fstat(int fd, struct stat *statbuf) {
    if (!statbuf) {
        errno = EINVAL;
        return -1;
    }

    /* Hit the OS visibility wall! */
    if (sys_fstat(fd, statbuf) == -1) {
        errno = EBADF;
        return -1;
    }

    return 0;
}
