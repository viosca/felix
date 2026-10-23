/**
 * @file felix/fs/rootfs/dev/flx_fs_dev.c
 * @brief Devfs Filesystem wrapper using the generic RAMFS driver
 */
#include "fs/rootfs/dev/flx_fs_dev.h"
#include "fs/rootfs/flx_fs_root.h"
#include "fs/flx_fs_ramfs.h"
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <string.h>

static FlxRamFs_t g_dev_fs;

/* =========================================================================
 * Standard Memory Character Devices (/dev/null, /dev/zero) - Major 1
 * ========================================================================= */

static ssize_t dev_null_read(FlxVfsFile_t *file, void *buf, size_t count) {
    (void)file;
    (void)buf;
    (void)count;
    return 0; /* Immediate EOF */
}

static ssize_t dev_null_write(FlxVfsFile_t *file, const void *buf, size_t count) {
    (void)file;
    (void)buf;
    return (ssize_t)count; /* Discard all written bytes */
}

static const FlxVfsOps_t g_dev_null_ops = {
    .read  = dev_null_read,
    .write = dev_null_write
};

static ssize_t dev_zero_read(FlxVfsFile_t *file, void *buf, size_t count) {
    (void)file;
    if (buf && count > 0) {
        memset(buf, 0, count);
    }
    return (ssize_t)count;
}

static const FlxVfsOps_t g_dev_zero_ops = {
    .read  = dev_zero_read,
    .write = dev_null_write
};

int flx_fs_dev_init(void) {
    flx_ramfs_init(&g_dev_fs, "dev", flx_vfs_root_get());

    flx_fs_root_add_dir(&g_dev_fs.node);

    /* Register standard POSIX memory character devices (Major 1) */
    flx_fs_dev_register_chr("null", 1, 3, &g_dev_null_ops);
    flx_fs_dev_register_chr("zero", 1, 5, &g_dev_zero_ops);

    return 0;
}

int flx_fs_dev_register_chr(const char *name, int major, int minor, const FlxVfsOps_t *ops) {
    return flx_ramfs_create_node(&g_dev_fs, name, S_IFCHR | 0666, makedev(major, minor), ops);
}

int flx_fs_dev_register_blk(const char *name, int major, int minor, const FlxVfsOps_t *ops) {
    return flx_ramfs_create_node(&g_dev_fs, name, S_IFBLK | 0666, makedev(major, minor), ops);
}

int flx_fs_dev_unregister(const char *name) {
    return flx_ramfs_remove_node(&g_dev_fs, name);
}
