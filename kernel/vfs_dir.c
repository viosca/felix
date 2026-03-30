#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "kernel/vfs.h"
#include "kernel/process.h"

/* =========================================================================
 * VFS Directory Stream Subsystem (Zero Heap Allocations!)
 * ========================================================================= */

int flx_vfs_opendir(const char *name) {
    FlxVfsNode_t *target_node;

    if (name == NULL) return -1;

    target_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), name);
    if (!target_node || !S_ISDIR(target_node->mode)) {
        return -1;
    }

    /* Grab a free Directory FD from the process control block */
    int fd = flx_proc_fd_dir_alloc();
    if (fd < 0) return -1;

    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    dir->node = target_node;
    dir->current_index = 0;
    memset(&dir->entry, 0, sizeof(struct dirent));

    return fd;
}

struct dirent *flx_vfs_readdir(int fd) {
    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    if (!dir || !dir->node) return NULL;

    /* Virtual Offset 0: The Current Directory "." */
    if (dir->current_index == 0) {
        dir->entry.d_ino = 1;
        strncpy(dir->entry.d_name, ".", sizeof(dir->entry.d_name) - 1);
        dir->entry.d_name[sizeof(dir->entry.d_name) - 1] = '\0';
        dir->current_index++;
        return &dir->entry;
    }

    /* Virtual Offset 1: The Parent Directory ".." */
    if (dir->current_index == 1) {
        dir->entry.d_ino = 1;
        strncpy(dir->entry.d_name, "..", sizeof(dir->entry.d_name) - 1);
        dir->entry.d_name[sizeof(dir->entry.d_name) - 1] = '\0';
        dir->current_index++;
        return &dir->entry;
    }

    /* Virtual Offset 2+: The Physical Children */
    int physical_index = dir->current_index - 2;
    FlxVfsNode_t **children = dir->node->dir.children;

    if (!children || !children[physical_index]) {
        return NULL; /* Reached the end of the directory array */
    }

    FlxVfsNode_t *child = children[physical_index];

    dir->entry.d_ino = 1; /* Dummy inode number for now */

    strncpy(dir->entry.d_name, child->name, sizeof(dir->entry.d_name) - 1);
    dir->entry.d_name[sizeof(dir->entry.d_name) - 1] = '\0';

    dir->current_index++;

    return &dir->entry;
}

int flx_vfs_closedir(int fd) {
    if (fd < 0) return -1;
    flx_proc_fd_dir_free(fd);
    return 0;
}

/* --- Positional Functions --- */

void flx_vfs_seekdir(int fd, long loc) {
    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    if (dir) dir->current_index = (int)loc;
}

long flx_vfs_telldir(int fd) {
    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    return dir ? (long)dir->current_index : -1;
}

void flx_vfs_rewinddir(int fd) {
    FlxDir_t *dir = flx_proc_fd_dir_get(fd);
    if (dir) dir->current_index = 0;
}
