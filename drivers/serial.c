/**
 * @file serial.c
 * @brief UART Driver Implementation for Felix OS.
 *
 * Implements the high-level serial driver logic, including interrupt service
 * routines (ISRs), ring buffer management, and hardware initialization.
 * This driver bridges the Microchip MSS UART hardware with the Felix
 * Virtual File System.
 */

#include <errno.h>
#include "drivers/serial.h"
#include "kernel/devices.h" // For ring buffers
#include "mpfs_hal/mss_hal.h"

/* -------------------------------------------------------------------------
 * Internal Helpers
 * ------------------------------------------------------------------------- */

/**
 * @brief Maps a UART instance pointer to its PLIC Interrupt ID.
 * * Used during initialization to determine which PLIC interrupt line needs
 * to be enabled for the specific UART hardware instance.
 *
 * @param uart Pointer to the MSS UART instance (e.g., &g_mss_uart0_lo).
 * @return The PLIC IRQ number (e.g., PLIC_MMUART0_INT_OFFSET).
 */
static PLIC_IRQn_Type get_uart_irq_source(mss_uart_instance_t *uart) {
    if (uart == &g_mss_uart0_lo || uart == &g_mss_uart0_hi) return PLIC_MMUART0_INT_OFFSET;
    if (uart == &g_mss_uart1_lo || uart == &g_mss_uart1_hi) return PLIC_MMUART1_INT_OFFSET;
    if (uart == &g_mss_uart2_lo || uart == &g_mss_uart2_hi) return PLIC_MMUART2_INT_OFFSET;
    if (uart == &g_mss_uart3_lo || uart == &g_mss_uart3_hi) return PLIC_MMUART3_INT_OFFSET;
    if (uart == &g_mss_uart4_lo || uart == &g_mss_uart4_hi) return PLIC_MMUART4_INT_OFFSET;
    return 0;
}

/**
 * @brief Maps a UART instance pointer to its MSS Peripheral ID.
 * * Used to control the clock gating for the specific UART peripheral.
 * The clock must be explicitly enabled via mss_config_clk_rst() before
 * the UART registers can be accessed reliably.
 *
 * @param uart Pointer to the MSS UART instance.
 * @return The MSS Peripheral ID (e.g., MSS_PERIPH_MMUART0).
 */
static mss_peripherals get_uart_peripheral_id(mss_uart_instance_t *uart) {
    if (uart == &g_mss_uart0_lo || uart == &g_mss_uart0_hi) return MSS_PERIPH_MMUART0;
    if (uart == &g_mss_uart1_lo || uart == &g_mss_uart1_hi) return MSS_PERIPH_MMUART1;
    if (uart == &g_mss_uart2_lo || uart == &g_mss_uart2_hi) return MSS_PERIPH_MMUART2;
    if (uart == &g_mss_uart3_lo || uart == &g_mss_uart3_hi) return MSS_PERIPH_MMUART3;
    if (uart == &g_mss_uart4_lo || uart == &g_mss_uart4_hi) return MSS_PERIPH_MMUART4;
    return MSS_PERIPH_MMUART0;
}

/**
 * @brief Type-safe converter from Vendor UART handle to Felix Device.
 * * Retrieves the Felix OS device context stored in the vendor driver's
 * user_data field. This wraps the void* cast to improve readability
 * and safety.
 *
 * @param uart Pointer to the MSS UART instance.
 * @return Pointer to the Felix device context, or NULL if not set.
 */
static inline FelixDev_t *felix_serial_dev_from_uart_get(mss_uart_instance_t *uart) {
    return (FelixDev_t *)uart->user_data;
}

/**
 * @brief Checks if a specific UART is managed by the Felix driver.
 *
 * The PLIC handlers check this to decide whether to route the interrupt
 * to Felix's ring-buffer driver (`serial_isr`) or the vendor's polled/legacy
 * driver (`uart_isr`).
 *
 * @param uart Pointer to the MSS UART instance.
 * @return true if the UART is registered with Felix (user_data points to device), false otherwise.
 */
static bool felix_serial_is_device(mss_uart_instance_t *uart) {
    if (!uart)
        return false;

    FelixDev_t *dev = felix_serial_dev_from_uart_get(uart);

    if (dev == NULL)
        return false;

    if (dev->type != FelixDevType_UART)
        return false;

    return true;
}

/* -------------------------------------------------------------------------
 * Core API
 * ------------------------------------------------------------------------- */

/**
 * @brief Initializes and mounts a UART device.
 * * @see serial.h for API definition.
 * * Implementation Notes:
 * 1. Registers device in Global Device Table.
 * 2. Initializes software Ring Buffers.
 * 3. Un-gates the hardware clock using the current Hart ID.
 * 4. Configures 16550 Line Control settings (8N1, 115200).
 * 5. Enables RX interrupts in the UART IER.
 * 6. Enables the PLIC line.
 */
int felix_serial_mount(int fd, mss_uart_instance_t *uart) {
    FelixDev_t *dev = felixdev_register(fd, FelixDevType_UART, uart);
    if (!dev)
        return -1;
    dev->fd_id = fd;
    dev->flags.lword = 0;
    dev->flags.cooked_mode = true;
    dev->flags.echo_mode = true;

    felix_ringbuffer_init(&dev->rx_buf);
    felix_ringbuffer_init(&dev->tx_buf);

    uart->user_data = (void *)dev;

    mss_peripherals periph_id = get_uart_peripheral_id(uart);
    uint64_t current_hartid = read_csr(mhartid);
    mss_config_clk_rst(periph_id, (uint8_t)current_hartid, PERIPHERAL_ON);

    MSS_UART_init(uart, MSS_UART_115200_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

    PLIC_IRQn_Type irq_source = get_uart_irq_source(uart);

    // Enable 16550 RX Interrupts
    uart->hw_reg->IER = UART_IER_ERBFI;

    PLIC_SetPriority(irq_source, 1);
    PLIC_EnableIRQ(irq_source);

    return 0;
}
/* -------------------------------------------------------------------------
 * Data Transfer Routines
 * ------------------------------------------------------------------------- */

ssize_t felix_serial_write(void *device, const void *ptr, size_t len) {
    FelixDev_t *dev = (FelixDev_t *)device;
    mss_uart_instance_t *uart = dev->hw.uart;
    const char *cptr = (const char *)ptr;

    for (size_t i = 0; i < len; i++) {
        char c = cptr[i];

        /* Cooked Mode: Map \n to \r\n */
        if (dev->flags.cooked_mode && c == '\n') {
            while (felix_ringbuffer_is_full(&dev->tx_buf)) {
                uart->hw_reg->IER |= UART_IER_ETBEI;
            }
            felix_ringbuffer_push(&dev->tx_buf, '\r');
        }

        /* Wait for space in software buffer */
        while (felix_ringbuffer_is_full(&dev->tx_buf)) {
             uart->hw_reg->IER |= UART_IER_ETBEI;
        }

        felix_ringbuffer_push(&dev->tx_buf, c);

        /* ---------------------------------------------------------
         * PRIME THE PUMP
         * If the UART TX interrupt is currently disabled, it means
         * the ISR went idle. We must manually kickstart the hardware.
         * --------------------------------------------------------- */
        if ((uart->hw_reg->IER & UART_IER_ETBEI) == 0) {
            /* Check 16550 LSR: Is the Holding Register Empty? */
            if (uart->hw_reg->LSR & UART_LSR_THRE) {
                uint8_t prime_byte;
                if (felix_ringbuffer_pop(&dev->tx_buf, &prime_byte)) {
                    uart->hw_reg->THR = prime_byte;
                }
            }
        }

        /* Re-enable TX Interrupts */
        uart->hw_reg->IER |= UART_IER_ETBEI;
    }

    return (ssize_t)len;
}

ssize_t felix_serial_read(void *device, void *ptr, size_t len) {
    FelixDev_t *dev = (FelixDev_t *)device;

    /* Handle Blocking / Non-Blocking Mode */
    if (!dev->flags.nonblock) {
        // Blocking: Spin until at least 1 byte is available
        while (felix_ringbuffer_is_empty(&dev->rx_buf)) {
            // Optional: __asm("wfi"); to save power
        }
    } else {
        // Non-Blocking: Return Error if empty
        if (felix_ringbuffer_is_empty(&dev->rx_buf)) {
            errno = EAGAIN;
            return -1;
        }
    }

    /* Drain available data up to 'len' */
    size_t count = 0;
    char *cptr = (char *)ptr;
    while (count < len && !felix_ringbuffer_is_empty(&dev->rx_buf)) {
        uint8_t c;
        if (felix_ringbuffer_pop(&dev->rx_buf, &c)) {
            cptr[count++] = (char)c;
        }
    }
    return (ssize_t)count;
}


/* -------------------------------------------------------------------------
 * Interrupt Service Routine
 * ------------------------------------------------------------------------- */

/**
 * @brief The Central UART Interrupt Service Routine.
 *
 * Handles both RX (Data Available) and TX (Transmitter Empty) interrupts.
 * It manages the transfer of data between the hardware FIFOs and the
 * software ring buffers. It also implements "Cooked Mode" processing
 * (echo, backspace) at the interrupt level for low-latency feedback.
 *
 * @param this_uart Pointer to the hardware instance triggering the IRQ.
 */
void felix_serial_isr(mss_uart_instance_t *this_uart) {
    uint8_t iir;
    FelixDev_t *dev = felix_serial_dev_from_uart_get(this_uart);

    iir = this_uart->hw_reg->IIR;

    while (!((iir & UART_IIR_ID_MASK) == UART_IIR_NO_INT)) {

        uint8_t id = iir & UART_IIR_ID_MASK;

        // --- RX Logic ---
        if (id == UART_IIR_ID_RDA || id == UART_IIR_ID_CTI) {
            uint8_t rx_byte = this_uart->hw_reg->RBR;

            if (!felix_ringbuffer_is_full(&dev->rx_buf)) {

                // Backspace Handling
                if (dev->flags.cooked_mode && (rx_byte == 0x7F || rx_byte == 0x08)) {
                    if (!felix_ringbuffer_is_empty(&dev->rx_buf)) {
                        felix_ringbuffer_unpush(&dev->rx_buf);
                        if (dev->flags.echo_mode) {
                            felix_ringbuffer_push(&dev->tx_buf, 0x08);
                            felix_ringbuffer_push(&dev->tx_buf, ' ');
                            felix_ringbuffer_push(&dev->tx_buf, 0x08);
                            this_uart->hw_reg->IER |= UART_IER_ETBEI;
                        }
                    }
                    continue;
                }

                // Enter Handling
                if (dev->flags.cooked_mode && rx_byte == '\r') {
                    rx_byte = '\n';
                }

                felix_ringbuffer_push(&dev->rx_buf, rx_byte);

                // Echo Handling
                if (dev->flags.echo_mode) {
                    if (rx_byte == '\n') {
                        felix_ringbuffer_push(&dev->tx_buf, '\r');
                        felix_ringbuffer_push(&dev->tx_buf, '\n');
                    } else {
                        felix_ringbuffer_push(&dev->tx_buf, rx_byte);
                    }
                    this_uart->hw_reg->IER |= UART_IER_ETBEI;
                }
            }
        }

        // --- TX Logic ---
        else if (id == UART_IIR_ID_THRE) {
            uint8_t tx_byte;
            if (felix_ringbuffer_pop(&dev->tx_buf, &tx_byte)) {
                this_uart->hw_reg->THR = tx_byte;
            }
            if (felix_ringbuffer_is_empty(&dev->tx_buf)) {
                 this_uart->hw_reg->IER &= ~UART_IER_ETBEI;
            }
        }

        iir = this_uart->hw_reg->IIR;
    }
}


/* -------------------------------------------------------------------------
 * Interrupt Vectors (Complete Set)
 * ------------------------------------------------------------------------- */

/* Weak symbol declaration for the vendor ISR logic */
extern void uart_isr(mss_uart_instance_t *this_uart);

inline static uint8_t dispatch_uart_handler(mss_uart_instance_t *uart_lo, mss_uart_instance_t* uart_hi) {
    if (felix_serial_is_device(uart_lo))
        felix_serial_isr(uart_lo);
    else if (felix_serial_is_device(uart_hi))
        felix_serial_isr(uart_hi);
    else
        uart_isr(uart_lo);
    return EXT_IRQ_KEEP_ENABLED;
}
/**
 * @brief MMUART0 Interrupt Handler.
 * Overrides the weak default in mss_uart.c. Routes to Felix or Vendor driver.
 */
uint8_t PLIC_mmuart0_IRQHandler(void) {
    return dispatch_uart_handler(&g_mss_uart0_lo, &g_mss_uart0_hi);
}

/**
 * @brief MMUART1 Interrupt Handler.
 * Overrides the weak default in mss_uart.c. Routes to Felix or Vendor driver.
 */
uint8_t PLIC_mmuart1_IRQHandler(void) {
    return dispatch_uart_handler(&g_mss_uart1_lo, &g_mss_uart1_hi);
}

/**
 * @brief MMUART2 Interrupt Handler.
 * Overrides the weak default in mss_uart.c. Routes to Felix or Vendor driver.
 */
uint8_t PLIC_mmuart2_IRQHandler(void) {
    return dispatch_uart_handler(&g_mss_uart2_lo, &g_mss_uart2_hi);
}

/**
 * @brief MMUART3 Interrupt Handler.
 * Overrides the weak default in mss_uart.c. Routes to Felix or Vendor driver.
 */
uint8_t PLIC_mmuart3_IRQHandler(void) {
    return dispatch_uart_handler(&g_mss_uart3_lo, &g_mss_uart3_hi);
}

/**
 * @brief MMUART4 Interrupt Handler.
 * Overrides the weak default in mss_uart.c. Routes to Felix or Vendor driver.
 */
uint8_t PLIC_mmuart4_IRQHandler(void) {
    return dispatch_uart_handler(&g_mss_uart4_lo, &g_mss_uart4_hi);
}
