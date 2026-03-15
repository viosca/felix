#include "felix.h"
#include <stdio.h>
#include <unistd.h>
#include "kernel/process.h"
#include "kernel/devices.h"
#include "fs/bin/flx_fs_bin.h"
#include "drivers/serial.h"

void felix_init(void) {
    /* Initialize core software subsystems */
    flx_dev_init();           /* Zero out the global device table */
    flx_proc_init_first();    /* Setup FD table and set CWD to VFS root (/) */
    flx_fs_bin_init();         /* Mounts /bin and registers the executables! */
}

int felix_register_uart(int dev_id, void *hw_instance) {
    /* Route the opaque hardware pointer safely down to the HAL */
    serial_init(dev_id, hw_instance);

    FlxDev_t *console_dev = flx_dev_get(0);
    if (!console_dev)
        return -1;

    flx_proc_fd_bind(FLX_PROC_PID_SHELL, STDIN_FILENO, console_dev);
    flx_proc_fd_bind(FLX_PROC_PID_SHELL, STDOUT_FILENO, console_dev);
    flx_proc_fd_bind(FLX_PROC_PID_SHELL, STDERR_FILENO, console_dev);

    /* =========================================================
     * ENTER USER SPACE
     * ========================================================= */

    return 0;
}
