/**
 * @file felix/include/unistd.h
 * @brief POSIX Unistd extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_UNISTD_H
#define _FLX_INCLUDE_UNISTD_H

/* Pull in the toolchain's base unistd.h */
#include_next <unistd.h>

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
