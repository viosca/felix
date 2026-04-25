/**
 * @file fs/rootfs/bin/flx_fs_bin_cmd_sys.h
 * @brief Headers for small system utility commands (mount, umount, etc.)
 */
#ifndef FELIX_FS_BIN_CMD_SYS_H
#define FELIX_FS_BIN_CMD_SYS_H

int flx_fs_bin_cmd_mount(int argc, char **argv);
int flx_fs_bin_cmd_umount(int argc, char **argv);

#endif /* FELIX_FS_BIN_CMD_SYS_H */
