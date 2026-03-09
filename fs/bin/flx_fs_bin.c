#include <stddef.h>
#include <sys/stat.h>
#include "kernel/vfs.h"
#include "fs/bin/flx_fs_bin.h"
#include "fs/bin/flx_fs_bin_cmds.h"
#include <fs/bin/flx_fs_bin_cmd_ls.h>

/* Forward declaration of the directory node so children can reference it as parent */
static FlxVfsNode_t node_bin;

/* Statically declare each command node as its own variable */
FLX_VFS_DECLARE_CMD(node_ls, "ls", &node_bin, flx_fs_bin_cmd_ls);
FLX_VFS_DECLARE_CMD(node_echo, "echo", &node_bin, flx_fs_bin_cmd_echo);
FLX_VFS_DECLARE_CMD(node_which, "which", &node_bin, flx_fs_bin_cmd_which);
FLX_VFS_DECLARE_CMD(node_pwd, "pwd", &node_bin, flx_fs_bin_cmd_pwd);
FLX_VFS_DECLARE_CMD(node_clear, "clear", &node_bin, flx_fs_bin_cmd_clear);
FLX_VFS_DECLARE_CMD(node_help, "help", &node_bin, flx_fs_bin_cmd_help);

/* 2. Compile-time array of pointers to those nodes (NULL-terminated) */
static FlxVfsNode_t *bin_children[] = {
    &node_ls,
    &node_echo,
    &node_which,
    &node_pwd,
    &node_clear,
    &node_help,
    NULL
};

/* 3. The /bin directory node itself */
static FlxVfsNode_t node_bin = {
    .name = "bin",
    .parent = NULL,
    .mode = S_IFDIR | 0755,
    .size = 0,
    .dir = { .children = bin_children }
};

int flx_fs_bin_init(void) {
    /* No runtime loops or pointer assignments required!
     * The entire tree structure is resolved by the linker. */

    /* Mount to the root switch */
    return flx_vfs_mount(&node_bin, "/bin");
}
