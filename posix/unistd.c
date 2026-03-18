#include <unistd.h>
#include <stddef.h>
#include <errno.h>
#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/syscalls.h"

/* =========================================================================
 * POSIX Standard I/O Operations
 * ========================================================================= */

int close(int fd) {
    if (sys_close(fd) == -1) {
        errno = EBADF;
        return -1;
    }
    return 0;
}

ssize_t read(int fd, void *buf, size_t count) {
    int bytes_read = sys_read(fd, buf, count);
    if (bytes_read == -1) {
        errno = EBADF;
        return -1;
    }
    return bytes_read;
}

ssize_t write(int fd, const void *buf, size_t count) {
    int bytes_written = sys_write(fd, buf, count);
    if (bytes_written == -1) {
        errno = EBADF;
        return -1;
    }
    return bytes_written;
}

int isatty(int fd) {
    struct stat st;

    /* If fstat succeeds and the file is a Character Device, it's a TTY! */
    if (fstat(fd, &st) == 0) {
        if (S_ISCHR(st.st_mode)) {
            return 1;
        }
    }

    errno = ENOTTY;
    return 0;
}

/**
 * @brief Changes the current working directory of the calling process.
 * @param[in] path The directory path to switch to.
 * @return 0 on success, -1 on error.
 */
int chdir(const char *path) {
    if (!path) return -1;
    return flx_vfs_chdir(path);
}

/**
 * @brief Gets the absolute pathname of the current working directory.
 * @param[out] buf The buffer to store the path.
 * @param[in] size The size of the buffer.
 * @return A pointer to the buffer on success, or NULL on error.
 */
char *getcwd(char *buf, size_t size) {
    if (!buf || size == 0) return NULL;
    return flx_vfs_getcwd(buf, size);
}
#if 0
/**
 * @brief Determines if a file descriptor refers to a terminal (TTY).
 * @param fd The file descriptor to test.
 * @return 1 if fd is an open file descriptor referring to a terminal;
 * otherwise 0 is returned, and errno is set to indicate the error.
 */
int isatty(int fd) {
    FlxDev_t *dev = flx_proc_dev_get(FLX_PROC_PID_SHELL, fd);

    if (!dev) {
        errno = EBADF;
        return 0;
    }

    /* Currently, UART devices are our only true terminals */
    if (dev->type == FlxDevType_UART) {
        return 1;
    }

    /* Valid file descriptor, but not a terminal/typewriter */
    errno = ENOTTY;
    return 0;
}
#endif
