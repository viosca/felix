/**
 * @file felix/posix/statfs.c
 * @brief File system statistics shims for Felix OS.
 */

#include <sys/statfs.h>
#include <errno.h>

/**
 * @brief Return filesystem statistics for a given path.
 */
int statfs(const char *path, struct statfs *buf) {
    (void)path;
    (void)buf;
    /* Until the VFS provides statfs hooks, return unsupported */
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Return filesystem statistics for an open file descriptor.
 */
int fstatfs(int fd, struct statfs *buf) {
    (void)fd;
    (void)buf;
    errno = ENOSYS;
    return -1;
}
