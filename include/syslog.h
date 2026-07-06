/**
 * @file felix/include/syslog.h
 * @brief POSIX standard system logging facility.
 */
#ifndef _FLX_INCLUDE_SYSLOG_H
#define _FLX_INCLUDE_SYSLOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Syslog Priorities (Levels)
 * ========================================================================= */
#define LOG_EMERG       0       /* System is unusable */
#define LOG_ALERT       1       /* Action must be taken immediately */
#define LOG_CRIT        2       /* Critical conditions */
#define LOG_ERR         3       /* Error conditions */
#define LOG_WARNING     4       /* Warning conditions */
#define LOG_NOTICE      5       /* Normal but significant condition */
#define LOG_INFO        6       /* Informational */
#define LOG_DEBUG       7       /* Debug-level messages */

/* =========================================================================
 * Syslog Facilities
 * ========================================================================= */
#define LOG_KERN        (0<<3)  /* Kernel messages */
#define LOG_USER        (1<<3)  /* Random user-level messages */
#define LOG_MAIL        (2<<3)  /* Mail system */
#define LOG_DAEMON      (3<<3)  /* System daemons */
#define LOG_AUTH        (4<<3)  /* Security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* Messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* Line printer subsystem */
#define LOG_NEWS        (7<<3)  /* Network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
#define LOG_CRON        (9<<3)  /* Clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* Security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* FTP daemon */
#define LOG_LOCAL0      (16<<3) /* Reserved for local use */
#define LOG_LOCAL1      (17<<3) /* Reserved for local use */
#define LOG_LOCAL2      (18<<3) /* Reserved for local use */
#define LOG_LOCAL3      (19<<3) /* Reserved for local use */
#define LOG_LOCAL4      (20<<3) /* Reserved for local use */
#define LOG_LOCAL5      (21<<3) /* Reserved for local use */
#define LOG_LOCAL6      (22<<3) /* Reserved for local use */
#define LOG_LOCAL7      (23<<3) /* Reserved for local use */

/* =========================================================================
 * Syslog Options
 * ========================================================================= */
#define LOG_PID         0x01    /* Log the pid with each message */
#define LOG_CONS        0x02    /* Log on the console if errors in sending */
#define LOG_ODELAY      0x04    /* Delay open until first syslog() (default) */
#define LOG_NDELAY      0x08    /* Don't delay open */
#define LOG_NOWAIT      0x10    /* Don't wait for console forks: DEPRECATED */
#define LOG_PERROR      0x20    /* Log to stderr as well */

/* =========================================================================
 * POSIX Syslog Function Prototypes
 * ========================================================================= */
void openlog(const char *ident, int option, int facility);
void syslog(int priority, const char *format, ...);
void vsyslog(int priority, const char *format, va_list ap);
void closelog(void);
int setlogmask(int maskpri);

/* Macro to generate a mask for setlogmask */
#define LOG_MASK(pri)   (1 << (pri))
#define LOG_UPTO(pri)   ((1 << ((pri)+1)) - 1)

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYSLOG_H */
