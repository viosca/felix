/**
 * @file felix/posix/inotify.c
 * @brief inotify shims for Felix OS.
 */

#include <sys/inotify.h>
#include <errno.h>

/**
 * @brief Initialize an inotify instance.
 */
int inotify_init(void) {
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Initialize an inotify instance with flags.
 */
int inotify_init1(int flags) {
    (void)flags;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Add a watch to an initialized inotify instance.
 */
int inotify_add_watch(int fd, const char *pathname, uint32_t mask) {
    (void)fd;
    (void)pathname;
    (void)mask;
    errno = ENOSYS;
    return -1;
}

/**
 * @brief Remove an existing watch from an inotify instance.
 */
int inotify_rm_watch(int fd, int wd) {
    (void)fd;
    (void)wd;
    errno = ENOSYS;
    return -1;
}
