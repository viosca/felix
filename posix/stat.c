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
