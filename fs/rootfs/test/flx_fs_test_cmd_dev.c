#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_DEV)

#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysmacros.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <flx_test.h>

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_dev)
int flx_fs_test_cmd_dev(int argc, char **argv) {
    FlxTestCtx_t t;
    int arg_rc = flx_test_begin_args(&t, "Felix OS Device & Syscall Verification Suite", argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    /* 1. Stat /dev/console device node */
    struct stat st;
    int rc = stat("/dev/console", &st);
    bool chr_ok = (rc == 0 && S_ISCHR(st.st_mode) && major(st.st_rdev) == 4 && minor(st.st_rdev) == 4);
    FLX_TEST_CHECK(&t, chr_ok, "/dev/console S_ISCHR with major=%u, minor=%u",
                   (unsigned)major(st.st_rdev), (unsigned)minor(st.st_rdev));

    /* 2. Readdir on /dev directory */
    DIR *dir = opendir("/dev");
    bool found_console = false;
    bool found_null = false;
    bool found_zero = false;
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, "console") == 0) found_console = true;
            if (strcmp(ent->d_name, "null") == 0)    found_null = true;
            if (strcmp(ent->d_name, "zero") == 0)    found_zero = true;
        }
        closedir(dir);
    }
    FLX_TEST_CHECK(&t, found_console && found_null && found_zero,
                   "/dev directory enumeration (found 'console', 'null', 'zero')");

    /* 3. /dev/null character device (major=1, minor=3): write sink & EOF read */
    struct stat st_null;
    bool null_ok = (stat("/dev/null", &st_null) == 0 &&
                    S_ISCHR(st_null.st_mode) &&
                    major(st_null.st_rdev) == 1 && minor(st_null.st_rdev) == 3);
    int fd_null = open("/dev/null", O_RDWR);
    if (fd_null >= 0) {
        char dummy[16] = "discard_me";
        ssize_t nw = write(fd_null, dummy, 10);
        ssize_t nr = read(fd_null, dummy, sizeof(dummy));
        bool not_tty = (isatty(fd_null) == 0);
        close(fd_null);
        null_ok = null_ok && (nw == 10 && nr == 0 && not_tty);
    } else {
        null_ok = false;
    }
    FLX_TEST_CHECK(&t, null_ok, "/dev/null (1,3) discards writes and returns 0 EOF on read");

    /* 4. /dev/zero character device (major=1, minor=5): zero-byte stream generator */
    struct stat st_zero;
    bool zero_ok = (stat("/dev/zero", &st_zero) == 0 &&
                    S_ISCHR(st_zero.st_mode) &&
                    major(st_zero.st_rdev) == 1 && minor(st_zero.st_rdev) == 5);
    int fd_zero = open("/dev/zero", O_RDWR);
    if (fd_zero >= 0) {
        uint8_t zbuf[16];
        memset(zbuf, 0xAA, sizeof(zbuf));
        ssize_t nr = read(fd_zero, zbuf, sizeof(zbuf));
        ssize_t nw = write(fd_zero, "sink", 4);
        close(fd_zero);
        bool all_zero = (nr == (ssize_t)sizeof(zbuf) && nw == 4);
        for (size_t i = 0; i < sizeof(zbuf); i++) {
            if (zbuf[i] != 0x00) all_zero = false;
        }
        zero_ok = zero_ok && all_zero;
    } else {
        zero_ok = false;
    }
    FLX_TEST_CHECK(&t, zero_ok, "/dev/zero (1,5) fills read buffer with 0x00 bytes");

    /* 5. uname syscall validation */
    struct utsname un;
    int uname_rc = uname(&un);
    bool uname_ok = (uname_rc == 0 &&
                     strcmp(un.sysname, "Felix") == 0 &&
                     strcmp(un.nodename, "felix-mpfs") == 0 &&
                     strcmp(un.machine, "riscv64") == 0);
    FLX_TEST_CHECK(&t, uname_ok, "uname() sysname='%s', nodename='%s', machine='%s'",
                   un.sysname, un.nodename, un.machine);

    /* 6. Environment variable manipulation (setenv/getenv/unsetenv) */
    setenv("FLX_TEST_KEY", "TEST_VAL_42", 1);
    const char *env_val = getenv("FLX_TEST_KEY");
    bool env_ok = (env_val != NULL && strcmp(env_val, "TEST_VAL_42") == 0);
    unsetenv("FLX_TEST_KEY");
    bool unset_ok = (getenv("FLX_TEST_KEY") == NULL);
    FLX_TEST_CHECK(&t, env_ok && unset_ok, "setenv / getenv / unsetenv round-trip ('%s')",
                   env_val ? env_val : "(null)");

    /* 7. Monotonic clock progression and usleep */
    struct timespec ts1, ts2;
    int c1 = clock_gettime(CLOCK_MONOTONIC, &ts1);
    usleep(10000); /* 10 ms sleep */
    int c2 = clock_gettime(CLOCK_MONOTONIC, &ts2);
    bool mono_ok = (c1 == 0 && c2 == 0) &&
                   ((ts2.tv_sec > ts1.tv_sec) ||
                    (ts2.tv_sec == ts1.tv_sec && ts2.tv_nsec >= ts1.tv_nsec));
    FLX_TEST_CHECK(&t, mono_ok, "clock_gettime(CLOCK_MONOTONIC) + usleep(10ms) progression");

    return flx_test_end(&t);
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_DEV */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_dev)
int flx_fs_test_cmd_dev(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_DEV */
