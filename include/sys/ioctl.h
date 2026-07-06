/**
 * @file felix/include/sys/ioctl.h
 * @brief POSIX ioctl extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_IOCTL_H
#define _FLX_INCLUDE_SYS_IOCTL_H

/* Safely pull in toolchain's base sys/ioctl.h if it exists */
#if defined(__has_include_next) && __has_include_next(<sys/ioctl.h>)
#include_next <sys/ioctl.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * POSIX / Linux ioctl commands
 * ========================================================================= */

/* 1. File IO */
#ifndef FIONBIO
#define FIONBIO     0x5421  /* Set/clear non-blocking I/O */
#endif
#ifndef FIONREAD
#define FIONREAD    0x541B  /* Get number of bytes ready to read */
#endif

/* 2. Terminal Control: Get/Set Attributes */
#ifndef TCGETS
#define TCGETS      0x5401  /* Get terminal attributes */
#endif
#ifndef TCSETS
#define TCSETS      0x5402  /* Set terminal attributes */
#endif
#ifndef TCSETSW
#define TCSETSW     0x5403
#endif
#ifndef TCSETSF
#define TCSETSF     0x5404
#endif
#ifndef TIOCNOTTY
#define TIOCNOTTY   0x5422  /* Give up controlling terminal */
#endif

/* 3. Terminal Window Size */
#ifndef TIOCGWINSZ
#define TIOCGWINSZ  0x5413  /* Get window size */
#endif
#ifndef TIOCSWINSZ
#define TIOCSWINSZ  0x5414  /* Set window size */
#endif

/* =========================================================================
 * Window Size Structure (Required by TIOCGWINSZ)
 * ========================================================================= */
#ifndef _STRUCT_WINSIZE
#define _STRUCT_WINSIZE
struct winsize {
    unsigned short ws_row;
    unsigned short ws_col;
    unsigned short ws_xpixel;
    unsigned short ws_ypixel;
};
#endif

/* =========================================================================
 * POSIX Function Prototypes
 * ========================================================================= */
int ioctl(int fd, unsigned long request, ...);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_IOCTL_H */
