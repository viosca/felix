/**
 * @file felix/posix/xattr.c
 * @brief Extended attribute shims for Felix OS.
 */

#include <sys/xattr.h>
#include <errno.h>

/* =========================================================================
 * Standard Linux POSIX Extended Attributes
 * ========================================================================= */
ssize_t getxattr(const char *path, const char *name, void *value, size_t size) { (void)path; (void)name; (void)value; (void)size; errno = ENOSYS; return -1; }
ssize_t lgetxattr(const char *path, const char *name, void *value, size_t size) { (void)path; (void)name; (void)value; (void)size; errno = ENOSYS; return -1; }
ssize_t fgetxattr(int fd, const char *name, void *value, size_t size) { (void)fd; (void)name; (void)value; (void)size; errno = ENOSYS; return -1; }

int setxattr(const char *path, const char *name, const void *value, size_t size, int flags) { (void)path; (void)name; (void)value; (void)size; (void)flags; errno = ENOSYS; return -1; }
int lsetxattr(const char *path, const char *name, const void *value, size_t size, int flags) { (void)path; (void)name; (void)value; (void)size; (void)flags; errno = ENOSYS; return -1; }
int fsetxattr(int fd, const char *name, const void *value, size_t size, int flags) { (void)fd; (void)name; (void)value; (void)size; (void)flags; errno = ENOSYS; return -1; }

ssize_t listxattr(const char *path, char *list, size_t size) { (void)path; (void)list; (void)size; errno = ENOSYS; return -1; }
ssize_t llistxattr(const char *path, char *list, size_t size) { (void)path; (void)list; (void)size; errno = ENOSYS; return -1; }
ssize_t flistxattr(int fd, char *list, size_t size) { (void)fd; (void)list; (void)size; errno = ENOSYS; return -1; }

/* =========================================================================
 * macOS-Style Extended Attributes (Expected by Toybox)
 * Defined as weak symbols to prevent linker collisions if Toybox's
 * lib/portability.c successfully compiles its own internal wrappers.
 * ========================================================================= */
__attribute__((weak)) ssize_t xattr_get(const char *path, const char *name, void *value, size_t size) { return getxattr(path, name, value, size); }
__attribute__((weak)) ssize_t xattr_lget(const char *path, const char *name, void *value, size_t size) { return lgetxattr(path, name, value, size); }
__attribute__((weak)) ssize_t xattr_fget(int fd, const char *name, void *value, size_t size) { return fgetxattr(fd, name, value, size); }

__attribute__((weak)) ssize_t xattr_set(const char *path, const char *name, const void *value, size_t size, int flags) { return setxattr(path, name, value, size, flags); }
__attribute__((weak)) ssize_t xattr_lset(const char *path, const char *name, const void *value, size_t size, int flags) { return lsetxattr(path, name, value, size, flags); }
__attribute__((weak)) ssize_t xattr_fset(int fd, const char *name, const void *value, size_t size, int flags) { return fsetxattr(fd, name, value, size, flags); }

__attribute__((weak)) ssize_t xattr_list(const char *path, char *list, size_t size) { return listxattr(path, list, size); }
__attribute__((weak)) ssize_t xattr_llist(const char *path, char *list, size_t size) { return llistxattr(path, list, size); }
__attribute__((weak)) ssize_t xattr_flist(int fd, char *list, size_t size) { return flistxattr(fd, list, size); }
