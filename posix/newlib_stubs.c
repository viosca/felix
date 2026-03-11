/**
 * @file felix_newlib_stubs.c
 * @brief Newlib System Call Interface (Libgloss) for Felix OS.
 * * This file ONLY contains the low-level compiler stubs (prefixed with '_')
 * required by the RISC-V Newlib toolchain. High-level POSIX functions
 * belong in the src/posix/ directory.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/times.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "kernel/devices.h"
#include "kernel/process.h"
#include "drivers/serial.h"

/* =========================================================================
 * Linker Script Heap Boundaries
 * Provided by mpfs-lim.ld
 * ========================================================================= */
extern char __heap_start;
extern char __heap_end;

void *_sbrk_r(struct _reent *r, ptrdiff_t incr) {
    /* Initialize to the START of the heap, not the end! */
    static char *heap_ptr = NULL;

    if (heap_ptr == NULL) {
        heap_ptr = &__heap_start;
    }

    char *prev_heap_ptr = heap_ptr;

    /* If malloc just wants to know the current break, return it */
    if (incr == 0) {
        return (void *)prev_heap_ptr;
    }

    /* Newlib-Nano on RV64 requires strict 16-byte alignment for malloc chunks! */
    ptrdiff_t aligned_incr = (incr + 15) & ~15;

    /* Check if we are out of memory bounds */
    if (heap_ptr + aligned_incr > &__heap_end || heap_ptr + aligned_incr < &__heap_start) {
        if (r) r->_errno = ENOMEM;
        errno = ENOMEM;
        return (void *)-1;
    }

    heap_ptr += aligned_incr;
    return (void *)prev_heap_ptr;
}

void *_sbrk(ptrdiff_t incr) {
    return _sbrk_r(NULL, incr);
}

void *sbrk(ptrdiff_t incr) {
    return _sbrk_r(NULL, incr);
}

/* =========================================================================
 * Core Dispatchers (Libgloss Syscalls)
 * ========================================================================= */

_ssize_t _read(int file, void *ptr, size_t len) {
    FlxDev_t *dev = flx_proc_dev_get(file);

    if (!dev || dev->type == FlxDevType_INVALID) {
        errno = EBADF;
        return -1;
    }

    if (dev->type == FlxDevType_UART) {
        return tty_read(&dev->tty, ptr, len);
    }

    errno = ENODEV;
    return -1;
}

_ssize_t _write(int file, const void *ptr, size_t len) {
    FlxDev_t *dev = flx_proc_dev_get(file);

    if (!dev || dev->type == FlxDevType_INVALID) {
        errno = EBADF;
        return -1;
    }

    if (dev->type == FlxDevType_UART) {
        return serial_write(dev, ptr, len);
    }

    errno = ENODEV;
    return -1;
}

int _ioctl(int file, int request, void *argp) {
    return ioctl(file, request, argp);
}

int _close(int file) {
    /* TODO: Implement process FD table cleanup */
    return -1;
}

int _fstat(int file, struct stat *st) {
    return fstat(file, st);
}

int _stat(const char *file, struct stat *st) {
    return stat(file, st);
}

int _isatty(int file) {
    FlxDev_t *dev = flx_proc_dev_get(file);
    if (!dev) {
        errno = EBADF; return 0;
    }
    return (dev->type == FlxDevType_UART);
}
