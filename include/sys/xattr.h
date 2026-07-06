/**
 * @file felix/include/sys/xattr.h
 * @brief POSIX Extended Attributes for Felix OS.
 */
#ifndef _FLX_INCLUDE_SYS_XATTR_H
#define _FLX_INCLUDE_SYS_XATTR_H

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Standard POSIX (Linux) extended attribute prototypes
 * ========================================================================= */
ssize_t getxattr(const char *path, const char *name, void *value, size_t size);
ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size);
ssize_t fgetxattr(int fd, const char *name, void *value, size_t size);

int setxattr(const char *path, const char *name, const void *value, size_t size, int flags);
int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags);
int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags);

ssize_t listxattr(const char *path, char *list, size_t size);
ssize_t llistxattr(const char *path, char *list, size_t size);
ssize_t flistxattr(int fd, char *list, size_t size);

/* macOS-style aliases expected by Toybox */
ssize_t xattr_get(const char *path, const char *name, void *value, size_t size);
ssize_t xattr_lget(const char *path, const char *name, void *value, size_t size);
ssize_t xattr_fget(int fd, const char *name, void *value, size_t size);

ssize_t xattr_set(const char *path, const char *name, const void *value, size_t size, int flags);
ssize_t xattr_lset(const char *path, const char *name, const void *value, size_t size, int flags);
ssize_t xattr_fset(int fd, const char *name, const void *value, size_t size, int flags);

ssize_t xattr_list(const char *path, char *list, size_t size);
ssize_t xattr_llist(const char *path, char *list, size_t size);
ssize_t xattr_flist(int fd, char *list, size_t size);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_SYS_XATTR_H */
