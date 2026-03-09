#include <sys/stat.h>
#include <errno.h>
#include <stddef.h>
#include "kernel/vfs.h"
#include "kernel/process.h"

/* =========================================================================
 * POSIX File Status (<sys/stat.h>)
 * ========================================================================= */

/**
 * @brief Retrieves information about a file based on its string path.
 * @param pathname The path to the file.
 * @param statbuf The structure to populate with file info.
 * @return 0 on success, -1 on error.
 */
int stat(const char *pathname, struct stat *statbuf) {
    if (!pathname || !statbuf) {
        errno = EINVAL;
        return -1;
    }

    /* Ask the VFS to find the exact node based on the path */
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), pathname);

    if (!node) {
        errno = ENOENT;
        return -1;
    }

    /* Populate the standard POSIX struct stat using our VFS function */
    return flx_vfs_stat(node, statbuf);
}

/**
 * @brief Retrieves information about an open file descriptor.
 * @param fd The file descriptor.
 * @param statbuf The structure to populate with file info.
 * @return 0 on success, -1 on error.
 */
int fstat(int fd, struct stat *statbuf) {
    if (!statbuf) {
        errno = EINVAL;
        return -1;
    }

    /* Check if it is an open device descriptor (like UART for stdin/stdout) */
    FlxDev_t *dev = flx_proc_dev_get(fd);
    if (dev) {
        statbuf->st_mode = S_IFCHR;
        return 0;
    }

    /* TODO: Add check for regular file descriptors once VFS files are openable */

    errno = EBADF;
    return -1;
}
