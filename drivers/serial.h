#ifndef FELIX_DRIVERS_SERIAL_H
#define FELIX_DRIVERS_SERIAL_H
#include <stdbool.h>
#include "drivers/mss/mss_mmuart/mss_uart.h"

/* =========================================================================
 * 16550 Register Manifest Constants
 * ========================================================================= */

/* IER - Interrupt Enable Register */
#define UART_IER_ERBFI      0x01  /* Enable Receiver Buffer Full Interrupt */
#define UART_IER_ETBEI      0x02  /* Enable Transmit Holding Register Empty Interrupt */

/* IIR - Interrupt Identity Register */
#define UART_IIR_NO_INT     0x01  /* Bit 0: 1 = No Interrupt Pending */
#define UART_IIR_ID_MASK    0x0F  /* Mask for Interrupt ID */

#define UART_IIR_ID_THRE    0x02  /* ID: Transmit Holding Register Empty */
#define UART_IIR_ID_RDA     0x04  /* ID: Received Data Available */
#define UART_IIR_ID_RLS     0x06  /* ID: Receiver Line Status (Errors) */
#define UART_IIR_ID_CTI     0x0C  /* ID: Character Timeout (Data in FIFO but idle) */

/* LSR - Line Status Register */
#define UART_LSR_DR         0x01  /* Data Ready */
#define UART_LSR_OE         0x02  /* Overrun Error */
#define UART_LSR_PE         0x04  /* Parity Error */
#define UART_LSR_FE         0x08  /* Framing Error */
#define UART_LSR_THRE       0x20  /* Transmitter Holding Register Empty */

static bool is_felix_device(mss_uart_instance_t *uart);

/* High-Level OS Mount
 * 1. Registers the UART to a File Descriptor (fd).
 * 2. Initializes the Ring Buffers for that fd.
 * 3. Calls serial_init_hw() to start the silicon.
 */
int serial_mount(int fd, mss_uart_instance_t *uart);

/* The Central ISR for Serial Devices
 * Use this in your vector table or call it from the main PLIC handler.
 */
void serial_isr(mss_uart_instance_t *uart);

#endif // FELIX_DRIVERS_SERIAL_H
