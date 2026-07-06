/**
 * @file felix/posix/dirent.c
 * @brief POSIX Directory stream implementations using standalone DIR.
 */

#include <dirent.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "kernel/syscalls.h"

/* =========================================================================
 * POSIX Directory Stream Overrides
 * ========================================================================= */

DIR *opendir(const char *name) {
    if (!name) {
        errno = EINVAL;
        return NULL;
    }

    int dir_fd = sys_opendir(name);
    if (dir_fd < 0) {
        errno = -dir_fd;
        return NULL;
    }

    /* Allocate our standalone DIR structure */
    DIR *dirp = (DIR *)calloc(1, sizeof(DIR));
    if (!dirp) {
        sys_closedir(dir_fd);
        errno = ENOMEM;
        return NULL;
    }

    dirp->dd_fd = dir_fd;
    return dirp;
}

DIR *fdopendir(int fd) {
    struct stat st;

    if (fstat(fd, &st) < 0) return NULL;

    if (!S_ISDIR(st.st_mode)) {
        errno = ENOTDIR;
        return NULL;
    }

    DIR *dirp = (DIR *)calloc(1, sizeof(DIR));
    if (!dirp) {
        errno = ENOMEM;
        return NULL;
    }

    dirp->dd_fd = fd;
    return dirp;
}

struct dirent *readdir(DIR *dirp) {
    if (!dirp) {
        errno = EBADF;
        return NULL;
    }

    /* Read directly into the DIR stream's private buffer */
    int ret = sys_readdir(dirp->dd_fd, &dirp->dd_ent);
    if (ret < 0) {
        errno = -ret;
        return NULL;
    }

    if (ret == 0) return NULL;

    return &dirp->dd_ent;
}

int readdir_r(DIR *dirp, struct dirent *entry, struct dirent **result) {
    if (!dirp || !entry || !result) return EBADF;

    /* Write directly to the caller's buffer */
    int ret = sys_readdir(dirp->dd_fd, entry);
    if (ret < 0) {
        *result = NULL;
        return -ret; /* readdir_r returns the error number directly, not via errno */
    }

    if (ret == 0) {
        *result = NULL; /* End of directory stream */
    } else {
        *result = entry; /* Success, point result to the caller's entry */
    }

    return 0;
}

int closedir(DIR *dirp) {
    if (!dirp) {
        errno = EBADF;
        return -1;
    }

    int ret = sys_closedir(dirp->dd_fd);
    free(dirp);

    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return 0;
}

int fdclosedir(DIR *dirp) {
    /* Same as closedir in our implementation */
    return closedir(dirp);
}

long telldir(DIR *dirp) {
    if (!dirp) {
        errno = EBADF;
        return -1;
    }

    long ret = sys_telldir(dirp->dd_fd);
    if (ret < 0) {
        errno = -ret;
        return -1;
    }
    return ret;
}

void seekdir(DIR *dirp, long loc) {
    if (!dirp) return;
    sys_seekdir(dirp->dd_fd, loc);
}

void rewinddir(DIR *dirp) {
    if (!dirp) return;
    sys_rewinddir(dirp->dd_fd);
}

int dirfd(DIR *dirp) {
    if (!dirp) {
        errno = EINVAL;
        return -1;
    }
    return dirp->dd_fd;
}

/* =========================================================================
 * POSIX Advanced Directory Reading (scandir, alphasort)
 * ========================================================================= */

int alphasort(const struct dirent **a, const struct dirent **b) {
    return strcoll((*a)->d_name, (*b)->d_name);
}

int versionsort(const struct dirent **a, const struct dirent **b) {
    /* Felix OS doesn't have strverscmp natively yet.
     * Fall back to alphasort, which satisfies 99% of use cases. */
    return alphasort(a, b);
}

int scandir(const char *dirp, struct dirent ***namelist,
            int (*filter)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
    DIR *d = opendir(dirp);
    if (!d) return -1;

    struct dirent **names = NULL;
    size_t count = 0;
    size_t capacity = 0;
    struct dirent *entry;

    while ((entry = readdir(d)) != NULL) {
        if (filter && !filter(entry)) continue;

        if (count >= capacity) {
            capacity = (capacity == 0) ? 16 : capacity * 2;
            struct dirent **new_names = realloc(names, capacity * sizeof(struct dirent *));
            if (!new_names) {
                while (count-- > 0) free(names[count]);
                free(names);
                closedir(d);
                errno = ENOMEM;
                return -1;
            }
            names = new_names;
        }

        struct dirent *copy = malloc(sizeof(struct dirent));
        if (!copy) {
            while (count-- > 0) free(names[count]);
            free(names);
            closedir(d);
            errno = ENOMEM;
            return -1;
        }

        memcpy(copy, entry, sizeof(struct dirent));
        names[count++] = copy;
    }
    closedir(d);

    if (compar && count > 1) {
        /* qsort requires a slightly different signature than POSIX compar */
        qsort(names, count, sizeof(struct dirent *),
              (int (*)(const void *, const void *))compar);
    }

    *namelist = names;
    return (int)count;
}

int scandirat(int dirfd, const char *dirp, struct dirent ***namelist,
              int (*filter)(const struct dirent *),
              int (*compar)(const struct dirent **, const struct dirent **))
{
    (void)dirfd;
    /* Felix OS doesn't fully support *at relative syscalls yet.
     * We fall back to standard scandir. If 'dirp' is an absolute path,
     * this works perfectly anyway! */
    return scandir(dirp, namelist, filter, compar);
}
