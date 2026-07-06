/**
 * @file felix/include/netinet/in.h
 * @brief POSIX standard Internet address family definitions.
 */
#ifndef _NETINET_IN_H
#define _NETINET_IN_H

#include <stdint.h>
/* Pull in in_addr_t, in_port_t, and struct in_addr from our earlier shim */
#include <arpa/inet.h>

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX Socket Address Family Type */
#ifndef __sa_family_t_defined
#define __sa_family_t_defined
typedef uint16_t sa_family_t;
#endif

/* =========================================================================
 * POSIX IPv4 Structures
 * ========================================================================= */
struct sockaddr_in {
    sa_family_t    sin_family; /* Address family (AF_INET) */
    in_port_t      sin_port;   /* Port number */
    struct in_addr sin_addr;   /* IPv4 address */
    uint8_t        sin_zero[8];/* Padding to match struct sockaddr size */
};

/* =========================================================================
 * POSIX IPv6 Structures
 * ========================================================================= */
struct in6_addr {
    uint8_t s6_addr[16];
};

struct sockaddr_in6 {
    sa_family_t     sin6_family;   /* Address family (AF_INET6) */
    in_port_t       sin6_port;     /* Port number */
    uint32_t        sin6_flowinfo; /* IPv6 flow information */
    struct in6_addr sin6_addr;     /* IPv6 address */
    uint32_t        sin6_scope_id; /* Scope ID */
};

/* =========================================================================
 * POSIX Standard IP Protocols
 * ========================================================================= */
#define IPPROTO_IP      0    /* Dummy protocol for TCP */
#define IPPROTO_ICMP    1    /* Internet Control Message Protocol */
#define IPPROTO_TCP     6    /* Transmission Control Protocol */
#define IPPROTO_UDP     17   /* User Datagram Protocol */
#define IPPROTO_IPV6    41   /* IPv6 header */
#define IPPROTO_RAW     255  /* Raw IP packets */

/* =========================================================================
 * POSIX Standard IP Addresses
 * ========================================================================= */
#define INADDR_ANY       ((in_addr_t) 0x00000000)
#define INADDR_BROADCAST ((in_addr_t) 0xffffffff)

/* =========================================================================
 * POSIX Address String Lengths
 * ========================================================================= */
#define INET_ADDRSTRLEN  16  /* Length of string form for IPv4 */
#define INET6_ADDRSTRLEN 46  /* Length of string form for IPv6 */

#ifdef __cplusplus
}
#endif

#endif /* _NETINET_IN_H */
