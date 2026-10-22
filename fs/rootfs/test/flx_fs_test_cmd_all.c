#include <stdio.h>
#include "autoconf.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"
#include <flx_test.h>

#if defined(CONFIG_FELIX_TEST_SUITE) && defined(CONFIG_FELIX_TEST_ALL)

struct test_suite_entry {
    const char *name;
    const char *path;
    int (*run)(int argc, char **argv);
};

static const struct test_suite_entry suites[] = {
#if defined(CONFIG_FELIX_TEST_MEM)
    { "Felix OS Heap & Memory Allocator Suite", "/test/mem_test", flx_fs_test_cmd_mem },
#endif
#if defined(CONFIG_FELIX_TEST_DEV)
    { "Felix OS Device & Syscall Verification Suite", "/test/dev_test", flx_fs_test_cmd_dev },
#endif
#if defined(CONFIG_FELIX_TEST_VFS)
    { "Felix OS VFS & Filesystem Verification Suite", "/test/vfs_test", flx_fs_test_cmd_vfs },
#endif
#if defined(CONFIG_FELIX_TEST_POSIX)
    { "Felix OS FreeRTOS+POSIX Verification Suite", "/test/posix_test", flx_fs_test_cmd_posix },
#endif
#if defined(CONFIG_FELIX_TEST_WORDEXP)
    { "Felix OS POSIX wordexp & Execution Suite", "/test/wordexp_test", flx_fs_test_cmd_wordexp },
#endif
#if defined(CONFIG_FELIX_TEST_IO)
    { "Felix OS POSIX File I/O & Stream Suite", "/test/io_test", flx_fs_test_cmd_io },
#endif
#if defined(CONFIG_FELIX_TEST_PATH)
    { "Felix OS VFS Path Traversal & Topology Suite", "/test/path_test", flx_fs_test_cmd_path },
#endif
#if defined(CONFIG_FELIX_TEST_STORAGE)
    { "Felix OS Storage & Block Subsystem Suite", "/test/storage_test", flx_fs_test_cmd_storage },
#endif
#if defined(CONFIG_FELIX_TEST_STRESS)
    { "Felix OS Multitasking Concurrency & Stress Suite", "/test/stress_test", flx_fs_test_cmd_stress },
#endif
};

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_all)
int flx_fs_test_cmd_all(int argc, char **argv) {
    FlxTestCtx_t t;
    flx_test_init(&t, "Felix OS Comprehensive Test Runner");
    int arg_rc = flx_test_parse_args(&t, argc, argv);
    if (arg_rc != 0) {
        return (arg_rc > 0) ? 0 : 1;
    }

    int total_suites = (int)(sizeof(suites) / sizeof(suites[0]));

    printf("=================================================================\n");
    printf("   Felix OS Automated Test Suite Runner (%d Suites)\n", total_suites);
    printf("=================================================================\n");

    int failed_suites = 0;
    for (int i = 0; i < total_suites; i++) {
        printf("\n>>> [%d/%d] Running %s <<<\n", i + 1, total_suites, suites[i].path);
        if (suites[i].run(argc, argv) != 0) {
            failed_suites++;
        }
    }

    printf("\n=================================================================\n");
    printf("=== Test Summary: %d/%d suites PASSED (%d failures) ===\n",
           total_suites - failed_suites, total_suites, failed_suites);
    printf("=================================================================\n");

    return (failed_suites == 0) ? 0 : 1;
}

#else /* !CONFIG_FELIX_TEST_SUITE || !CONFIG_FELIX_TEST_ALL */

FLX_BINFMT_CMD_SECTION(flx_fs_test_cmd_all)
int flx_fs_test_cmd_all(int argc, char **argv) {
    (void)argc;
    (void)argv;
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE && CONFIG_FELIX_TEST_ALL */
