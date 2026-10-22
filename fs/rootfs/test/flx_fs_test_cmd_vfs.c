#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_VFS)

#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <flx_test.h>

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_vfs)
int flx_fs_test_cmd_vfs(int argc, char **argv) {
    FlxTestCtx_t t;
    flx_test_init(&t, "Felix OS VFS & Filesystem Verification Suite");

    const char *target_dir = "/tmp";
    if (argc > 1 && argv) {
        struct optparse options;
        optparse_init(&options, argv);
        int opt;
        while ((opt = optparse(&options, FLX_TEST_OPTSTR "d:")) != -1) {
            if (opt == 'd') {
                target_dir = options.optarg;
                continue;
            }
            if (opt == '?') {
                printf("%s: %s -- '%c'\n", argv[0] ? argv[0] : "vfs_test", options.errmsg, options.optopt);
                flx_test_usage(argv[0], "[-d <target_dir>]");
                return 1;
            }
            int rc = flx_test_handle_opt(&t, opt, argv[0], "[-d <target_dir>]");
            if (rc < 0) {
                return 0;
            }
        }
    }
    flx_test_banner(&t);

    char filepath[128];
    char dirpath[128];
    snprintf(filepath, sizeof(filepath), "%s/vfs_test.txt", target_dir);
    snprintf(dirpath, sizeof(dirpath), "%s/vfs_test_sub", target_dir);

    const char *payload = "Felix VFS Verification Payload 1234567890";
    size_t payload_len = strlen(payload);

    /* 1. File creation and write */
    int fd = open(filepath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ssize_t nw = -1;
    if (fd >= 0) {
        nw = write(fd, payload, payload_len);
        close(fd);
    }
    FLX_TEST_INT(&t, nw, "==", (long)payload_len,
                 "open(O_CREAT|O_WRONLY) & write (%ld/%ld bytes to %s)",
                 nw, (long)payload_len, filepath);

    /* 2. stat & fstat */
    struct stat st;
    int stat_rc = stat(filepath, &st);
    int fstat_rc = -1;
    fd = open(filepath, O_RDONLY);
    if (fd >= 0) {
        fstat_rc = fstat(fd, &st);
    }
    FLX_TEST_CHECK(&t,
                   stat_rc == 0 && fstat_rc == 0 && S_ISREG(st.st_mode) && st.st_size == (off_t)payload_len,
                   "stat & fstat verification (size=%ld, S_ISREG=1)", (long)st.st_size);

    /* 3. lseek & read */
    off_t seek_rc = -1;
    char rdbuf[64] = {0};
    ssize_t nr = -1;
    if (fd >= 0) {
        seek_rc = lseek(fd, 6, SEEK_SET);
        nr = read(fd, rdbuf, sizeof(rdbuf) - 1);
        close(fd);
    }
    FLX_TEST_CHECK(&t,
                   seek_rc == 6 && nr == (ssize_t)(payload_len - 6) && strcmp(rdbuf, payload + 6) == 0,
                   "lseek(SEEK_SET, 6) & read ('%s')", rdbuf);

    /* 4. Directory creation and readdir enumeration */
    int mk_rc = mkdir(dirpath, 0755);
    DIR *d = opendir(target_dir);
    bool found_file = false;
    bool found_dir = false;
    if (d) {
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (strcmp(de->d_name, "vfs_test.txt") == 0) found_file = true;
            if (strcmp(de->d_name, "vfs_test_sub") == 0) found_dir = true;
        }
        closedir(d);
    }
    FLX_TEST_CHECK(&t, mk_rc == 0 && found_file && found_dir,
                   "mkdir & opendir/readdir enumeration (found file=%d, subdir=%d)",
                   found_file, found_dir);

    /* 5. *at syscall shims and cleanup */
    int fstatat_rc = fstatat(AT_FDCWD, filepath, &st, 0);
    int unlinkat_rc = unlinkat(AT_FDCWD, filepath, 0);
    int rmdir_rc = rmdir(dirpath);
    int post_stat = stat(filepath, &st);
    FLX_TEST_CHECK(&t,
                   fstatat_rc == 0 && unlinkat_rc == 0 && rmdir_rc == 0 && post_stat < 0,
                   "fstatat, unlinkat, rmdir cleanup");

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_VFS */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_vfs)
int flx_fs_test_cmd_vfs(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_VFS */
