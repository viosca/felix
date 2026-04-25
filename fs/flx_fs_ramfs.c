/**
 * @file felix/fs/flx_fs_ramfs.c
 * @brief Generic RAM Filesystem Implementation (tmpfs)
 */
#include "fs/flx_fs_ramfs.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Forward declaration for ops table */
extern FlxVfsOps_t g_ramfs_ops;

/* =========================================================================
 * VFS Directory & Node Operations
 * ========================================================================= */

static int ramfs_mkdir(FlxVfsNode_t *parent, const char *name, mode_t mode) {
    if (!parent || !parent->data) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent->data;

    /* VFS mkdir requests create standard directories. Pass g_ramfs_ops so children inherit them! */
    return flx_ramfs_create_node(ramfs, name, S_IFDIR | mode, 0, &g_ramfs_ops);
}

static int ramfs_create(FlxVfsNode_t *parent, const char *name, mode_t mode) {
    if (!parent || !parent->data) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent->data;

    /* Create a regular file (S_IFREG) */
    return flx_ramfs_create_node(ramfs, name, S_IFREG | mode, 0, &g_ramfs_ops);
}

static int ramfs_mknod(FlxVfsNode_t *parent, const char *name, mode_t mode, dev_t dev) {
    if (!parent || !parent->data) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent->data;
    return flx_ramfs_create_node(ramfs, name, mode, dev, &g_ramfs_ops);
}

static int ramfs_unlink(FlxVfsNode_t *parent, const char *name) {
    if (!parent || !parent->data) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent->data;
    return flx_ramfs_remove_node(ramfs, name);
}

/* =========================================================================
 * VFS File Storage Operations
 * ========================================================================= */

static ssize_t ramfs_read(FlxFile_t *file, void *buf, size_t count) {
    FlxVfsNode_t *node = file->node;
    if (!S_ISREG(node->mode)) return -EISDIR;

    if (file->offset >= node->size) return 0; /* EOF */

    size_t available = node->size - file->offset;
    size_t to_read = (count < available) ? count : available;

    memcpy(buf, (uint8_t*)node->data + file->offset, to_read);
    file->offset += to_read;

    return to_read;
}

static ssize_t ramfs_write(FlxFile_t *file, const void *buf, size_t count) {
    FlxVfsNode_t *node = file->node;
    if (!S_ISREG(node->mode)) return -EISDIR;

    size_t new_size = file->offset + count;

    /* Grow the file buffer in RAM if necessary */
    if (new_size > node->size) {
        uint8_t *new_data = realloc(node->data, new_size);
        if (!new_data) return -ENOMEM;

        node->data = new_data;
        node->size = new_size;
    }

    memcpy((uint8_t*)node->data + file->offset, buf, count);
    file->offset += count;

    return count;
}

static int ramfs_truncate(FlxVfsNode_t *node, off_t length) {
    if (!S_ISREG(node->mode)) return -EISDIR;
    if (length < 0) return -EINVAL;

    if (length == 0) {
        free(node->data);
        node->data = NULL;
        node->size = 0;
    } else if ((size_t)length != node->size) {
        uint8_t *new_data = realloc(node->data, length);
        if (!new_data) return -ENOMEM;

        /* If expanding, zero out the new memory */
        if ((size_t)length > node->size) {
            memset(new_data + node->size, 0, length - node->size);
        }

        node->data = new_data;
        node->size = length;
    }

    return 0;
}

FlxVfsOps_t g_ramfs_ops = {
    .mkdir    = ramfs_mkdir,
    .rmdir    = ramfs_unlink,  /* Directories and files map to the same unlink logic */
    .mknod    = ramfs_mknod,
    .create   = ramfs_create,
    .unlink   = ramfs_unlink,
    .read     = ramfs_read,
    .write    = ramfs_write,
    .truncate = ramfs_truncate
};

/* =========================================================================
 * Core RAMFS Memory Management
 * ========================================================================= */

void flx_ramfs_init(FlxRamFs_t *ramfs, const char *name, FlxVfsNode_t *parent) {
    memset(ramfs, 0, sizeof(FlxRamFs_t));

    ramfs->node.name = name;
    ramfs->node.mode = S_IFDIR | 0777;
    ramfs->node.parent = parent;
    ramfs->node.ops = &g_ramfs_ops;

    /* Crucial: Link the VFS node back to this RAMFS container so the ops can find it! */
    ramfs->node.data = ramfs;

    ramfs->node.dir.children = ramfs->child_ptrs;
    ramfs->child_ptrs[0] = NULL;
}

int flx_ramfs_create_node(FlxRamFs_t *ramfs, const char *name, mode_t mode, dev_t dev_id, FlxVfsOps_t *ops) {
    if (ramfs->child_count >= FLX_RAMFS_MAX_CHILDREN) return -ENOMEM;

    /* 1. Ensure node doesn't already exist */
    for (int i = 0; i < ramfs->child_count; i++) {
        if (ramfs->child_ptrs[i] && strcmp(ramfs->child_ptrs[i]->name, name) == 0) {
            return -EEXIST;
        }
    }

    /* 2. Find a free struct in the physical memory pool */
    int free_idx = -1;
    for (int i = 0; i < FLX_RAMFS_MAX_CHILDREN; i++) {
        if (ramfs->child_nodes[i].name == NULL) {
            free_idx = i;
            break;
        }
    }
    if (free_idx == -1) return -ENOMEM;

    /* 3. Initialize the stable struct */
    ramfs->child_nodes[free_idx].name = strdup(name); /* Safe copy from user space */
    ramfs->child_nodes[free_idx].mode = mode;
    ramfs->child_nodes[free_idx].parent = &ramfs->node;
    ramfs->child_nodes[free_idx].dev_id = dev_id;
    ramfs->child_nodes[free_idx].ops = ops;

    /* If this is a regular file, start with 0 bytes allocated */
    if (S_ISREG(mode)) {
        ramfs->child_nodes[free_idx].data = NULL;
        ramfs->child_nodes[free_idx].size = 0;
    } else if (S_ISDIR(mode)) {
        /* If it's a nested directory, it needs its own FlxRamFs_t container.
         * For this simple flat RAMFS, we won't fully implement deep recursion yet,
         * but we clear data to prevent dangling pointers. */
         ramfs->child_nodes[free_idx].data = NULL;
    }

    /* 4. Append a pointer to it to the VFS array */
    ramfs->child_ptrs[ramfs->child_count++] = &ramfs->child_nodes[free_idx];
    ramfs->child_ptrs[ramfs->child_count] = NULL; /* Keep array null-terminated */

    return 0;
}

int flx_ramfs_remove_node(FlxRamFs_t *ramfs, const char *name) {
    for (int i = 0; i < ramfs->child_count; i++) {
        if (ramfs->child_ptrs[i] && strcmp(ramfs->child_ptrs[i]->name, name) == 0) {
            FlxVfsNode_t *target = ramfs->child_ptrs[i];

            /* 1. Free file contents if it's a regular file! */
            if (S_ISREG(target->mode) && target->data) {
                free(target->data);
                target->data = NULL;
                target->size = 0;
            }

            /* 2. Free the name string */
            free((void*)target->name);
            target->name = NULL;

            /* 3. Shift ONLY the pointers to close the gap */
            for (int j = i; j < ramfs->child_count - 1; j++) {
                ramfs->child_ptrs[j] = ramfs->child_ptrs[j + 1];
            }

            ramfs->child_count--;
            ramfs->child_ptrs[ramfs->child_count] = NULL;
            return 0;
        }
    }
    return -ENOENT;
}
