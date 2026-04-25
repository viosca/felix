/**
 * @file felix/fs/rootfs/tmp/flx_fs_tmp.c
 * @brief Temporary directory instance
 */
#include "fs/rootfs/tmp/flx_fs_tmp.h"
#include "fs/flx_fs_ramfs.h"
#include "fs/rootfs/flx_fs_root.h"

static FlxRamFs_t g_tmp_fs;

int flx_fs_tmp_init(void) {
    /* Initialize the RAMFS container */
    flx_ramfs_init(&g_tmp_fs, "tmp", flx_vfs_root_get());

    /* Dynamically plug the /tmp node into the root filesystem array */
    flx_fs_root_add_dir(&g_tmp_fs.node);
    return 0;
}
