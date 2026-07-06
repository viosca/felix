/**
 * @file felix/include/netdb.h
 * @brief POSIX Network Database extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_NETDB_H
#define _FLX_INCLUDE_NETDB_H

/* Safely pull in toolchain's base netdb.h if it exists */
#if defined(__has_include_next) && __has_include_next(<netdb.h>)
#include_next <netdb.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Standard h_errno error codes */
#ifndef HOST_NOT_FOUND
#define HOST_NOT_FOUND 1
#define TRY_AGAIN      2
#define NO_RECOVERY    3
#define NO_DATA        4
#define NO_ADDRESS     NO_DATA
#endif

/* Declare the global network error variable */
extern int h_errno;

/**
 * @brief Convert a network host error code to a string.
 * Expected by Toybox's 'hostname' utility.
 */
const char *hstrerror(int err);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_NETDB_H */
