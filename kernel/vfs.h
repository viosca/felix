/**
 * @file vfs.h
 * @brief Virtual File System Definitions.
 */

#ifndef FELIX_KERNEL_VFS_H
#define FELIX_KERNEL_VFS_H

#include <sys/stat.h> /* For POSIX mode_t and struct stat */
#include <dirent.h>   /* For POSIX directory streams */
#include "kernel/devices.h"

typedef struct FlxVfsNode FlxVfsNode_t;

#define FLX_VFS_MAX_PATH_LEN 256

/**
 * @brief Function pointer for executable commands (like 'ls', 'cat')
 */
//typedef int (*FlxVfsCmdHandler)(int argc, char **argv);

/**
 * @brief The "Fat" Command Structure (Executable Definition)
 * Represents a built-in application in the VFS with execution,
 * completion, and help metadata.
 */
typedef struct FlxBuiltinCmd {
    int  (*execute)(int argc, char **argv);
    void (*complete)(const char *prefix, void *linenoise_completions);
    const char *short_help;
    const char *long_help;
} FlxBuiltinCmd_t;

struct FlxVfsNode {
    const char *name;
    FlxVfsNode_t *parent;

    mode_t mode; /* POSIX file mode/permissions */
    size_t size; /* File size in bytes */

    union {
        /* Directory: List of children (NULL terminated array) */
        struct {
            FlxVfsNode_t **children;
        } dir;

        /* File: Function to execute */
        struct {
            const FlxBuiltinCmd_t *cmd;
        } file;

        /* Device: Pointer to FlxDev_t */
        struct {
            FlxDev_t *dev;
        } device;
    };
};

/**
 * @brief Universal macro to statically declare an executable command node.
 * @param var_name   The C variable name for the resulting FlxVfsNode_t.
 * @param cmd_name   The string name of the command (e.g., "ls").
 * @param parent_ptr Pointer to the parent directory node (e.g., &node_bin).
 * @param func       The execution function pointer.
 */
#define FLX_VFS_DECLARE_CMD(var_name, cmd_name, parent_ptr, func) \
    static FlxVfsNode_t var_name = { \
        .name = (cmd_name), \
        .parent = (parent_ptr), \
        .mode = S_IFREG | 0555, \
        .size = 0, \
        .file = { .cmd = &(FlxBuiltinCmd_t){ .execute = (func) } } \
    }

/* API */
FlxVfsNode_t *flx_vfs_root_init(void);
int flx_vfs_mount(FlxVfsNode_t *fs_node, const char *target_path);
FlxVfsNode_t *flx_vfs_root_get(void);

/* vfs_namei */
FlxVfsNode_t *flx_vfs_path_resolve(FlxVfsNode_t *cwd_node, const char *path);

/* POSIX API */
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

/* Positional functions */
void flx_vfs_seekdir(int fd, long loc);
long flx_vfs_telldir(int fd);
void flx_vfs_rewinddir(int fd);

/**
 * @brief Constructs the absolute string path for a given node by tracing its parents.
 */
char *flx_vfs_get_path(FlxVfsNode_t *node, char *buf, size_t size);

/**
 * @brief POSIX-compliant system call to get the current working directory.
 * @param[out] buf Buffer to store the path (if NULL, it will be malloc'd).
 * @param[in] size Size of the buffer.
 * @return A pointer to the buffer, or NULL on failure.
 */
char *flx_vfs_getcwd(char *buf, size_t size);


#endif /* FELIX_KERNEL_VFS_H */
