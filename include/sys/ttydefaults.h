/**
 * @file felix/include/sys/ttydefaults.h
 * @brief POSIX/BSD Terminal defaults for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_TTYDEFAULTS_H
#define _FLX_INCLUDE_SYS_TTYDEFAULTS_H

#include <termios.h>

#ifndef CTRL
#define CTRL(x) (x & 037)
#endif
/* =========================================================================
 * Standard Terminal Control Character Defaults
 * ========================================================================= */
#define CEOF        CTRL('d')
#define CEOL        0
#define CERASE      0177        /* DEL */
#define CINTR       CTRL('c')
#define CSTATUS     CTRL('t')
#define CKILL       CTRL('u')
#define CMIN        1
#define CQUIT       034         /* FS, ^\ */
#define CSUSP       CTRL('z')
#define CTIME       0
#define CDSUSP      CTRL('y')
#define CSTART      CTRL('q')
#define CSTOP       CTRL('s')
#define CLNEXT      CTRL('v')
#define CDISCARD    CTRL('o')
#define CWERASE     CTRL('w')
#define CREPRINT    CTRL('r')
#define CEOT        CEOF

/* =========================================================================
 * Default Termios Flags (For convenience)
 * ========================================================================= */
#define TTYDEF_IFLAG  (BRKINT | ICRNL | IMAXBEL | IXON | IXANY)
#define TTYDEF_OFLAG  (OPOST | ONLCR)
#define TTYDEF_LFLAG  (ECHO | ICANON | ISIG | IEXTEN | ECHOE|ECHOKE|ECHOCTL)
#define TTYDEF_CFLAG  (CREAD | CS8 | HUPCL)
#define TTYDEF_SPEED  (B9600)

#endif /* _FLX_INCLUDE_SYS_TTYDEFAULTS_H */
