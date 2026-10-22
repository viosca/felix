/**
 * @file felix/osal/freertos/FreeRTOS_POSIX_portable.h
 * @brief Port-specific configuration of FreeRTOS+POSIX for Felix OS (RISC-V 64 / Newlib).
 */

#ifndef _FREERTOS_POSIX_PORTABLE_H_
#define _FREERTOS_POSIX_PORTABLE_H_

#include "autoconf.h"

/* Pull in standard Newlib/Felix headers first so base POSIX types, errno,
 * file control flags, and signal structures are defined canonically. */
#include <sys/types.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#ifndef ENOTSUP
#define ENOTSUP EOPNOTSUPP
#endif

/* Suppress FreeRTOS+POSIX private redefinitions of errno.h, fcntl.h, and signal.h
 * so FreeRTOS+POSIX C sources use Newlib's errno, O_* flags, and struct sigevent. */
#define _FREERTOS_POSIX_ERRNO_H_
#define _FREERTOS_POSIX_FCNTL_H_
#define _FREERTOS_POSIX_SIGNAL_H_

/* Enable errno assignment inside FreeRTOS+POSIX C sources (which now routes
 * directly to Newlib's per-context errno). */
#ifndef configUSE_POSIX_ERRNO
#define configUSE_POSIX_ERRNO 1
#endif

/* Disable FreeRTOS+POSIX re-declarations of types already provided by Newlib
 * and felix/include/sys/types.h. */
#define posixconfigENABLE_CLOCK_T                0
#define posixconfigENABLE_CLOCKID_T              0
#define posixconfigENABLE_MODE_T                 0
#define posixconfigENABLE_PID_T                  0
#define posixconfigENABLE_SSIZE_T                0
#define posixconfigENABLE_TIME_T                 0
#define posixconfigENABLE_TIMER_T                0
#define posixconfigENABLE_USECONDS_T             0
#define posixconfigENABLE_OFF_T                  0
#define posixconfigENABLE_TIMESPEC               0
#define posixconfigENABLE_ITIMERSPEC             0
#define posixconfigENABLE_PTHREAD_T              0
#define posixconfigENABLE_PTHREAD_ATTR_T         0

/* Enable FreeRTOS+POSIX full internal structure types for synchronization primitives. */
#define posixconfigENABLE_PTHREAD_MUTEX_T        1
#define posixconfigENABLE_PTHREAD_MUTEXATTR_T    1
#define posixconfigENABLE_PTHREAD_COND_T         1
#define posixconfigENABLE_PTHREAD_CONDATTR_T     1
#define posixconfigENABLE_PTHREAD_BARRIER_T      1
#define posixconfigENABLE_SEM_T                  1
#define posixconfigENABLE_SCHED_PARAM            1

#endif /* _FREERTOS_POSIX_PORTABLE_H_ */
