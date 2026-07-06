/**
 * @file felix/posix/syslog.c
 * @brief System logging shims for Felix OS.
 */

#include <syslog.h>
#include <stddef.h>
#include <stdarg.h>

/**
 * @brief Open connection to system logger.
 * Felix OS has no log daemon, so this is a no-op.
 */
void openlog(const char *ident, int option, int facility) {
    (void)ident;
    (void)option;
    (void)facility;
}

/**
 * @brief Generate a log message.
 */
void syslog(int priority, const char *format, ...) {
    (void)priority;
    (void)format;
    /* If you want to see syslog output, you could #include "kernel/klog.h"
     * here and route this to kprintf! For now, we safely drop it. */
}

/**
 * @brief Generate a log message using a variadic list.
 * Required by Toybox's lib/lib.c `loggit` function.
 */
void vsyslog(int priority, const char *format, va_list ap) {
    (void)priority;
    (void)format;
    (void)ap;
}

/**
 * @brief Close descriptor used to write to system logger.
 */
void closelog(void) {
    /* No-op */
}

/**
 * @brief Set the log priority mask.
 */
int setlogmask(int mask) {
    (void)mask;
    return 0;
}
