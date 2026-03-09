#include <dirent.h>
#include <stdlib.h>
#include "kernel/vfs.h"

/* =========================================================================
 * Directory Stream Overrides (<dirent.h>)
 * Maps standard POSIX opendir/readdir to our VFS kernel functions.
 * ========================================================================= */

/* A pure user-space wrapper to hide the kernel integer FD! */
typedef struct {
    int kernel_dir_fd;
} user_DIR_t;

DIR *opendir(const char *name) {
    int dir_fd = flx_vfs_opendir(name);
    if (dir_fd < 0) return NULL;

    user_DIR_t *dir = (user_DIR_t *)malloc(sizeof(user_DIR_t));
    if (!dir) {
        flx_vfs_closedir(dir_fd);
        return NULL;
    }

    dir->kernel_dir_fd = dir_fd;
    return (DIR *)dir;
}

struct dirent *readdir(DIR *dirp) {
    if (!dirp) return NULL;
    return flx_vfs_readdir(((user_DIR_t *)dirp)->kernel_dir_fd);
}

int closedir(DIR *dirp) {
    if (!dirp) return -1;
    int res = flx_vfs_closedir(((user_DIR_t *)dirp)->kernel_dir_fd);
    free(dirp);
    return res;
}

void seekdir(DIR *dirp, long loc) {
    if (dirp) flx_vfs_seekdir(((user_DIR_t *)dirp)->kernel_dir_fd, loc);
}

long telldir(DIR *dirp) {
    if (!dirp) return -1;
    return flx_vfs_telldir(((user_DIR_t *)dirp)->kernel_dir_fd);
}

void rewinddir(DIR *dirp) {
    if (dirp) flx_vfs_rewinddir(((user_DIR_t *)dirp)->kernel_dir_fd);
}
