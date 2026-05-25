/**
 * @file felix/fs/flx_fs_ramfs.h
 * @brief Generic RAM Filesystem Driver for dynamic in-memory nodes
 */
#ifndef _FLX_FS_RAMFS_H
#define _FLX_FS_RAMFS_H

#include "kernel/vfs.h"

/* The maximum number of children per RAMFS instance (e.g., max files in /dev) */
#define FLX_RAMFS_MAX_CHILDREN 64

/**
 * @brief Instance structure representing a single RAMFS directory
 */
typedef struct {
    FlxVfsNode_t node;                                     /* The root node of this RAMFS instance */
    FlxVfsNode_t child_nodes[FLX_RAMFS_MAX_CHILDREN];      /* The physical memory pool for child nodes */
    FlxVfsNode_t *child_ptrs[FLX_RAMFS_MAX_CHILDREN + 1];  /* Null-terminated array of pointers for VFS */
    int child_count;                                       /* Current number of allocated children */
} FlxRamFs_t;

extern FlxVfsOps_t g_ramfs_ops;

/**
 * @brief Initializes a new RAMFS instance structure.
 */
void flx_ramfs_init(FlxRamFs_t *ramfs, const char *name, FlxVfsNode_t *parent);

/**
 * @brief Internal helper to allocate a node inside a RAMFS instance.
 * Used by device drivers or internally by ramfs_mkdir.
 */
int flx_ramfs_create_node(FlxRamFs_t *ramfs, const char *name, mode_t mode, dev_t dev_id, const FlxVfsOps_t *ops);

/**
 * @brief Internal helper to free a node from a RAMFS instance.
 */
int flx_ramfs_remove_node(FlxRamFs_t *ramfs, const char *name);

#endif /* _FLX_FS_RAMFS_H */
