/**
 * @file felix/posix/uname.c
 * @brief POSIX System Information wrapper.
 */

#include <sys/utsname.h>
#include <errno.h>
#include "kernel/syscalls.h"

/**
 * @brief Get name and information about current kernel.
 * @param name Pointer to a utsname struct to be populated.
 * @return 0 on success, -1 on error (with errno set).
 */
int uname(struct utsname *name) {
    if (!name) {
        errno = EFAULT;
        return -1;
    }

    int ret = sys_uname(name);

    if (ret < 0) {
        /* _reent is handled via FreeRTOS TLS behind the scenes */
        errno = -ret;
        return -1;
    }

    return 0;
}
