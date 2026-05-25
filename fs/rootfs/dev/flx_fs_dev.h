/**
 * @file felix/fs/rootfs/dev/flx_fs_dev.h
 * @brief Devfs Initialization and Management Wrapper
 */
#ifndef FLX_FS_DEV_H
#define FLX_FS_DEV_H

#include "kernel/vfs.h"

int flx_fs_dev_init(void);
int flx_fs_dev_register_chr(const char *name, int major, int minor, const FlxVfsOps_t *ops);
int flx_fs_dev_register_blk(const char *name, int major, int minor, const FlxVfsOps_t *ops);
int flx_fs_dev_unregister(const char *name);

/* Retrieves the root node of the /dev filesystem so it can be attached to the tree */
FlxVfsNode_t* flx_fs_dev_get_node(void);

#endif /* FLX_FS_DEV_H */
