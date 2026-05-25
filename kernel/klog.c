/**
 * @file kernel/klog.c
 * @brief Direct-to-hardware kernel logging (bypasses POSIX/TTY).
 */
#include "kernel/klog.h"
#include <stdio.h>
#include <stdbool.h>

#if CONFIG_ENABLE_KLOG

/* The function pointer that links generic logging to the hardware HAL */
static void (*g_flx_earlycon_putc)(char c) = NULL;

/* Direct pointer to the console VFS node */
static FlxVfsNode_t *g_console_vfs_node = NULL;

void flx_klog_set_earlycon(void (*putc_fn)(char c)) {
    g_flx_earlycon_putc = putc_fn;
}

void flx_klog_switch_to_vfs(FlxVfsNode_t *console_node) {
    g_console_vfs_node = console_node;
}

void kprintf(const char *fmt, ...) {
    char buf[256]; /* Kernel stack buffer */
    va_list args;

    /* vsnprintf is SAFE: It only formats memory, it doesn't use FDs */
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    /* If VFS is alive, bypass Newlib and write directly to the driver */
    if (g_console_vfs_node && g_console_vfs_node->ops && g_console_vfs_node->ops->write) {

        /* Create a temporary File container so the driver can extract the node's dev_id */
        FlxFile_t dummy_file;
        dummy_file.node = g_console_vfs_node;

        /* Chunk the buffer to automatically expand '\n' to '\r\n' */
        char *chunk_start = buf;
        char *p = buf;

        while (*p != '\0') {
            if (*p == '\n') {
                /* Write everything up to the newline */
                if (p > chunk_start) {
                    g_console_vfs_node->ops->write(&dummy_file, chunk_start, p - chunk_start);
                }
                /* Inject the \r\n */
                g_console_vfs_node->ops->write(&dummy_file, "\r\n", 2);
                chunk_start = p + 1;
            }
            p++;
        }
        /* Write any remaining characters */
        if (p > chunk_start) {
            g_console_vfs_node->ops->write(&dummy_file, chunk_start, p - chunk_start);
        }

    }
    /* Otherwise, fall back to the early boot hardware polling */
    else if (g_flx_earlycon_putc) {
        char c;
        for (int i = 0; (c = buf[i]) != '\0'; i++) {
            if (c == '\n') {
                g_flx_earlycon_putc('\r');
            }
            g_flx_earlycon_putc(buf[i]);
        }
    }
}
#endif /* CONFIG_ENABLE_KLOG */
