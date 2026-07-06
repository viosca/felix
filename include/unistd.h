/**
 * @file felix/include/unistd.h
 * @brief POSIX Unistd extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_UNISTD_H
#define _FLX_INCLUDE_UNISTD_H

/* Pull in the toolchain's base unistd.h */
#include_next <unistd.h>

/* =========================================================================
 * POSIX sysconf / confstr Macros (Missing from Newlib)
 * ========================================================================= */
#ifndef _SC_UIO_MAXIOV
#define _SC_UIO_MAXIOV 9999
#endif

#ifndef _CS_PATH
#define _CS_PATH 9998
#endif

#ifndef _CS_V7_ENV
#define _CS_V7_ENV 9997
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief POSIX password encryption function.
 */
char *crypt(const char *key, const char *salt);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_UNISTD_H */
