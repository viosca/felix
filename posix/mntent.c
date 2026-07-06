/**
 * @file felix/posix/mntent.c
 * @brief Mount entity shims for Felix OS.
 */

#include <mntent.h>
#include <stddef.h>
#include <string.h>

/**
 * @brief Open a mount table file.
 * Felix OS does not use fstab/mtab, so we return NULL.
 */
FILE *setmntent(const char *filename, const char *type) {
    (void)filename;
    (void)type;
    return NULL;
}

/**
 * @brief Read the next entry from the mount table.
 */
struct mntent *getmntent(FILE *stream) {
    (void)stream;
    return NULL;
}

/**
 * @brief Add an entry to the mount table.
 */
int addmntent(FILE *stream, const struct mntent *mnt) {
    (void)stream;
    (void)mnt;
    return 1; /* Return 1 to indicate error/failure */
}

/**
 * @brief Close the mount table file.
 */
int endmntent(FILE *stream) {
    (void)stream;
    return 1; /* Always returns 1 */
}

/**
 * @brief Check if a specific option string is present in mnt_opts.
 */
char *hasmntopt(const struct mntent *mnt, const char *opt) {
    if (!mnt || !mnt->mnt_opts || !opt) {
        return NULL;
    }
    return strstr(mnt->mnt_opts, opt);
}
