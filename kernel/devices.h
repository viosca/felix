/**
 * @file devices.h
 * @brief Core Device Abstraction Layer for Felix OS.
 *
 * This file defines the structures and types used to manage the global device table.
 * It acts as the bridge between standard POSIX system calls (open, read, write, ioctl)
 * and the specific hardware drivers (UART, SPI, etc.).
 */

#ifndef FLX_KERNEL_DEVICES_H
#define FLX_KERNEL_DEVICES_H

#include <stddef.h>
#include "kernel/tty.h"
#include "kernel/ring_buffer.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"

/** * @brief Maximum number of simultaneously open files/devices.
 * Defines the size of the global device table.
 * Here we define one for each uart.
 */
#define FLX_DEV_MAX_DEVICES 5


/**
 * @brief Supported Device Types.
 * Used to dispatch system calls to the correct driver logic.
 */
typedef enum {
    FlxDevType_INVALID = -1,/**< Invalid File Descriptor (Out of bounds) */
    FlxDevType_UNKNOWN = 0, /**< Uninitialized or invalid device */
    FlxDevType_UART,        /**< Serial UART Device (16550 compatible) */
    FlxDevType_SPI,         /**< SPI Controller */
    FlxDevType_I2C          /**< I2C Controller */
} FlxDevType_t;

/**
 * @brief The Device Control Block (DCB).
 * * Represents a single open file/device in the system. Contains identity,
 * configuration flags, hardware references, and data buffers.
 */
typedef struct {
    /* --- Identity --- */
    int id; /**< Kernel Pool ID */
    FlxDevType_t type;     /**< The type of hardware driver to use */

    mss_uart_instance_t *uart; /**< */

    /* * The TTY Line Discipline.
     * NOTE: The RX ring buffer is located inside the tty struct
     * (tty.rx_buffer) because the TTY manages canonical line buffering.
     */
    tty_t tty;

    /* --- Buffers --- */
    ring_buffer_t tx_buffer; /**< uart tx ring buffer. */

    void *rx_semaphore; /**< */

} FlxDev_t;

/**
 * @brief Allocates a new Device Object from the pool.
 * Does NOT assign a file descriptor.
 */
FlxDev_t *flx_dev_create(FlxDevType_t type, void *hw_handle);

FlxDev_t *flx_dev_get(int id);

static inline mss_uart_instance_t *flx_dev_uart_get(FlxDev_t *dev) {
    return dev->uart;
}

/**
 * @brief Safely retrieves a device type from a device.
 * Provides read-only access to the device type without needing the full struct.
 * @param dev A pointer to the device.
 * @return The device type, or DEV_TYPE_INVALID if fd is bad.
 */
static inline FlxDevType_t flx_dev_type_get(FlxDev_t *dev) {
    if (!dev)
        return FlxDevType_INVALID;

    return dev->type;
}

void flx_dev_init(void);

/**
 * @brief Claim and register a new device slot.
 */
FlxDev_t *flx_dev_register(int id, FlxDevType_t type);


#endif /* FLX_KERNEL_DEVICES_H */
