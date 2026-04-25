/* @file felix/kernel/vfs.c
 */
#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>

#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/binfmt.h"
#include "kernel/kmalloc.h"

/* Helper macro to determine if a node manages its own internal filesystem tree.
 * Tests for the explicit OPAQUE flag, or falls back to checking for an .open hook. */
#define IS_OPAQUE(node) (((node)->kflags & FLX_VFS_KFLAG_OPAQUE_NODE) || ((node)->ops && (node)->ops->open))

static FlxVfsNode_t *global_root_node = NULL;

void flx_vfs_root_set(FlxVfsNode_t *root) {
    global_root_node = root;
}

FlxVfsNode_t *flx_vfs_root_get(void) {
    return global_root_node;
}

/* =========================================================================
 * VFS Filesystem Registry
 * ========================================================================= */

static FlxFsType_t *registered_filesystems = NULL;

int flx_vfs_register_fs(FlxFsType_t *fs) {
    if (!fs || !fs->name || !fs->mount) return -EINVAL;
    
    fs->next = registered_filesystems;
    registered_filesystems = fs;
    return 0;
}

int flx_vfs_mount_type(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
    /* Note: 'source' can legitimately be NULL for pseudo-filesystems 
     * (like procfs or tmpfs) that do not have a physical backing device. 
     * The specific filesystem driver will validate 'source' if it needs one. */
    if (!filesystemtype || !target) return -EINVAL;

    FlxFsType_t *fs = registered_filesystems;
    
    while (fs) {
        if (strcmp(fs->name, filesystemtype) == 0) {
            FlxVfsNode_t *fs_node = NULL;
            int err = fs->mount(source, mountflags, data, &fs_node);
            if (err < 0) return err;
            if (!fs_node) return -ENOMEM; /* Failsafe */
            
            int ret = flx_vfs_mount(fs_node, target);
            if (ret < 0) {
                if (fs_node->ops && fs_node->ops->unmount) fs_node->ops->unmount(fs_node);
                if (fs_node->kflags & FLX_VFS_KFLAG_ALLOCATED_NAME) {
                    kfree((void *)fs_node->name);
                }
                
                if (fs_node->kflags & FLX_VFS_KFLAG_ALLOCATED_NODE) {
                    kfree(fs_node);
                }
            }
            return ret;
        }
        fs = fs->next;
    }

    return -ENODEV;
}

/* =========================================================================
 * VFS Mount API
 * ========================================================================= */

int flx_vfs_mount(FlxVfsNode_t *fs_node, const char *target_path) {
    if (!fs_node || !target_path) return -EINVAL;

    const char *remainder = NULL;
    FlxVfsNode_t *target_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), target_path, &remainder);
    
    /* Guard: We can only mount to static directories. 
     * If remainder has text, the target is inside a dynamic FS! */
    if (!target_node || (remainder && *remainder != '\0')) return -ENOENT;
    if (!S_ISDIR(target_node->mode)) return -ENOTDIR;
    if (target_node->mount_overlay) return -EBUSY;

    target_node->mount_overlay = fs_node;
    
    /* Inherit the parent directory so things like ".." resolve correctly 
     * out of the mounted filesystem */
    fs_node->parent = target_node->parent;

    return 0;
}

int flx_vfs_unmount(const char *target_path) {
    if (!target_path) return -EINVAL;

    const char *remainder = NULL;
    /* This resolves to the top-level overlay node */
    FlxVfsNode_t *target_node = flx_vfs_path_resolve(flx_proc_cwd_node_get(), target_path, &remainder);
    
    /* Guard: Target must fully resolve to a static mount point */
    if (!target_node || (remainder && *remainder != '\0')) return -ENOENT;

    /* Verify this is actually a mounted overlay by finding the underlying node */
    bool is_overlay = false;
    
    /* SAFETY FIX: Ensure we only scan dir.children if the parent is transparent! */
    if (target_node->parent && S_ISDIR(target_node->parent->mode) && !IS_OPAQUE(target_node->parent)) {
        FlxVfsNode_t **children = target_node->parent->dir.children;
        if (children) {
            for (int i = 0; children[i] != NULL; i++) {
                if (children[i]->mount_overlay == target_node) {
                    /* Found the original node hiding underneath! Undo the overlay. */
                    children[i]->mount_overlay = NULL;
                    is_overlay = true;
                    break;
                }
            }
        }
    }

    if (!is_overlay) {
        /* If it's not an overlay, it's a built-in static node like /bin or /.
         * We refuse to unmount static kernel structures. */
        return -EINVAL;
    }

    if (target_node->ops && target_node->ops->unmount) {
        int res = target_node->ops->unmount(target_node);
        if (res < 0) return res; 
    }

    if (target_node->kflags & FLX_VFS_KFLAG_ALLOCATED_NAME) {
        kfree((void *)target_node->name);
    }
    
    /* Only free the node if it was actually heap-allocated! */
    if (target_node->kflags & FLX_VFS_KFLAG_ALLOCATED_NODE) {
        kfree(target_node);
    }

    return 0;
}

/* =========================================================================
 * Path Resolution & Traversal
 * ========================================================================= */

char *flx_vfs_get_path(FlxVfsNode_t *node, char *buf, size_t size) {
    if (!node || !buf || size == 0) return NULL;

    if (node->parent == node || !node->parent) {
        strncpy(buf, "/", size);
        return buf;
    }

    char *ptr = buf + size - 1;
    *ptr = '\0';

    FlxVfsNode_t *curr = node;
    while (curr && curr->parent != curr && curr->name) {
        size_t len = strlen(curr->name);
        if (ptr - buf < (ptrdiff_t)(len + 1)) return NULL;

        ptr -= len;
        memcpy(ptr, curr->name, len);
        ptr--;
        *ptr = '/';

        curr = curr->parent;
    }

    memmove(buf, ptr, (buf + size) - ptr);
    return buf;
}

FlxVfsNode_t *flx_vfs_path_resolve(FlxVfsNode_t *start_node, const char *path, const char **remainder) {
    if (!path || *path == '\0') {
        if (remainder) *remainder = NULL;
        return NULL;
    }

    FlxVfsNode_t *current = start_node ? start_node : global_root_node;

    if (*path == '/') {
        current = global_root_node;
        path++;
    }

    while (*path != '\0') {
        const char *end = path;
        while (*end != '/' && *end != '\0') end++;

        size_t len = end - path;

        if (len == 0 || (len == 1 && path[0] == '.')) {
            /* Ignore */
        } else if (len == 2 && path[0] == '.' && path[1] == '.') {
            if (current->parent) current = current->parent;
        } else {
            /* Traverse INTO mount overlays before checking children! */
            if (current->mount_overlay) current = current->mount_overlay;

            if (!S_ISDIR(current->mode)) {
                if (remainder) *remainder = path;
                return NULL;
            }

            /* STOP TRAVERSAL: If the node is Opaque, the driver handles the rest!
             * The union holds driver context (node->data), NOT a children array. */
            if (IS_OPAQUE(current)) {
                if (remainder) *remainder = path;
                return current;
            }

            FlxVfsNode_t **children = current->dir.children;
            FlxVfsNode_t *found = NULL;

            if (children) {
                for (int i = 0; children[i] != NULL; i++) {
                    if (strlen(children[i]->name) == len &&
                        strncmp(children[i]->name, path, len) == 0) {
                        found = children[i];
                        break;
                    }
                }
            }

            if (!found) {
                if (remainder) *remainder = path;
                return current;
            }
            current = found;
        }

        path = end;
        if (*path == '/') path++;
    }

    /* Final overlay check if the destination itself is a mount point */
    if (current && current->mount_overlay) {
        current = current->mount_overlay;
    }

    if (remainder) *remainder = NULL; /* Fully resolved statically */
    return current;
}

/* =========================================================================
 * VFS I/O & Stat Implementation
 * ========================================================================= */

int flx_vfs_stat(FlxVfsNode_t *node, struct stat *st) {
    if (!node || !st) return -EINVAL;

    memset(st, 0, sizeof(struct stat));

    /* Ops->getattr is now called in sys_stat directly so it can pass the remainder path!
     * This generic fallback is for in-memory nodes only. */
    
    st->st_mode = node->mode;
    st->st_size = node->size;

    /* Ensure we only cast node->data to FlxLinkedExec_t if it's a built-in static node!
     * (!node->ops guarantees it isn't an opaque driver context or RAMFS node) */
    if (S_ISREG(node->mode) && !node->ops && node->data) {
        FlxLinkedExec_t *payload = (FlxLinkedExec_t *)node->data;
        if (payload->start_ptr && payload->end_ptr) {
            st->st_size = (size_t)payload->end_ptr - (size_t)payload->start_ptr;
        }
    }

    if (S_ISCHR(node->mode) || S_ISBLK(node->mode)) {
        st->st_rdev = node->dev_id;
    }

    st->st_uid = 0;
    st->st_gid = 0;
    st->st_nlink = S_ISDIR(node->mode) ? 2 : 1;
    st->st_blksize = 1024;
    st->st_blocks = (st->st_size + 511) / 512;

    return 0;
}

ssize_t flx_vfs_read(FlxVfsNode_t *node, void *buf, size_t count, off_t offset) {
    if (!node || !buf) return -EFAULT;

    if (node->ops && node->ops->read) {
        /* Pack the transient offset state into a FlxFile_t before calling the driver */
        FlxFile_t temp_file = {
            .node = node,
            .offset = offset,
            .flags = 0 /* Default for simple path operations */
        };
        return node->ops->read(&temp_file, buf, count);
    }

    /* SAFETY FIX: Fallback for static in-memory executable nodes only. 
     * !node->ops ensures we don't accidentally cast opaque driver memory. */
    if (S_ISREG(node->mode) && !node->ops && node->data) {
        FlxLinkedExec_t *payload = (FlxLinkedExec_t *)node->data;
        const uint8_t *file_start = NULL;
        size_t file_size = 0;

        if (payload->start_ptr && payload->end_ptr) {
            file_start = (const uint8_t *)payload->start_ptr;
            file_size = (size_t)payload->end_ptr - (size_t)payload->start_ptr;
        }

        if (!file_start) return 0; 
        if (offset >= file_size) return 0; 

        size_t available = file_size - offset;
        size_t to_read = (count < available) ? count : available;

        memcpy(buf, file_start + offset, to_read);
        return to_read;
    }

    return -1; 
}

ssize_t flx_vfs_write(FlxVfsNode_t *node, const void *buf, size_t count, off_t offset) {
    if (!node || !buf) return -EFAULT;

    if (node->ops && node->ops->write) {
        /* Pack the transient offset state into a FlxFile_t before calling the driver */
        FlxFile_t temp_file = {
            .node = node,
            .offset = offset,
            .flags = 0 /* Default for simple path operations */
        };
        return node->ops->write(&temp_file, buf, count);
    }

    return -EROFS;
}

/* =========================================================================
 * POSIX Directory and Link Operations
 * ========================================================================= */

int flx_vfs_mkdir(const char *path, mode_t mode) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_vfs_root_get(), path, &remainder);

    if (!node) return -ENOENT;
    if (!remainder || *remainder == '\0') return -EEXIST;

    /* If this is the static RAMFS, standard mkdir doesn't do "mkdir -p"
     * If the remainder has a slash, an intermediate dir is missing. */
    if (!node->ops && strchr(remainder, '/')) return -ENOENT;

    /* Pass the remainder path to the filesystem driver */
    if (node->ops && node->ops->mkdir) {
        return node->ops->mkdir(node, remainder, mode);
    }

    return -EROFS; /* Filesystem is read-only or doesn't support dynamic creation */
}

int flx_vfs_create(const char *path, mode_t mode) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_vfs_root_get(), path, &remainder);

    if (!node) return -ENOENT;
    if (!remainder || *remainder == '\0') return -EEXIST;
    if (!node->ops && strchr(remainder, '/')) return -ENOENT;

    if (node->ops && node->ops->create) {
        return node->ops->create(node, remainder, mode);
    }
    return -EROFS;
}

int flx_vfs_mknod(const char *path, mode_t mode, dev_t dev) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(flx_vfs_root_get(), path, &remainder);

    if (!node) return -ENOENT;
    if (!remainder || *remainder == '\0') return -EEXIST;
    if (!node->ops && strchr(remainder, '/')) return -ENOENT;

    if (node->ops && node->ops->mknod) {
        return node->ops->mknod(node, remainder, mode, dev);
    }
    return -EROFS;
}

int flx_vfs_rmdir(const char *path) {
    const char *remainder = NULL;
    FlxVfsNode_t *target = flx_vfs_path_resolve(flx_vfs_root_get(), path, &remainder);

    if (!target) return -ENOENT;

    /* Dynamic Filesystem Routing (e.g., FatFs) */
    if (remainder && *remainder != '\0') {
        if (target->ops && target->ops->rmdir) {
            return target->ops->rmdir(target, remainder);
        }
        return -EROFS;
    }

    /* Static RAMFS routing */
    FlxVfsNode_t *parent = target->parent;
    if (!parent) return -EBUSY; /* Cannot remove root */

    if (parent->ops && parent->ops->rmdir) {
        return parent->ops->rmdir(parent, target->name);
    }
    return -EROFS;
}

int flx_vfs_unlink(const char *path) {
    const char *remainder = NULL;
    FlxVfsNode_t *target = flx_vfs_path_resolve(flx_vfs_root_get(), path, &remainder);

    if (!target) return -ENOENT;

    /* Dynamic Filesystem Routing (e.g., FatFs) */
    if (remainder && *remainder != '\0') {
        if (target->ops && target->ops->unlink) {
            return target->ops->unlink(target, remainder);
        }
        return -EROFS;
    }

    /* Static RAMFS routing */
    FlxVfsNode_t *parent = target->parent;
    if (!parent) return -EBUSY;

    if (parent->ops && parent->ops->unlink) {
        return parent->ops->unlink(parent, target->name);
    }
    return -EROFS;
}

int flx_vfs_link(const char *oldpath, const char *newpath) {
    const char *old_rem = NULL;
    FlxVfsNode_t *target = flx_vfs_path_resolve(flx_vfs_root_get(), oldpath, &old_rem);
    if (!target || (old_rem && *old_rem != '\0')) return -ENOENT;

    const char *new_rem = NULL;
    FlxVfsNode_t *new_parent = flx_vfs_path_resolve(flx_vfs_root_get(), newpath, &new_rem);
    if (!new_parent) return -ENOENT;
    if (!new_rem || *new_rem == '\0') return -EEXIST;

    if (new_parent->ops && new_parent->ops->link) {
        return new_parent->ops->link(new_parent, new_rem, target);
    }
    return -EROFS;
}

int flx_vfs_rename(const char *oldpath, const char *newpath) {
    const char *old_rem = NULL;
    const char *new_rem = NULL;

    FlxVfsNode_t *old_target = flx_vfs_path_resolve(flx_vfs_root_get(), oldpath, &old_rem);
    FlxVfsNode_t *new_target = flx_vfs_path_resolve(flx_vfs_root_get(), newpath, &new_rem);

    if (!old_target || !new_target) return -ENOENT;

    /* Check for cross-device rename attempt (e.g., moving from /mnt/sd0 to /bin) */
    if (old_target != new_target) {
        return -EXDEV; /* POSIX standard: Invalid cross-device link */
    }

    if (old_target->ops && old_target->ops->rename) {
        return old_target->ops->rename(old_target, old_rem, new_rem);
    }

    return -EROFS;
}

int flx_vfs_readdir(FlxVfsNode_t *node, struct dirent *entry, int index) {
    if (!node || !entry) return -EINVAL;
    if (!S_ISDIR(node->mode)) return -ENOTDIR;

    if (node->ops && node->ops->readdir) {
        int ret = node->ops->readdir(node, entry, index);
        if (ret == 0) return 1; 
        return 0; 
    }

    /* SAFETY FIX: If the node is OPAQUE but lacked a readdir operation, 
     * we cannot safely scan its union data as a children array! */
    if (IS_OPAQUE(node)) {
        return 0; /* EOF */
    }

    if (index == 0) {
        entry->d_ino = 1;
        strncpy(entry->d_name, ".", sizeof(entry->d_name) - 1);
        entry->d_name[sizeof(entry->d_name) - 1] = '\0';
        return 1;
    }

    if (index == 1) {
        entry->d_ino = 1;
        strncpy(entry->d_name, "..", sizeof(entry->d_name) - 1);
        entry->d_name[sizeof(entry->d_name) - 1] = '\0';
        return 1;
    }

    int physical_index = index - 2;
    FlxVfsNode_t **children = node->dir.children;

    if (!children || !children[physical_index]) {
        return 0; /* EOF */
    }

    FlxVfsNode_t *child = children[physical_index];

    entry->d_ino = 1; 
    strncpy(entry->d_name, child->name, sizeof(entry->d_name) - 1);
    entry->d_name[sizeof(entry->d_name) - 1] = '\0';

    return 1;
}

/* =========================================================================
 * Helpers
 * ========================================================================= */
#if 0
FlxVfsNode_t* flx_vfs_create_exec_node(const char *name, int (*main_func)(int argc, char **argv), size_t explicit_size, const void *start_ptr, const void *end_ptr) {
    /* Basic allocator for dynamic executable nodes utilizing kernel heap */
    FlxVfsNode_t *node = kmalloc(sizeof(FlxVfsNode_t));
    if (!node) return NULL;
    
    memset(node, 0, sizeof(FlxVfsNode_t));
    node->name = strdup(name);
    node->kflags = FLX_VFS_KFLAG_ALLOCATED_NAME | FLX_VFS_KFLAG_ALLOCATED_NODE;
    node->mode = S_IFREG | 0555; /* Executable */
    
    if (explicit_size > 0) {
        node->size = explicit_size;
    } else if (start_ptr && end_ptr) {
        node->size = (size_t)((const uint8_t*)end_ptr - (const uint8_t*)start_ptr);
    }

    return node;
}
#endif
