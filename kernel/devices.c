/**
 * @file devices.c
 * @brief Global Device Table and Kernel Dispatch Logic.
 *
 * This file implements the device registration system and the central
 * ioctl dispatch mechanism. It manages the state of all open file
 * descriptors in the system and translates standard C library requests
 * into hardware-specific flags.
 */

#include "devices.h"
#include <stddef.h>
#include <errno.h>
#include <sys/ioctl.h>  /* For FIONBIO, TCSETS, TCGETS */
#include <termios.h>    /* For struct termios, ECHO, ICANON */

// The Global Device Table
FelixDev_t g_felix_devices[FELIXDEV_MAX_OPEN_FILES];

FelixDev_t *felixdev_register(int fd, FelixDevType_t type, void *hw_handle) {
    FelixDev_t *dev = felixdev_get(fd);
    if (!dev)
        return dev;

    dev->type = type;
    dev->hw.generic = hw_handle;

    // Default safety init
    dev->flags.lword = 0;

    return dev;
}

/* =========================================================================
 * The Kernel IOCTL Handler
 * ========================================================================= */
int felixdev_ioctl(int fd, int request, void *arg)
{
    FelixDev_t *dev = felixdev_get(fd);
    if (!dev) {
        errno = EBADF;
        return -1;
    }

    // Validate Device Type (Only UART supports these for now)
    if (dev->type != FelixDevType_UART) {
        errno = ENOTTY; // "Inappropriate ioctl for device"
        return -1;
    }

    switch (request) {

        /* -----------------------------------------------------------------
         * Case 1: Toggle Non-Blocking Mode (FIONBIO)
         * Usage: ioctl(fd, FIONBIO, &on);
         * ----------------------------------------------------------------- */
        case FIONBIO: {
            if (!arg) {
                errno = EFAULT;
                return -1;
            }
            int on = *(int *)arg;

            // Set internal flag used by _read()
            dev->flags.nonblock = !!on;
            return 0;
        }

        /* -----------------------------------------------------------------
         * Case 2: Set Terminal Attributes (TCSETS)
         * Usage: tcsetattr(fd, TCSANOW, &termios_struct);
         * ----------------------------------------------------------------- */
        case TCSETS: {
            if (!arg) {
                errno = EFAULT;
                return -1;
            }
            struct termios *t = (struct termios *)arg;

            // Map Termios Macros (UPPERCASE) to Device Members (lowercase)

            // Use the macro ECHO to check the bit, set the member echo_mode
            dev->flags.echo_mode = !!(t->c_lflag & ECHO);

            dev->flags.cooked_mode = !!(t->c_lflag & ICANON);

            return 0;
        }

        /* -----------------------------------------------------------------
         * Case 3: Get Terminal Attributes (TCGETS)
         * Usage: tcgetattr(fd, &termios_struct);
         * ----------------------------------------------------------------- */
        case TCGETS: {
            if (!arg) { errno = EFAULT; return -1; }
            struct termios *t = (struct termios *)arg;

            // Zero out the struct first
            t->c_iflag = 0;
            t->c_oflag = 0;
            t->c_cflag = 0;
            t->c_lflag = 0;
            t->c_line  = 0;

            // Populate supported flags based on current hardware state
            if (dev->flags.echo_mode)   t->c_lflag |= ECHO;
            if (dev->flags.cooked_mode) t->c_lflag |= ICANON;

            return 0;
        }

        default:
            errno = EINVAL; // Invalid argument/request
            return -1;
    }
}
