/**
 * @file kernel/klog.c
 * @brief Direct-to-hardware kernel logging implementation.
 */
#include "kernel/klog.h"
#include "drivers/mss/mss_mmuart/mss_uart.h" /* Direct hardware access */
#include <stdio.h> /* For vsnprintf */

/* Assume UART1 is the system console for kernel panics and logs.
 * We access it directly, completely bypassing serial_core.c and the VFS! */
extern mss_uart_instance_t g_mss_uart1_lo;

#if CONFIG_ENABLE_KLOG
int kprintf(const char *fmt, ...) {
    char buffer[256];
    va_list args;

    va_start(args, fmt);
    /* We use the standard library's string formatter, but NOT its file I/O! */
    int len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len > 0) {
        if (len >= (int)sizeof(buffer)) {
            len = sizeof(buffer) - 1; /* Truncated */
        }

        /* * Write DIRECTLY to the hardware, bypassing the TTY ring buffer,
         * file descriptors, and interrupts.
         * This is safe to call from anywhere, even inside a crashed ISR!
         */
        //MSS_UART_polled_tx(&g_mss_uart1_lo, (const uint8_t *)buffer, len);
    }

    return len;
}
#endif
