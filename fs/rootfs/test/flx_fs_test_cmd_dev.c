#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_DEV)

#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <sys/utsname.h>
#include <sys/sysmacros.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
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
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, "console") == 0) {
                found_console = true;
                break;
            }
        }
        closedir(dir);
    }
    FLX_TEST_CHECK(&t, found_console, "/dev directory enumeration (found 'console')");

    /* 3. uname syscall validation */
    struct utsname un;
    int uname_rc = uname(&un);
    bool uname_ok = (uname_rc == 0 &&
                     strcmp(un.sysname, "Felix") == 0 &&
                     strcmp(un.nodename, "felix-mpfs") == 0 &&
                     strcmp(un.machine, "riscv64") == 0);
    FLX_TEST_CHECK(&t, uname_ok, "uname() sysname='%s', nodename='%s', machine='%s'",
                   un.sysname, un.nodename, un.machine);

    /* 4. Environment variable manipulation (setenv/getenv/unsetenv) */
    setenv("FLX_TEST_KEY", "TEST_VAL_42", 1);
    const char *env_val = getenv("FLX_TEST_KEY");
    bool env_ok = (env_val != NULL && strcmp(env_val, "TEST_VAL_42") == 0);
    unsetenv("FLX_TEST_KEY");
    bool unset_ok = (getenv("FLX_TEST_KEY") == NULL);
    FLX_TEST_CHECK(&t, env_ok && unset_ok, "setenv / getenv / unsetenv round-trip ('%s')",
                   env_val ? env_val : "(null)");

    /* 5. Monotonic clock progression and usleep */
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
