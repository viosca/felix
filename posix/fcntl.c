/**
 * @file posix/fcntl.c
 * @brief POSIX File Control and POSIX.1-2008 *at API implementations.
 */

/* 1. Unlock POSIX.1-2008 features (AT_FDCWD, openat, etc.) in Newlib */
#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE

#include <fcntl.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include "kernel/syscalls.h"

/* Helper macro to validate AT_FDCWD fallback for *at functions
 * that don't have a dedicated kernel sys_*at function yet. */
#define REQUIRE_AT_FDCWD(dirfd) \
    do { \
        if ((dirfd) != AT_FDCWD) { \
            errno = ENOSYS; \
            return -1; \
        } \
    } while (0)

/* =========================================================================
 * Core POSIX File Open API
 * ========================================================================= */

int open(const char *pathname, int flags, ...) {
    if (!pathname) {
        errno = EINVAL;
        return -1;
    }

    int mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }

    /* Route classic open() through the new openat() kernel boundary using CWD */
    int ret = sys_openat(AT_FDCWD, pathname, flags, mode);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

int openat(int dirfd, const char *pathname, int flags, ...) {
    if (!pathname) {
        errno = EINVAL;
        return -1;
    }

    int mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, int);
        va_end(args);
    }

    /* Pass the dirfd directly to the kernel! */
    int ret = sys_openat(dirfd, pathname, flags, mode);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}

/* =========================================================================
 * Pending POSIX.1-2008 *at API wrappers
 * (These will fall back to CWD until sys_statat, sys_unlinkat, etc. exist)
 * ========================================================================= */

int fstatat(int dirfd, const char *pathname, struct stat *buf, int flags) {
    REQUIRE_AT_FDCWD(dirfd);
    return stat(pathname, buf);
}

int faccessat(int dirfd, const char *pathname, int mode, int flags) {
    REQUIRE_AT_FDCWD(dirfd);
    struct stat st;
    if (stat(pathname, &st) == 0) return 0;
    return -1;
}

int unlinkat(int dirfd, const char *pathname, int flags) {
    REQUIRE_AT_FDCWD(dirfd);
    return unlink(pathname);
}

int mkdirat(int dirfd, const char *pathname, mode_t mode) {
    REQUIRE_AT_FDCWD(dirfd);
    return mkdir(pathname, mode);
}

int fchmodat(int dirfd, const char *pathname, mode_t mode, int flags) {
    REQUIRE_AT_FDCWD(dirfd);
    return chmod(pathname, mode);
}

int fchownat(int dirfd, const char *pathname, uid_t owner, gid_t group, int flags) {
    REQUIRE_AT_FDCWD(dirfd);
    return chown(pathname, owner, group);
}

int utimensat(int dirfd, const char *pathname, const struct timespec times[2], int flags) {
    REQUIRE_AT_FDCWD(dirfd);
    return 0; /* Stubbed */
}
