/* @file felix/posix/resource.c
 */
#include <sys/resource.h>
#include <errno.h>
/* Include your internal kernel syscall header ONLY here */
#include "kernel/syscalls.h"

int getpriority(int which, int who) {
    /* * If your kernel doesn't support this yet, implement the logic
     * here or call a sys_getpriority() if one exists.
     */
    return 0;
}

int setpriority(int which, int who, int prio) {
    return -ENOSYS; // Indicate not implemented in kernel yet
}
