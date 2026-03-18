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
    /* The OS blindly passes whatever ops table the board gave it down to the driver */
    ret = serial_init(dev_id, hw_instance, ops);
    if (ret != 0) {
        return ret;
    }

    FlxDev_t *console_dev = flx_dev_get(0);
    if (!console_dev)
        return -1;

    flx_proc_fd_bind(STDIN_FILENO, console_dev);
    flx_proc_fd_bind(STDOUT_FILENO, console_dev);
    flx_proc_fd_bind(STDERR_FILENO, console_dev);

    /* =========================================================
     * ENTER USER SPACE
     * ========================================================= */

    return 0;
}
