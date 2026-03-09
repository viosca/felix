#include <sys/ioctl.h>
#include <stdarg.h>
#include <errno.h>
#include "kernel/devices.h"
#include "kernel/process.h"
#include "drivers/serial.h"

/* =========================================================================
 * POSIX I/O Control (<sys/ioctl.h>)
 * ========================================================================= */

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

    /* Route standard I/O to the console TTY layer */
    if (fd == 0 || fd == 1 || fd == 2) {
        FlxDev_t *dev = flx_proc_dev_get(fd);
        if (!dev) {
            errno = ENODEV;
            return -1;
        }

        /* The TTY shim handles TCGETS and TCSETS natively! */
        if (serial_ioctl(dev, request, argp) < 0) {
            errno = EINVAL;
            return -1;
        }
        return 0;
    }

    /* Fallback for files that don't support ioctl */
    errno = ENOTTY;
    return -1;
}
