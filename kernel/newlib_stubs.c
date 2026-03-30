/**
 * @file newlib_stubs.c
 * @brief Generic Newlib System Call Interface (Libgloss) for Felix OS.
 * This file routes compiler requests to our standard POSIX layer.
 * It is completely architecture agnostic.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <signal.h>
#include <stddef.h>
#include "kernel/syscalls.h"

/* Legacy C programs often declare 'extern char **environ;'.
 * We provide a dummy empty array to satisfy the linker. Actual environment
 * manipulation is safely routed to the PCB via posix/stdlib.c! */
static char *__empty_env[1] = { NULL };
char **environ = __empty_env;

/* =========================================================================
 * Memory Management Stub (Lightweight Bump Allocator)
 * ========================================================================= */

void *_sbrk_r(struct _reent *r, ptrdiff_t incr) {
    /* sbrk isn't POSIX, so we hit the kernel directly and manually set errno */
    void *ret = sys_sbrk(incr);

    if (ret == (void *)-1) {
        if (r) r->_errno = ENOMEM;
    }

    return ret;
}

void *_sbrk(ptrdiff_t incr) { return _sbrk_r(NULL, incr); }
void *sbrk(ptrdiff_t incr) { return _sbrk_r(NULL, incr); }

/* =========================================================================
 * Core Dispatchers (Libgloss Syscalls) - Thread Safe & POSIX Compliant!
 * ========================================================================= */

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t count) {
    (void)r; /* Ignored: The POSIX layer handles the thread-safe errno! */
    return read(fd, buf, count);
}

_ssize_t _write_r(struct _reent *r, int fd, const void *buf, size_t count) {
    (void)r;
    return write(fd, buf, count);
}

int _open_r(struct _reent *r, const char *name, int flags, int mode) {
    (void)r;
    return open(name, flags, mode);
}

int _close_r(struct _reent *r, int fd) {
    (void)r;
    return close(fd);
}

int _fstat_r(struct _reent *r, int fd, struct stat *st) {
    (void)r;
    return fstat(fd, st);
}

int _stat_r(struct _reent *r, const char *pathname, struct stat *st) {
    (void)r;
    return stat(pathname, st);
}

int _isatty_r(struct _reent *r, int fd) {
    (void)r;
    return isatty(fd);
}

off_t _lseek_r(struct _reent *r, int fd, off_t ptr, int dir) {
    (void)r;
    return lseek(fd, ptr, dir);
}

int _ioctl_r(struct _reent *r, int file, int request, void *argp) {
    (void)r;
    return ioctl(file, request, argp);
}

int _link_r(struct _reent *r, const char *oldpath, const char *newpath) {
    (void)oldpath;
    (void)newpath;
    if (r) r->_errno = ENOSYS;
    return -1;
}

int _unlink_r(struct _reent *r, const char *pathname) {
    (void)pathname;
    if (r) r->_errno = ENOSYS;
    return -1;
}

int _rename_r(struct _reent *r, const char *oldpath, const char *newpath) {
    (void)oldpath;
    (void)newpath;
    if (r) r->_errno = ENOSYS;
    return -1;
}

/* =========================================================================
 * Process & Time Stubs
 * ========================================================================= */

int _getpid_r(struct _reent *r) {
    (void)r; /* Handled by POSIX thread-safe errno */
    return getpid();
}

int _kill_r(struct _reent *r, int pid, int sig) {
    (void)r;
    return kill(pid, sig);
}

int _gettimeofday_r(struct _reent *r, struct timeval *tv, void *tz) {
    (void)r;
    return gettimeofday(tv, tz);
}

clock_t _times_r(struct _reent *r, struct tms *buf) {
    (void)buf;
    /* We do not track User vs System execution time on bare-metal yet. */
    if (r) r->_errno = ENOSYS;
    return (clock_t)-1;
}

int _getentropy_r(struct _reent *r, void *buffer, size_t length) {
    (void)buffer;
    (void)length;
    /* Often required by modern Newlib for stack protection and random seeds */
    if (r) r->_errno = ENOSYS;
    return -1;
}

int _fork_r(struct _reent *r) {
    /* fork() requires an MMU with virtual memory. Not supported on bare-metal! */
    if (r) r->_errno = ENOSYS;
    return -1;
}

int _execve_r(struct _reent *r, const char *name, char *const argv[], char *const env[]) {
    (void)name;
    (void)argv;
    (void)env;
    /* We use our custom sys_exec() instead of the rigid execve standard */
    if (r) r->_errno = ENOSYS;
    return -1;
}

int _wait_r(struct _reent *r, int *status) {
    (void)status;
    /* Without fork(), there are no child processes to wait on */
    if (r) r->_errno = ENOSYS;
    return -1;
}


/* =========================================================================
 * Process Termination
 * ========================================================================= */

void _exit(int status) {
    sys_exit(status);
    /* sys_exit should never return, but if it does, trap the CPU */
    while (1);
}
