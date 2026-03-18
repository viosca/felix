/**
 * @file vfs.h
 * @brief Virtual File System Definitions.
 */

#ifndef FELIX_KERNEL_VFS_H
#define FELIX_KERNEL_VFS_H

#include <sys/stat.h> /* For POSIX mode_t and struct stat */
#include <dirent.h>   /* For POSIX directory streams */
#include "kernel/devices.h"
#include "kernel/binfmt.h"

typedef struct FlxVfsNode FlxVfsNode_t;

struct FlxVfsNode {
    const char *name;
    FlxVfsNode_t *parent;

    mode_t mode; /* POSIX file mode/permissions */
    size_t size; /* File size in bytes */

    /* VFS Tree / Payload Union */
    union {
        void *data; /* S_IFREG: Points to text, scripts, or FlxLinkedExec_t payloads */
        struct {
            FlxVfsNode_t **children; /* S_IFDIR: Null-terminated array of children */
        } dir;
        int dev_id; /* S_IFCHR: Device ID */
    };
};

/**
 * @brief Macro to statically declare a VFS Node that points to a payload.
 */
#define FLX_VFS_DECLARE_NODE(node_var, node_name_str, parent_ptr, payload_ptr) \
    static FlxVfsNode_t node_var = { \
        .name = (node_name_str), \
        .parent = (parent_ptr), \
        .mode = S_IFREG | 0555, \
        .size = sizeof(FlxLinkedExec_t), \
        .data = (void*)(payload_ptr) \
    }

/**
 * @brief Combined macro: Declares both the payload and the VFS node automatically.
 */
#define FLX_VFS_DECLARE_CMD(var_name, cmd_name, parent_ptr, exec_func, comp_func, help_text) \
    FLX_BINFMT_DECLARE_PAYLOAD(__payload_##var_name, exec_func, comp_func, help_text); \
    FLX_VFS_DECLARE_NODE(var_name, cmd_name, parent_ptr, &__payload_##var_name)

/* API */
FlxVfsNode_t *flx_vfs_root_init(void);
int flx_vfs_mount(FlxVfsNode_t *fs_node, const char *target_path);
FlxVfsNode_t *flx_vfs_root_get(void);

/* vfs_namei */
FlxVfsNode_t *flx_vfs_path_resolve(FlxVfsNode_t *cwd_node, const char *path);

/* POSIX API */

/**
 * @brief Performs a POSIX stat() on a node to retrieve size, permissions, and device IDs.
 */
int flx_vfs_stat(FlxVfsNode_t *node, struct stat *st);

int flx_vfs_chdir(const char *path);

/* --- Directory Stream API --- */
typedef struct {
    FlxVfsNode_t *node;
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
 * @return Pointer to the populated VFS Node, or NULL if the pool is full.
 */
FlxVfsNode_t* flx_vfs_create_exec_node(const char *name, int (*main_func)(int argc, char **argv));



#endif /* FELIX_KERNEL_VFS_H */
