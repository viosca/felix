#ifndef FELIX_FS_TEST_CMDS_H
#define FELIX_FS_TEST_CMDS_H

/**
 * @file flx_fs_test_cmds.h
 * @brief Public interface and declarations for Felix OS unit test commands.
 */

int flx_fs_test_cmd_all(int argc, char **argv);
int flx_fs_test_cmd_mem(int argc, char **argv);
int flx_fs_test_cmd_dev(int argc, char **argv);
int flx_fs_test_cmd_vfs(int argc, char **argv);
int flx_fs_test_cmd_posix(int argc, char **argv);
int flx_fs_test_cmd_wordexp(int argc, char **argv);
int flx_fs_test_cmd_io(int argc, char **argv);
int flx_fs_test_cmd_path(int argc, char **argv);
int flx_fs_test_cmd_storage(int argc, char **argv);
int flx_fs_test_cmd_stress(int argc, char **argv);

#endif /* FELIX_FS_TEST_CMDS_H */
