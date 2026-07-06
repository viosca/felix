/**
 * @file felix/include/signal.h
 * @brief POSIX Signal extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SIGNAL_H
#define _FLX_INCLUDE_SIGNAL_H

/* Pull in the toolchain's base signal.h */
#include_next <signal.h>

/* =========================================================================A
 * POSIX sigaction flags missing from bare-metal Newlib
 * ========================================================================= */

#ifndef SA_NOCLDSTOP
#define SA_NOCLDSTOP 0x00000001
#endif

#ifndef SA_SIGINFO
#define SA_SIGINFO   0x00000004
#endif

/* Required by Toybox xargs */
#ifndef SA_RESTART
#define SA_RESTART   0x10000000
#endif

#ifndef SA_NODEFER
#define SA_NODEFER   0x40000000
#endif

#ifndef SA_RESETHAND
#define SA_RESETHAND 0x80000000
#endif

#ifndef SIGHUP
#define SIGHUP    1
#define SIGQUIT   3
#define SIGTRAP   5
#define SIGBUS    7
#define SIGKILL   9
#define SIGUSR1   10
#define SIGUSR2   12
#define SIGPIPE   13
#define SIGALRM   14
#define SIGCHLD   17
#define SIGCONT   18
#define SIGSTOP   19
#define SIGTSTP   20
#define SIGTTIN   21
#define SIGTTOU   22
#define SIGURG    23
#define SIGXCPU   24
#define SIGXFSZ   25
#define SIGVTALRM 26
#define SIGPROF   27
#define SIGWINCH  28
#define SIGIO     29
#define SIGPWR    30
#define SIGSYS    31
#endif


#endif /* _FLX_INCLUDE_SIGNAL_H */
