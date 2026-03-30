/* File: posix/time.c */
#include <sys/time.h>
#include <errno.h>
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
