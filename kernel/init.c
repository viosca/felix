/** @file felix/kernel/init.c
 */
#include "kernel/klog.h"

#include <fs/rootfs/dev/flx_fs_dev.h>
#include "felix.h"
#include <stdio.h>
#include <unistd.h>
#include "kernel/process.h"
#include "kernel/devices.h"
#include "fs/rootfs/bin/flx_fs_bin.h"
#include "fs/rootfs/etc/flx_fs_etc.h"
#include "fs/rootfs/test/flx_fs_test.h"
#include "drivers/flx_serial_core.h"
#include "fs/rootfs/flx_fs_root.h"
#include "fs/fatfs/fatfs_vfs.h"
#include "kernel/blk_manager.h"
void felix_init(void) {
    /* Build the base Root Filesystem (/) First */
    flx_fs_root_init();

    /* Initialize core software subsystems */
    flx_dev_init();

    /* Register filesystem drivers with the VFS */
    fatfs_vfs_register();
}

int felix_register_uart(int dev_id, void *hw_instance, const FlxUartOps_t *ops) {
    int ret;

    /* Route the opaque hardware pointer safely down to the HAL */
    return flx_serial_init(dev_id, hw_instance, ops);
}

/**
 * @brief Binds the standard I/O file descriptors to a specific VFS node path.
 */
int felix_console_bind(const char *dev_path) {
    const char *remainder = NULL;
    FlxVfsNode_t *console_node = flx_vfs_path_resolve(flx_vfs_root_get(), dev_path, &remainder);

    if (!console_node || (remainder && *remainder != '\0')) {
        return -1;
    }

    flx_proc_fd_init(STDIN_FILENO);
    flx_proc_fd_init(STDOUT_FILENO);
    flx_proc_fd_init(STDERR_FILENO);

    flx_proc_fd_install(STDIN_FILENO, console_node);
    flx_proc_fd_install(STDOUT_FILENO, console_node);
    flx_proc_fd_install(STDERR_FILENO, console_node);

    /* The POSIX standard I/O is alive. Tame the C-library buffers immediately. */
    clearerr(stdin); clearerr(stdout); clearerr(stderr);
    setvbuf(stdin,  NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    /* Hand over the kernel logs */
    flx_klog_switch_to_vfs(console_node);

    return 0;
}
