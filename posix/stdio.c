/**
 * @file felix/posix/stdio.c
 * @brief POSIX standard I/O extensions (getline, getdelim, etc.)
 * missing from bare-metal Newlib.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <unistd.h>
#include <grp.h>

/* =========================================================================
 * POSIX Stdio Extensions
 * ========================================================================= */

/**
 * @brief Reads a line from a stream until a specific delimiter is found.
 */
ssize_t getdelim(char **lineptr, size_t *n, int delim, FILE *stream) {
    if (!lineptr || !n || !stream) {
        errno = EINVAL;
        return -1;
    }

    if (*lineptr == NULL || *n == 0) {
        *n = 128; /* Initial buffer size */
        *lineptr = malloc(*n);
        if (!*lineptr) {
            errno = ENOMEM;
            return -1;
        }
    }

    size_t count = 0;
    int c;

    while ((c = fgetc(stream)) != EOF) {
        if (count + 1 >= *n) {
            size_t new_size = *n * 2;
            char *new_ptr = realloc(*lineptr, new_size);
            if (!new_ptr) {
                errno = ENOMEM;
                return -1;
            }
            *lineptr = new_ptr;
            *n = new_size;
        }

        (*lineptr)[count++] = (char)c;
        if (c == delim) break;
    }

    if (count == 0 && c == EOF) return -1;

    (*lineptr)[count] = '\0';
    return (ssize_t)count;
}

/**
 * @brief Reads a line from a stream until newline.
 */
ssize_t getline(char **lineptr, size_t *n, FILE *stream) {
    return getdelim(lineptr, n, '\n', stream);
}

/**
 * @brief Formats output into a dynamically allocated string.
 */
int asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    /* Determine required size */
    int size = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (size < 0) return -1;

    *strp = malloc(size + 1);
    if (!*strp) {
        errno = ENOMEM;
        return -1;
    }

    va_start(ap, fmt);
    int ret = vsnprintf(*strp, size + 1, fmt, ap);
    va_end(ap);

    return ret;
}

int dprintf(int fd, const char *restrict format, ...) {
    /* Implementation would map fd to a stream or format to a buffer and sys_write */
    va_list args;
    va_start(args, format);
    int ret = vdprintf(fd, format, args);
    va_end(args);
    return ret;
}
#if 0
/**
 * @brief Populates the list of groups for a given user.
 * Felix OS is a single-user system; this implementation returns only the
 * primary GID provided by the caller to satisfy POSIX interface expectations.
 */
int getgrouplist(const char *user, gid_t group, gid_t *groups, int *ngroups) {
    if (!user || !groups || !ngroups) {
        errno = EINVAL;
        return -1;
    }

    /* POSIX requirement: the first entry is always the provided base group */
    if (*ngroups < 1) {
        *ngroups = 1;
        return -1; /* Buffer too small */
    }

    groups[0] = group;
    *ngroups = 1;

    return 1;
}
#endif
