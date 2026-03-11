#ifndef FELIX_ARCH_UART_HAL_H
#define FELIX_ARCH_UART_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "kernel/devices.h"

/* =========================================================================
 * 1. OS Callbacks (Implemented in drivers/serial.c)
 * The hardware vendor's ISRs call these to move data in/out of the OS!
 * ========================================================================= */

/**
 * @brief Called by the Hardware RX Interrupt to pass a received byte to the OS.
 */
void flx_serial_os_rx_isr(FlxDev_t *dev, char c);

/**
 * @brief Called by the Hardware TX Interrupt to grab pending bytes from the OS.
 * @param buf Buffer provided by the HAL to fill.
 * @param max_bytes The maximum size of the hardware FIFO.
 * @param buffer_empty [out] Set to true if the OS ring buffer is now empty.
 * @return The number of bytes actually copied into the buffer.
 */
size_t flx_serial_os_tx_isr(FlxDev_t *dev, uint8_t *buf, size_t max_bytes, bool *buffer_empty);

/* =========================================================================
 * 2. Hardware Operations Table (Implemented in arch/.../uart_xxx.c)
 * The OS calls these to control the hardware without knowing what it is!
 * ========================================================================= */

typedef struct {
    void (*disable_irq)(void *hw_instance);
    void (*enable_irq)(void *hw_instance);
    void (*wake_tx)(void *hw_instance);
    void (*sleep_cpu)(void); /* e.g., 'wfi' on RISC-V or '__WFI()' on ARM */
} flx_uart_hal_ops_t;

/**
 * @brief Bootstraps the specific hardware architecture for a UART device.
 * @param dev The VFS device node being initialized.
 * @param hw_instance Opaque pointer to the hardware struct (e.g. mss_uart_instance_t)
 * @param ops [out] The operations table the OS will use to control it.
 */
int flx_hal_uart_init(FlxDev_t *dev, void *hw_instance, flx_uart_hal_ops_t *ops);

#endif /* FELIX_ARCH_UART_HAL_H */