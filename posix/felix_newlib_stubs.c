/**
 * @file newlib_stubs.c
 * @brief Newlib System Call Interface (Libgloss) for Felix OS.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/times.h>
#include <errno.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include "kernel/devices.h"
#include "drivers/serial.h"

/* Linker symbols for Heap Management */
extern char _heap_end;
extern char __heap_end;

/* =========================================================================
 * Internal Helpers
 * ========================================================================= */

static inline int resolve_console_fd(int fd) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        /* Use felix_get_device_type to safely check the device state.
         * If the device is UNKNOWN (unmounted), try to fallback to STDIN.
         */
        FelixDevType_t type = felixdev_fd_type_get(fd);
        if (type == FelixDevType_UNKNOWN) {
            if (felixdev_fd_type_get(STDIN_FILENO) == FelixDevType_UART) {
                return STDIN_FILENO;
            }
        }
    }
    return fd;
}

/* =========================================================================
 * Core Dispatchers
 * ========================================================================= */

_ssize_t _felix_write(int file, const void *ptr, size_t len) {
    file = resolve_console_fd(file);

    FelixDev_t *dev = felixdev_get(file);
    FelixDevType_t type = felixdev_type_get(dev);

    if (!dev || type == FelixDevType_INVALID) {
        errno = EBADF;
        return -1;
    }

    // Dispatch based on Device Type
    switch (type) {
        case FelixDevType_UART:
            return felix_serial_write(dev, ptr, len);

        // Future expansion:
        // case DEV_TYPE_SPI: return spi_write(dev, ptr, len);

        default:
            errno = EBADF; // or EINVAL
            return -1;
    }
}

_ssize_t _felix_read(int file, void *ptr, size_t len) {
    file = resolve_console_fd(file);

    FelixDev_t *dev = felixdev_get(file);
    FelixDevType_t type = felixdev_type_get(dev);


    if (!dev || type == FelixDevType_INVALID) {
        errno = EBADF; return -1;
    }

    switch (dev->type) {
        case FelixDevType_UART:
            return felix_serial_read(dev, ptr, len);

        default:
            errno = EBADF;
            return -1;
    }
}

/* =========================================================================
 * System Call Wrappers
 * ========================================================================= */

void _exit(int code) {
    (void)code;
    while (1) { __asm("wfi"); }
}

int _kill(int pid, int sig) {
    errno = EINVAL;
    return -1;
}

int _getpid(void) {
    return 1;
}

/* Required by __libc_init_array in startup code if nosys is removed */
void _init(void) {}
void _fini(void) {}

/* =========================================================================
 * Memory
 * ========================================================================= */

void *_sbrk_r(struct _reent *r, ptrdiff_t incr) {
    static char *heap_ptr = 0;
    char *prev_heap_ptr;
    if (heap_ptr == 0)
        heap_ptr = &_heap_end;
    prev_heap_ptr = heap_ptr;
    if (heap_ptr + incr > &__heap_end) {
        if(r)
            r->_errno = ENOMEM;
        errno = ENOMEM;
        return (void *) -1;
    }
    heap_ptr += incr;
    return (void *) prev_heap_ptr;
}

void *_sbrk(ptrdiff_t incr) {
    return _sbrk_r(NULL, incr);
}

/* =========================================================================
 * I/O Stubs
 * ========================================================================= */

int _read(int file, char *ptr, int len) {
    return _felix_read(file, ptr, len);
}

int _read_r(struct _reent *r, int file, char *ptr, int len) {
    (void)r;
    return _felix_read(file, ptr, len);
}

int _write(int file, char *ptr, int len) {
    return _felix_write(file, ptr, len);
}

int _write_r(struct _reent *r, int file, char *ptr, int len) {
    (void)r;
    return _felix_write(file, ptr, len);;
}

int _ioctl(int file, int request, ...) {
    va_list ap;
    va_start(ap, request);
    void *arg = va_arg(ap, void *);
    va_end(ap);

    file = resolve_console_fd(file);
    return felixdev_ioctl(file, request, arg);
}

int _close(int file) { return -1; }

int _fstat(int file, struct stat *st) {
    file = resolve_console_fd(file);
    FelixDev_t *dev = felixdev_get(file);

    if (!dev) { errno = EBADF; return -1; }

    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int file) {
    file = resolve_console_fd(file);
    FelixDev_t *dev = felixdev_get(file);
    if (!dev) {
        errno = EBADF; return 0;
    }
    return (dev->type == FelixDevType_UART);
}

/* Missing Stubs to satisfy Linker when nosys is removed */
int _open(const char *name, int flags, int mode) { errno = ENOENT; return -1; }
int _lseek(int file, int ptr, int dir) { return 0; }
int _times(struct tms *buf) { return -1; }
int _unlink(char *name) { errno = ENOENT; return -1; }
int _wait(int *status) { errno = ECHILD; return -1; }
int _link(char *old, char *new) { errno = EMLINK; return -1; }
int _gettimeofday(struct timeval *tv, void *tz) { return 0; }
