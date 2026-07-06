/**
 * @file kernel/klog.h
 * @brief Direct-to-hardware kernel logging (bypasses POSIX/TTY).
 */
#ifndef _FLX_KERNEL_LOG_H
#define _FLX_KERNEL_LOG_H

#include <stdarg.h>
#include "kernel/vfs.h" /* For FlxVfsNode_t */

/* Toggle this to 1 to enable kprintf, or 0 to completely compile it out */
#define CONFIG_ENABLE_KLOG 1

void flx_klog_set_earlycon(void (*putc_fn)(char c));

/**
 * @brief Signals the kernel logger that the VFS console is ready.
 * All subsequent kprintf calls will be routed through the POSIX standard I/O.
 */
void flx_klog_switch_to_vfs(FlxVfsNode_t *console_node);

/**
 * @brief Prints formatted text directly to the hardware console.
 * Safe to call from interrupts, drivers, and before the VFS is initialized.
 */
#if CONFIG_ENABLE_KLOG
void kprintf(const char *fmt, ...);
#else
#define kprintf(...) do {} while(0)
#endif

#endif /* _FLX_KERNEL_LOG_H */
