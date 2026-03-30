/* File: posix/signal.c */
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include "kernel/syscalls.h"

int kill(pid_t pid, int sig) {
    int ret = sys_kill(pid, sig);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}
