/**
 * @file time.c
 * @brief Kernel time management subsystem implementation.
 */

#include "kernel/time.h"
#include "arch/riscv64/include/timer.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

/* A global offset to fake a battery-backed Real Time Clock (RTC).
 * Wall Clock Time = Hardware Uptime + Offset
 */
static uint64_t g_wall_clock_offset_ms = 0;

int flx_time_get_timeofday(struct timeval *tv) {
    if (!tv) return -EINVAL;

    /* 1. Get the monotonic hardware uptime */
    uint64_t uptime_ms = arch_timer_get_ms();

    /* 2. Apply our fake RTC offset to get actual Wall Clock Time */
    uint64_t current_ms = uptime_ms + g_wall_clock_offset_ms;

    /* 3. Break it down into the POSIX struct format */
    tv->tv_sec = current_ms / 1000;
    tv->tv_usec = (current_ms % 1000) * 1000;

    return 0;
}

int flx_time_set_timeofday(const struct timeval *tv) {
    if (!tv) return -EINVAL;

    /* 1. Convert the user's requested human time into raw milliseconds */
    uint64_t requested_ms = ((uint64_t)tv->tv_sec * 1000) + (tv->tv_usec / 1000);

    /* 2. Determine where the hardware timer is right now */
    uint64_t uptime_ms = arch_timer_get_ms();

    /* 3. Store the difference. Now, gettimeofday() will reflect this change! */
    g_wall_clock_offset_ms = requested_ms - uptime_ms;

    return 0;
}

int flx_time_usleep(uint64_t usec) {
    uint64_t start = arch_timer_get_us();

    /* Spin-wait. We deliberately do NOT use arch_cpu_idle() here because
     * we haven't programmed a hardware alarm to wake the core back up! */
    while ((arch_timer_get_us() - start) < usec) {
        /* TODO: When cooperative multitasking is implemented, call flx_sched_yield() here! */
    }

    return 0;
}
