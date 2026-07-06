/**
 * @file felix/posix/klog.c
 * @brief Kernel logging shims for Felix OS.
 */

#include <sys/klog.h>
#include <errno.h>

/**
 * @brief Stub for reading the kernel log buffer.
 * * Since Felix OS handles logging through its own serial abstractions rather
 * than a standard Linux ring buffer, we stub this out for now to safely
 * fail when POSIX utilities (like dmesg) try to read it.
 */
int klogctl(int type, char *bufp, int len) {
    (void)type;
    (void)bufp;
    (void)len;

    /* Tell the utility that this feature is not implemented */
    errno = ENOSYS;
    return -1;
}
