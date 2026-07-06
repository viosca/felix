/**
 * @file kernel/time.h
 * @brief Kernel time management subsystem.
 */

#ifndef _FLX_KERNEL_TIME_H
#define _FLX_KERNEL_TIME_H

#include <sys/time.h>

/**
 * @brief Gets the current system wall-clock time.
 * @param tv Pointer to a timeval struct to populate.
 * @return 0 on success, or a negative error code on failure.
 */
int flx_time_get_timeofday(struct timeval *tv);

/**
 * @brief Sets the system wall-clock time offset.
 * @param tv Pointer to a timeval struct containing the requested epoch time.
 * @return 0 on success, or a negative error code on failure.
 */
int flx_time_set_timeofday(const struct timeval *tv);

/**
 * @brief Suspends execution for the specified number of microseconds.
 * @param usec The number of microseconds to sleep.
 * @return 0 on success.
 */
int flx_time_usleep(uint64_t usec);


#endif /* _FLX_KERNEL_TIME_H */
