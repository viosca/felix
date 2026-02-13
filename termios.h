#ifndef _TERMIOS_H
#define _TERMIOS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Type Definitions */
typedef unsigned int  tcflag_t;
typedef unsigned char cc_t;
typedef unsigned int  speed_t;

#define NCCS 32

/* The Terminal Control Structure */
struct termios {
    tcflag_t c_iflag; /* input mode flags */
    tcflag_t c_oflag; /* output mode flags */
    tcflag_t c_cflag; /* control mode flags */
    tcflag_t c_lflag; /* local mode flags (ECHO, ICANON) */
    cc_t     c_line;  /* line discipline */
    cc_t     c_cc[NCCS]; /* control characters */
};

/* Local Mode Flags (c_lflag) */
#define ECHO    0x0000010 /* Enable echo */
#define ICANON  0x0000002 /* Canonical mode (Cooked) */

/* Attribute Selection for tcsetattr() - Optional but standard */
#define TCSANOW   0
#define TCSADRAIN 1
#define TCSAFLUSH 2

#ifdef __cplusplus
}
#endif

#endif /* _TERMIOS_H */
