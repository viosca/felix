/**
 * @file felix/include/sys/socket.h
 * @brief POSIX standard socket API definitions.
 */
#ifndef _FLX_INCLUDE_SYS_SOCKET_H
#define _FLX_INCLUDE_SYS_SOCKET_H

#include <sys/types.h>
#include <sys/uio.h> /* For struct iovec */
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX Base socket types */
#ifndef __socklen_t_defined
#define __socklen_t_defined
typedef uint32_t socklen_t;
#endif

#ifndef __sa_family_t_defined
#define __sa_family_t_defined
typedef uint16_t sa_family_t;
#endif

/* =========================================================================
 * POSIX Socket Types and Protocols
 * ========================================================================= */
#define SOCK_STREAM     1   /* Sequenced, reliable, connection-based byte streams */
#define SOCK_DGRAM      2   /* Connectionless, unreliable datagrams */
#define SOCK_RAW        3   /* Raw protocol interface */
#define SOCK_RDM        4   /* Reliably-delivered messages */
#define SOCK_SEQPACKET  5   /* Sequenced, reliable, connection-based, datagrams */

/* =========================================================================
 * POSIX Address Families
 * ========================================================================= */
#define AF_UNSPEC       0   /* Unspecified */
#define AF_UNIX         1   /* Local to host (pipes) */
#define AF_LOCAL        1   /* POSIX name for AF_UNIX */
#define AF_INET         2   /* IP protocol family (IPv4) */
#define AF_INET6        10  /* IP version 6 */
#define AF_PACKET       17  /* Packet family */

/* Protocol families (usually the same as address families) */
#define PF_UNSPEC       AF_UNSPEC
#define PF_UNIX         AF_UNIX
#define PF_LOCAL        AF_LOCAL
#define PF_INET         AF_INET
#define PF_INET6        AF_INET6
#define PF_PACKET       AF_PACKET

/* =========================================================================
 * Standard Socket Options (level = SOL_SOCKET)
 * ========================================================================= */
#define SOL_SOCKET      1
#define SO_DEBUG        1
#define SO_REUSEADDR    2
#define SO_TYPE         3
#define SO_ERROR        4
#define SO_DONTROUTE    5
#define SO_BROADCAST    6
#define SO_SNDBUF       7
#define SO_RCVBUF       8
#define SO_KEEPALIVE    9
#define SO_OOBINLINE    10
#define SO_LINGER       13
#define SO_RCVTIMEO     20
#define SO_SNDTIMEO     21

/* =========================================================================
 * POSIX Socket Structures
 * ========================================================================= */

/**
 * @brief Generic socket address structure.
 */
struct sockaddr {
    sa_family_t sa_family;      /* Address family */
    char        sa_data[14];    /* Socket address (variable-length data) */
};

/**
 * @brief Socket address structure large enough to hold any socket address.
 */
struct sockaddr_storage {
    sa_family_t ss_family;      /* Address family */
    char        __ss_padding[128 - sizeof(sa_family_t)];
};

/**
 * @brief Message header for sendmsg and recvmsg.
 */
struct msghdr {
    void         *msg_name;       /* Optional address */
    socklen_t     msg_namelen;    /* Size of address */
    struct iovec *msg_iov;        /* Scatter/gather array */
    int           msg_iovlen;     /* Members in msg_iov */
    void         *msg_control;    /* Ancillary data */
    socklen_t     msg_controllen; /* Ancillary data buffer len */
    int           msg_flags;      /* Flags on received message */
};

/**
 * @brief Control message header.
 */
struct cmsghdr {
    socklen_t cmsg_len;   /* Data byte count, including header */
    int       cmsg_level; /* Originating protocol */
    int       cmsg_type;  /* Protocol-specific type */
    /* Followed by unsigned char cmsg_data[] */
};

/**
 * @brief Linger structure for SO_LINGER option.
 */
struct linger {
    int l_onoff;  /* Nonzero to linger on close */
    int l_linger; /* Time to linger */
};

/* =========================================================================
 * POSIX Message Flags
 * ========================================================================= */
#define MSG_OOB         0x01    /* Process out-of-band data */
#define MSG_PEEK        0x02    /* Peek at incoming messages */
#define MSG_DONTROUTE   0x04    /* Don't use local routing */
#define MSG_DONTWAIT    0x40    /* Nonblocking I/O for this operation */
#define MSG_NOSIGNAL    0x4000  /* Do not generate SIGPIPE */

/* =========================================================================
 * POSIX Shutdown Options
 * ========================================================================= */
#define SHUT_RD         0       /* No more receptions */
#define SHUT_WR         1       /* No more transmissions */
#define SHUT_RDWR       2       /* No more receptions or transmissions */

/* =========================================================================
 * POSIX Socket Function Prototypes (Stubs until network stack is added)
 * ========================================================================= */
int     socket(int domain, int type, int protocol);
int     bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int     listen(int sockfd, int backlog);
int     accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addrlen);
ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t sendmsg(int sockfd, const struct msghdr *msg, int flags);
ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags);
int     getsockopt(int sockfd, int level, int optname, void *optval, socklen_t *optlen);
int     setsockopt(int sockfd, int level, int optname, const void *optval, socklen_t optlen);
int     getsockname(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     getpeername(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int     shutdown(int sockfd, int how);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_SOCKET_H */
