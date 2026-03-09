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
//#include "kernel/ring_buffer.h"
#include "kernel/tty.h"
#include "mpfs_hal/mss_hal.h"
#include "drivers/uart_userdata.h"
#include "drivers/mss/mss_mmuart/mss_uart.h"

//#include "mss_uart.h"

/* Static allocation for our userdata wrappers so they survive scope */
static UARTUserData_t g_uart_userdata[FLX_DEV_MAX_DEVICES];

/* State machine arrays to map our ring buffer to the vendor's block API */
static volatile bool g_tx_active[FLX_DEV_MAX_DEVICES];

/**
 * @brief Maps a UART instance pointer to its PLIC Interrupt ID.
 */
static PLIC_IRQn_Type get_uart_irq_source(mss_uart_instance_t *uart) {
    if (uart == &g_mss_uart0_lo || uart == &g_mss_uart0_hi) return PLIC_MMUART0_INT_OFFSET;
    if (uart == &g_mss_uart1_lo || uart == &g_mss_uart1_hi) return PLIC_MMUART1_INT_OFFSET;
    if (uart == &g_mss_uart2_lo || uart == &g_mss_uart2_hi) return PLIC_MMUART2_INT_OFFSET;
    if (uart == &g_mss_uart3_lo || uart == &g_mss_uart3_hi) return PLIC_MMUART3_INT_OFFSET;
    if (uart == &g_mss_uart4_lo || uart == &g_mss_uart4_hi) return PLIC_MMUART4_INT_OFFSET;

    return MMUART0_PLIC_77;
}

/**
 * @brief Maps a UART instance pointer to its MSS Peripheral ID.
 */
static mss_peripherals get_uart_peripheral_id(mss_uart_instance_t *uart) {
    if (uart == &g_mss_uart0_lo || uart == &g_mss_uart0_hi) return MSS_PERIPH_MMUART0;
    if (uart == &g_mss_uart1_lo || uart == &g_mss_uart1_hi) return MSS_PERIPH_MMUART1;
    if (uart == &g_mss_uart2_lo || uart == &g_mss_uart2_hi) return MSS_PERIPH_MMUART2;
    if (uart == &g_mss_uart3_lo || uart == &g_mss_uart3_hi) return MSS_PERIPH_MMUART3;
    if (uart == &g_mss_uart4_lo || uart == &g_mss_uart4_hi) return MSS_PERIPH_MMUART4;
    return MSS_PERIPH_MMUART0;
}

/* --- Interrupt Handlers (Registered with Vendor HAL) --- */
#define MSS_UART_DATA_READY 0x01
static void flx_serial_rx_handler(mss_uart_instance_t *uart)
{
    FlxDev_t *dev = uart_get_dev(uart);
    uint8_t rx_byte;

    if (!dev) return;
#if 1
    /* BYPASS VENDOR LOGIC: Drain the hardware FIFO directly!
     * Line Status Register (LSR) bit 0 (0x01) is the Data Ready flag.
     * Receive Buffer Register (RBR) contains the actual byte.
     */
    while (uart->hw_reg->LSR & MSS_UART_DATA_READY) {
        /* Reading RBR physically removes the byte from the hardware FIFO
         * and automatically clears the hardware interrupt flag! */
        char rx_byte = (char)uart->hw_reg->RBR;

        /* Send it to our POSIX Line Discipline */
        tty_receive_char(&dev->tty, rx_byte);
    }
#else
    /* Extract bytes from the hardware FIFO using the vendor API */
    while (MSS_UART_get_rx(uart, &rx_byte, 1) > 0) {
        tty_receive_char(&dev->tty, (char)rx_byte);
    }
#endif
}

static void flx_serial_tx_handler(mss_uart_instance_t *uart)
{
    FlxDev_t *dev = uart_get_dev(uart);
    uint8_t count = 0;
    uint8_t chunk[16];

    if (!dev) return;

    /* Pop up to 16 bytes (16550 FIFO size) from our ring buffer */
    while (count < sizeof(chunk) && !ring_buffer_is_empty(&dev->tx_buffer)) {
        ring_buffer_read(&dev->tx_buffer, &chunk[count++]);
    }

    if (count > 0) {
        /* Manually push data into the hardware FIFO */
        MSS_UART_fill_tx_fifo(uart, chunk, count);

        /* TRICK THE VENDOR ISR:
         * The vendor's mss_uart_isr executes the following after calling us:
         * if (uart->tx_idx == uart->tx_buff_size) { IER &= ~TBE_IRQ; }
         * We MUST ensure this evaluates to FALSE, otherwise the vendor ISR
         * silently murders our interrupt chain!
         */
        uart->tx_buff_size = 1;
        uart->tx_idx = 0;
    } else {
        /* Ring buffer is totally empty. Let the vendor ISR naturally
         * disable the hardware interrupt for us. */
        uart->tx_buff_size = 0;
        uart->tx_idx = 0;
        g_tx_active[dev->id] = false;
    }
}

/* --- Hardware TTY Callbacks --- */

static void uart_hw_putc(void *ctx, char c)
{
    FlxDev_t *dev = (FlxDev_t *)ctx;

    if (!dev || !dev->uart) return;

    /* 0. APPLY BACKPRESSURE (FLOW CONTROL)
     * If the software ring buffer fills up, we MUST wait for the hardware
     * TX interrupt to drain some data before we push more.
     * By using 'wfi' (Wait For Interrupt), we put the RISC-V core into a
     * low-power sleep state instead of busy-looping. It will instantly wake
     * up the moment the UART ISR fires.
     */
    while (ring_buffer_is_full(&dev->tx_buffer)) {
        __asm__ volatile ("wfi");
    }

    /* 1. CRITICAL SECTION START
     * Disable this specific UART's interrupt in the PLIC.
     * This prevents the TX ISR from firing and modifying the g_tx_active flag
     * or the IER register while we are trying to update them! */
    PLIC_IRQn_Type irqn = get_uart_irq_source(dev->uart);
    PLIC_DisableIRQ(irqn);

    /* Push the byte into the software TX ring buffer */
    ring_buffer_write(&dev->tx_buffer, (uint8_t)c);

    /* If the hardware isn't currently transmitting, kickstart it! */
    if (!g_tx_active[dev->id]) {
        g_tx_active[dev->id] = true;
        /* Unmasking the TBE interrupt while the FIFO is empty will naturally
         * assert the interrupt line, safely triggering the first ISR.
         */
        dev->uart->hw_reg->IER |= MSS_UART_TBE_IRQ;
    }
    /* 4. CRITICAL SECTION END
     * Re-enable the UART interrupt so the hardware can instantly pull
     * the byte we just deposited. */
    PLIC_EnableIRQ(irqn);


}

/* --- Initialization --- */

int serial_init(int dev_id, mss_uart_instance_t *uart)
{
    FlxDev_t *dev;

    if (dev_id < 0 || dev_id >= FLX_DEV_MAX_DEVICES || !uart) return -1;

    dev = flx_dev_register(dev_id, FlxDevType_UART);
    if (!dev)
        return -ENOENT;

    dev->uart = uart;
    g_tx_active[dev_id] = false;

    /* Configure the Type-Safe Wrapper from uart_userdata.h (CRITICAL FOR ISR) */
    g_uart_userdata[dev_id].type = UARTUserDataType_DEVICE;
    g_uart_userdata[dev_id].device = dev;
    uart_set_userdata(uart, &g_uart_userdata[dev_id]);

    ring_buffer_init(&dev->tx_buffer);

    /* Initialize TTY, explicitly mapping the correct signature */
    tty_init(&dev->tty, uart_hw_putc, dev);

    /* -------------------------------------------------------------
     * PORTABILITY BOUNDARY (For future ALIF / other SoC Support)
     * Everything below this line is specific to the PolarFire SoC.
     * In a fully portable system, these clock/PLIC functions should
     * eventually be moved to a board_init() routine in main.c
     * ------------------------------------------------------------- */
    PLIC_IRQn_Type uart_irq_source = get_uart_irq_source(uart);
    mss_peripherals uart_peripheral_id = get_uart_peripheral_id(uart);
    uint32_t hart_id = read_csr(mhartid);

    /* 1. Hardware Initialization */
    (void)mss_config_clk_rst(uart_peripheral_id, (uint8_t) hart_id, PERIPHERAL_ON);
    MSS_UART_init(uart,
                  MSS_UART_115200_BAUD,
                  MSS_UART_DATA_8_BITS | MSS_UART_NO_PARITY | MSS_UART_ONE_STOP_BIT);

    /* 2. Register custom OS handlers with the Vendor HAL. */
    MSS_UART_set_rx_handler(uart, flx_serial_rx_handler, MSS_UART_FIFO_SINGLE_BYTE);
    MSS_UART_set_tx_handler(uart, flx_serial_tx_handler);

    PLIC_SetPriority(uart_irq_source, 1);
    PLIC_EnableIRQ(uart_irq_source);

    set_csr(mie, MIP_MSIP | MIP_MEIP);

    return 0;
}

/* --- VFS / POSIX Pass-through --- */

int serial_read(FlxDev_t *dev, char *buf, size_t count) {
    return tty_read(&dev->tty, buf, count);
}

int serial_write(FlxDev_t *dev, const char *buf, size_t count) {
    return tty_write(&dev->tty, buf, count);
}

int serial_ioctl(FlxDev_t *dev, int request, void *arg) {
    return tty_ioctl(&dev->tty, request, arg);
}
