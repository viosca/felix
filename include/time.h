/**
 * @file felix/include/time.h
 * @brief Felix OS extensions to Newlib's time.h
 */
#ifndef _FELIX_TIME_H
#define _FELIX_TIME_H

/* Pull in the REAL Newlib time.h */
#include_next <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Felix OS POSIX Extensions
 * Newlib hides these prototypes on bare-metal targets unless specific
 * POSIX feature macros are set. We expose them here for native apps.
 * ========================================================================= */
struct tm;
struct timespec;

/* Standard POSIX UTIME constants for utimensat() and related calls */
#ifndef UTIME_NOW
#define UTIME_NOW  ((1l << 30) - 1l)
#endif

#ifndef UTIME_OMIT
#define UTIME_OMIT ((1l << 30) - 2l)
#endif

/* Felix-specific time structures */
typedef struct {
    int64_t tv_sec;
    long    tv_nsec;
} FlxTimespec_t;

char *strptime(const char *buf, const char *format, struct tm *tm);
int clock_gettime(clockid_t clock_id, struct timespec *tp);

/**
 * @brief High-resolution sleep.
 * Suspends the execution of the calling thread until either at least the time
 * specified in *req has elapsed, or the delivery of a signal that triggers the
 * invocation of a handler in the calling thread or that terminates the process.
 * * @param req The requested sleep time.
 * @param rem The remaining sleep time (if interrupted).
 * @return 0 on success, -1 on failure (with errno set).
 */
int nanosleep(const struct timespec *req, struct timespec *rem);

/* Standard POSIX Clock IDs */
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FELIX_TIME_H */
