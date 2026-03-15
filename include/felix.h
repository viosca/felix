#ifndef FELIX_OS_H
#define FELIX_OS_H

#include "drivers/serial.h"

/**
 * @brief Bootstraps the core Felix OS subsystems (VFS, Process Table, etc.).
 * Must be called before any POSIX APIs or the shell are used.
 */
void felix_init(void);

/**
 * @brief Registers a hardware UART instance to a POSIX device ID.
 * @param dev_id The file descriptor ID (e.g., 0 for stdin/stdout/stderr).
 * @param hw_instance Opaque pointer to the vendor hardware struct.
 * @return 0 on success, negative on error.
 */
int felix_register_uart(int dev_id, void *hw_instance, const FlxUartOps_t *ops);
#endif /* FELIX_OS_H */
