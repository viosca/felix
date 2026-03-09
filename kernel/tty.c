/**
 * @file tty.c
 * @brief POSIX Line Discipline Implementation.
 */

#include "kernel/tty.h"
#include <errno.h>
#include <string.h>

/* Note: Ensure these ioctl numbers match your sys/ioctl.h */
#ifndef TCGETS
#define TCGETS 0x5401
#define TCSETS 0x5402
#endif

void tty_init(tty_t *tty, void (*hw_putc)(void *, char), void *hw_ctx)
{
    memset(tty, 0, sizeof(struct tty));

    tty->hw_putc = hw_putc;
    tty->hw_ctx = hw_ctx;

    /* Initialize your existing ring buffer */
    ring_buffer_init(&tty->rx_buffer);

    /* Sane POSIX Defaults */
    tty->termios.c_iflag = ICRNL;
    tty->termios.c_oflag = OPOST | ONLCR;
    tty->termios.c_lflag = ECHO | ECHOE | ICANON;
    tty->termios.c_cflag = 0;
}

int tty_write(tty_t *tty, const char *buf, size_t count)
{
    size_t i;
    char c;

    if (!tty || !tty->hw_putc)
        return -1;

    for (i = 0; i < count; i++) {
        c = buf[i];

        /* Output Processing (OPOST) */
        if (tty->termios.c_oflag & OPOST) {
            if (c == '\n' && (tty->termios.c_oflag & ONLCR))
                tty->hw_putc(tty->hw_ctx, '\r');
            else if (c == '\r' && (tty->termios.c_oflag & OCRNL))
                c = '\n';
        }

        tty->hw_putc(tty->hw_ctx, c);
    }

    return count;
}

void tty_receive_char(tty_t *tty, char c)
{
    size_t i;

    if (!tty)
        return;

    /* 1. Input Translation (ICRNL) */
    if (c == '\r') {
        if (tty->termios.c_iflag & IGNCR)
            return;
        if (tty->termios.c_iflag & ICRNL)
            c = '\n';
    } else if (c == '\n') {
        if (tty->termios.c_iflag & INLCR)
            c = '\r';
    }

    /* 2. Canonical Mode (Line Editing) */
    if (tty->termios.c_lflag & ICANON) {
        /* Handle Backspace/Delete */
        if (c == '\b' || c == 0x7F) {
            if (tty->edit_len > 0) {
                tty->edit_len--;
                if ((tty->termios.c_lflag & ECHO) && (tty->termios.c_lflag & ECHOE)) {
                    /* Visually erase the character on screen */
                    tty->hw_putc(tty->hw_ctx, '\b');
                    tty->hw_putc(tty->hw_ctx, ' ');
                    tty->hw_putc(tty->hw_ctx, '\b');
                }
            }
            return;
        }

        /* Echo normal characters */
        if (tty->termios.c_lflag & ECHO) {
            if (c == '\n' && (tty->termios.c_oflag & OPOST) && (tty->termios.c_oflag & ONLCR)) {
                tty->hw_putc(tty->hw_ctx, '\r');
                tty->hw_putc(tty->hw_ctx, '\n');
            } else {
                tty->hw_putc(tty->hw_ctx, c);
            }
        }

        /* Buffer the character */
        if (tty->edit_len < sizeof(tty->edit_buf)) {
            tty->edit_buf[tty->edit_len++] = c;

            /* If it's a newline, flush the edit buffer to the read ring-buffer */
            if (c == '\n' || c == tty->termios.c_cc[VEOF]) {
                for (i = 0; i < tty->edit_len; i++) {
                    ring_buffer_write(&tty->rx_buffer, tty->edit_buf[i]);
                }
                tty->lines_available++;
                tty->edit_len = 0;
            }
        }
    } else {
        /* 3. Raw Mode */
        if (tty->termios.c_lflag & ECHO)
            tty->hw_putc(tty->hw_ctx, c);

        ring_buffer_write(&tty->rx_buffer, c);
        tty->lines_available++; /* In raw mode, every char counts */
    }
}

int tty_read(tty_t *tty, char *buf, size_t count)
{
    size_t bytes_read = 0;
    char c;

    if (!tty || count == 0)
        return 0;

    /* BAREMETAL BLOCKING READ: Spin until data is available.
     * This prevents read() from returning 0 (which triggers EOF in newlib).
     */
    while (1) {
        if (tty->termios.c_lflag & ICANON) {
            if (tty->lines_available > 0)
                break;
        } else {
            if (!ring_buffer_is_empty(&tty->rx_buffer))
                break;
        }
    }


    while (bytes_read < count && !ring_buffer_is_empty(&tty->rx_buffer) ) {
        c = 0;
        ring_buffer_read(&tty->rx_buffer, &c);

        buf[bytes_read++] = c;

        if ((tty->termios.c_lflag & ICANON) && (c == '\n' || c == tty->termios.c_cc[VEOF])) {
            tty->lines_available--;
            break; /* Stop reading at the newline */
        }
    }

    if (!(tty->termios.c_lflag & ICANON))
        tty->lines_available -= bytes_read;

    return bytes_read;
}

int tty_ioctl(tty_t *tty, int request, void *arg)
{
    struct termios *t;

    if (!tty || !arg)
        return -1; /* EFAULT */

    switch (request) {
    case TCGETS:
        t = (struct termios *)arg;
        *t = tty->termios;
        return 0;
    case TCSETS:
        t = (struct termios *)arg;
        tty->termios = *t;
        return 0;
    default:
        return -1; /* EINVAL */
    }
}
