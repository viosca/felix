/**
 * @file felix/include/sys/klog.h
 * @brief Kernel logging (dmesg) shims for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_KLOG_H
#define _FLX_INCLUDE_SYS_KLOG_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * klogctl() / syslog() Action Types
 * ========================================================================= */
#define SYSLOG_ACTION_CLOSE          0
#define SYSLOG_ACTION_OPEN           1
#define SYSLOG_ACTION_READ           2
#define SYSLOG_ACTION_READ_ALL       3
#define SYSLOG_ACTION_READ_CLEAR     4
#define SYSLOG_ACTION_CLEAR          5
#define SYSLOG_ACTION_CONSOLE_OFF    6
#define SYSLOG_ACTION_CONSOLE_ON     7
#define SYSLOG_ACTION_CONSOLE_LEVEL  8
#define SYSLOG_ACTION_SIZE_UNREAD    9
#define SYSLOG_ACTION_SIZE_BUFFER   10

/**
 * @brief Control the kernel printk buffer.
 * Expected by Toybox's 'dmesg' utility.
 */
int klogctl(int type, char *bufp, int len);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_KLOG_H */
