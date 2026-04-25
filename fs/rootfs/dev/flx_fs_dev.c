/**
 * @file felix/fs/rootfs/dev/flx_fs_dev.c
 * @brief Devfs Filesystem wrapper using the generic RAMFS driver
 */
#include "fs/rootfs/dev/flx_fs_dev.h"
#include "fs/flx_fs_ramfs.h"
#include <sys/stat.h>

#define FLX_MAKEDEV(maj, min) (int)(((maj & 0xFF) << 8) | (min & 0xFF))

static FlxRamFs_t g_dev_fs;

int flx_fs_dev_init(void) {
    flx_ramfs_init(&g_dev_fs, "dev", flx_vfs_root_get());

    flx_fs_root_add_dir(&g_dev_fs.node);

    return 0;
}

int flx_fs_dev_register_chr(const char *name, int major, int minor, FlxVfsOps_t *ops) {
    return flx_ramfs_create_node(&g_dev_fs, name, S_IFCHR | 0666, FLX_MAKEDEV(major, minor), ops);
}

int flx_fs_dev_register_blk(const char *name, int major, int minor, FlxVfsOps_t *ops) {
    return flx_ramfs_create_node(&g_dev_fs, name, S_IFBLK | 0666, FLX_MAKEDEV(major, minor), ops);
}

int flx_fs_dev_unregister(const char *name) {
    return flx_ramfs_remove_node(&g_dev_fs, name);
}
