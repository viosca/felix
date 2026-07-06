/**
 * @file felix/posix/netdb.c
 * @brief Network database shims for Felix OS.
 */

#include <netdb.h>

/* Define the global h_errno variable */
int h_errno = 0;

/**
 * @brief Returns a string describing the given h_errno value.
 */
const char *hstrerror(int err) {
    switch (err) {
        case HOST_NOT_FOUND: return "Host not found";
        case TRY_AGAIN:      return "Try again";
        case NO_RECOVERY:    return "Non-recoverable error";
        case NO_DATA:        return "No data record of requested type";
        default:             return "Unknown network error";
    }
}
