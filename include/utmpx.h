/**
 * @file felix/include/utmpx.h
 * @brief POSIX User Accounting extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_UTMPX_H
#define _FLX_INCLUDE_UTMPX_H

/* Safely mask and pull in any existing system definition if it exists,
 * preventing 'no include path' crashes if the toolchain completely lacks it. */
#if defined(__has_include_next) && __has_include_next(<utmpx.h>)
#define utmpx __newlib_utmpx
#include_next <utmpx.h>
#undef utmpx
#endif

#include <sys/types.h>
#include <sys/time.h>

/* Standard POSIX utmpx types */
#ifndef EMPTY
#define EMPTY           0
#define RUN_LVL         1
#define BOOT_TIME       2
#define NEW_TIME        3
#define OLD_TIME        4
#define INIT_PROCESS    5
#define LOGIN_PROCESS   6
#define USER_PROCESS    7
#define DEAD_PROCESS    8
#endif

/* Provide the complete POSIX struct expected by Toybox's 'who' command */
struct utmpx {
    short ut_type;              /* Type of login. */
    pid_t ut_pid;               /* Process ID of login process. */
    char ut_line[32];           /* Devicename. */
    char ut_id[4];              /* Inittab ID. */
    char ut_user[32];           /* Username. */
    char ut_host[256];          /* Hostname for remote login. */
    struct timeval ut_tv;       /* Time entry was made. */
};

#ifdef __cplusplus
extern "C" {
#endif

/* Stubs for user accounting database access */
void setutxent(void);
void endutxent(void);
struct utmpx *getutxent(void);
struct utmpx *getutxid(const struct utmpx *id);
struct utmpx *getutxline(const struct utmpx *line);
struct utmpx *pututxline(const struct utmpx *ut);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_UTMPX_H */
