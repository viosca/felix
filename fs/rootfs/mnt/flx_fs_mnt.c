/**
 * @file felix/fs/rootfs/mnt/flx_fs_mnt.c
 * @brief Mount directory instance
 */
#include "fs/rootfs/mnt/flx_fs_mnt.h"
#include "fs/flx_fs_ramfs.h"
#include "fs/rootfs/flx_fs_root.h"

static FlxRamFs_t g_mnt_fs;

int flx_fs_mnt_init(void) {
    /* Initialize the RAMFS container */
    flx_ramfs_init(&g_mnt_fs, "mnt", flx_vfs_root_get());

    /* Dynamically plug the /mnt node into the root filesystem array */
    flx_fs_root_add_dir(&g_mnt_fs.node);
    return 0;
}
