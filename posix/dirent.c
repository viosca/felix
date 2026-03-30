/* File: posix/dirent.c */
#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include "kernel/syscalls.h"

/* =========================================================================
 * Directory Stream Overrides (<dirent.h>)
 * ========================================================================= */

/* A pure user-space wrapper to hide the kernel integer FD! */
typedef struct {
    int kernel_dir_fd;
} user_DIR_t;

DIR *opendir(const char *name) {
    if (!name) {
        errno = EINVAL;
        return NULL;
    }

    int dir_fd = sys_opendir(name);
    if (dir_fd < 0) {
        errno = -dir_fd;
        return NULL;
    }

    user_DIR_t *dir = (user_DIR_t *)malloc(sizeof(user_DIR_t));
    if (!dir) {
        sys_closedir(dir_fd);
        errno = ENOMEM;
        return NULL;
    }

    dir->kernel_dir_fd = dir_fd;
    return (DIR *)dir;
}

/* Static buffer since user_DIR_t doesn't natively hold the parsed struct */
static struct dirent g_readdir_entry;

struct dirent *readdir(DIR *dirp) {
    if (!dirp) {
        errno = EBADF;
        return NULL;
    }

    int ret = sys_readdir(((user_DIR_t *)dirp)->kernel_dir_fd, &g_readdir_entry);
    if (ret < 0) {
        errno = -ret;
        return NULL;
    }

    if (ret == 0) {
        return NULL; /* End of directory stream */
    }

    return &g_readdir_entry;
}

int closedir(DIR *dirp) {
    if (!dirp) {
        errno = EBADF;
        return -1;
    }

    int ret = sys_closedir(((user_DIR_t *)dirp)->kernel_dir_fd);
    free(dirp);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

long telldir(DIR *dirp) {
    if (!dirp) {
        errno = EBADF;
        return -1;
    }

    long ret = sys_telldir(((user_DIR_t *)dirp)->kernel_dir_fd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

void seekdir(DIR *dirp, long loc) {
    if (!dirp) return;
    sys_seekdir(((user_DIR_t *)dirp)->kernel_dir_fd, loc);
}

void rewinddir(DIR *dirp) {
    if (!dirp) return;
    sys_rewinddir(((user_DIR_t *)dirp)->kernel_dir_fd);
}
