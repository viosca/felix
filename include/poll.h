/**
 * @file felix/include/poll.h
 * @brief POSIX standard synchronous I/O multiplexing.
 */
#ifndef _FLX_INCLUDE_POLL_H
#define _FLX_INCLUDE_POLL_H

#ifdef __cplusplus
extern "C" {
#endif

/* Type used for the number of file descriptors */
typedef unsigned long nfds_t;

/* Data structure describing a polling request */
struct pollfd {
    int   fd;       /* File descriptor */
    short events;   /* Requested events */
    short revents;  /* Returned events (populated by the kernel) */
};

/* =========================================================================
 * POSIX Poll Event Flags
 * ========================================================================= */
#define POLLIN      0x001  /* There is data to read */
#define POLLPRI     0x002  /* There is urgent data to read */
#define POLLOUT     0x004  /* Writing now will not block */
#define POLLERR     0x008  /* Error condition */
#define POLLHUP     0x010  /* Hung up (connection closed) */
#define POLLNVAL    0x020  /* Invalid request: fd not open */

/* XPG4.2 / POSIX.1-2001 extended flags */
#define POLLRDNORM  0x040  /* Normal data may be read */
#define POLLRDBAND  0x080  /* Priority data may be read */
#define POLLWRNORM  0x100  /* Writing now will not block */
#define POLLWRBAND  0x200  /* Priority data may be written */

/* =========================================================================
 * POSIX Function Prototypes
 * ========================================================================= */
int poll(struct pollfd *fds, nfds_t nfds, int timeout);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_POLL_H */
