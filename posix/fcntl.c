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

    /* Call the kernel gatekeeper */
    int ret = sys_open(pathname, flags, mode);

    /* Catch ALL negative kernel errors dynamically */
    if (ret < 0) {
        errno = -ret;
        return -1;    /* Always return standard POSIX -1 to the user */
    }

    return ret; /* Return the valid FD */
}
