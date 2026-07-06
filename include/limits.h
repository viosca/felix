/**
 * @file felix/include/limits.h
 * @brief Core Felix OS system limits.
 */
#ifndef _FLX_INCLUDE_LIMITS_H
#define _FLX_INCLUDE_LIMITS_H

/* 1. Pull in the toolchain's limits.h (for INT_MAX, LONG_MAX, etc.) */
#include_next <limits.h>

/* Define the POSIX standard maximum for a file name component */
#ifndef _POSIX_NAME_MAX
#define _POSIX_NAME_MAX 255
#endif

/* Define PATH_MAX relative to _POSIX_NAME_MAX to maintain internal logic */
#ifndef PATH_MAX
#define PATH_MAX (_POSIX_NAME_MAX * 4)
#endif

/* Define LINE_MAX for POSIX shell utilities (required by flx_sh) */
#ifndef LINE_MAX
#define LINE_MAX 1024
#endif

/* =========================================================================
 * POSIX.1 Minimum Acceptable Values (and Felix OS specific limits)
 * ========================================================================= */

#ifndef _POSIX_AIO_LISTIO_MAX
#define _POSIX_AIO_LISTIO_MAX 2
#endif
#ifndef _POSIX_AIO_MAX
#define _POSIX_AIO_MAX 1
#endif
#ifndef _POSIX_ARG_MAX
#define _POSIX_ARG_MAX 4096
#endif
#ifndef _POSIX_CHILD_MAX
#define _POSIX_CHILD_MAX 25
#endif
#ifndef _POSIX_DELAYTIMER_MAX
#define _POSIX_DELAYTIMER_MAX 32
#endif
#ifndef _POSIX_HOST_NAME_MAX
#define _POSIX_HOST_NAME_MAX 255
#endif
#ifndef _POSIX_LINK_MAX
#define _POSIX_LINK_MAX 8
#endif
#ifndef _POSIX_LOGIN_NAME_MAX
#define _POSIX_LOGIN_NAME_MAX 9
#endif
#ifndef _POSIX_MAX_CANON
#define _POSIX_MAX_CANON 255
#endif
#ifndef _POSIX_MAX_INPUT
#define _POSIX_MAX_INPUT 255
#endif
#ifndef _POSIX_NAME_MAX
#define _POSIX_NAME_MAX 255 /* Matched to FatFs LFN maximum */
#endif
#ifndef _POSIX_NGROUPS_MAX
#define _POSIX_NGROUPS_MAX 8
#endif
#ifndef _POSIX_OPEN_MAX
#define _POSIX_OPEN_MAX 20
#endif
#ifndef _POSIX_PATH_MAX
#define _POSIX_PATH_MAX 256 /* Matched to Felix OS standard */
#endif
#ifndef _POSIX_PIPE_BUF
#define _POSIX_PIPE_BUF 512
#endif
#ifndef _POSIX_RE_DUP_MAX
#define _POSIX_RE_DUP_MAX 255
#endif
#ifndef _POSIX_RTSIG_MAX
#define _POSIX_RTSIG_MAX 8
#endif
#ifndef _POSIX_SEM_NSEMS_MAX
#define _POSIX_SEM_NSEMS_MAX 256
#endif
#ifndef _POSIX_SEM_VALUE_MAX
#define _POSIX_SEM_VALUE_MAX 32767
#endif
#ifndef _POSIX_SIGQUEUE_MAX
#define _POSIX_SIGQUEUE_MAX 32
#endif
#ifndef _POSIX_SSIZE_MAX
#define _POSIX_SSIZE_MAX 32767
#endif
#ifndef _POSIX_STREAM_MAX
#define _POSIX_STREAM_MAX 8
#endif
#ifndef _POSIX_SYMLINK_MAX
#define _POSIX_SYMLINK_MAX 255
#endif
#ifndef _POSIX_SYMLOOP_MAX
#define _POSIX_SYMLOOP_MAX 8
#endif
#ifndef _POSIX_THREAD_DESTRUCTOR_ITERATIONS
#define _POSIX_THREAD_DESTRUCTOR_ITERATIONS 4
#endif
#ifndef _POSIX_THREAD_KEYS_MAX
#define _POSIX_THREAD_KEYS_MAX 128
#endif
#ifndef _POSIX_THREAD_THREADS_MAX
#define _POSIX_THREAD_THREADS_MAX 64
#endif
#ifndef _POSIX_TIMER_MAX
#define _POSIX_TIMER_MAX 32
#endif
#ifndef _POSIX_TTY_NAME_MAX
#define _POSIX_TTY_NAME_MAX 9
#endif
#ifndef _POSIX_TZNAME_MAX
#define _POSIX_TZNAME_MAX 6
#endif

/* System architecture limits */
#ifndef SSIZE_MAX
#define SSIZE_MAX __LONG_MAX__ /* Automatically maps to 64-bit for your riscv64 target */
#endif

/* POSIX.2 (Shell and Utilities) limits */
#ifndef _POSIX2_BC_BASE_MAX
#define _POSIX2_BC_BASE_MAX 99
#endif
#ifndef _POSIX2_BC_DIM_MAX
#define _POSIX2_BC_DIM_MAX 2048
#endif
#ifndef _POSIX2_BC_SCALE_MAX
#define _POSIX2_BC_SCALE_MAX 99
#endif
#ifndef _POSIX2_BC_STRING_MAX
#define _POSIX2_BC_STRING_MAX 1000
#endif
#ifndef _POSIX2_CHARCLASS_NAME_MAX
#define _POSIX2_CHARCLASS_NAME_MAX 14
#endif
#ifndef _POSIX2_COLL_WEIGHTS_MAX
#define _POSIX2_COLL_WEIGHTS_MAX 2
#endif
#ifndef _POSIX2_EXPR_NEST_MAX
#define _POSIX2_EXPR_NEST_MAX 32
#endif
#ifndef _POSIX2_LINE_MAX
#define _POSIX2_LINE_MAX 2048
#endif
#ifndef _POSIX2_RE_DUP_MAX
#define _POSIX2_RE_DUP_MAX 255
#endif

#endif /* _FLX_INCLUDE_LIMITS_H */
