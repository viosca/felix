/** @file felix/kernel/init.c
 */
#include <fs/rootfs/dev/flx_fs_dev.h.bak>
#include "felix.h"
#include <stdio.h>
#include <unistd.h>
#include "kernel/process.h"
#include "kernel/devices.h"
#include "fs/rootfs/bin/flx_fs_bin.h"
#include "fs/rootfs/etc/flx_fs_etc.h"
#include "fs/rootfs/test/flx_fs_test.h"
#include "drivers/serial.h"
#include "fs/rootfs/flx_fs_root.h"
#include "fs/fatfs/fatfs_vfs.h"
#include "kernel/blk_manager.h"

void felix_init(void) {
    /* Build the base Root Filesystem (/) First!
     * This ensures the VFS engine has a root node before processes start. */
    flx_fs_root_init();

    /* Initialize core software subsystems */
    flx_dev_init();           /* Zero out the global device table */
    flx_proc_init_first();    /* Setup FD table and safely fetch the VFS root (/) for CWD */

    fatfs_vfs_register();

    /* This starts the Block Manager, flips the SD Card GPIO multiplexer,
     * and forces an initial boot scan of the hardware slot to trigger auto-mounting */
    flx_blk_manager_init();

}

int felix_register_uart(int dev_id, void *hw_instance, const FlxUartOps_t *ops) {
    int ret;

    /* Route the opaque hardware pointer safely down to the HAL */
    return serial_init(dev_id, hw_instance, ops);
}

/**
 * @brief Binds the standard I/O file descriptors to a specific VFS node path.
 */
int felix_console_bind(const char *dev_path) {
    const char *remainder = NULL;

    /* 1. Pass the &remainder pointer to capture any unparsed path segments */
    FlxVfsNode_t *console_node = flx_vfs_path_resolve(flx_vfs_root_get(), dev_path, &remainder);

    /* 2. Safety Guard!
     * If console_node is NULL, the path is invalid.
     * If remainder contains text, the path hit a mount boundary (like /mnt)
     * and cannot be raw-bound because it requires sys_open() to generate a dynamic node! */
    if (!console_node || (remainder && *remainder != '\0')) {
        return -1;
    }

    /* Bind the VFS Node to standard I/O */
    flx_proc_fd_init(STDIN_FILENO);
    flx_proc_fd_init(STDOUT_FILENO);
    flx_proc_fd_init(STDERR_FILENO);

    flx_proc_fd_install(STDIN_FILENO, console_node);
    flx_proc_fd_install(STDOUT_FILENO, console_node);
    flx_proc_fd_install(STDERR_FILENO, console_node);
    return 0;
}
