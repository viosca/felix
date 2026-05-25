/* @file felix/include/kernel/vfs.h
 * @brief Virtual File System Definitions.
 */

#ifndef FELIX_KERNEL_VFS_H
#define FELIX_KERNEL_VFS_H

#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/types.h>
#include "kernel/devices.h"
#include "kernel/binfmt.h"
#include "kernel/kmalloc.h"

typedef struct FlxVfsNode FlxVfsNode_t;

/**
 * @brief Represents an active connection to a VFS Node.
 * Future-proofs the API for a full File Descriptor table.
 */
typedef struct FlxFile {
    FlxVfsNode_t *node; /* The underlying device or file */
    off_t offset;       /* The current read/write position */
    int flags;          /* O_RDONLY, O_NONBLOCK, etc. */
} FlxFile_t;


/**
 * @brief The VFS Operations Table
 */
typedef struct {
    int (*getattr)(FlxVfsNode_t *node, const char *path, struct stat *st);
    int (*open)(FlxVfsNode_t *node, const char *path, int flags, FlxVfsNode_t **out_node);
    ssize_t (*read)(FlxFile_t *file, void *buf, size_t count);
    ssize_t (*write)(FlxFile_t *file, const void *buf, size_t count);
    int (*close)(FlxVfsNode_t *node);
    int (*unmount)(FlxVfsNode_t *node); 
    int (*truncate)(FlxVfsNode_t *node, off_t length);
    int (*readdir)(FlxVfsNode_t *node, struct dirent *entry, int index);
    
    /* POSIX Creation and Deletion Operations */
    int (*mkdir)(FlxVfsNode_t *parent, const char *name, mode_t mode);
    int (*rmdir)(FlxVfsNode_t *parent, const char *name);
    int (*mknod)(FlxVfsNode_t *parent, const char *name, mode_t mode, dev_t dev);
    int (*create)(FlxVfsNode_t *parent, const char *name, mode_t mode);
    int (*unlink)(FlxVfsNode_t *parent, const char *name);
    int (*link)(FlxVfsNode_t *parent, const char *name, FlxVfsNode_t *target);
    int (*rename)(FlxVfsNode_t *node, const char *oldname, const char *newname);
} FlxVfsOps_t;

/* KERNEL INTERNAL FLAGS */
#define FLX_VFS_KFLAG_ALLOCATED_NAME (1 << 0)
#define FLX_VFS_KFLAG_ALLOCATED_NODE (1 << 1)
#define FLX_VFS_KFLAG_OPAQUE_NODE (1 << 2)

struct FlxVfsNode {
    const char *name;
    uint8_t kflags;       
    FlxVfsNode_t *parent;
    struct FlxVfsNode *mount_overlay;
    mode_t mode; 

    union {
        void *data; 
        struct {
            FlxVfsNode_t **children; 
        } dir;
        int dev_id; /* S_IFCHR: Device ID */
    };

    size_t size; /* Added for dynamic file tracking (tmpfs/ramfs) */

    /* Pointer to the filesystem operations table */
    const FlxVfsOps_t *ops;
};

/* =========================================================================
 * Static VFS Tree Macros
 * ========================================================================= */

/**
 * @brief Macro to statically declare a VFS Directory Node.
 */
#define FLX_VFS_DECLARE_DIR(node_var, node_name_str, parent_ptr, children_array) \
    static FlxVfsNode_t node_var = { \
        .name = node_name_str, \
        .kflags = 0, \
        .parent = (parent_ptr), \
        .mode = S_IFDIR | 0755, \
        .dir = { .children = (children_array) }, \
        .ops = NULL \
    }

/**
 * @brief Macro to statically declare a VFS File Node.
 */
#define FLX_VFS_DECLARE_NODE(node_var, node_name_str, parent_ptr, payload_ptr) \
    static FlxVfsNode_t node_var = { \
        .name = node_name_str, \
        .kflags = 0, \
        .parent = (parent_ptr), \
        .mode = S_IFREG | 0555, \
        .data = (void*)(payload_ptr), \
        .ops = NULL \
    }

/**
 * @brief Declares a STANDARD command/file using a hardcoded, explicit size.
 * @param var_name The base name for the generated variables.
 * @param cmd_name The string name of the command (e.g., "ls").
 * @param parent_ptr Pointer to the parent directory node.
 * @param exec_func The main entry point function.
 * @param payload_size The exact size of the payload in bytes.
 */
#define FLX_VFS_DECLARE_CMD(var_name, cmd_name, parent_ptr, exec_func, payload_size) \
    FLX_BINFMT_DECLARE_PAYLOAD(__payload_##var_name, exec_func, payload_size); \
    FLX_VFS_DECLARE_NODE(var_name, cmd_name, parent_ptr, &__payload_##var_name)

/**
 * @brief Declares a LINKED command using automatic linker bounds for dynamic sizing.
 * @param var_name The base name for the generated variables.
 * @param cmd_name The string name of the command (e.g., "ls").
 * @param parent_ptr Pointer to the parent directory node.
 * @param exec_func The main entry point function (also used as the custom section name).
 */
#define FLX_VFS_DECLARE_LINKED_CMD(var_name, cmd_name, parent_ptr, exec_func) \
    FLX_BINFMT_EXTERN_BOUNDS(exec_func); \
    FLX_BINFMT_DECLARE_CMD_PAYLOAD(__payload_##var_name, exec_func, exec_func); \
    FLX_VFS_DECLARE_NODE(var_name, cmd_name, parent_ptr, &__payload_##var_name)

/* --- VFS Core API --- */
void flx_vfs_root_set(FlxVfsNode_t *root);
FlxVfsNode_t *flx_vfs_root_get(void);

FlxVfsNode_t *flx_vfs_path_resolve(FlxVfsNode_t *start_node, const char *path, const char **remainder);

int flx_vfs_mount(FlxVfsNode_t *fs_node, const char *target_path);
int flx_vfs_unmount(const char *target_path);
int flx_vfs_stat(FlxVfsNode_t *node, struct stat *st);
ssize_t flx_vfs_read(FlxVfsNode_t *node, void *buf, size_t count, off_t offset);
ssize_t flx_vfs_write(FlxVfsNode_t *node, const void *buf, size_t count, off_t offset);
int flx_vfs_readdir(FlxVfsNode_t *node, struct dirent *entry, int index);
int flx_vfs_truncate(FlxVfsNode_t *node, off_t length);
char *flx_vfs_get_path(FlxVfsNode_t *node, char *buf, size_t size);

/* POSIX Directory and Link API */
int flx_vfs_mkdir(const char *path, mode_t mode);
int flx_vfs_rmdir(const char *path);
int flx_vfs_create(const char *path, mode_t mode);
int flx_vfs_mknod(const char *path, mode_t mode, dev_t dev);
int flx_vfs_unlink(const char *path);
int flx_vfs_link(const char *oldpath, const char *newpath);
int flx_vfs_rename(const char *oldpath, const char *newpath);

/**
 * @brief Allocates an executable node and binary payload from the global kernel pool.
 * @param name The filename of the command (e.g., "ls")
 * @param main_func Function pointer to the command's entry point
 * @param explicit_size Explicit size (for text/configs, 0 if using linker bounds)
 * @param start_ptr Pointer to the linker section start (NULL if using explicit size)
 * @param end_ptr Pointer to the linker section end (NULL if using explicit size)
 * @return Pointer to the populated VFS Node, or NULL if the pool is full.
 */
FlxVfsNode_t* flx_vfs_create_exec_node(const char *name, int (*main_func)(int argc, char **argv), size_t explicit_size, const void *start_ptr, const void *end_ptr);

/* =========================================================================
 * VFS Node Memory Management
 * ========================================================================= */

/**
 * @brief Safely destructs a generic VFS node's internal union payload.
 * Storage drivers must call this before freeing the node container.
 */
static inline void flx_vfs_node_cleanup(FlxVfsNode_t *node) {
    if (!node) return;

    /* Safely destruct the union payload (Only files have data pointers!) */
    if (S_ISREG(node->mode) && node->data) {
        kfree(node->data);
        node->data = NULL;
    }

    /* Protect the CPU: Only free the name if it lives on the heap! */
    if ((node->kflags & FLX_VFS_KFLAG_ALLOCATED_NAME) && node->name) {
        kfree((void *)node->name);
        node->name = NULL;
    }
}

/**
 * @brief Safely initializes a generic VFS node, handling internal union routing
 * based on the POSIX file mode.
 */
static inline void flx_vfs_node_init_base(FlxVfsNode_t *node, const char *name, mode_t mode, uint32_t kflags, FlxVfsNode_t *parent, const FlxVfsOps_t *ops, dev_t dev_id) {
    node->name = name;
    node->mode = mode;
    node->kflags = kflags; /* Inject the flags here! */
    node->parent = parent;
    node->ops = ops;
    node->size = 0;

    if (S_ISCHR(mode) || S_ISBLK(mode)) {
        node->dev_id = dev_id;
    } else if (S_ISDIR(mode)) {
        node->dir.children = NULL;
    } else {
        node->data = NULL;
    }
}

/* =========================================================================
 * VFS Filesystem Registry API
 * ========================================================================= */

/**
 * @brief Defines a mountable filesystem driver
 */
typedef struct FlxFsType {
    const char *name;
    /* The driver initializes the hardware and returns its root node via out_node.
       Returns 0 on success, or a negative POSIX error code on failure. */
    int (*mount)(const char *source, unsigned long mountflags, const void *data, FlxVfsNode_t **out_node);
    struct FlxFsType *next;
} FlxFsType_t;

/**
 * @brief Registers a new filesystem driver with the kernel.
 */
int flx_vfs_register_fs(FlxFsType_t *fs);

/**
 * @brief Locates a registered filesystem by name and calls its mount hook.
 */
int flx_vfs_mount_type(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data);

#endif /* FELIX_KERNEL_VFS_H */
