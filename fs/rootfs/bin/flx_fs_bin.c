#include <sys/stat.h>
#include <stddef.h>
#include "kernel/vfs.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/flx_fs_root.h"
#include "fs/rootfs/bin/flx_fs_bin.h"
#include "fs/rootfs/bin/flx_fs_bin_cmds.h"
#include "fs/rootfs/bin/flx_fs_bin_cmd_ls.h"
#include "fs/rootfs/bin/flx_fs_bin_cmd_utils.h"
#include "fs/rootfs/bin/flx_fs_bin_cmd_hexdump.h"
#include "fs/rootfs/bin/flx_fs_bin_cmd_sys.h"

/* Forward-declare the parent directory so the children can reference it */
static FlxVfsNode_t node_bin;

/* Declare the external linker bounds for each command */
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_clear);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_date);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_echo);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_help);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_hexdump);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_ls);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_mkdir);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_pwd);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_which);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_tree);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_cat);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_cp);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_rm);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_rmdir);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_touch);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_mount);
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_bin_cmd_umount);

/* Declare the unique Payloads (Inodes) using the Linker Bounds macro */
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_clear,  flx_fs_bin_cmd_clear,  flx_fs_bin_cmd_clear);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_date,   flx_fs_bin_cmd_date,   flx_fs_bin_cmd_date);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_echo,   flx_fs_bin_cmd_echo,   flx_fs_bin_cmd_echo);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_help,   flx_fs_bin_cmd_help,   flx_fs_bin_cmd_help);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_hexdump,  flx_fs_bin_cmd_hexdump,  flx_fs_bin_cmd_hexdump);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_ls,     flx_fs_bin_cmd_ls,     flx_fs_bin_cmd_ls);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_mkdir,  flx_fs_bin_cmd_mkdir,  flx_fs_bin_cmd_mkdir);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_pwd,    flx_fs_bin_cmd_pwd,    flx_fs_bin_cmd_pwd);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_which,  flx_fs_bin_cmd_which,  flx_fs_bin_cmd_which);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_tree,   flx_fs_bin_cmd_tree,   flx_fs_bin_cmd_tree);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_cat,    flx_fs_bin_cmd_cat,    flx_fs_bin_cmd_cat);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_cp,     flx_fs_bin_cmd_cp,     flx_fs_bin_cmd_cp);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_rm,     flx_fs_bin_cmd_rm,     flx_fs_bin_cmd_rm);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_rmdir,  flx_fs_bin_cmd_rmdir,  flx_fs_bin_cmd_rmdir);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_touch,  flx_fs_bin_cmd_touch,  flx_fs_bin_cmd_touch);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_mount,  flx_fs_bin_cmd_mount,  flx_fs_bin_cmd_mount);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_umount, flx_fs_bin_cmd_umount, flx_fs_bin_cmd_umount);

/* Declare the VFS Nodes (Dentries) */
/* Notice how ls, ll, and dir all share payload_ls! These are true hard links. */

FLX_VFS_DECLARE_NODE(node_clear,    "clear",   &node_bin, &payload_clear);
FLX_VFS_DECLARE_NODE(node_dir,      "dir",     &node_bin, &payload_ls); // ALIAS
FLX_VFS_DECLARE_NODE(node_date,     "date",    &node_bin, &payload_date);
FLX_VFS_DECLARE_NODE(node_echo,     "echo",    &node_bin, &payload_echo);
FLX_VFS_DECLARE_NODE(node_help,     "help",    &node_bin, &payload_help);
FLX_VFS_DECLARE_NODE(node_hexdump,  "hexdump", &node_bin, &payload_hexdump);
FLX_VFS_DECLARE_NODE(node_ls,       "ls",      &node_bin, &payload_ls);
FLX_VFS_DECLARE_NODE(node_ll,       "ll",      &node_bin, &payload_ls); // ALIAS
FLX_VFS_DECLARE_NODE(node_mkdir,    "mkdir",   &node_bin, &payload_mkdir);
FLX_VFS_DECLARE_NODE(node_pwd,      "pwd",     &node_bin, &payload_pwd);
FLX_VFS_DECLARE_NODE(node_which,    "which",   &node_bin, &payload_which);
FLX_VFS_DECLARE_NODE(node_tree,     "tree",    &node_bin, &payload_tree);
FLX_VFS_DECLARE_NODE(node_cat,      "cat",     &node_bin, &payload_cat);
FLX_VFS_DECLARE_NODE(node_cp,       "cp",      &node_bin, &payload_cp);
FLX_VFS_DECLARE_NODE(node_rm,       "rm",      &node_bin, &payload_rm);
FLX_VFS_DECLARE_NODE(node_rmdir,    "rmdir",   &node_bin, &payload_rmdir);
FLX_VFS_DECLARE_NODE(node_touch,    "touch",   &node_bin, &payload_touch);
FLX_VFS_DECLARE_NODE(node_mount,    "mount",   &node_bin, &payload_mount);
FLX_VFS_DECLARE_NODE(node_umount,   "umount",  &node_bin, &payload_umount);

/* 6. Link them into the static children array */
static FlxVfsNode_t *bin_children[] = {
    &node_clear,
    &node_cp,
    &node_date,
    &node_dir,
    &node_echo,
    &node_help,
    &node_hexdump,
    &node_ls,
    &node_ll,
    &node_mkdir,
    &node_pwd,
    &node_rm,
    &node_rmdir,
    &node_touch,
    &node_which,
    &node_tree,
    &node_cat,
    &node_mount,
    &node_umount,
    NULL /* Must be null-terminated */
};

/* 7. Define the actual /bin directory node */
static FlxVfsNode_t node_bin = {
    .name = "bin",
    .kflags = 0,
    .parent = NULL, /* Root will assign this during mount */
    .mode = S_IFDIR | 0755,
    .dir = { .children = bin_children },
    .ops = NULL
};

/* 8. Initialization is now a single, O(1) step! */
int flx_fs_bin_init(void) {
    return flx_fs_root_add_dir(&node_bin);
}
