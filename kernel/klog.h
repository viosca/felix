/**
 * @file kernel/klog.h
 * @brief Direct-to-hardware kernel logging (bypasses POSIX/TTY).
 */
#ifndef FELIX_KERNEL_KLOG_H
#define FELIX_KERNEL_KLOG_H

#include <stdarg.h>

/* Toggle this to 1 to enable kprintf, or 0 to completely compile it out */
#define CONFIG_ENABLE_KLOG 0

/**
 * @brief Prints formatted text directly to the hardware console.
 * Safe to call from interrupts, drivers, and before the VFS is initialized.
 */

#if CONFIG_ENABLE_KLOG

    /* Your existing declaration */
    void kprintf(const char *fmt, ...);

#else

    /* Safely swallows the function and all arguments, optimizing to nothing */
    #define kprintf(...) do {} while(0)

#endif

#endif /* FELIX_KERNEL_KLOG_H */
