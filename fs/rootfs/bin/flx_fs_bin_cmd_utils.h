/**
 * @file fs/rootfs/bin/flx_fs_bin_cmd_utils.h
 * @brief Headers for general-purpose utility commands (cat, touch, rm, rmdir, cp, etc.)
 */
#ifndef FELIX_FS_BIN_CMD_UTILS_H
#define FELIX_FS_BIN_CMD_UTILS_H

int flx_fs_bin_cmd_cat(int argc, char **argv);
int flx_fs_bin_cmd_touch(int argc, char **argv);
int flx_fs_bin_cmd_rm(int argc, char **argv);
int flx_fs_bin_cmd_rmdir(int argc, char **argv);
int flx_fs_bin_cmd_cp(int argc, char **argv);

#endif /* FELIX_FS_BIN_CMD_UTILS_H */
