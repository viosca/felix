/**
 * @file vfs.h
 * @brief Virtual File System Definitions.
 */

#ifndef FELIX_KERNEL_VFS_H
#define FELIX_KERNEL_VFS_H

#include <sys/stat.h> /* For POSIX mode_t and struct stat */
#include <dirent.h>   /* For POSIX directory streams */
#include <stdint.h>
#include <sys/types.h> /* For ssize_t and off_t */
#include "kernel/devices.h"
#include "kernel/binfmt.h"

typedef struct FlxVfsNode FlxVfsNode_t;

/**
 * @brief The VFS Operations Table
 * Every filesystem type (devfs, binfs, etc.) defines ONE of these as a const global.
 */
typedef struct {
    int (*getattr)(FlxVfsNode_t *node, struct stat *st);
    int (*open)(FlxVfsNode_t *node, int flags);
    ssize_t (*read)(FlxVfsNode_t *node, void *buf, size_t count, off_t offset);
    ssize_t (*write)(FlxVfsNode_t *node, const void *buf, size_t count, off_t offset);
    int (*close)(FlxVfsNode_t *node);
} FlxVfsOps_t;

/* VFS Node Flags */
#define FLX_VFS_FLAG_ALLOCATED_NAME (1 << 0) /* If set, the VFS must free() the name upon node destruction */
#define FLX_VFS_FLAG_ALLOCATED_NODE (1 << 1) /* If set, the VFS must free() the node structure itself */

struct FlxVfsNode {
    const char *name;     /* Pointer to string (either in .rodata or heap) */
    uint8_t flags;        /* Node lifecycle flags */

    FlxVfsNode_t *parent;

    mode_t mode; /* POSIX file mode/permissions (S_IFREG, S_IFDIR, etc.) */

    /* VFS Tree / Payload Union */
    union {
        void *data; /* S_IFREG: Points to text, scripts, or FlxLinkedExec_t payloads */
        struct {
            FlxVfsNode_t **children; /* S_IFDIR: Null-terminated array of children */
        } dir;
        int dev_id; /* S_IFCHR: Device ID */
    };

    /* Pointer to the filesystem operations table */
    const FlxVfsOps_t *ops;
};

/**
 * @brief Macro to statically declare a VFS Node that points to a payload.
 */
#define FLX_VFS_DECLARE_NODE(node_var, node_name_str, parent_ptr, payload_ptr) \
    static FlxVfsNode_t node_var = { \
        .name = node_name_str, \
        .flags = 0, \
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
 * @brief Declares a LINKED command using automatic linker bounds for dynamic sizing!
 * @param var_name The base name for the generated variables.
 * @param cmd_name The string name of the command (e.g., "ls").
 * @param parent_ptr Pointer to the parent directory node.
 * @param exec_func The main entry point function (also used as the custom section name).
 */
#define FLX_VFS_DECLARE_LINKED_CMD(var_name, cmd_name, parent_ptr, exec_func) \
    FLX_BINFMT_EXTERN_BOUNDS(exec_func); \
    FLX_BINFMT_DECLARE_CMD_PAYLOAD(__payload_##var_name, exec_func, exec_func); \
    FLX_VFS_DECLARE_NODE(var_name, cmd_name, parent_ptr, &__payload_##var_name)

/* API */
FlxVfsNode_t *flx_vfs_root_init(void);
int flx_vfs_mount(FlxVfsNode_t *fs_node, const char *target_path);
FlxVfsNode_t *flx_vfs_root_get(void);

/* vfs_namei */
FlxVfsNode_t *flx_vfs_path_resolve(FlxVfsNode_t *cwd_node, const char *path);

/* POSIX API */
int flx_vfs_stat(FlxVfsNode_t *node, struct stat *st);
ssize_t flx_vfs_read(FlxVfsNode_t *node, void *buf, size_t count, off_t offset);

int flx_vfs_chdir(const char *path);

/* --- Directory Stream API --- */
typedef struct {
    FlxVfsNode_t *node;
    bool in_use;
    int current_index;
    struct dirent entry;
} FlxDir_t;

int flx_vfs_opendir(const char *name);
struct dirent *flx_vfs_readdir(int fd);
int flx_vfs_closedir(int fd);
void flx_vfs_seekdir(int fd, long loc);
long flx_vfs_telldir(int fd);
void flx_vfs_rewinddir(int fd);
char *flx_vfs_get_path(FlxVfsNode_t *node, char *buf, size_t size);
char *flx_vfs_getcwd(char *buf, size_t size);

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

#endif /* FELIX_KERNEL_VFS_H */
