/** @ile felix/posix/time.c
 */
#include <time.h>
#include <sys/time.h>
#include <errno.h>
#include <stdint.h>
#include "kernel/syscalls.h"

int gettimeofday(struct timeval *restrict tv, void *restrict tz) {
    int ret = sys_gettimeofday(tv, tz);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz) {
    int ret = sys_settimeofday(tv, (const void *)tz);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

/* =========================================================================
 * POSIX High-Resolution Timers
 * ========================================================================= */

int clock_gettime(clockid_t clock_id, struct timespec *tp) {
    if (!tp) {
        errno = EFAULT;
        return -1;
    }

    /* Felix OS currently uses the same time base for all clocks.
     * In the future, CLOCK_MONOTONIC can map to raw hardware uptime,
     * while CLOCK_REALTIME maps to the RTC offset. */
    (void)clock_id;

    struct timeval tv;
    int ret = sys_gettimeofday(&tv, NULL);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    /* Convert timeval (microseconds) to timespec (nanoseconds) */
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;

    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem) {
    if (!req) {
        errno = EFAULT;
        return -1;
    }

    if (req->tv_sec < 0 || req->tv_nsec < 0 || req->tv_nsec >= 1000000000L) {
        errno = EINVAL;
        return -1;
    }

    /* Convert the timespec duration into standard microseconds */
    uint64_t total_us = ((uint64_t)req->tv_sec * 1000000ULL) + (req->tv_nsec / 1000);

    /* If the request is greater than 0 but less than a full microsecond,
       delay for at least 1 us to ensure a yield/context switch occurs. */
    if (total_us == 0 && req->tv_nsec > 0) {
        total_us = 1;
    }

    if (total_us > 0) {
        /* --- FULL STRUCTURE IMPLEMENTATION (Optional) ---
         * To fully implement the 'rem' structure, uncomment the time-tracking
         * logic below and replace 'sys_get_time_us()' with your actual kernel
         * uptime syscall.
         */

        // uint64_t start_us = sys_get_time_us();

        sys_usleep(total_us);

        // if (rem) {
        //     uint64_t elapsed_us = sys_get_time_us() - start_us;
        //     if (elapsed_us < total_us) {
        //         uint64_t remaining_us = total_us - elapsed_us;
        //         rem->tv_sec = remaining_us / 1000000ULL;
        //         rem->tv_nsec = (remaining_us % 1000000ULL) * 1000;
        //     } else {
        //         rem->tv_sec = 0;
        //         rem->tv_nsec = 0;
        //     }
        // }
    }

    /* In a full POSIX implementation with signals, if sleep is interrupted
       we would calculate the remaining time. Since Felix OS does not currently
       interrupt threads with POSIX signals in the middle of sleep,
       we assume the full sleep duration was achieved. */
    if (rem) {
        rem->tv_sec = 0;
        rem->tv_nsec = 0;
    }

    return 0;
}
