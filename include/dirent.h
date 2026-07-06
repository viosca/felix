/**
 * @file felix/include/dirent.h
 * @brief Standalone POSIX directory stream definitions for Felix OS.
 */
#ifndef _FLX_INCLUDE_DIRENT_H
#define _FLX_INCLUDE_DIRENT_H

#ifdef __cplusplus
extern "C" {
#endif

/* POSIX Directory Types (BSD/Linux extension) */
#ifndef DT_UNKNOWN
#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      4
#define DT_BLK      6
#define DT_REG      8
#define DT_LNK      10
#define DT_SOCK     12
#define DT_WHT      14
#endif

/* Standard POSIX Directory Entry */
struct dirent {
    unsigned long d_ino;    /* Inode number */
    unsigned char d_type;   /* Type of file (DT_DIR, DT_REG, etc.) */
    char d_name[256];       /* Null-terminated filename */
};

/* Opaque DIR type natively containing the kernel file descriptor.
 * (Matches Newlib's internal naming convention for compatibility) */
typedef struct {
    int dd_fd;
    struct dirent dd_ent;   /* Per-stream buffer for reentrant readdir */
} DIR;

/* =========================================================================
 * Standard POSIX API Declarations
 * ========================================================================= */
DIR *opendir(const char *name);
DIR *fdopendir(int fd);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);
int  fdclosedir(DIR *);
void rewinddir(DIR *dirp);
void seekdir(DIR *dirp, long loc);
long telldir(DIR *dirp);

int dirfd(DIR *dirp);
int  scandir(const char *, struct dirent ***,
        int (*)(const struct dirent *), int (*)(const struct dirent **,
        const struct dirent **));
int  alphasort(const struct dirent **, const struct dirent **);
int  scandirat(int, const char *, struct dirent ***,
        int (*) (const struct dirent *), int (*) (const struct dirent **,
        const struct dirent **));
int  versionsort(const struct dirent **, const struct dirent **);

#ifdef __cplusplus
}
#endif

#endif /* _FLX_INCLUDE_DIRENT_H */

#if 0

#ifndef _FELIX_DIRENT_H_
#define _FELIX_DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif


/* File types for d_type (BSD/Linux extension) */
#define DT_UNKNOWN  0
#define DT_FIFO     1
#define DT_CHR      2
#define DT_DIR      4
#define DT_BLK      6
#define DT_REG      8
#define DT_LNK      10
#define DT_SOCK     12
#define DT_WHT      14

/* Standard POSIX Directory Entry */
struct dirent {
    unsigned long d_ino;    /* Inode number */
    unsigned char d_type;   /* Type of file (DT_DIR, DT_REG, etc.) */
    char d_name[256];       /* Null-terminated filename */
};

/* Opaque DIR type expected by POSIX */
typedef struct {
    int _dummy;
} DIR;

/* --- Standard POSIX API Declarations --- */
DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

/* Positional extensions */
void rewinddir(DIR *dirp);
void seekdir(DIR *dirp, long loc);
long telldir(DIR *dirp);

#ifdef __cplusplus
}
#endif

#endif

#endif /* _FLX_INCLUDE_DIRENT_H_ */

