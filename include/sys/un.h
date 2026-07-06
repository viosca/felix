/**
 * @file felix/include/sys/un.h
 * @brief POSIX standard UNIX domain sockets.
 */
#ifndef _FLX_INCLUDE_SYS_UN_H
#define _FLX_INCLUDE_SYS_UN_H

#include <stdint.h>
#include <string.h> /* Required for strlen in SUN_LEN */
#include <stddef.h> /* Required for offsetof in SUN_LEN */

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX Base socket types */
#ifndef __sa_family_t_defined
#define __sa_family_t_defined
typedef uint16_t sa_family_t;
#endif

/* Maximum length of the sun_path array.
 * POSIX doesn't mandate a strict size, but 108 is the historic Linux/BSD standard. */
#define UNIX_PATH_MAX 108

/* =========================================================================
 * POSIX UNIX Domain Socket Structure
 * ========================================================================= */
struct sockaddr_un {
    sa_family_t sun_family;         /* Address family (AF_UNIX) */
    char        sun_path[UNIX_PATH_MAX]; /* Null-terminated Socket pathname */
};

/* =========================================================================
 * Helper Macros
 * ========================================================================= */
/* * Macro to compute the actual length of a UNIX domain socket address,
 * including the null-terminated path and the family field.
 */
#define SUN_LEN(ptr) ((size_t)(offsetof(struct sockaddr_un, sun_path) + strlen((ptr)->sun_path)))

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_UN_H */
