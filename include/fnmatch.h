/**
 * @file felix/include/fnmatch.h
 * @brief POSIX Filename matching definitions for Felix OS.
 */
#ifndef _FLX_INCLUDE_FNMATCH_H
#define _FLX_INCLUDE_FNMATCH_H

#ifdef __cplusplus
extern "C" {
#endif

#define FNM_NOMATCH      1
#define FNM_PATHNAME     (1 << 0)
#define FNM_NOESCAPE     (1 << 1)
#define FNM_PERIOD       (1 << 2)
#define FNM_LEADING_DIR  (1 << 3)
#define FNM_CASEFOLD     (1 << 4)

/**
 * @brief Match filename or pathname.
 * Stubbed for now. Always returns no match to prevent wildcard crashes.
 */
static inline int fnmatch(const char *pattern, const char *string, int flags) {
    (void)pattern;
    (void)string;
    (void)flags;
    return FNM_NOMATCH;
}

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_FNMATCH_H */
