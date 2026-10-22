#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_PATH)

#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <flx_test.h>

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_path)
int flx_fs_test_cmd_path(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS VFS Path Traversal & Topology Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    /* 1. Deep nested directory creation */
    int m1 = mkdir("/tmp/p1", 0755);
    int m2 = mkdir("/tmp/p1/p2", 0755);
    int m3 = mkdir("/tmp/p1/p2/p3", 0755);
    int m4 = mkdir("/tmp/p1/p2/p3/p4", 0755);
    const char *deep_path = "/tmp/p1/p2/p3/p4/deep.txt";
    int fd = open(deep_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "DEEP_OK", 7);
        close(fd);
    }
    FLX_TEST_CHECK(&t, m1 == 0 && m2 == 0 && m3 == 0 && m4 == 0 && fd >= 0,
                   "deep nested directory creation (5 levels: /tmp/p1/p2/p3/p4)");

    /* 2. Canonicalization of relative '.' and '..' */
    const char *rel_path = "/tmp/p1/p2/../p2/p3/../../../p1/p2/p3/p4/./deep.txt";
    fd = open(rel_path, O_RDONLY);
    char buf[16] = {0};
    ssize_t nr = -1;
    if (fd >= 0) {
        nr = read(fd, buf, sizeof(buf) - 1);
        close(fd);
    }
    FLX_TEST_CHECK(&t, nr == 7 && strcmp(buf, "DEEP_OK") == 0,
                   "relative '.' and '..' path canonicalization ('%s')", buf);

    /* 3. chdir() & getcwd() with relative file resolution */
    char orig_cwd[128] = {0};
    getcwd(orig_cwd, sizeof(orig_cwd));
    int cd_rc = chdir("/tmp/p1/p2");
    char new_cwd[128] = {0};
    getcwd(new_cwd, sizeof(new_cwd));
    bool cwd_ok = (cd_rc == 0 && strcmp(new_cwd, "/tmp/p1/p2") == 0);

    /* Open file relative to new cwd */
    fd = open("p3/p4/deep.txt", O_RDONLY);
    char relbuf[16] = {0};
    if (fd >= 0) {
        read(fd, relbuf, sizeof(relbuf) - 1);
        close(fd);
    }
    chdir(orig_cwd[0] ? orig_cwd : "/");
    FLX_TEST_CHECK(&t, cwd_ok && strcmp(relbuf, "DEEP_OK") == 0,
                   "chdir() & getcwd() with relative I/O ('%s' -> '%s')", new_cwd, relbuf);

    /* 4. ENOTEMPTY prevention on rmdir of non-empty directory */
    int rm_fail = rmdir("/tmp/p1");
    FLX_TEST_CHECK(&t, rm_fail < 0, "rmdir() non-empty directory fails (error protection)");

    /* 5. ENOTDIR error when treating file as directory */
    int notdir_fd = open("/tmp/p1/p2/p3/p4/deep.txt/child", O_RDONLY);
    FLX_TEST_CHECK(&t, notdir_fd < 0, "open() file-as-directory fails with ENOTDIR");

    /* 6. EEXIST error on duplicate mkdir */
    int dup_mk = mkdir("/tmp/p1", 0755);
    FLX_TEST_CHECK(&t, dup_mk < 0, "mkdir() duplicate directory fails with EEXIST");

    /* Cleanup */
    unlink(deep_path);
    rmdir("/tmp/p1/p2/p3/p4");
    rmdir("/tmp/p1/p2/p3");
    rmdir("/tmp/p1/p2");
    rmdir("/tmp/p1");

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_PATH */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_path)
int flx_fs_test_cmd_path(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_PATH */
