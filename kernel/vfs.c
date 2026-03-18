#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include "kernel/vfs.h"
#include "kernel/process.h"

/* =========================================================================
 * Core VFS Root Manager
 * ========================================================================= */

#define FLX_MAX_ROOT_MOUNTS 8

/* The root node starts completely empty, waiting for filesystems to mount */
static FlxVfsNode_t *root_children[FLX_MAX_ROOT_MOUNTS + 1];

static FlxVfsNode_t node_root = {
    .name = "/",
    .parent = &node_root,   /* UNIX standard: Root's parent is itself! */
    .mode = S_IFDIR | 0755,
    .size = 0,
    .dir = { .children = root_children }
};

/* =========================================================================
 * VFS Mount API
 * ========================================================================= */

int flx_vfs_mount(FlxVfsNode_t *fs_node, const char *target_path) {
    if (!fs_node) return -1;

    for (int i = 0; i < FLX_MAX_ROOT_MOUNTS; i++) {
        /* Find the first available mount point */
        if (root_children[i] == NULL) {
            /* Link the new filesystem to the root tree */
            fs_node->parent = &node_root;
            root_children[i] = fs_node;

            /* Shift the NULL terminator down */
            root_children[i + 1] = NULL;

            return 0; /* Success */
        }
    }

    return -1; /* Out of root mount points */
}

/* =========================================================================
 * VFS API Implementation
 * ========================================================================= */

FlxVfsNode_t *flx_vfs_root_init(void) {
    /* Initialize the root directory to be empty */
    root_children[0] = NULL;
    return &node_root;
}

FlxVfsNode_t *flx_vfs_root_get(void) {
    return &node_root;
}

int flx_vfs_stat(FlxVfsNode_t *node, struct stat *st) {
    if (!node || !st) return -1;
    memset(st, 0, sizeof(struct stat));

    st->st_mode = node->mode;
    st->st_size = node->size;

    /* Expose Major/Minor numbers to the POSIX layer via st_rdev */
    if (S_ISCHR(node->mode) || S_ISBLK(node->mode)) {
        st->st_rdev = (dev_t)((uintptr_t)node->data);
    }

    return 0;
}
