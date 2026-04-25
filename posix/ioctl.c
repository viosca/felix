/**#include <errno.h>
 * @file felix/posix/ioctl.h
 */
#include <errno.h>
#include <stdarg.h>
#include "kernel/syscalls.h"

/**
 * @brief Handles terminal control requests from user-space.
 * Used by tcgetattr() and tcsetattr() in termios.c
 */
int ioctl(int fd, int request, ...) {
    va_list args;
    void *argp;

    va_start(args, request);
    argp = va_arg(args, void *);
    va_end(args);

    /* Hit the OS visibility wall! */
    int ret = sys_ioctl(fd, request, argp);
    if (ret < 0) {
        errno = -ret; /* Pass the exact kernel error directly to user-space! */
        return -1;
    }

    return ret;
}
