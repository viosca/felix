/**
 * @file felix/posix/inet.c
 * @brief IP address conversion shims for Felix OS.
 */

#include <arpa/inet.h>
#include <errno.h>
#include <stddef.h>
#include <string.h>

/**
 * @brief Convert a string IP address to a numeric IP address.
 */
in_addr_t inet_addr(const char *cp) {
    (void)cp;
    /* Return INADDR_NONE (0xFFFFFFFF) as a fallback */
    return 0xFFFFFFFF;
}

/**
 * @brief Convert a numeric IP address to a string.
 */
char *inet_ntoa(struct in_addr in) {
    (void)in;
    /* Return a dummy static string so utilities printing IPs don't crash */
    static char buffer[] = "offline";
    return buffer;
}

/**
 * @brief Network to Presentation (Binary to String)
 * Felix OS currently has no network stack, so this returns an unsupported error.
 */
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
    (void)af;
    (void)src;

    if (dst && size > 0) {
        strncpy(dst, "offline", size - 1);
        dst[size - 1] = '\0';
        return dst;
    }

    errno = EAFNOSUPPORT; /* Address family not supported */
    return NULL;
}

/**
 * @brief Presentation to Network (String to Binary)
 */
int inet_pton(int af, const char *src, void *dst) {
    (void)af;
    (void)src;
    (void)dst;

    /* Return 0 to indicate the input string is not a valid network address */
    errno = EAFNOSUPPORT;
    return 0;
}
