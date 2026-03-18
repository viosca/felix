/* @file flx_fs_bin.c
 */
#include <sys/stat.h>
#include <stddef.h>
#include "kernel/vfs.h"
#include "fs/bin/flx_fs_bin.h"
#include "fs/bin/flx_fs_bin_cmds.h"
#include "fs/bin/flx_fs_bin_cmd_ls.h"

#define FLX_MAX_BIN_CHILDREN 64

static FlxVfsNode_t *bin_children[FLX_MAX_BIN_CHILDREN + 1];
static int bin_node_count = 0;

static FlxVfsNode_t node_bin = {
    .name = "bin",
    .mode = S_IFDIR | 0755,
    .size = 0,
    .dir = { .children = bin_children }
};

int flx_fs_bin_register(const char *name, int (*main_func)(int argc, char **argv)) {
    if (bin_node_count >= FLX_MAX_BIN_CHILDREN) return -1;

    /* Ask the VFS core to generate the executable payload */
    FlxVfsNode_t *node = flx_vfs_create_exec_node(name, main_func);
    if (!node) return -1;

    node->parent = &node_bin;

    /* Link it into the directory */
    bin_children[bin_node_count++] = node;
    bin_children[bin_node_count] = NULL;

    return 0;
}

int flx_fs_bin_init(void) {
    bin_children[0] = NULL;
    bin_node_count = 0;
    int status = flx_vfs_mount(&node_bin, "/bin");
    if (status)
        return status;

    status |= flx_fs_bin_register("dir", flx_fs_bin_cmd_ls);
    status |= flx_fs_bin_register("echo", flx_fs_bin_cmd_echo);
    status |= flx_fs_bin_register("help", flx_fs_bin_cmd_help);
    status |= flx_fs_bin_register("ll", flx_fs_bin_cmd_ls);
    status |= flx_fs_bin_register("ls", flx_fs_bin_cmd_ls);
    status |= flx_fs_bin_register("pwd", flx_fs_bin_cmd_pwd);
    status |= flx_fs_bin_register("which", flx_fs_bin_cmd_which);
    status |= flx_fs_bin_register("tree", flx_fs_bin_cmd_tree);
    return status;
}
