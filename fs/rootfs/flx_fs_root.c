#include <stddef.h>
#include "kernel/vfs.h"
#include "fs/rootfs/dev/flx_fs_dev.h"
#include "fs/rootfs/mnt/flx_fs_mnt.h"
#include "fs/rootfs/tmp/flx_fs_tmp.h"
#include "fs/rootfs/bin/flx_fs_bin.h"
#include "fs/rootfs/etc/flx_fs_etc.h"
#include "fs/rootfs/test/flx_fs_test.h"

#define FLX_FS_ROOTFS_MAX_ROOT_MOUNTS 8

/* The physical storage for the root directory */
static FlxVfsNode_t *root_children[FLX_FS_ROOTFS_MAX_ROOT_MOUNTS + 1];

static FlxVfsNode_t node_root = {
    .name = "/",
    .parent = &node_root, /* UNIX standard: Root's parent is itself */
    .mode = S_IFDIR | 0755,
    .dir = { .children = root_children },
    .ops = NULL
};

/* Standard empty mount points */
static FlxVfsNode_t *empty_children[] = { NULL };

/**
 * @brief Initializes the root filesystem and injects it into the VFS.
 */
void flx_fs_root_init(void) {
    /* Clear the array */
    for (int i = 0; i <= FLX_FS_ROOTFS_MAX_ROOT_MOUNTS; i++) {
        root_children[i] = NULL;
    }

    flx_vfs_root_set(&node_root);

    /* Populate the Root Filesystem with dynamic and static subsystems */
    flx_fs_dev_init();
    flx_fs_mnt_init();
    flx_fs_tmp_init();

    flx_fs_etc_init();
    flx_fs_bin_init();
    flx_fs_test_init();
}

/**
 * @brief Safely plugs a new static directory into the root tree.
 */
int flx_fs_root_add_dir(FlxVfsNode_t *dir_node) {
    if (!dir_node) return -1;

    for (int i = 0; i < FLX_FS_ROOTFS_MAX_ROOT_MOUNTS; i++) {
        if (root_children[i] == NULL) {
            dir_node->parent = &node_root;
            root_children[i] = dir_node;
            root_children[i + 1] = NULL;
            return 0;
        }
    }
    return -1; /* Out of space */
}
