/**
 * @file felix/include/arpa/inet.h
 * @brief POSIX IP address conversion definitions for Felix OS.
 */
#ifndef _FLX_INCLUDE_ARPA_INET_H
#define _FLX_INCLUDE_ARPA_INET_H

#include <stdint.h>

/* Safely pull in toolchain's base arpa/inet.h if it exists */
#if defined(__has_include_next) && __has_include_next(<arpa/inet.h>)
#include_next <arpa/inet.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * POSIX Base Definitions for Internet Addresses
 * ========================================================================= */
#ifndef __in_addr_t_defined
#define __in_addr_t_defined
typedef uint32_t in_addr_t;
typedef uint16_t in_port_t;

struct in_addr {
    in_addr_t s_addr;
};
#endif

/* =========================================================================
 * POSIX Byte-Order Conversion Macros
 * Mapped to GCC built-in byte swap functions for zero-overhead RTOS performance.
 * ========================================================================= */
#ifndef htonl
#define htonl(x) __builtin_bswap32(x)
#define htons(x) __builtin_bswap16(x)
#define ntohl(x) __builtin_bswap32(x)
#define ntohs(x) __builtin_bswap16(x)
#endif

/* =========================================================================
 * Address String Lengths & Types
 * ========================================================================= */
#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

/* Ensure socklen_t is defined (matches netdb.h) */
#ifndef __socklen_t_defined
#define __socklen_t_defined
typedef uint32_t socklen_t;
#endif

/* =========================================================================
 * POSIX Function Prototypes (Stubs until lwIP/Networking is added)
 * ========================================================================= */
in_addr_t inet_addr(const char *cp);
char *inet_ntoa(struct in_addr in);

/**
 * @brief Convert IPv4 and IPv6 addresses from binary to text form.
 */
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size);

/**
 * @brief Convert IPv4 and IPv6 addresses from text to binary form.
 */
int inet_pton(int af, const char *src, void *dst);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_ARPA_INET_H */
