/* @file felix/kernel/vfs.c
 */
#include <sys/stat.h>
#include <string.h>
#include <stddef.h>
#include <errno.h>
#include <fcntl.h>

#include "kernel/vfs.h"
#include "kernel/process.h"
#include "kernel/binfmt.h"
#include "kernel/kmalloc.h"

/* =========================================================================
 * VFS Macros & Boilerplate Routing
 * ========================================================================= */

/* Helper macro to determine if a node manages its own internal filesystem tree.
 * Tests for the explicit OPAQUE flag, or falls back to checking for an .open hook. */
#define IS_OPAQUE(node) (((node)->kflags & FLX_VFS_KFLAG_OPAQUE_NODE) || ((node)->ops && (node)->ops->open))

/**
 * @brief Handles boilerplate for path-based VFS delegation.
 * If a custom hook exists, it routes to it. If the driver is opaque and 
 * missing the hook, it returns the specified error. Otherwise, it falls through.
 */
#define VFS_ROUTE_PATH(node, path, op_name, opaque_err, ...) \
    do { \
        if (!(node)) return -EINVAL; \
        if ((node)->ops && (node)->ops->op_name) { \
            return (node)->ops->op_name((node), (path), __VA_ARGS__); \
        } \
        if ((path) && *(path) != '\0') return (opaque_err); \
    } while (0)

/**
 * @brief Handles boilerplate for file-descriptor-based VFS delegation.
 * If a custom hook exists, it routes to it. Otherwise, it falls through.
 */
#define VFS_ROUTE_FD(file, op_name, ...) \
    do { \
        if (!(file) || !(file)->node) return -EINVAL; \
        if ((file)->node->ops && (file)->node->ops->op_name) { \
            return (file)->node->ops->op_name((file), __VA_ARGS__); \
        } \
    } while (0)


/* =========================================================================
 * Core State & Filesystem Registry
 * ========================================================================= */

static FlxVfsNode_t *global_root_node = NULL;
static FlxFsType_t *registered_filesystems = NULL;

void flx_vfs_root_set(FlxVfsNode_t *root) {
    global_root_node = root;
}

FlxVfsNode_t *flx_vfs_root_get(void) {
    return global_root_node;
}

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
        const char *name_to_use = curr->name;

        /* Detect if curr is the root of a mounted filesystem.
         * If so, we must use the name of the original mount point instead! */
        if (curr->parent && S_ISDIR(curr->parent->mode) && !IS_OPAQUE(curr->parent) && curr->parent->dir.children) {
            FlxVfsNode_t **children = curr->parent->dir.children;
            for (int i = 0; children[i] != NULL; i++) {
                if (children[i]->mount_overlay == curr) {
                    name_to_use = children[i]->name;
                    break;
                }
            }
        }

        size_t len = strlen(name_to_use);

        /* Skip zero-length names (prevents double slashes '//') */
        if (len > 0) {
            if (ptr - buf < (ptrdiff_t)(len + 1)) return NULL;

            ptr -= len;
            memcpy(ptr, name_to_use, len);
            ptr--;
            *ptr = '/';
        }

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
 * Core I/O (Open, Read, Write)
 * ========================================================================= */

int flx_vfs_open_path(FlxVfsNode_t *base_dir, const char *path, int flags, int mode, FlxVfsNode_t **out_node) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(base_dir, path, &remainder);

    if (!node) return -ENOENT;

    /* 1. Dynamic Filesystem Routing (e.g., FatFs) */
    if (remainder && *remainder != '\0') {
        if (node->ops && node->ops->open) {
            /* The driver handles dynamic O_CREAT internally if passed in flags */
            return node->ops->open(node, remainder, flags, out_node);
        }

        /* 2. Static RAMFS Routing: Try to create the missing file if O_CREAT is set */
        if (flags & O_CREAT) {
            int ret = flx_vfs_create(base_dir, path, mode);
            if (ret < 0 && ret != -EEXIST) return ret;

            /* Re-resolve now that it exists */
            remainder = NULL;
            node = flx_vfs_path_resolve(base_dir, path, &remainder);
            if (!node || (remainder && *remainder != '\0')) return -ENOENT;
        } else {
            return -ENOENT;
        }
    }

    /* At this point, 'node' is fully resolved (Static RAMFS or DevFS file) */
    if (S_ISDIR(node->mode) && ((flags & O_WRONLY) || (flags & O_RDWR))) {
        return -EISDIR; /* POSIX: Cannot open directories for writing */
    }

    /* 3. Handle O_TRUNC for existing static files */
    if ((flags & O_TRUNC) && ((flags & O_WRONLY) || (flags & O_RDWR))) {
         if (node->ops && node->ops->truncate) {
             node->ops->truncate(node, remainder ? remainder : "", 0);
         }
    }

    /* 4. Character/Block Devices might have hardware initialization in their open hook */
    if (node->ops && node->ops->open) {
        int err = node->ops->open(node, NULL, flags, out_node);
        if (err < 0) return err;

        /* If the driver succeeded but didn't allocate a specialized node, use the generic one */
        if (!*out_node) *out_node = node;
        return 0;
    }

    /* Default success for standard RAMFS files */
    *out_node = node;
    return 0;
}

ssize_t flx_vfs_read(FlxVfsNode_t *node, void *buf, size_t count, off_t offset) {
    if (!node || !buf) return -EFAULT;

    if (node->ops && node->ops->read) {
        /* Pack the transient offset state into a FlxFile_t before calling the driver */
        FlxVfsFile_t temp_file = {
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
        FlxVfsFile_t temp_file = {
            .node = node,
            .offset = offset,
            .flags = 0 /* Default for simple path operations */
        };
        return node->ops->write(&temp_file, buf, count);
    }

    return -EROFS;
}

/* =========================================================================
 * Attribute Pipeline (Stat, Chmod, Truncate)
 * ========================================================================= */

int flx_vfs_stat_path(FlxVfsNode_t *base_dir, const char *path, struct stat *st, int flags) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(base_dir, path, &remainder);

    if (!node) return -ENOENT;

    /* Dynamic Filesystem Routing (e.g., FatFs) */
    if (node->ops && node->ops->getattr) {
        /* Note: We drop POSIX 'flags' (like AT_SYMLINK_NOFOLLOW) here for now,
         * as FatFs doesn't support symlinks anyway */
        return node->ops->getattr(node, remainder, st);
    } else if (remainder && *remainder != '\0') {
        return -ENOENT;
    }

    /* Static RAMFS routing */
    if (flx_vfs_stat(node, st) < 0) return -EIO;
    return 0;
}

int flx_vfs_stat(FlxVfsNode_t *node, struct stat *st) {
    if (!node || !st) return -EINVAL;

    memset(st, 0, sizeof(struct stat));

    /* Ops->getattr is now called in sys_stat directly so it can pass the remainder path!
     * This generic fallback is for in-memory nodes only. */
    
    st->st_mode = node->mode;
    st->st_size = node->size;

    if (S_ISDIR(node->mode)) {
        st->st_size = 4096;
    }

    /* Store in-memory node address for ls -i inspection */
    st->st_spare4[0] = (long)(uintptr_t)node;

    /* Ensure we only cast node->data to FlxLinkedExec_t if it's a built-in static node!
     * (!node->ops guarantees it isn't an opaque driver context or RAMFS node) */
    if (S_ISREG(node->mode) && !node->ops && node->data) {
        FlxLinkedExec_t *payload = (FlxLinkedExec_t *)node->data;
        if (payload->start_ptr && payload->end_ptr) {
            st->st_size = (size_t)payload->end_ptr - (size_t)payload->start_ptr;
            st->st_spare4[0] = (long)(uintptr_t)payload->start_ptr;
        } else if (payload->main_func) {
            st->st_spare4[0] = (long)(uintptr_t)payload->main_func;
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

int flx_vfs_fstat(FlxVfsFile_t *file, struct stat *st) {
    if (!file || !file->node || !st) return -EINVAL;

    FlxVfsNode_t *node = file->node;

    /* Fill the baseline attributes using the standard VFS helper */
    int err = flx_vfs_stat(node, st);
    if (err < 0) return err;

    /* Dynamic Filesystem Routing for OPEN files
     * Pass the file object itself so the driver can inspect private state
     * (like a FatFs FIL struct) to get live, un-flushed sizes. */
    if (node->ops && node->ops->fgetattr) {
        return node->ops->fgetattr(file, st);
    }

    return 0;
}

int flx_vfs_chmod(FlxVfsNode_t *node, mode_t mode) {
    if (!node) return -EINVAL;

    /* In-memory RAM node fallback: apply the mode, preserving file type bits */
    node->mode = (node->mode & S_IFMT) | (mode & 07777);
    return 0;
}

int flx_vfs_chmod_path(FlxVfsNode_t *node, const char *path, mode_t mode) {
    /* Route to ops->chmod, return 0 (success stub) if opaque, or fall through */
    VFS_ROUTE_PATH(node, path, chmod, 0, mode);
    return flx_vfs_chmod(node, mode);
}

int flx_vfs_fchmod(FlxVfsFile_t *file, mode_t mode) {
    VFS_ROUTE_FD(file, fchmod, mode);
    return flx_vfs_chmod(file->node, mode);
}

int flx_vfs_truncate(FlxVfsNode_t *node, off_t length) {
    if (!node) return -EINVAL;
    if (S_ISDIR(node->mode)) return -EISDIR; /* Cannot truncate directories */
    if (length < 0) return -EINVAL;

    if (node->ops && node->ops->truncate) {
        return node->ops->truncate(node, "", length);
    }

    node->size = length;
    return 0;
}

int flx_vfs_truncate_path(FlxVfsNode_t *base_dir, const char *path, off_t length) {
    if (!path) return -EINVAL;
    if (length < 0) return -EINVAL;

    const char *remainder = NULL;
    FlxVfsNode_t *target = flx_vfs_path_resolve(base_dir, path, &remainder);
    if (!target) return -ENOENT;

    /* Dynamic Filesystem Routing (e.g., FatFs) */
    if (remainder && *remainder != '\0') {
        if (target->ops && target->ops->truncate) {
            return target->ops->truncate(target, remainder, length);
        }
        return -ENOTSUP;
    }

    return flx_vfs_truncate(target, length);
}

int flx_vfs_ftruncate(FlxVfsFile_t *file, off_t length) {
    VFS_ROUTE_FD(file, ftruncate, length);
    return flx_vfs_truncate(file->node, length);
}

/* =========================================================================
 * Directory & Topology Operations
 * ========================================================================= */

int flx_vfs_mkdir(FlxVfsNode_t *base_dir, const char *path, mode_t mode) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(base_dir, path, &remainder);

    if (!node) return -ENOENT;
    if (!remainder || *remainder == '\0') return -EEXIST;

    /* If this is the static RAMFS, standard mkdir doesn't do "mkdir -p"
     * If the remainder has a slash (that isn't just a trailing slash), an intermediate dir is missing. */
    const char *slash = strchr(remainder, '/');
    if (!node->ops && slash && *(slash + 1) != '\0') return -ENOENT;

    /* Pass the remainder path to the filesystem driver */
    if (node->ops && node->ops->mkdir) {
        return node->ops->mkdir(node, remainder, mode);
    }

    return -EROFS; /* Filesystem is read-only or doesn't support dynamic creation */
}

int flx_vfs_create(FlxVfsNode_t *base_dir, const char *path, mode_t mode) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(base_dir, path, &remainder);

    if (!node) return -ENOENT;
    if (!remainder || *remainder == '\0') return -EEXIST;

    const char *slash = strchr(remainder, '/');
    if (!node->ops && slash && *(slash + 1) != '\0') return -ENOENT;

    if (node->ops && node->ops->create) {
        return node->ops->create(node, remainder, mode);
    }
    return -EROFS;
}

int flx_vfs_mknod(FlxVfsNode_t *base_dir, const char *path, mode_t mode, dev_t dev) {
    const char *remainder = NULL;
    FlxVfsNode_t *node = flx_vfs_path_resolve(base_dir, path, &remainder);

    if (!node) return -ENOENT;
    if (!remainder || *remainder == '\0') return -EEXIST;

    const char *slash = strchr(remainder, '/');
    if (!node->ops && slash && *(slash + 1) != '\0') return -ENOENT;

    if (node->ops && node->ops->mknod) {
        return node->ops->mknod(node, remainder, mode, dev);
    }
    return -EROFS;
}

int flx_vfs_rmdir(FlxVfsNode_t *base_dir, const char *path) {
    const char *remainder = NULL;
    FlxVfsNode_t *target = flx_vfs_path_resolve(base_dir, path, &remainder);

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

int flx_vfs_unlink(FlxVfsNode_t *base_dir, const char *path) {
    const char *remainder = NULL;
    FlxVfsNode_t *target = flx_vfs_path_resolve(base_dir, path, &remainder);

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

int flx_vfs_link(FlxVfsNode_t *old_dir, const char *oldpath, FlxVfsNode_t *new_dir, const char *newpath) {
    const char *old_rem = NULL;
    FlxVfsNode_t *old_target = flx_vfs_path_resolve(old_dir, oldpath, &old_rem);
    if (!old_target) return -ENOENT;

    const char *new_rem = NULL;
    FlxVfsNode_t *new_target = flx_vfs_path_resolve(new_dir, newpath, &new_rem);
    if (!new_target) return -ENOENT;
    if (!new_rem || *new_rem == '\0') return -EEXIST;

    /* Check for cross-device link attempt for opaque file systems */
    if ((old_rem && *old_rem != '\0') || (new_rem && *new_rem != '\0')) {
        if (old_target != new_target) return -EXDEV;
    }

    if (new_target->ops && new_target->ops->link) {
        return new_target->ops->link(old_target, old_rem, new_target, new_rem);
    }
    return -EROFS;
}

int flx_vfs_rename(FlxVfsNode_t *old_dir, const char *oldpath, FlxVfsNode_t *new_dir, const char *newpath) {
    const char *old_rem = NULL;
    const char *new_rem = NULL;

    FlxVfsNode_t *old_target = flx_vfs_path_resolve(old_dir, oldpath, &old_rem);
    FlxVfsNode_t *new_target = flx_vfs_path_resolve(new_dir, newpath, &new_rem);

    if (!old_target || !new_target) return -ENOENT;

    /* Check for cross-device rename attempt (e.g., moving from /mnt/sd0 to /mnt/sd1) */
    if ((old_rem && *old_rem != '\0') || (new_rem && *new_rem != '\0')) {
        if (old_target != new_target) {
            return -EXDEV; /* POSIX standard: Invalid cross-device link */
        }
    }

    if (old_target->ops && old_target->ops->rename) {
        return old_target->ops->rename(old_target, old_rem, new_target, new_rem);
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
