#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include <string.h>

#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/binfmt.h"

#define FLX_MAX_ROOT_MOUNTS 8

/* The root node starts empty, waiting for filesystems to mount */
static FlxVfsNode_t *root_children[FLX_MAX_ROOT_MOUNTS + 1];

static FlxVfsNode_t node_root = {
    .name = "/",
    .parent = &node_root,   /* UNIX standard: Root's parent is itself! */
    .mode = S_IFDIR | 0755,
    .dir = { .children = root_children }
};

/* =========================================================================
 * VFS Mount API
 * ========================================================================= */

int flx_vfs_mount(FlxVfsNode_t *fs_node, const char *target_path) {
    if (!fs_node) return -1;

    for (int i = 0; i < FLX_MAX_ROOT_MOUNTS; i++) {
        /* Find the first available mount point */
        if (root_children[i] == NULL) {
            /* Link the new filesystem to the root tree */
            fs_node->parent = &node_root;
            root_children[i] = fs_node;

            /* Shift the NULL terminator down */
            root_children[i + 1] = NULL;

            return 0; /* Success */
        }
    }

    return -1; /* Out of root mount points */
}

/* =========================================================================
 * VFS API Implementation
 * ========================================================================= */

FlxVfsNode_t *flx_vfs_root_init(void) {
    /* Initialize dynamic root array to be empty */
    root_children[0] = NULL;

    return &node_root;
}

FlxVfsNode_t *flx_vfs_root_get(void) {
    return &node_root;
}

/**
 * @brief Retrieves status information about a VFS node.
 * This is the core VFS backend for the stat() syscall.
 */
int flx_vfs_stat(FlxVfsNode_t *node, struct stat *st) {
    if (!node || !st) {
        //return -EINVAL;
        return -1;
    }

    /* Clear the struct to ensure no garbage data is passed to user space */
    memset(st, 0, sizeof(struct stat));

    /* 1. If the node's filesystem provides a custom getattr (like FAT32 or a dynamic /etc), use it! */
    if (node->ops && node->ops->getattr) {
        return node->ops->getattr(node, st);
    }

    /* 2. GENERIC FALLBACK: For 90% of in-memory files, just use the node's static properties */
    st->st_mode = node->mode;

    /* Extract size dynamically for Linked Executables or Data Payloads */
    if (S_ISREG(node->mode)) {
        if (node->data) {
            FlxLinkedExec_t *payload = (FlxLinkedExec_t *)node->data;
            if (payload->size > 0) {
                st->st_size = payload->size; /* Fixed size (like text/config files) */
            } else if (payload->start_ptr && payload->end_ptr) {
                /* Dynamically calculate size using linker-generated boundaries! */
                st->st_size = (size_t)payload->end_ptr - (size_t)payload->start_ptr;
            } else {
                st->st_size = 0;
            }
        } else {
            st->st_size = 0;
        }
    } else {
        /* Directories and devices default to 0 in memory */
        st->st_size = 0;
    }

    /* If it's a character device and relying on the fallback, populate the device ID */
    if (S_ISCHR(node->mode)) {
        st->st_rdev = node->dev_id;
    }

    /* Standard generic values for an in-memory OS */
    st->st_uid = 0; /* root */
    st->st_gid = 0; /* root */

    /* Directories typically have 2 links (themselves, and the parent's link to them) */
    st->st_nlink = S_ISDIR(node->mode) ? 2 : 1;

    /* VERY IMPORTANT FOR FOPEN: C libraries use this for malloc() stream buffering! */
    st->st_blksize = 1024;
    st->st_blocks = (st->st_size + 511) / 512;

    return 0;
}

ssize_t flx_vfs_read(FlxVfsNode_t *node, void *buf, size_t count, off_t offset) {
    if (!node || !buf) return -1;

    /* 1. If the filesystem provides a custom read operation, use it! */
    if (node->ops && node->ops->read) {
        return node->ops->read(node, buf, count, offset);
    }

    /* 2. GENERIC FALLBACK: Read from in-memory payloads */
    if (S_ISREG(node->mode) && node->data) {
        FlxLinkedExec_t *payload = (FlxLinkedExec_t *)node->data;
        const uint8_t *file_start = NULL;
        size_t file_size = 0;

        /* Verify linker boundaries exist */
        if (payload->start_ptr && payload->end_ptr) {
            file_start = (const uint8_t *)payload->start_ptr;
            file_size = (size_t)payload->end_ptr - (size_t)payload->start_ptr;
        }

        if (!file_start) return 0; /* No data (EOF) */
        if (offset >= file_size) return 0; /* Trying to read past EOF */

        /* Calculate how many bytes we can actually read without overflowing */
        size_t available = file_size - offset;
        size_t to_read = (count < available) ? count : available;

        memcpy(buf, file_start + offset, to_read);
        return to_read;
    }

    return -1; /* Not a readable file type */
}
