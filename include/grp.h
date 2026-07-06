/**
 * @file felix/include/grp.h
 * @brief Felix OS extensions to the standard group definitions.
 */
#ifndef _FLX_INCLUDE_GRP_H
#define _FLX_INCLUDE_GRP_H

/* Pull in Newlib's base group structures */
#include_next <grp.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Calculate group access list.
 * @note This is a GNU/BSD extension often expected by Linux utilities.
 */
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_GRP_H */
