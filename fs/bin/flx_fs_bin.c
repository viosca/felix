#include <stddef.h>
#include <sys/stat.h>
#include "kernel/vfs.h"
#include "fs/bin/flx_fs_bin.h"
#include "fs/bin/flx_fs_bin_cmds.h"
#include <fs/bin/flx_fs_bin_cmd_ls.h>

/* Forward declaration of the directory node so children can reference it as parent */
static FlxVfsNode_t node_bin;

/* Statically declare each command node as its own variable */
/* Update your declarations in fs/bin/flx_fs_bin.c to match the new 6-argument macro: */

//FLX_VFS_DECLARE_CMD(node_ls, "ls", &node_bin, flx_fs_bin_cmd_ls, NULL, "List directory contents");
FLX_BINFMT_DECLARE_PAYLOAD(payload_ls, flx_fs_bin_cmd_ls, NULL, "List directory contents");
FLX_VFS_DECLARE_NODE(node_ls,  "ls",  &node_bin, &payload_ls);
FLX_VFS_DECLARE_NODE(node_ll,  "ll",  &node_bin, &payload_ls);
FLX_VFS_DECLARE_NODE(node_dir, "dir", &node_bin, &payload_ls);

FLX_VFS_DECLARE_CMD(node_echo, "echo", &node_bin, flx_fs_bin_cmd_echo, NULL, "Print text to the terminal");
FLX_VFS_DECLARE_CMD(node_which, "which", &node_bin, flx_fs_bin_cmd_which, NULL, "Locate a command");
FLX_VFS_DECLARE_CMD(node_pwd, "pwd", &node_bin, flx_fs_bin_cmd_pwd, NULL, "Print working directory");
FLX_VFS_DECLARE_CMD(node_clear, "clear", &node_bin, flx_fs_bin_cmd_clear, NULL, "Clear the terminal screen");
FLX_VFS_DECLARE_CMD(node_help, "help", &node_bin, flx_fs_bin_cmd_help, NULL, "Display built-in commands");

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
