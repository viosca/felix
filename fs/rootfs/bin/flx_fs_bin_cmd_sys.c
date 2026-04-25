/**
 * @file fs/rootfs/bin/flx_fs_bin_cmd_sys.c
 * @brief Collection of small system utility commands.
 */
#include <stdio.h>
#include "kernel/syscalls.h"
#include "kernel/binfmt.h"
#include "fs/rootfs/bin/flx_fs_bin_cmd_sys.h"

/* =========================================================================
 * MOUNT COMMAND
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_mount)
int flx_fs_bin_cmd_mount(int argc, char **argv) {
    if (argc < 4) {
        printf("Usage: mount <source> <target> <fstype>\n");
        printf("Example: mount 0: /mnt fatfs\n");
        return 1;
    }

    const char *source = argv[1];
    const char *target = argv[2];
    const char *fstype = argv[3];

    int res = sys_mount(source, target, fstype, 0, NULL);
    if (res < 0) {
        printf("mount: failed to mount '%s' to '%s' (Error %d)\n", source, target, res);
        return res;
    }

    printf("mount: successfully mounted '%s' on '%s' type '%s'\n", source, target, fstype);
    return 0;
}

/* =========================================================================
 * UMOUNT COMMAND
 * ========================================================================= */
FLX_BINFMT_CMD_SECTION(flx_fs_bin_cmd_umount)
int flx_fs_bin_cmd_umount(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: umount <target>\n");
        printf("Example: umount /mnt\n");
        return 1;
    }

    const char *target = argv[1];

    int res = sys_umount(target);
    if (res < 0) {
        printf("umount: failed to unmount '%s' (Error %d)\n", target, res);
        return res;
    }

    printf("umount: successfully unmounted '%s'\n", target);
    return 0;
}
