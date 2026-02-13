#include "drivers/serial.h"
#include "kernel/devices.h" // For ring buffers
#include "mpfs_hal/mss_hal.h"

/* -------------------------------------------------------------------------
 * Internal Helpers (Moved from u54_1.c)
 * ------------------------------------------------------------------------- */

// Map pointer to PLIC IRQ Offset
static PLIC_IRQn_Type get_uart_irq_source(mss_uart_instance_t *uart) {
    if (uart == &g_mss_uart0_lo || uart == &g_mss_uart0_hi) return PLIC_MMUART0_INT_OFFSET;
    if (uart == &g_mss_uart1_lo || uart == &g_mss_uart1_hi) return PLIC_MMUART1_INT_OFFSET;
    if (uart == &g_mss_uart2_lo || uart == &g_mss_uart2_hi) return PLIC_MMUART2_INT_OFFSET;
    if (uart == &g_mss_uart3_lo || uart == &g_mss_uart3_hi) return PLIC_MMUART3_INT_OFFSET;
    if (uart == &g_mss_uart4_lo || uart == &g_mss_uart4_hi) return PLIC_MMUART4_INT_OFFSET;
    return 0;
}

// Map pointer to Peripheral ID
static mss_peripherals get_uart_peripheral_id(mss_uart_instance_t *uart) {
    if (uart == &g_mss_uart0_lo) return MSS_PERIPH_MMUART0;
    if (uart == &g_mss_uart1_lo) return MSS_PERIPH_MMUART1;
    if (uart == &g_mss_uart2_lo) return MSS_PERIPH_MMUART2;
    if (uart == &g_mss_uart3_lo) return MSS_PERIPH_MMUART3;
    if (uart == &g_mss_uart4_lo) return MSS_PERIPH_MMUART4;
    return MSS_PERIPH_MMUART0;
}

/* =========================================================================
 * Helper: Detect if this UART is owned by Felix OS
 * ========================================================================= */
static bool is_felix_device(mss_uart_instance_t *uart) {
    // If serial_mount() wasn't called, user_data is NULL.
    if (uart->user_data == NULL) {
        return false;
    }

    // (Optional Safety) Verify the device type signature
    felix_device_t *dev = (felix_device_t *)uart->user_data;
    if (dev->type != DEV_TYPE_UART) {
        return false;
    }

    // It is a Felix device.
    return true;
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */
#if 0
int serial_init_hw(mss_uart_instance_t *uart) {
    PLIC_IRQn_Type irq_source = get_uart_irq_source(uart);
    if (irq_source == 0) return -1;

    // 1. Turn on the Clock (Sub-block clock enable)
    // Note: This register access is specific to MPFS; usually safe to do here.
    mss_peripherals periph_id = get_uart_peripheral_id(uart);
    SYSREG->SUBBLK_CLOCK_CR |= (1u << periph_id);

    // 2. Initialize MSS UART Driver
    MSS_UART_init(uart, MSS_UART_115200_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

    // 3. Enable Local UART Interrupts (RX and TX)
    // We enable TBE (Transmit Buffer Empty) only when we actually have data to send.
    // For init, we just set the mode.
    // MSS_UART_set_mode(uart, MSS_UART_FIFO_MODE); // If you want FIFO
    // OR just enable RX for now:
    MSS_UART_enable_irq(uart, MSS_UART_RBF_IRQ);

    // 4. Configure PLIC
    PLIC_SetPriority(irq_source, 1);
    PLIC_EnableIRQ(irq_source);

    // Ensure Global Interrupts are enabled (handled in main usually, but good to check)
    // __enable_irq();

    return 0;
}
#endif

int serial_mount(int fd, mss_uart_instance_t *uart) {
    // 1. Register with VFS
    if (felix_register_device(fd, DEV_TYPE_UART, uart) < 0) return -1;

    // 2. Setup Device Context
    felix_device_t *dev = &g_felix_devices[fd];
    dev->fd_id = fd;
    dev->flags.lword = 0;
    dev->flags.cooked_mode = true; // Default to cooked for Console
    dev->flags.echo_mode = true; // Default to cooked for Console

    rb_init(&dev->rx_buf);
    rb_init(&dev->tx_buf);

    // Link Hardware -> Software
    // This connects the 'user_data' pointer to our device structure
    uart->user_data = (void *)dev;

    // Hardware Init (MSS Driver)

    PLIC_IRQn_Type irq_source = get_uart_irq_source(uart);
    mss_peripherals periph_id = get_uart_peripheral_id(uart);
    uint64_t current_hartid = read_csr(mhartid);

    // Un-gate the clock to the specific UART
    mss_config_clk_rst(periph_id, (uint8_t) current_hartid, PERIPHERAL_ON);

    MSS_UART_init(uart, MSS_UART_115200_BAUD, MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

    // Interrupt Setup
    // Explicitly Enable RX Interrupt, Explicitly Disable TX Interrupt
    uart->hw_reg->IER = UART_IER_ERBFI;

    // Enable in PLIC
    PLIC_SetPriority(irq_source, 1);
    PLIC_EnableIRQ(irq_source);

    return 0;
}

uint8_t PLIC_mmuart1_IRQHandler(void)
{
    extern void uart_isr(mss_uart_instance_t *this_uart);
    // Check if this UART is managed by Felix (Console)
    if (is_felix_device(&g_mss_uart1_lo)) {
        serial_isr(&g_mss_uart1_lo);
    }
    // Otherwise fallback to vendor driver (e.g. if used for something else)
    else {
        uart_isr(&g_mss_uart1_lo);
    }

    return EXT_IRQ_KEEP_ENABLED;
}

void serial_isr(mss_uart_instance_t *this_uart) {
    felix_device_t *dev = (felix_device_t *)this_uart->user_data;
    if (!dev) return;

    const uint8_t Char_BS = 0x08;
    const uint8_t Char_Del = 0x7F;

    volatile uint8_t iir = this_uart->hw_reg->IIR;

    while ((iir & UART_IIR_NO_INT) == 0) {
        uint8_t id = iir & UART_IIR_ID_MASK;

        // --- RX Logic (The Cooked Part) ---
        if (id == UART_IIR_ID_RDA || id == UART_IIR_ID_CTI) {
            uint8_t rx_byte;
            while (this_uart->hw_reg->LSR & UART_LSR_DR) {
                rx_byte = this_uart->hw_reg->RBR;

                // 1. Handle Backspace (0x08 or 0x7F)
                if (dev->flags.cooked_mode && (rx_byte == Char_BS || rx_byte == Char_Del)) {
                    // Try to remove last char from RX Buffer
                    if (rb_unpush(&dev->rx_buf)) {
                        // If successful, echo "Backspace-Space-Backspace" to erase on terminal
                        if (dev->flags.echo_mode) {
                            rb_push(&dev->tx_buf, Char_BS);
                            rb_push(&dev->tx_buf, ' ');
                            rb_push(&dev->tx_buf, Char_BS);
                            this_uart->hw_reg->IER |= UART_IER_ETBEI; // Kick TX
                        }
                    }
                    continue; // Done, don't push the 0x7F
                }

                // 2. Handle Enter (\r)
                if (dev->flags.cooked_mode && rx_byte == '\r') {
                    rx_byte = '\n'; // Normalize to \n for the OS
                }

                // 3. Push to RX Buffer
                rb_push(&dev->rx_buf, rx_byte);

                // 4. Handle Echo
                if (dev->flags.echo_mode) {
                    // For Enter, we must echo \r\n (CRLF)
                    if (rx_byte == '\n') {
                        rb_push(&dev->tx_buf, '\r');
                        rb_push(&dev->tx_buf, '\n');
                    } else {
                        rb_push(&dev->tx_buf, rx_byte);
                    }
                    // Kick TX Interrupt so the echo goes out
                    this_uart->hw_reg->IER |= UART_IER_ETBEI;
                }
            }
        }

        // --- TX Logic (Standard) ---
        else if (id == UART_IIR_ID_THRE) {
            uint8_t tx_byte;
            if (rb_pop(&dev->tx_buf, &tx_byte)) {
                this_uart->hw_reg->THR = tx_byte;
            }
            if (rb_is_empty(&dev->tx_buf)) {
                 this_uart->hw_reg->IER &= ~UART_IER_ETBEI;
            }
        }

        iir = this_uart->hw_reg->IIR;
    }
}
