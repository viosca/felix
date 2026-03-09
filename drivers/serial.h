/**
 * @file serial.h
 * @brief Dumb serial driver wrapping the MSS UART hardware.
 */

#ifndef _FLX_SERIAL_H
#define _FLX_SERIAL_H

#include <stddef.h>
#include "kernel/devices.h"

#ifdef __cplusplus
extern "C" {
#endif
/* --- Generic HAL User Data Container --- */

#define FLX_SERIAL_MAX_PORTS 5

/* --- Inline Type-Safe Getters --- */

/**
 * @brief Initialize the serial subsystem, TTY shim, and hardware.
 */
int serial_init(int dev_id, mss_uart_instance_t *uart);

/**
 * @brief VFS/Posix Read wrapper. Hands off to the TTY shim.
 */
int serial_read(FlxDev_t *dev, char *buf, size_t count);

/**
 * @brief VFS/Posix Write wrapper. Hands off to the TTY shim.
 */
int serial_write(FlxDev_t *dev, const char *buf, size_t count);

/**
 * @brief VFS/Posix Ioctl wrapper. Hands off to the TTY shim.
 */
int serial_ioctl(FlxDev_t *dev, int request, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_SERIAL_H */
