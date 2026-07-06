/**
 * @file felix/include/paths.h
 * @brief POSIX standard paths for Felix OS.
 */
#ifndef _FLX_INCLUDE_PATHS_H
#define _FLX_INCLUDE_PATHS_H

/* Safely pull in toolchain's base paths.h if it ever gets added */
#if defined(__has_include_next) && __has_include_next(<paths.h>)
#include_next <paths.h>
#endif

/* =========================================================================
 * Standard POSIX / Linux System Paths
 * ========================================================================= */

/* Default search path for executables */
#ifndef _PATH_DEFPATH
#define _PATH_DEFPATH "/bin"
#endif

/* Default user path */
#ifndef _PATH_STDPATH
#define _PATH_STDPATH "/bin"
#endif

/* Standard shell */
#ifndef _PATH_BSHELL
#define _PATH_BSHELL "/bin/sh"
#endif

/* Common devices */
#ifndef _PATH_CONSOLE
#define _PATH_CONSOLE "/dev/console"
#endif

#ifndef _PATH_DEVNULL
#define _PATH_DEVNULL "/dev/null"
#endif

#ifndef _PATH_TTY
#define _PATH_TTY "/dev/tty"
#endif

/* System files and directories */
#ifndef _PATH_MAILDIR
#define _PATH_MAILDIR "/var/mail"
#endif

#ifndef _PATH_SHADOW
#define _PATH_SHADOW "/etc/shadow"
#endif

#ifndef _PATH_PASSWD
#define _PATH_PASSWD "/etc/passwd"
#endif

#ifndef _PATH_WTMP
#define _PATH_WTMP "/var/log/wtmp"
#endif

#ifndef _PATH_UTMP
#define _PATH_UTMP "/var/run/utmp"
#endif

#ifndef _PATH_MNTTAB
#define _PATH_MNTTAB "/etc/fstab"
#endif

#ifndef _PATH_MOUNTED
#define _PATH_MOUNTED "/etc/mtab"
#endif

#endif /* _FLX_INCLUDE_PATHS_H */
