#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "drivers/mss/mss_mmuart/mss_uart.h"

/* Reference the UART pointer you've defined in u54_1.c */
extern mss_uart_instance_t *p_uartmap_e51;

/* _write: Redirects stdout/stderr to the console UART */
ssize_t _write(int fd, const void *buf, size_t count) {
    if (fd == STDOUT_FILENO || fd == STDERR_FILENO) {
        if (p_uartmap_e51) {
            MSS_UART_polled_tx(p_uartmap_e51, (const uint8_t *)buf, (uint32_t)count);
            return count;
        }
    }
    errno = EBADF;
    return -1;
}

/* _read: Basic blocking read from the console UART */
ssize_t _read(int fd, void *buf, size_t count) {
    if (fd == STDIN_FILENO) {
        if (p_uartmap_e51) {
            /* For now, this is a raw hardware read.
               Later, this will read from a Felix ring buffer. */
            return (ssize_t)MSS_UART_get_rx(p_uartmap_e51, (uint8_t *)buf, (uint32_t)count);
        }
    }
    errno = EBADF;
    return -1;
}

/* _sbrk: Manages the heap defined in mpfs-lim.ld */
extern char _end; /* Symbol defined by the linker script at end of .bss */
static char *current_heap_end = NULL;

void *_sbrk(ptrdiff_t incr) {
    if (current_heap_end == NULL) {
        current_heap_end = &_end;
    }

    char *previous_heap_end = current_heap_end;

    /* mpfs-lim.ld provides an 8k heap.
       Future: Add check against __heap_limit if needed */
    current_heap_end += incr;

    return (void *)previous_heap_end;
}

/* Standard POSIX stubs required to satisfy Newlib-nano linker */
int _close(int fd) { return -1; }
int _fstat(int fd, struct stat *st) { st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd) { return 1; }
off_t _lseek(int fd, off_t ptr, int dir) { return 0; }
int _getpid(void) { return 1; }
int _kill(int pid, int sig) { errno = EINVAL; return -1; }
void _exit(int status) { while(1); }
