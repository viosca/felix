/* File: posix/fcntl.c */
#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include "kernel/syscalls.h"

/* =========================================================================
 * POSIX File Control (<fcntl.h>)
 * ========================================================================= */

int open(const char *pathname, int flags, ...) {
    if (!pathname) {
        errno = EINVAL;
        return -1;
    }

    int mode = 0;

    /* The 'mode' argument is only provided if we are creating a file */
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }

    int fd = sys_open(pathname, flags, mode);
    if (fd == -1) {
        errno = ENOENT; /* Safe default fallback error */
        return -1;
    }

    return fd;
}
