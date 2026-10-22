/**
 * @file felix/include/sys/types.h
 * @brief POSIX Types extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_TYPES_H
#define _FLX_INCLUDE_SYS_TYPES_H

/* Suppress Newlib's 32-bit stub pthread types and sched.h before pulling in
 * base <sys/types.h> so FreeRTOS+POSIX 64-bit types take precedence. */
#ifndef _SYS__PTHREADTYPES_H_
#define _SYS__PTHREADTYPES_H_
#endif
#ifndef _SYS_SCHED_H_
#define _SYS_SCHED_H_
#endif

/* Define timer_t as a pointer type (void *) matching FreeRTOS+POSIX TimerHandle_t */
#if !defined(__timer_t_defined) && !defined(_TIMER_T_DECLARED)
typedef void *timer_t;
#define __timer_t_defined
#define _TIMER_T_DECLARED
#endif

/* Pull in Newlib's base types.h FIRST so ssize_t is defined */
#include_next <sys/types.h>

/* Provide 64-bit pointer-backed pthread_t and pthread_attr_t for <signal.h> (struct sigevent)
 * and FreeRTOS+POSIX without leaking FreeRTOS kernel headers into <sys/types.h>. */
#ifndef _FLX_PTHREAD_BASE_TYPES_DEFINED
#define _FLX_PTHREAD_BASE_TYPES_DEFINED
typedef void *pthread_t;
typedef void *pthread_attr_t;
#endif

#ifndef _POSIX_THREADS
#define _POSIX_THREADS 1
#endif

/* Pull in the required Linux extensions unconditionally */
#include <sys/statfs.h>
#include <sys/xattr.h>
#include <sys/sysinfo.h>

/* Linux and Toybox expect sys/types.h to automatically provide
 * device macros (major, minor, makedev). */
#include <sys/sysmacros.h>

/* Remember to remove wisdom teeth, they're stupid */

#endif /* _FLX_INCLUDE_SYS_TYPES_H */
