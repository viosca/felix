/**
 * @file felix/include/mntent.h
 * @brief POSIX Mount Entity extensions for Felix OS.
 */
#ifndef _FLX_INCLUDE_MNTENT_H
#define _FLX_INCLUDE_MNTENT_H

#include <stdio.h>

/* Safely pull in toolchain's base mntent.h if it exists */
#if defined(__has_include_next) && __has_include_next(<mntent.h>)
#include_next <mntent.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MNTENT_H_DEFINED
#define _MNTENT_H_DEFINED

/* Structure describing a mount table entry */
struct mntent {
    char *mnt_fsname;   /* name of mounted file system */
    char *mnt_dir;      /* file system path prefix */
    char *mnt_type;     /* mount type (e.g., "ext4", "fat") */
    char *mnt_opts;     /* mount options (e.g., "ro,nosuid") */
    int   mnt_freq;     /* dump frequency in days */
    int   mnt_passno;   /* pass number on parallel fsck */
};

/* POSIX Prototypes expected by Toybox */
FILE *setmntent(const char *filename, const char *type);
struct mntent *getmntent(FILE *stream);
int addmntent(FILE *stream, const struct mntent *mnt);
int endmntent(FILE *stream);
char *hasmntopt(const struct mntent *mnt, const char *opt);

#endif /* _MNTENT_H_DEFINED */

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_MNTENT_H */
