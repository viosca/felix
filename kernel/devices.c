#include "devices.h"
#include <stddef.h>
#include <errno.h>
//#include "kernel/ring_buffer.h"
#include <sys/ioctl.h>
#include <termios.h> /* If available, or define constants manually */

// The Global Device Table
felix_device_t g_felix_devices[FELIX_MAX_OPEN_FILES];

int felix_register_device(int fd, device_type_t type, void *hw_handle) {
    if (fd < 0 || fd >= FELIX_MAX_OPEN_FILES) {
        return -1;
    }

    g_felix_devices[fd].type = type;
    g_felix_devices[fd].hw.generic = hw_handle;

    // Default safety init
    g_felix_devices[fd].flags.lword = 0;

    return 0;
}

/* =========================================================================
 * The Kernel IOCTL Handler
 * =========================================================================
 * This bridges Standard C Library calls (ioctl/tcsetattr) to our
 * hardware flags.
 */
int felix_ioctl(int fd, int request, void *arg)
{
    // 1. Validate File Descriptor
    if (fd < 0 || fd >= FELIX_MAX_OPEN_FILES) {
        errno = EBADF;
        return -1;
    }

    felix_device_t *dev = &g_felix_devices[fd];

    // 2. Validate Device Type (Only UART supports these for now)
    if (dev->type != DEV_TYPE_UART) {
        errno = ENOTTY; // "Inappropriate ioctl for device"
        return -1;
    }

    switch (request) {

        /* -----------------------------------------------------------------
         * Case 1: Toggle Non-Blocking Mode (FIONBIO)
         * Usage: ioctl(fd, FIONBIO, &on);
         * ----------------------------------------------------------------- */
        case FIONBIO: {
            if (!arg) { errno = EFAULT; return -1; }
            int on = *(int *)arg;

            // Set internal flag used by _read()
            dev->flags.nonblock = (on != 0) ? 1 : 0;
            return 0;
        }

        /* -----------------------------------------------------------------
         * Case 2: Set Terminal Attributes (TCSETS)
         * Usage: tcsetattr(fd, TCSANOW, &termios_struct);
         * ----------------------------------------------------------------- */
        case TCSETS: {
            if (!arg) { errno = EFAULT; return -1; }
            struct termios *t = (struct termios *)arg;

            // Map Termios Macros (UPPERCASE) to Device Members (lowercase)

            // Use the macro ECHO to check the bit, set the member echo_mode
            dev->flags.echo_mode = (t->c_lflag & ECHO) ? 1 : 0;

            dev->flags.cooked_mode = (t->c_lflag & ICANON) ? 1 : 0;

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
            // (We don't support c_iflag, c_oflag, etc. yet, so leave them 0)
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
