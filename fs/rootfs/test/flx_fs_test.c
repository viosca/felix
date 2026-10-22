#define _POSIX_C_SOURCE 200809L
#include <sys/stat.h>
#include <stddef.h>
#include "autoconf.h"
#include "kernel/vfs.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/flx_fs_root.h"
#include "fs/rootfs/test/flx_fs_test.h"
#include "fs/rootfs/test/flx_fs_test_cmds.h"

#if defined(CONFIG_FELIX_TEST_SUITE)

/* Forward declaration for /test directory node */
static FlxVfsNode_t node_test;

#if defined(CONFIG_FELIX_TEST_ALL)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_all);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_all, flx_fs_test_cmd_all, flx_fs_test_cmd_all);
FLX_VFS_DECLARE_NODE(node_all, "all", &node_test, &payload_all);
#endif

#if defined(CONFIG_FELIX_TEST_MEM)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_mem);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_mem_test, flx_fs_test_cmd_mem, flx_fs_test_cmd_mem);
FLX_VFS_DECLARE_NODE(node_mem_test, "mem_test", &node_test, &payload_mem_test);
#endif

#if defined(CONFIG_FELIX_TEST_DEV)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_dev);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_dev_test, flx_fs_test_cmd_dev, flx_fs_test_cmd_dev);
FLX_VFS_DECLARE_NODE(node_dev_test, "dev_test", &node_test, &payload_dev_test);
#endif

#if defined(CONFIG_FELIX_TEST_VFS)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_vfs);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_vfs_test, flx_fs_test_cmd_vfs, flx_fs_test_cmd_vfs);
FLX_VFS_DECLARE_NODE(node_vfs_test, "vfs_test", &node_test, &payload_vfs_test);
#endif

#if defined(CONFIG_FELIX_TEST_POSIX)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_posix);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_posix_test, flx_fs_test_cmd_posix, flx_fs_test_cmd_posix);
FLX_VFS_DECLARE_NODE(node_posix_test, "posix_test", &node_test, &payload_posix_test);
#endif

#if defined(CONFIG_FELIX_TEST_WORDEXP)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_wordexp);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_wordexp_test, flx_fs_test_cmd_wordexp, flx_fs_test_cmd_wordexp);
FLX_VFS_DECLARE_NODE(node_wordexp_test, "wordexp_test", &node_test, &payload_wordexp_test);
#endif

#if defined(CONFIG_FELIX_TEST_IO)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_io);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_io_test, flx_fs_test_cmd_io, flx_fs_test_cmd_io);
FLX_VFS_DECLARE_NODE(node_io_test, "io_test", &node_test, &payload_io_test);
#endif

#if defined(CONFIG_FELIX_TEST_PATH)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_path);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_path_test, flx_fs_test_cmd_path, flx_fs_test_cmd_path);
FLX_VFS_DECLARE_NODE(node_path_test, "path_test", &node_test, &payload_path_test);
#endif

#if defined(CONFIG_FELIX_TEST_STORAGE)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_storage);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_storage_test, flx_fs_test_cmd_storage, flx_fs_test_cmd_storage);
FLX_VFS_DECLARE_NODE(node_storage_test, "storage_test", &node_test, &payload_storage_test);
#endif

#if defined(CONFIG_FELIX_TEST_STRESS)
FLX_BINFMT_EXTERN_BOUNDS(flx_fs_test_cmd_stress);
FLX_BINFMT_DECLARE_CMD_PAYLOAD(payload_stress_test, flx_fs_test_cmd_stress, flx_fs_test_cmd_stress);
FLX_VFS_DECLARE_NODE(node_stress_test, "stress_test", &node_test, &payload_stress_test);
#endif

/* Compile-time array of pointers to /test nodes (NULL-terminated) */
static FlxVfsNode_t *test_children[] = {
#if defined(CONFIG_FELIX_TEST_ALL)
    &node_all,
#endif
#if defined(CONFIG_FELIX_TEST_MEM)
    &node_mem_test,
#endif
#if defined(CONFIG_FELIX_TEST_DEV)
    &node_dev_test,
#endif
#if defined(CONFIG_FELIX_TEST_VFS)
    &node_vfs_test,
#endif
#if defined(CONFIG_FELIX_TEST_POSIX)
    &node_posix_test,
#endif
#if defined(CONFIG_FELIX_TEST_WORDEXP)
    &node_wordexp_test,
#endif
#if defined(CONFIG_FELIX_TEST_IO)
    &node_io_test,
#endif
#if defined(CONFIG_FELIX_TEST_PATH)
    &node_path_test,
#endif
#if defined(CONFIG_FELIX_TEST_STORAGE)
    &node_storage_test,
#endif
#if defined(CONFIG_FELIX_TEST_STRESS)
    &node_stress_test,
#endif
    NULL
};

/* The /test directory node itself */
static FlxVfsNode_t node_test = {
    .name = "test",
    .parent = NULL,
    .mode = S_IFDIR | 0755,
    .dir = { .children = test_children }
};

int flx_fs_test_init(void) {
    return flx_fs_root_add_dir(&node_test);
}

#else /* !CONFIG_FELIX_TEST_SUITE */

int flx_fs_test_init(void) {
    return 0;
}

#endif /* CONFIG_FELIX_TEST_SUITE */
