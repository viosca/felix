/**
 * @file felix/kernel/tty.c
 * @brief POSIX Line Discipline Implementation.
 */

#include "kernel/tty.h"
#include "kernel/sched.h"
#include <sys/ioctl.h>
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

    ring_buffer_init(&tty->rx_buffer);

    /* Sane POSIX Defaults */
    tty->termios.c_iflag = ICRNL;
    tty->termios.c_oflag = OPOST | ONLCR;
    tty->termios.c_lflag = ECHO | ECHOE | ICANON;
    tty->termios.c_cflag = 0;

    /* Default Terminal Size (80 columns x 24 rows) */
    tty->winsize.ws_row = 24;
    tty->winsize.ws_col = 80;
    tty->winsize.ws_xpixel = 0;
    tty->winsize.ws_ypixel = 0;
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
    if (!tty)
        return;

    /* BOTTOM HALF (ISR CONTEXT):
     * No echoing! No blocking! Just safely push the raw byte. */
    ring_buffer_write(&tty->rx_buffer, (uint8_t)c);

#if 0
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
#endif
}

int tty_read(tty_t *tty, char *buf, size_t count)
{
    size_t bytes_read = 0;
    char c;

    if (!tty || count == 0)
        return 0;
#if 0
#if 0
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
#else
    /* NON-BLOCKING RETURN: If no data is available, return 0 immediately.
     * The higher-level VFS driver (mmuart_vfs_read) will handle the blocking 
     * loop and tick the background OS tasks! */
    if (tty->termios.c_lflag & ICANON) {
        if (tty->lines_available == 0) return 0;
    } else {
        if (ring_buffer_is_empty(&tty->rx_buffer)) return 0;
    }
#endif


    while (bytes_read < count) {
        c = 0;

        /* --- CRITICAL SECTION START ---
         * Pause hardware interrupts so the ISR cannot write to the buffer
         * while we are pulling data out of it */
        sched_critical_enter();

        if (ring_buffer_is_empty(&tty->rx_buffer)) {
            sched_critical_exit(); /* --- CRITICAL SECTION END --- */
            break;
        }
        ring_buffer_read(&tty->rx_buffer, &c);

        if ((tty->termios.c_lflag & ICANON) && (c == '\n' || c == tty->termios.c_cc[VEOF])) {
            tty->lines_available--;
            sched_critical_exit(); /* --- CRITICAL SECTION END --- */
            buf[bytes_read++] = c;
            break; /* Stop reading at the newline */
        }

        sched_critical_exit(); /* --- CRITICAL SECTION END --- */

        buf[bytes_read++] = c;
    }

    if (!(tty->termios.c_lflag & ICANON))
        tty->lines_available -= bytes_read;

    return bytes_read;
#endif
    while (bytes_read < count) {
        /* --- CRITICAL SECTION START --- */
        sched_critical_enter();
        if (ring_buffer_is_empty(&tty->rx_buffer)) {
            sched_critical_exit();
            break; /* No more raw data available right now */
        }
        ring_buffer_read(&tty->rx_buffer, (uint8_t *)&c);
        sched_critical_exit();
        /* --- CRITICAL SECTION END --- */
#if 0
        /* 1. Unconditional Enter Key Translation */
        if (c == '\r') {
            c = '\n'; /* Force \r to \n so linenoise always sees the Enter key */
        }
#endif

        /* 1. Input Translation (ICRNL / INLCR) */
        if (c == '\r') {
            if (tty->termios.c_iflag & IGNCR) continue;
            if (tty->termios.c_iflag & ICRNL) c = '\n';
        } else if (c == '\n') {
            if (tty->termios.c_iflag & INLCR) c = '\r';
        }

        /* 2. Canonical Mode (Line Editing) */
        if (tty->termios.c_lflag & ICANON) {
            /* Handle Backspace/Delete */
            if (c == '\b' || c == 0x7F) {
                if (tty->edit_len > 0) {
                    tty->edit_len--;
                    if ((tty->termios.c_lflag & ECHO) && (tty->termios.c_lflag & ECHOE)) {
                        /* Safe to write from Top Half! */
                        tty_write(tty, "\b \b", 3);
                    }
                }
                continue;
            }

            /* Buffer the character */
            if (tty->edit_len < sizeof(tty->edit_buf)) {
                tty->edit_buf[tty->edit_len++] = c;

                /* Echo normal characters */
                if (tty->termios.c_lflag & ECHO) {
                    tty_write(tty, &c, 1);
                }

                /* End of line? Flush to user buffer! */
                if (c == '\n' || c == tty->termios.c_cc[VEOF]) {
                    size_t to_copy = (tty->edit_len <= (count - bytes_read)) ? tty->edit_len : (count - bytes_read);
                    memcpy(&buf[bytes_read], tty->edit_buf, to_copy);
                    bytes_read += to_copy;

                    tty->edit_len = 0; /* Reset edit buffer for the next line */
                    break;
                }
            }
        } else {
            /* 3. Raw Mode */
            if (tty->termios.c_lflag & ECHO) {
                tty_write(tty, &c, 1);
            }
            buf[bytes_read++] = c;
        }
    }
    return bytes_read;
}

int tty_ioctl(tty_t *tty, int request, void *arg)
{
    struct termios *t;
    struct winsize *ws;
    int *bytes_avail;

    if (!tty) return -1; /* EFAULT */

    switch (request) {
    case TCGETS:
        if (!arg) return -1;
        t = (struct termios *)arg;
        *t = tty->termios;
        return 0;
    case TCSETS:
    case TCSETSW:
    case TCSETSF:
        if (!arg) return -1;
        t = (struct termios *)arg;
        tty->termios = *t;
        return 0;
    case TIOCGWINSZ:
        if (!arg) return -1;
        /* Failsafe: Prevent linenoise infinite loops */
        if (tty->winsize.ws_col == 0) tty->winsize.ws_col = 80;
        if (tty->winsize.ws_row == 0) tty->winsize.ws_row = 24;

        memcpy(arg, &tty->winsize, sizeof(struct winsize));
        return 0;
    case TIOCSWINSZ:
        if (!arg) return -1;
        memcpy(&tty->winsize, arg, sizeof(struct winsize));
        return 0;
    case TIOCNOTTY:
        return 0; /* Pretend we successfully gave up the controlling terminal */
    case FIONBIO:
        return 0; /* Non-blocking I/O toggle. Usually handled in VFS, but safe to ACK here */
    case FIONREAD:
        if (!arg) return -1;
        *(int *)arg = ring_buffer_is_empty(&tty->rx_buffer) ? 0 : 1;
        return 0;
    default:
        return -1; /* EINVAL */
    }
}
