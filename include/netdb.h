/**
 * @file felix/include/netdb.h
 * @brief POSIX standard network database operations.
 */
#ifndef _FLX_INCLUDE_NETDB_H
#define _FLX_INCLUDE_NETDB_H

/* Safely pull in toolchain's base netdb.h if it exists */
#if defined(__has_include_next) && __has_include_next(<netdb.h>)
#include_next <netdb.h>
#endif

#include <stdint.h>

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

/* Basic types needed by netdb.h if sys/socket.h isn't included yet */
#ifndef __socklen_t_defined
#define __socklen_t_defined
typedef uint32_t socklen_t;
#endif

struct sockaddr; /* Forward declaration */

/* Only define hostent if the toolchain didn't provide it.
 * The standard h_addr macro is a reliable indicator. */
#ifndef h_addr
/* =========================================================================
 * POSIX Host Structure
 * ========================================================================= */
struct hostent {
    char  *h_name;            /* Official name of host */
    char **h_aliases;         /* Alias list */
    int    h_addrtype;        /* Host address type */
    int    h_length;          /* Length of address */
    char **h_addr_list;       /* List of addresses */
};
#define h_addr h_addr_list[0] /* For backward compatibility */

struct hostent *gethostbyname(const char *name);
#endif

/* Only define addrinfo if the toolchain didn't provide it.
 * AI_PASSIVE is a reliable indicator. */
#ifndef AI_PASSIVE
/* =========================================================================
 * POSIX Network Structures
 * ========================================================================= */
struct addrinfo {
    int              ai_flags;      /* Input flags */
    int              ai_family;     /* Protocol family for socket */
    int              ai_socktype;   /* Socket type */
    int              ai_protocol;   /* Protocol for socket */
    socklen_t        ai_addrlen;    /* Length of socket-address */
    struct sockaddr *ai_addr;       /* Socket-address for socket */
    char            *ai_canonname;  /* Canonical name for service location */
    struct addrinfo *ai_next;       /* Pointer to next in list */
};

/* =========================================================================
 * POSIX getaddrinfo Flags
 * ========================================================================= */
#define AI_PASSIVE      0x0001
#define AI_CANONNAME    0x0002
#define AI_NUMERICHOST  0x0004
#define AI_NUMERICSERV  0x0008
#define AI_V4MAPPED     0x0010
#define AI_ALL          0x0020
#define AI_ADDRCONFIG   0x0040

/* =========================================================================
 * POSIX getnameinfo Flags
 * ========================================================================= */
#define NI_NAMEREQD     0x01
#define NI_DGRAM        0x02
#define NI_NOFQDN       0x04
#define NI_NUMERICHOST  0x08
#define NI_NUMERICSERV  0x10

/* =========================================================================
 * POSIX Error Codes for getaddrinfo
 * ========================================================================= */
#define EAI_AGAIN       1  /* Temporary failure in name resolution */
#define EAI_BADFLAGS    2  /* Invalid value for ai_flags */
#define EAI_FAIL        3  /* Non-recoverable failure in name resolution */
#define EAI_FAMILY      4  /* ai_family not supported */
#define EAI_MEMORY      5  /* Memory allocation failure */
#define EAI_NONAME      6  /* Name or service not known */
#define EAI_SERVICE     8  /* Service not supported for ai_socktype */
#define EAI_SOCKTYPE    10 /* ai_socktype not supported */
#define EAI_SYSTEM      11 /* System error returned in errno */

/* =========================================================================
 * POSIX Function Prototypes (Stubs until lwIP/Networking is added)
 * ========================================================================= */
void freeaddrinfo(struct addrinfo *ai);
const char *gai_strerror(int ecode);
int getaddrinfo(const char *nodename, const char *servname,
                const struct addrinfo *hints, struct addrinfo **res);
int getnameinfo(const struct sockaddr *sa, socklen_t salen,
                char *node, socklen_t nodelen, char *service,
                socklen_t servicelen, int flags);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_NETDB_H */
