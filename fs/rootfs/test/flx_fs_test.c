#include <sys/stat.h>
#include <stddef.h>
#include "kernel/vfs.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/flx_fs_root.h"
#include "fs/rootfs/test/flx_fs_test.h"

/* Forward declaration of the directory node so children can reference it as parent */
static FlxVfsNode_t node_test;

/* Statically declare each command node as its own variable */
/* Update your declarations in fs/test/flx_fs_test.c to match the new 6-argument macro: */

/* Compile-time array of pointers to those nodes (NULL-terminated) */
static FlxVfsNode_t *test_children[] = {
    NULL
};

/* 3. The /etc directory node itself */
static FlxVfsNode_t node_test = {
    .name = "test",
    .parent = NULL,
    .mode = S_IFDIR | 0755,
    .dir = { .children = test_children }
};

int flx_fs_test_init(void) {
    return flx_fs_root_add_dir(&node_test);
}
