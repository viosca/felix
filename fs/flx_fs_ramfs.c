/**
 * @file felix/fs/flx_fs_ramfs.c
 * @brief Generic RAM Filesystem Implementation (tmpfs)
 */
#include "fs/flx_fs_ramfs.h"
#include "kernel/kmalloc.h"
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Forward declaration for ops table */
extern FlxVfsOps_t g_ramfs_ops;

/* =========================================================================
 * VFS Directory & Node Operations
 * ========================================================================= */

static int ramfs_mkdir(FlxVfsNode_t *parent, const char *name, mode_t mode) {
    if (!parent) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent;
    return flx_ramfs_create_node(ramfs, name, S_IFDIR | mode, 0, &g_ramfs_ops);
}

static int ramfs_create(FlxVfsNode_t *parent, const char *name, mode_t mode) {
    if (!parent) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent;
    return flx_ramfs_create_node(ramfs, name, S_IFREG | mode, 0, &g_ramfs_ops);
}

static int ramfs_mknod(FlxVfsNode_t *parent, const char *name, mode_t mode, dev_t dev) {
    if (!parent) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent;
    return flx_ramfs_create_node(ramfs, name, mode, dev, &g_ramfs_ops);
}

static int ramfs_unlink(FlxVfsNode_t *parent, const char *name) {
    if (!parent) return -EINVAL;
    FlxRamFs_t *ramfs = (FlxRamFs_t *)parent;
    return flx_ramfs_remove_node(ramfs, name);
}

/* =========================================================================
 * VFS File Storage Operations
 * ========================================================================= */

static ssize_t ramfs_read(FlxVfsFile_t *file, void *buf, size_t count) {
    FlxVfsNode_t *node = file->node;
    if (!S_ISREG(node->mode)) return -EISDIR;

    if (file->offset >= node->size) return 0; /* EOF */

    size_t available = node->size - file->offset;
    size_t to_read = (count < available) ? count : available;

    memcpy(buf, (uint8_t*)node->data + file->offset, to_read);
    file->offset += to_read;

    return to_read;
}

static ssize_t ramfs_write(FlxVfsFile_t *file, const void *buf, size_t count) {
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

static int ramfs_truncate(FlxVfsNode_t *node, const char *path, off_t length) {
    (void)path; /* Path is already fully resolved for static RAMFS nodes */

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

    ramfs->node.name = kstrdup(name);
    ramfs->node.mode = S_IFDIR | 0777;
    ramfs->node.parent = parent;
    ramfs->node.ops = &g_ramfs_ops;

    /* SURGICAL FIX 3: We exclusively use dir.children. Do NOT set node.data! */
    ramfs->node.dir.children = ramfs->child_ptrs;
    ramfs->child_ptrs[0] = NULL;
}

int flx_ramfs_create_node(FlxRamFs_t *ramfs, const char *name, mode_t mode, dev_t dev_id, const FlxVfsOps_t *ops) {
    /* Check bounds before allocating */
    if (ramfs->child_count >= FLX_RAMFS_MAX_CHILDREN)
        return -ENOMEM;

    /* Ensure node doesn't already exist */
    for (int i = 0; i < ramfs->child_count; i++) {
        if (ramfs->child_ptrs[i] && strcmp(ramfs->child_ptrs[i]->name, name) == 0) {
            return -EEXIST;
        }
    }

    /* =========================================================================
     * DIRECTORY CREATION
     * Bypasses the memory pool entirely. Creates a new RAMFS container and
     * links its internal struct-aligned node directly into the tree.
     * ========================================================================= */
    if (S_ISDIR(mode)) {
        FlxRamFs_t *nested_fs = kmalloc(sizeof(FlxRamFs_t));
        if (!nested_fs) return -ENOMEM;
        
        /* Initialize the new container (Sets up the node internally) */
        flx_ramfs_init(nested_fs, name, &ramfs->node);
        
        /* If flx_ramfs_init dynamically duplicated the name, we must flag it! */
        nested_fs->node.kflags |= FLX_VFS_KFLAG_ALLOCATED_NAME;

        /* Inject the container's internal node directly into the VFS tree */
        ramfs->child_ptrs[ramfs->child_count++] = &nested_fs->node;
        ramfs->child_ptrs[ramfs->child_count] = NULL;

        return 0;
    }

    /* =========================================================================
     * REGULAR FILE & DEVICE CREATION
     * Uses the parent's pre-allocated memory pool.
     * ========================================================================= */
    int free_idx = -1;
    for (int i = 0; i < FLX_RAMFS_MAX_CHILDREN; i++) {
        if (ramfs->child_nodes[i].name == NULL) {
            free_idx = i;
            break;
        }
    }
    if (free_idx == -1) return -ENOMEM;

    /* Let the VFS Firewall safely handle the union routing */
    flx_vfs_node_init_base(
        &ramfs->child_nodes[free_idx],
        kstrdup(name),                             /* Dynamically allocate the name string */
        mode,
        FLX_VFS_KFLAG_ALLOCATED_NAME,              /* Tell the VFS Destructor to free() this string later */
        &ramfs->node,                              /* Parent linkage */
        ops,
        dev_id                                     /* VFS logic routes this ONLY if S_ISCHR/S_ISBLK */
    );

    /* Append to the directory traversal array */
    ramfs->child_ptrs[ramfs->child_count++] = &ramfs->child_nodes[free_idx];
    ramfs->child_ptrs[ramfs->child_count] = NULL;

    return 0;
}

int flx_ramfs_remove_node(FlxRamFs_t *ramfs, const char *name) {
    for (int i = 0; i < ramfs->child_count; i++) {
        if (ramfs->child_ptrs[i] && strcmp(ramfs->child_ptrs[i]->name, name) == 0) {
            FlxVfsNode_t *target = ramfs->child_ptrs[i];

            /* Prevent removing non-empty directories */
            if (S_ISDIR(target->mode)) {
                FlxRamFs_t *nested_fs = (FlxRamFs_t *)target;
                if (nested_fs->child_count > 0) return -ENOTEMPTY;
            }

            /* Delegate all union payload and string memory cleanup to the VFS firewall */
            flx_vfs_node_cleanup(target);

            /* If it was a dynamically allocated RAMFS container (Directory), free it! */
            if (S_ISDIR(target->mode)) {
                kfree(target); /* Free the malloc'd FlxRamFs_t container */
            }
            /* Note: Regular files/devices live in the pre-allocated child_nodes array
             * inside the parent RAMFS struct, so we NEVER kfree() the target pointer itself here */

            /* 4. Shift ONLY the VFS pointers to close the gap in the tree */
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
