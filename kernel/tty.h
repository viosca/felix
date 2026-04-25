/**
 * @file felix/kernel/tty.h
 * @brief POSIX Line Discipline (TTY Shim) for Felix OS.
 */

#ifndef _FLX_TTY_H
#define _FLX_TTY_H

#include <stddef.h>
#include "termios.h"
#include "kernel/ring_buffer.h" /* Your existing 128-byte ring buffer */

typedef struct tty {
    /* POSIX Terminal State */
    struct termios termios;

    /*
     * The underlying ring buffer for ready-to-read data.
     * In raw mode, bytes go straight here.
     * In cooked mode, full lines are pushed here when Enter is pressed.
     */
    ring_buffer_t rx_buffer; /* Adjust to 'struct ring_buffer' if applicable */

    /* Line editing buffer for Canonical (Cooked) mode */
    char edit_buf[128];
    size_t edit_len;

    /* Number of completed lines available in rx_buffer (for ICANON blocking) */
    size_t lines_available;

    /* Hardware Abstraction Callbacks */
    void (*hw_putc)(void *ctx, char c);
    void *hw_ctx; /* Passed to hw_putc (e.g., pointer to MSS_uart instance) */
} tty_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the TTY instance with POSIX defaults.
 */
void tty_init(tty_t *tty, void (*hw_putc)(void *, char), void *hw_ctx);

/**
 * @brief Write raw data from the hardware interrupt into the TTY.
 * Applies ICRNL, ECHO, and handles line-editing backspaces.
 */
void tty_receive_char(tty_t *tty, char c);

/**
 * @brief Read cooked data from the TTY to user-space (Syscall implementation).
 */
int tty_read(tty_t *tty, char *buf, size_t count);

/**
 * @brief Write data from user-space to the hardware (Syscall implementation).
 * Applies ONLCR stairstep fixes.
 */
int tty_write(tty_t *tty, const char *buf, size_t count);

/**
 * @brief Handle TCGETS / TCSETS ioctls.
 */
int tty_ioctl(tty_t *tty, int request, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_TTY_H */
