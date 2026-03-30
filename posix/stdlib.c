/* File: posix/stdlib.c */
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "kernel/syscalls.h"

/* * By defining these here, the linker will use our POSIX implementations
 * instead of Newlib's built-in (malloc-based) versions. This ensures
 * environment variables are stored safely inside the kernel's fixed PCB
 */

char *getenv(const char *name) {
    if (!name) {
        errno = EINVAL;
        return NULL;
    }
    /* getenv doesn't return an error code, just NULL if not found */
    return (char *)sys_getenv(name);
}

int setenv(const char *name, const char *value, int overwrite) {
    if (!name || !value) {
        errno = EINVAL;
        return -1;
    }
    int ret = sys_setenv(name, value, overwrite);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int unsetenv(const char *name) {
    if (!name) {
        errno = EINVAL;
        return -1;
    }
    int ret = sys_unsetenv(name);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int putenv(char *string) {
    if (!string) {
        errno = EINVAL;
        return -1;
    }

    char *eq = strchr(string, '=');
    if (!eq) {
        /* POSIX behavior is unspecified if there is no '='. We reject safely. */
        errno = EINVAL;
        return -1;
    }

    size_t name_len = eq - string;

    /* Allocate a temporary buffer for the name to avoid magic stack limits */
    char *name = malloc(name_len + 1);
    if (!name) {
        errno = ENOMEM;
        return -1;
    }

    strncpy(name, string, name_len);
    name[name_len] = '\0';

    const char *value = eq + 1;

    /* POSIX putenv always overwrites existing variables */
    int ret = setenv(name, value, 1);

    /* Clean up the temporary buffer immediately since setenv copies it into the kernel PCB */
    free(name);

    return ret;
}
