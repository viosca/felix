/**
 * @file felix/include/shadow.h
 * @brief POSIX Shadow Password extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_SHADOW_H
#define _FLX_INCLUDE_SHADOW_H

#ifdef __cplusplus
extern "C" {
#endif

/* Structure expected by POSIX utilities parsing /etc/shadow */
struct spwd {
    char *sp_namp;         /* Login name */
    char *sp_pwdp;         /* Encrypted password */
    long  sp_lstchg;       /* Date of last change */
    long  sp_min;          /* Minimum number of days between changes */
    long  sp_max;          /* Maximum number of days between changes */
    long  sp_warn;         /* Number of days of warning before password expires */
    long  sp_inact;        /* Number of days after password expires until account is disabled */
    long  sp_expire;       /* Date when account expires */
    unsigned long sp_flag; /* Reserved */
};

struct spwd *getspnam(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SHADOW_H */
