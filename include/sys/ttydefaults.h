/**
 * @file felix/include/sys/ttydefaults.h
 * @brief POSIX/BSD Terminal defaults for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_TTYDEFAULTS_H
#define _FLX_INCLUDE_SYS_TTYDEFAULTS_H

#include <termios.h>

/* =========================================================================
 * Standard Terminal Control Character Defaults
 * ========================================================================= */
#define CEOF        ('d' & 037) /* Ctrl+D */
#define CEOL        0
#define CERASE      0177        /* DEL */
#define CINTR       ('c' & 037) /* Ctrl+C */
#define CSTATUS     ('t' & 037) /* Ctrl+T */
#define CKILL       ('u' & 037) /* Ctrl+U */
#define CMIN        1
#define CQUIT       034         /* FS, ^\ */
#define CSUSP       ('z' & 037) /* Ctrl+Z */
#define CTIME       0
#define CDSUSP      ('y' & 037) /* Ctrl+Y */
#define CSTART      ('q' & 037) /* Ctrl+Q */
#define CSTOP       ('s' & 037) /* Ctrl+S */
#define CLNEXT      ('v' & 037) /* Ctrl+V */
#define CDISCARD    ('o' & 037) /* Ctrl+O */
#define CWERASE     ('w' & 037) /* Ctrl+W */
#define CREPRINT    ('r' & 037) /* Ctrl+R */
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
