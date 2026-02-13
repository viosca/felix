#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#ifdef __cplusplus
extern "C" {
#endif

/* * Standard IOCTL Commands
 * These numbers don't have to match Linux exactly, but it helps.
 */

/* 1. File IO: Non-Blocking I/O */
#define FIONBIO 0x5421

/* 2. Terminal Control: Get/Set Attributes */
#define TCGETS  0x5401
#define TCSETS  0x5402

/* The standard ioctl prototype */
int ioctl(int fd, int request, ...);

#ifdef __cplusplus
}
#endif

#endif /* _SYS_IOCTL_H */
