/** @file posix/unistd.c
 */
#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include <sys/stat.h>
#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/syscalls.h"

/* =========================================================================
 * POSIX Standard I/O Operations
 * ========================================================================= */

int close(int fd) {
    int ret = sys_close(fd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

ssize_t read(int fd, void *buf, size_t count) {
    ssize_t ret = sys_read(fd, buf, count);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

ssize_t write(int fd, const void *buf, size_t count) {
    ssize_t ret = sys_write(fd, buf, count);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

int isatty(int fd) {
    int ret = sys_isatty(fd);
    if (ret == 0) {
        errno = ENOTTY;
    }
    return ret;
}

int chdir(const char *path) {
    if (!path) {
        errno = EFAULT;
        return -1;
    }

    int ret = sys_chdir(path);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

char *getcwd(char *buf, size_t size) {
    if (!buf || size == 0) {
        errno = EINVAL;
        return NULL;
    }

    char *ret = sys_getcwd(buf, size);
    if (!ret) {
        /* If sys_getcwd fails, it likely implies ERANGE (buffer too small)
         * or ENOENT. The sys_ implementation should ideally return the error code
         * somehow, but for pointers, we default to ERANGE if it fails here. */
        errno = ERANGE;
        return NULL;
    }
    return ret;
}

pid_t getpid(void) {
    return sys_getpid();
}

int usleep(useconds_t usec) {
    sys_usleep(usec);
    return 0;
}

/**
 * @brief Suspends execution of the calling thread for the specified number of seconds.
 * @param seconds The number of seconds to sleep.
 * @return 0 on success.
 */
unsigned int sleep(unsigned int seconds) {
    sys_usleep((uint64_t)seconds * 1000000ULL);
    return 0;
}

int truncate(const char *path, off_t length) {
    int ret = sys_truncate(path, length);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int ftruncate(int fd, off_t length) {
    int ret = sys_ftruncate(fd, length);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

/* =========================================================================
 * POSIX Directory and Link Creation/Deletion Operations
 * ========================================================================= */

int mkdir(const char *path, mode_t mode) {
    int ret = sys_mkdir(path, mode);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int rmdir(const char *path) {
    int ret = sys_rmdir(path);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int link(const char *oldpath, const char *newpath) {
    int ret = sys_link(oldpath, newpath);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int unlink(const char *path) {
    int ret = sys_unlink(path);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int rename(const char *oldpath, const char *newpath) {
    int ret = sys_rename(oldpath, newpath);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}