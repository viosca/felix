#include "felix.h"
#include <stdio.h>
#include <unistd.h>
#include "kernel/process.h"
#include "kernel/devices.h"
#include "fs/bin/flx_fs_bin.h"
#include "fs/dev/flx_fs_dev.h"
#include "fs/etc/flx_fs_etc.h"
#include "fs/test/flx_fs_test.h"
#include "drivers/serial.h"

void felix_init(void) {
    /* Initialize core software subsystems */
    flx_dev_init();           /* Zero out the global device table */
    flx_proc_init_first();    /* Setup FD table and set CWD to VFS root (/) */
    flx_fs_bin_init();         /* Mounts /bin and registers the executables */
    flx_fs_dev_init();         /* Mounts /dev and registers the executables */
    flx_fs_etc_init();         /* Mounts /etc and registers the files */
    flx_fs_test_init();         /* Mounts /test and registers the executables */
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
    FlxVfsNode_t *console_node = flx_vfs_path_resolve(flx_vfs_root_get(), dev_path);
    if (!console_node) return -1;

    /* Bind the VFS Node to standard I/O */

    flx_proc_fd_init(STDIN_FILENO);
    flx_proc_fd_init(STDOUT_FILENO);
    flx_proc_fd_init(STDERR_FILENO);

    flx_proc_fd_install(STDIN_FILENO, console_node);
    flx_proc_fd_install(STDOUT_FILENO, console_node);
    flx_proc_fd_install(STDERR_FILENO, console_node);
    return 0;
}
