/**
 * @file devices.h
 * @brief Core Device Abstraction Layer for Felix OS.
 *
 * This file defines the structures and types used to manage the global device table.
 * It acts as the bridge between standard POSIX system calls (open, read, write, ioctl)
 * and the specific hardware drivers (UART, SPI, etc.).
 */

#ifndef FELIX_KERNEL_DEVICES_H
#define FELIX_KERNEL_DEVICES_H

#include "drivers/mss/mss_mmuart/mss_uart.h"
#include "kernel/ring_buffer.h"

/** * @brief Maximum number of simultaneously open files/devices.
 * Defines the size of the global device table.
 */
#define FELIXDEV_MAX_OPEN_FILES    16

/**
 * @brief Internal Device Flags.
 * * These flags control the behavior of the device driver. They are typically
 * modified via ioctl() calls using standard termios flags (ECHO, ICANON),
 * which are mapped to these internal bitfields.
 *
 * @note We use lowercase names (e.g., echo_mode) to avoid naming conflicts
 * with the standard POSIX macros (like #define ECHO) found in termios.h.
 */
typedef union {
    uint32_t lword; /**< Access flags as a single 32-bit word */
    struct {
        /**
         * @brief Canonical Mode (Cooked).
         * - 1: Input is processed line-by-line. Backspace and Enter are handled.
         * - 0: Raw mode. Input is available immediately.
         */
        uint32_t cooked_mode : 1;

        /**
         * @brief Non-Blocking I/O.
         * - 1: read() returns -1 (errno=EAGAIN) if buffer is empty.
         * - 0: read() blocks (waits) until data is available.
         */
        uint32_t nonblock    : 1;

        /**
         * @brief Echo Mode.
         * - 1: Characters are echoed back to the sender as they are received.
         * - 0: Silent mode (useful for binary transfers or password entry).
         */
        uint32_t echo_mode   : 1;

        uint32_t reserved    : 29; /**< Reserved for future flags */
    };
} FelixDevIOFlags_t;

/**
 * @brief Supported Device Types.
 * Used to dispatch system calls to the correct driver logic.
 */
typedef enum {
    FelixDevType_INVALID = -1,/**< Invalid File Descriptor (Out of bounds) */
    FelixDevType_UNKNOWN = 0, /**< Uninitialized or invalid device */
    FelixDevType_UART,        /**< Serial UART Device (16550 compatible) */
    FelixDevType_SPI,         /**< SPI Controller */
    FelixDevType_I2C          /**< I2C Controller */
} FelixDevType_t;

/**
 * @brief The Device Control Block (DCB).
 * * Represents a single open file/device in the system. Contains identity,
 * configuration flags, hardware references, and data buffers.
 */
typedef struct {
    /* --- Identity --- */
    int fd_id;               /**< The File Descriptor index (0, 1, 2...) */
    FelixDevType_t type;     /**< The type of hardware driver to use */
    FelixDevIOFlags_t flags; /**< Current configuration state */

    /* --- Hardware Pointer --- */
    union {
        mss_uart_instance_t *uart; /**< Pointer to Microchip MSS UART instance */
        void *generic;             /**< Generic pointer for other drivers */
    } hw;

    /* --- Buffers --- */
    FelixRingBuffer_t rx_buf; /**< Software Receive Ring Buffer (ISR -> User) */
    FelixRingBuffer_t tx_buf; /**< Software Transmit Ring Buffer (User -> ISR) */
} FelixDev_t;
/**
 * @brief The Global Device Table.
 * * An array of device structures indexed by the file descriptor ID.
 * Accessing g_felix_devices[fd] provides the context for that file.
 */
extern FelixDev_t g_felix_devices[FELIXDEV_MAX_OPEN_FILES];

/**
 * @brief Safely retrieves a device context from a file descriptor.
 * * This static inline helper abstracts the storage mechanism of the device table.
 * It performs bounds checking and returns NULL if the FD is invalid.
 * * @param fd The file descriptor index.
 * @return Pointer to the device context, or NULL if invalid.
 */
static inline FelixDev_t* felixdev_get(int fd) {
    if (fd < 0 || fd >= FELIXDEV_MAX_OPEN_FILES) {
        return NULL; /* Or (void*)0 */
    }
    return &g_felix_devices[fd];
}

/**
 * @brief Safely retrieves a device type from a device.
 * Provides read-only access to the device type without needing the full struct.
 * @param dev A pointer to the device.
 * @return The device type, or DEV_TYPE_INVALID if fd is bad.
 */
static inline FelixDevType_t felixdev_type_get(FelixDev_t *dev) {
    if (!dev)
        return FelixDevType_INVALID;

    return dev->type;
}


/**
 * @brief Safely retrieves a device type from a file descriptor.
 * Provides read-only access to the device type without needing the full struct.
 * @param fd The file descriptor index.
 * @return The device type, or DEV_TYPE_INVALID if fd is bad.
 */
static inline FelixDevType_t felixdev_fd_type_get(int fd) {
    FelixDev_t *dev = felixdev_get(fd);
    return felixdev_type_get(dev);
}

/**
 * @brief Registers a hardware peripheral into the Virtual File System.
 *
 * @param fd The file descriptor index to assign (e.g., 0 for stdin).
 * @param type The type of device being registered.
 * @param hw_handle Pointer to the low-level hardware structure (e.g., mss_uart_instance_t*).
 * @return 0 on success, -1 on failure.
 */
FelixDev_t *felixdev_register(int fd, FelixDevType_t type, void *hw_handle);

/**
 * @brief Kernel-side handler for the ioctl() system call.
 *
 * Handles configuration requests such as setting baud rate, toggling echo,
 * or enabling non-blocking mode.
 *
 * @param fd The file descriptor.
 * @param request The ioctl request code (e.g., TCSETS, FIONBIO).
 * @param arg Pointer to the argument data (depends on request).
 * @return 0 on success, -1 on error (with errno set).
 */
int felixdev_ioctl(int fd, int request, void *arg);

#endif /* FELIX_KERNEL_DEVICES_H */
