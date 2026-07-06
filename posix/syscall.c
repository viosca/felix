/**
 * @file felix/posix/syscall.c
 * @brief Raw POSIX syscall wrapper for Felix OS libc.
 */

#include <sys/syscall.h>
#include <sys/utsname.h>
#include <stdarg.h>
#include <errno.h>
#include <fcntl.h>
#include "kernel/syscalls.h"

/**
 * @brief Execute a raw numbered system call (e.g., syscall(__NR_copy_file_range)).
 * Extracts up to 6 generic arguments and vectors them to the explicitly
 * named Felix OS kernel backend (kernel/syscalls.c) wrappers.
 */
long syscall(long number, ...) {
    va_list args;
    va_start(args, number);

    /* POSIX system calls take a maximum of 6 arguments.
     * We blindly extract all 6 as generic longs because we don't know
     * the exact signature until we evaluate 'number'. */
    long arg1 = va_arg(args, long);
    long arg2 = va_arg(args, long);
    long arg3 = va_arg(args, long);
    long arg4 = va_arg(args, long);
    long arg5 = va_arg(args, long);
    long arg6 = va_arg(args, long);

    va_end(args);

    long ret = -ENOSYS;

    /* Vector the numbered trap to our named C-functions */
    switch (number) {
        /* =====================================================================
         * Directory & Path Operations
         * ===================================================================== */
        case __NR_getcwd: {
            char *cwd_ret = sys_getcwd((char *)arg1, (size_t)arg2);
            ret = (long)(intptr_t)cwd_ret;

            if (!ret) ret = -ERANGE; /* sys_getcwd returns a pointer, handle failure */
            break;
        }
        case __NR_mkdirat:
            if ((int)arg1 == AT_FDCWD) ret = sys_mkdir((const char *)arg2, (mode_t)arg3);
            break;
        case __NR_unlinkat:
            if ((int)arg1 == AT_FDCWD) ret = sys_unlink((const char *)arg2);
            break;
        case __NR_linkat:
            if ((int)arg1 == AT_FDCWD && (int)arg3 == AT_FDCWD)
                ret = sys_link((const char *)arg2, (const char *)arg4);
            break;
        case __NR_renameat:
            if ((int)arg1 == AT_FDCWD && (int)arg3 == AT_FDCWD)
                ret = sys_rename((const char *)arg2, (const char *)arg4);
            break;
        case __NR_chdir:
            ret = sys_chdir((const char *)arg1);
            break;

        /* =====================================================================
         * File IO Operations
         * ===================================================================== */
        case __NR_ioctl:
            /* Expected to map to sys_ioctl in the future */
            ret = sys_ioctl((int)arg1, (int)arg2, (void *)arg3);
            break;
        case __NR_lseek:
            ret = (long)sys_lseek((int)arg1, (off_t)arg2, (int)arg3);
            break;
        case __NR_truncate:
            ret = sys_truncate((const char *)arg1, (off_t)arg2);
            break;
        case __NR_ftruncate:
            ret = sys_ftruncate((int)arg1, (off_t)arg2);
            break;
        case __NR_faccessat:
            /* Expected to map to sys_faccessat in the future */
            ret = -ENOSYS;
            break;
        case __NR_openat:
            ret = sys_openat((int)arg1, (const char *)arg2, (int)arg3, (int)arg4);
            break;
        case __NR_close:
            ret = sys_close((int)arg1);
            break;
        case __NR_read:
            ret = sys_read((int)arg1, (void *)arg2, (size_t)arg3);
            break;
        case __NR_write:
            ret = sys_write((int)arg1, (const void *)arg2, (size_t)arg3);
            break;
        case __NR_fstat:
            ret = sys_fstat((int)arg1, (struct stat *)arg2);
            break;
        case __NR_stat:
            ret = sys_stat((const char *)arg1, (struct stat *)arg2);
            break;
        case __NR_lstat:
            ret = sys_lstat((const char *)arg1, (struct stat *)arg2);
            break;
        /* =====================================================================
         * Process, Time, & System Info
         * ===================================================================== */
        case __NR_exit:
            sys_exit((int)arg1);
            ret = 0; /* sys_exit does not return, but this satisfies the compiler */
            break;
        case __NR_nanosleep:
            /* Newlib passes struct timespec, sys_usleep expects uint64_t usec.
             * A proper conversion can be mapped here later. */
            ret = -ENOSYS;
            break;
        case __NR_gettimeofday:
            ret = sys_gettimeofday((struct timeval *)arg1, (void *)arg2);
            break;
        case __NR_uname:
            ret = sys_uname((struct utsname *)arg1);
            break;
        case __NR_getpid:
            ret = sys_getpid();
            break;
        case __NR_getuid:
        case __NR_geteuid:
        case __NR_getgid:
        case __NR_getegid:
            ret = 0; /* Felix OS uses a flat root permission model */
            break;

        /* =====================================================================
         * Legacy Syscalls (Expected by older code / explicit fallbacks)
         * ===================================================================== */
        case __NR_open:
            ret = sys_open((const char *)arg1, (int)arg2, (int)arg3);
            break;
        case __NR_link:
            ret = sys_link((const char *)arg1, (const char *)arg2);
            break;
        case __NR_unlink:
            ret = sys_unlink((const char *)arg1);
            break;
        case __NR_mkdir:
            ret = sys_mkdir((const char *)arg1, (mode_t)arg2);
            break;
        case __NR_rename:
            ret = sys_rename((const char *)arg1, (const char *)arg2);
            break;

        /* =====================================================================
         * Advanced / Unimplemented
         * ===================================================================== */
        case __NR_copy_file_range:
        default:
            /* Safely reject unknown or unimplemented syscalls */
            ret = -ENOSYS;
            break;
    }

    /* Standardize the kernel return into the POSIX errno format */
    if (ret < 0) {
        errno = -ret;
        return -1;
    }

    return ret;
}
