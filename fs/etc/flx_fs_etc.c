#include <stddef.h>
#include <sys/stat.h>
#include "kernel/vfs.h"
#include "fs/etc/flx_fs_etc.h"

/* Forward declaration of the directory node so children can reference it as parent */
static FlxVfsNode_t node_etc;

/* Statically declare each command node as its own variable */
/* Update your declarations in fs/bin/flx_fs_bin.c to match the new 6-argument macro: */

/* Compile-time array of pointers to those nodes (NULL-terminated) */
static FlxVfsNode_t *etc_children[] = {
    NULL
};

/* 3. The /etc directory node itself */
static FlxVfsNode_t node_etc = {
    .name = "etc",
    .parent = NULL,
    .mode = S_IFDIR | 0755,
    .dir = { .children = etc_children }
};

int flx_fs_etc_init(void) {
    /* No runtime loops or pointer assignments required!
     * The entire tree structure is resolved by the linker. */

    /* Mount to the root switch */
    return flx_vfs_mount(&node_etc, "/etc");
}
