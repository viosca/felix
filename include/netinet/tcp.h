/**
 * @file felix/include/netinet/tcp.h
 * @brief POSIX standard TCP protocol definitions.
 */
#ifndef _NETINET_TCP_H
#define _NETINET_TCP_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Standard TCP Socket Options
 * ========================================================================= */
#define TCP_NODELAY         1  /* Disable Nagle's algorithm */
#define TCP_MAXSEG          2  /* Set maximum segment size */
#define TCP_CORK            3  /* Control sending of partial frames */
#define TCP_KEEPIDLE        4  /* Start keepalives after this period */
#define TCP_KEEPINTVL       5  /* Interval between keepalives */
#define TCP_KEEPCNT         6  /* Number of keepalives before death */
#define TCP_SYNCNT          7  /* Number of SYN retransmits */
#define TCP_LINGER2         8  /* Life time of orphaned FIN-WAIT-2 state */
#define TCP_DEFER_ACCEPT    9  /* Wake up listener only when data arrives */
#define TCP_WINDOW_CLAMP    10 /* Bound advertised window */
#define TCP_INFO            11 /* Information about this connection */
#define TCP_QUICKACK        12 /* Block/reenable quick acks */
#define TCP_CONGESTION      13 /* Congestion control algorithm */
#define TCP_MD5SIG          14 /* TCP MD5 Signature (RFC2385) */
#define TCP_THIN_LINEAR_TIMEOUTS 16 /* Use linear timeouts for thin streams */
#define TCP_THIN_DUPACK     17 /* Fast retrans. after 1 dupack */
#define TCP_USER_TIMEOUT    18 /* How long for loss retry before timeout */
#define TCP_REPAIR          19 /* TCP sock is under repair right now */
#define TCP_REPAIR_QUEUE    20
#define TCP_QUEUE_SEQ       21
#define TCP_REPAIR_OPTIONS  22
#define TCP_FASTOPEN        23 /* Enable FastOpen on listeners */
#define TCP_TIMESTAMP       24 /* TCP time stamp */
#define TCP_NOTSENT_LOWAT   25 /* Limit number of unsent bytes in write queue */
#define TCP_CC_INFO         26 /* Get Congestion Control (optional) info */
#define TCP_SAVE_SYN        27 /* Record SYN headers for new connections */
#define TCP_SAVED_SYN       28 /* Get SYN headers recorded for connection */

#ifdef __cplusplus
}
#endif

#endif /* _NETINET_TCP_H */
