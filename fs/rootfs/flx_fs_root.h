/**
 * @file felix/fs/rootfs/flx_fs_root.h
 * @brief Root Filesystem Initialization and Management
 */

#ifndef FELIX_FS_ROOT_H
#define FELIX_FS_ROOT_H

#include "kernel/vfs.h"

/**
 * @brief Initializes the static root filesystem (/) and injects it into the VFS.
 * Call this exactly once during boot in felix_init() BEFORE initializing
 * other subsystems like /bin or /dev.
 */
void flx_fs_root_init(void);

/**
 * @brief Safely plugs a new static directory (like /bin or /dev) into the root tree.
 * * @param dir_node Pointer to the statically allocated directory node.
 * @return 0 on success, or -1 if the root directory is full.
 */
int flx_fs_root_add_dir(FlxVfsNode_t *dir_node);

#endif /* FELIX_FS_ROOT_H */
