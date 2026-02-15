/**
 * @file serial.h
 * @brief High-level Serial Driver for Felix OS.
 *
 * This file contains the definitions for the 16550-compatible UART registers
 * (IER, IIR, LSR) and the public API for mounting a UART hardware instance
 * into the Felix Virtual File System.
 */

#ifndef FELIX_DRIVERS_SERIAL_H
#define FELIX_DRIVERS_SERIAL_H

#include <stdbool.h>
#include <sys/types.h> // For ssize_t
#include "drivers/mss/mss_mmuart/mss_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * 16550 Register Manifest Constants
 * Used for direct hardware access in the ISR.
 * ========================================================================= */

/**
 * @name Interrupt Enable Register (IER)
 * @brief Offset 0x01 (DLAB=0). Controls which events generate interrupts.
 * @{
 */
#define UART_IER_ERBFI      0x01  /**< Enable Receiver Buffer Full Interrupt */
#define UART_IER_ETBEI      0x02  /**< Enable Transmit Holding Register Empty Interrupt */
#define UART_IER_ELSI       0x04  /**< Enable Receiver Line Status Interrupt */
#define UART_IER_EDSSI      0x08  /**< Enable Modem Status Interrupt */
/** @} */

/**
 * @name Interrupt Identity Register (IIR)
 * @brief Offset 0x02. Read-only. Identifies the highest priority pending interrupt.
 * @{
 */

#define UART_IIR_ID_MS      0x00  /**< ID: Modem Status (Lowest Priority) */
#define UART_IIR_NO_INT     0x01  /**< Bit 0: 1 = No Interrupt Pending */
#define UART_IIR_ID_THRE    0x02  /**< ID: Transmit Holding Register Empty */
#define UART_IIR_ID_RDA     0x04  /**< ID: Received Data Available */
#define UART_IIR_ID_RLS     0x06  /**< ID: Receiver Line Status (Highest Priority) */
#define UART_IIR_ID_CTI     0x0C  /**< ID: Character Timeout (Data in FIFO but idle) */
#define UART_IIR_ID_MASK    0x0F  /**< Mask for Interrupt ID bits */
/** @} */

/**
 * @name Line Status Register (LSR)
 * @brief Offset 0x05. Provides status of data transfer.
 * @{
 */
#define UART_LSR_DR         0x01  /**< Data Ready */
#define UART_LSR_OE         0x02  /**< Overrun Error */
#define UART_LSR_PE         0x04  /**< Parity Error */
#define UART_LSR_FE         0x08  /**< Framing Error */
#define UART_LSR_BI         0x10  /**< Break Interrupt */
#define UART_LSR_THRE       0x20  /**< Transmitter Holding Register Empty */
#define UART_LSR_TEMT       0x40  /**< Transmitter Empty (Shift Register Empty) */
#define UART_LSR_RXFE       0x80  /**< Error in RX FIFO */
/** @} */

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Mounts a physical UART peripheral to a file descriptor.
 *
 * This function initializes the hardware, enables the specific interrupts,
 * sets up the software ring buffers, and registers the device in the
 * global device table.
 *
 * @param fd The file descriptor to assign (e.g., 0, 1, 2 for stdio).
 * @param uart Pointer to the MSS UART hardware instance (e.g., &g_mss_uart0_lo).
 * @return 0 on success, -1 on failure.
 */
int felix_serial_mount(int fd, mss_uart_instance_t *uart);

/**
 * @brief writes data to the serial device.
 * Encapsulates ring buffer logic, blocking, and interrupt kicking.
 * @param device Pointer to the felix_device_t context.
 * @param ptr Data buffer.
 * @param len Length of data.
 * @return Number of bytes written, or -1 on error.
 */
ssize_t felix_serial_write(void *device, const void *ptr, size_t len);

/**
 * @brief Reads data from the serial device.
 * Encapsulates ring buffer logic and blocking waits.
 * @param device Pointer to the felix_device_t context.
 * @param ptr Output buffer.
 * @param len Maximum bytes to read.
 * @return Number of bytes read, or -1 on error.
 */
ssize_t felix_serial_read(void *device, void *ptr, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* FELIX_DRIVERS_SERIAL_H */
