/**
 * @file felix/include/sys/uio.h
 * @brief POSIX standard vector I/O operations (scatter/gather I/O).
 */
#ifndef _FLX_INCLUDE_SYS_UIO_H
#define _FLX_INCLUDE_SYS_UIO_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I/O vector structure for scatter/gather operations.
 */
struct iovec {
    void   *iov_base; /* Base address of a memory region for input or output */
    size_t  iov_len;  /* The size of the memory pointed to by iov_base */
};

/* POSIX vector I/O function prototypes */
ssize_t readv(int fd, const struct iovec *iov, int iovcnt);
ssize_t writev(int fd, const struct iovec *iov, int iovcnt);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_UIO_H */
