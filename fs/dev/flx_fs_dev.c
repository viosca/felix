#include <sys/stat.h>
#include <stddef.h>
#include <stdint.h>
#include "kernel/vfs.h"
#include "fs/dev/flx_fs_dev.h"

/* The maximum number of devices that can be registered in /dev */
#define FLX_MAX_DEV_CHILDREN 64

/* --- POSIX Major/Minor Device Macros --- */
/* Combines major and minor into a single 32-bit integer for dev_id */
#define FLX_MAKEDEV(maj, min) (int)(((maj & 0xFF) << 8) | (min & 0xFF))

/* Forward declaration for parent linking */
static FlxVfsNode_t node_dev;

/* Memory pool for actual device node structs */
static FlxVfsNode_t dev_nodes[FLX_MAX_DEV_CHILDREN];
static int dev_node_count = 0;

/* Directory index: Array of pointers to registered devices */
static FlxVfsNode_t *dev_children[FLX_MAX_DEV_CHILDREN + 1];

/* =========================================================================
 * The /dev Directory Node
 * ========================================================================= */

static FlxVfsNode_t node_dev = {
    .name = "dev",
    .flags = 0,
    .parent = NULL,
    .mode = S_IFDIR | 0755,
    .dir = { .children = dev_children },
    .ops = NULL
};

/* =========================================================================
 * API Implementation
 * ========================================================================= */

int flx_fs_dev_register(const char *name, int major, int minor) {
    if (dev_node_count >= FLX_MAX_DEV_CHILDREN) {
        return -1; /* Out of space in the /dev array */
    }

    int i = dev_node_count++;

    /* Populate the actual struct in the static memory pool */
    dev_nodes[i].name = name;
    dev_nodes[i].flags = 0;
    dev_nodes[i].parent = &node_dev;
    dev_nodes[i].mode = S_IFCHR | 0666;

    /* Store the combined Major/Minor number directly in the union! */
    dev_nodes[i].dev_id = FLX_MAKEDEV(major, minor);
    dev_nodes[i].ops = NULL;

    /* Link the pointer into the directory iteration array */
    dev_children[i] = &dev_nodes[i];

    /* Shift the NULL terminator down so directory iterators know when to stop */
    dev_children[i + 1] = NULL;

    return 0;
}

int flx_fs_dev_init(void) {
    /* Ensure the array starts perfectly empty */
    dev_children[0] = NULL;
    dev_node_count = 0;

    /* Ask the VFS to mount this directory at the root level */
    return flx_vfs_mount(&node_dev, "/dev");
}
