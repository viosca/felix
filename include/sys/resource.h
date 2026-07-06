/**
 * @file felix/include/sys/resource.h
 * @brief POSIX Resource Limits and Usage for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_RESOURCE_H
#define _FLX_INCLUDE_SYS_RESOURCE_H

/* Mask Newlib's stripped-down definitions to avoid conflicts */
#define rusage __newlib_rusage
#define getrusage __newlib_getrusage
#include_next <sys/resource.h>
#undef rusage
#undef getrusage

#include <sys/types.h>
#include <sys/time.h>

/* =========================================================================
 * Resource Usage (rusage) - Full POSIX Definition
 * ========================================================================= */

#ifndef RUSAGE_SELF
#define RUSAGE_SELF 0
#define RUSAGE_CHILDREN -1
#endif

/* Provide the complete 16-member POSIX struct expected by Toybox */
struct rusage {
    struct timeval ru_utime; /* user time used */
    struct timeval ru_stime; /* system time used */
    long ru_maxrss;          /* maximum resident set size */
    long ru_ixrss;           /* integral shared memory size */
    long ru_idrss;           /* integral unshared data size */
    long ru_isrss;           /* integral unshared stack size */
    long ru_minflt;          /* page reclaims */
    long ru_majflt;          /* page faults */
    long ru_nswap;           /* swaps */
    long ru_inblock;         /* block input operations */
    long ru_oublock;         /* block output operations */
    long ru_msgsnd;          /* messages sent */
    long ru_msgrcv;          /* messages received */
    long ru_nsignals;        /* signals received */
    long ru_nvcsw;           /* voluntary context switches */
    long ru_nivcsw;          /* involuntary context switches */
};

/* =========================================================================
 * Process Priorities
 * ========================================================================= */

/* --- POSIX Priority definitions (Required by toybox) --- */
#ifndef PRIO_PROCESS
#define PRIO_PROCESS 0
#define PRIO_PGRP    1
#define PRIO_USER    2
#endif

/* =========================================================================
 * Resource Limits (rlimit)
 * ========================================================================= */

/* Define rlim_t if Newlib didn't provide it */
#ifndef _RLIM_T_DECLARED
#define _RLIM_T_DECLARED
typedef unsigned long rlim_t;
#endif

/* Infinity constant */
#ifndef RLIM_INFINITY
#define RLIM_INFINITY (~0UL)
#define RLIM_SAVED_MAX RLIM_INFINITY
#define RLIM_SAVED_CUR RLIM_INFINITY
#endif

/* Provide struct rlimit if missing */
#ifndef _SYS_RESOURCE_H_HAS_RLIMIT
struct rlimit {
    rlim_t rlim_cur; /* Soft limit */
    rlim_t rlim_max; /* Hard limit (ceiling for rlim_cur) */
};
#endif

/* Standard POSIX and Linux-specific RLIMIT constants expected by Toybox */
#ifndef RLIMIT_CPU
#define RLIMIT_CPU      0   /* CPU time in sec */
#define RLIMIT_FSIZE    1   /* Maximum filesize */
#define RLIMIT_DATA     2   /* max data size */
#define RLIMIT_STACK    3   /* max stack size */
#define RLIMIT_CORE     4   /* max core file size */
#define RLIMIT_RSS      5   /* max resident set size */
#define RLIMIT_NPROC    6   /* max number of processes */
#define RLIMIT_NOFILE   7   /* max number of open files */
#define RLIMIT_MEMLOCK  8   /* max locked-in-memory address space */
#define RLIMIT_AS       9   /* address space limit */
#define RLIMIT_LOCKS    10  /* maximum file locks held */
#define RLIMIT_SIGPENDING 11 /* max number of pending signals */
#define RLIMIT_MSGQUEUE 12  /* maximum bytes in POSIX mqueues */
#define RLIMIT_NICE     13  /* max nice prio allowed to raise to */
#define RLIMIT_RTPRIO   14  /* maximum realtime priority */
#define RLIMIT_RTTIME   15  /* timeout for RT tasks in us */
#define RLIMIT_NLIMITS  16
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* --- Function Signatures --- */
int getrusage(int who, struct rusage *usage);
int getpriority(int which, int who);
int setpriority(int which, int who, int prio);

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

/**
 * @brief Linux-specific extension to get/set limits for an arbitrary PID.
 */
int prlimit(pid_t pid, int resource, const struct rlimit *new_limit, struct rlimit *old_limit);

/* Provide a dummy wait4 that satisfies the linker and implies
 * that resource usage reporting is not supported. */
static inline int wait4(int pid, int *status, int options, struct rusage *ru) {
    if (ru) {
        /* Zero out the structure to prevent junk data */
        __builtin_memset(ru, 0, sizeof(struct rusage));
    }
    /* Return -1 (error) because we don't have a real wait4 */
    return -1;
}

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_RESOURCE_H */
